"""Simulated CPU2 that answers like the firmware should (doc/sci_protocol.md).

Stands in for serial.Serial, so the GUI and the link can be used without
hardware. The limits below are simulator assumptions, not the real ratings
(§9, still open).
"""

from __future__ import annotations

import random
import threading
import time

from labpsu_sci.protocol import (
    KEYS_BY_CODE,
    Command,
    Frame,
    FrameAssembler,
    ValueType,
    decode_value,
    encode_value,
)
from labpsu_sci.transport import SerialConfig

LIMITS = {
    0x0001: (0.0, 30.0),  # VOLTAGE_SETPOINT [V]
    0x0002: (0.0, 5.0),  # CURRENT_SETPOINT [A]
    0x0003: (0, 1),  # ENABLE
    0x0004: (0, 0xFFFF),  # OPMODE, others = don't care
}
LOAD_OHM = 10.0


class SimulatedPort:
    def __init__(self, cfg: SerialConfig | None = None) -> None:
        self._rx = bytearray()
        self._lock = threading.Lock()
        self._assembler = FrameAssembler()
        self._params = {
            0x0001: encode_value(ValueType.FLOAT32, 0.0),
            0x0002: encode_value(ValueType.FLOAT32, 0.0),
            0x0003: 0,
            0x0004: 1,
        }

    @property
    def in_waiting(self) -> int:
        with self._lock:
            return len(self._rx)

    def read(self, size: int = 1) -> bytes:
        with self._lock:
            data = bytes(self._rx[:size])
            del self._rx[:size]
        if not data:
            time.sleep(0.01)  # behave like a serial read timeout
        return data

    def write(self, data: bytes) -> int:
        for frame in self._assembler.feed(data):
            response = self._handle(frame).encode()
            with self._lock:
                self._rx += response
        return len(data)

    def close(self) -> None:
        pass

    def _handle(self, req: Frame) -> Frame:
        kd = KEYS_BY_CODE.get(req.key)
        if req.command not in (Command.READ, Command.WRITE):
            return Frame(Command.NACK, req.key, 1)
        if kd is None:
            return Frame(Command.NACK, req.key, 2)
        if req.command == Command.READ:
            return Frame(Command.READ_RESP, req.key, self._read(req.key))
        if not kd.writable:
            return Frame(Command.NACK, req.key, 3)
        lo, hi = LIMITS[req.key]
        if not lo <= decode_value(kd.vtype, req.value) <= hi:
            return Frame(Command.NACK, req.key, 4)
        self._params[req.key] = req.value
        return Frame(Command.WRITE_ACK, req.key, req.value)

    def _read(self, key: int) -> int:
        if key in self._params:
            return self._params[key]
        enabled = self._params[0x0003] == 1
        u_set = decode_value(ValueType.FLOAT32, self._params[0x0001])
        i_set = decode_value(ValueType.FLOAT32, self._params[0x0002])
        # CV until the load would draw more than the current setpoint, then CC.
        u = min(u_set, i_set * LOAD_OHM) if enabled else 0.0
        noise = random.uniform(-0.002, 0.002) if enabled else 0.0
        if key == 0x0101:
            return encode_value(ValueType.FLOAT32, u + noise)
        if key == 0x0102:
            return encode_value(ValueType.FLOAT32, u / LOAD_OHM + noise)
        cc = enabled and i_set * LOAD_OHM < u_set
        return int(enabled) | (int(cc) << 1)  # STATUS: bit0 enabled, bit1 CC (sim only)
