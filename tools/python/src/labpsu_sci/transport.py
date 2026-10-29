"""Serial link to CPU2: one request at a time, wait for the response with timeout.

The link runs in its own thread and reports everything (log lines, results,
connection changes) through an event queue that the GUI drains each frame.
"""

from __future__ import annotations

import queue
import threading
import time
from dataclasses import dataclass
from enum import Enum
from typing import Callable, Protocol

import serial

from labpsu_sci.protocol import Command, Frame, FrameAssembler, hexdump


@dataclass
class SerialConfig:
    port: str
    baudrate: int = 115200
    bytesize: int = serial.EIGHTBITS
    parity: str = serial.PARITY_NONE
    stopbits: float = serial.STOPBITS_ONE
    response_timeout: float = 0.2  # seconds per attempt
    retries: int = 0


class Status(str, Enum):
    ACK = "ACK"
    NACK = "NACK"
    TIMEOUT = "TIMEOUT"
    ERROR = "ERROR"


@dataclass
class LogEvent:
    kind: str  # "tx", "rx", "info", "warn", "error"
    text: str
    raw: bytes = b""


@dataclass
class ResultEvent:
    request: Frame
    response: Frame | None
    status: Status


@dataclass
class ConnectionEvent:
    connected: bool
    text: str


class Port(Protocol):
    """The part of serial.Serial the link uses (also met by SimulatedPort)."""

    @property
    def in_waiting(self) -> int: ...
    def read(self, size: int = 1) -> bytes: ...
    def write(self, data: bytes) -> int | None: ...
    def close(self) -> None: ...


def open_serial(cfg: SerialConfig) -> Port:
    return serial.Serial(
        port=cfg.port,
        baudrate=cfg.baudrate,
        bytesize=cfg.bytesize,
        parity=cfg.parity,
        stopbits=cfg.stopbits,
        timeout=0.01,
        write_timeout=0.5,
    )


class Link:
    def __init__(
        self,
        events: queue.Queue,
        port_factory: Callable[[SerialConfig], Port] = open_serial,
    ) -> None:
        self._events = events
        self._port_factory = port_factory
        self._requests: queue.Queue[Frame] = queue.Queue()
        self._port: Port | None = None
        self._cfg: SerialConfig | None = None
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()
        self._busy = False
        self._assembler = FrameAssembler()

    @property
    def is_open(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    @property
    def pending(self) -> int:
        """Requests queued or in flight."""
        return self._requests.qsize() + int(self._busy)

    def open(self, cfg: SerialConfig) -> None:
        """Open the port and start the worker. Raises on failure."""
        self.close()
        self._port = self._port_factory(cfg)
        self._cfg = cfg
        self._assembler = FrameAssembler()
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="sci-link", daemon=True)
        self._thread.start()
        self._events.put(ConnectionEvent(True, f"Connected to {cfg.port} @ {cfg.baudrate}"))

    def close(self) -> None:
        if self._thread is None:
            return
        self._stop.set()
        self._thread.join(timeout=2.0)
        self._thread = None
        self._drop_requests()
        if self._port is not None:
            try:
                self._port.close()
            except Exception:
                pass
            self._port = None
        self._events.put(ConnectionEvent(False, "Disconnected"))

    def submit(self, request: Frame) -> None:
        self._requests.put(request)

    def _drop_requests(self) -> None:
        while True:
            try:
                self._requests.get_nowait()
            except queue.Empty:
                return

    def _run(self) -> None:
        try:
            while not self._stop.is_set():
                try:
                    request = self._requests.get(timeout=0.02)
                except queue.Empty:
                    self._read_unsolicited()
                    continue
                self._busy = True
                try:
                    self._transact(request)
                finally:
                    self._busy = False
        except (serial.SerialException, OSError) as exc:
            self._events.put(LogEvent("error", f"Serial error: {exc}"))
            self._events.put(ConnectionEvent(False, f"Connection lost: {exc}"))
            self._drop_requests()

    def _transact(self, request: Frame) -> None:
        assert self._cfg is not None
        attempts = self._cfg.retries + 1
        for attempt in range(1, attempts + 1):
            self._read_unsolicited()  # do not mistake stale bytes for the answer
            raw = request.encode()
            self._port.write(raw)
            self._events.put(LogEvent("tx", request.describe(), raw))
            response = self._wait_for(request)
            if response is not None:
                status = Status.NACK if response.command == Command.NACK else Status.ACK
                self._events.put(ResultEvent(request, response, status))
                return
            self._events.put(
                LogEvent("warn", f"No response to {request.describe()} (attempt {attempt}/{attempts})")
            )
        self._events.put(ResultEvent(request, None, Status.TIMEOUT))

    def _wait_for(self, request: Frame) -> Frame | None:
        deadline = time.monotonic() + self._cfg.response_timeout
        match = None
        while match is None and time.monotonic() < deadline and not self._stop.is_set():
            for frame in self._receive():
                if match is None and frame.answers(request):
                    match = frame
                    self._events.put(LogEvent("rx", frame.describe(), frame.encode()))
                else:
                    self._events.put(LogEvent("warn", f"Unexpected frame: {frame.describe()}", frame.encode()))
        return match

    def _read_unsolicited(self) -> None:
        for frame in self._receive():
            self._events.put(LogEvent("warn", f"Unsolicited frame: {frame.describe()}", frame.encode()))

    def _receive(self) -> list[Frame]:
        data = self._port.read(self._port.in_waiting or 1)
        if not data:
            return []
        before = self._assembler.discarded
        frames = self._assembler.feed(data)
        dropped = self._assembler.discarded - before
        if dropped:
            self._events.put(LogEvent("warn", f"Discarded {dropped} byte(s) outside a frame", data))
        return frames
