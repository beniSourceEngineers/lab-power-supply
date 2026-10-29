"""Dear PyGui front end: port settings, key table, raw frames and a log."""

from __future__ import annotations

import argparse
import queue
import time
from collections import deque
from datetime import datetime

import dearpygui.dearpygui as dpg
import serial
import serial.tools.list_ports

from labpsu_sci import __version__
from labpsu_sci.protocol import (
    KEYS,
    KEYS_BY_CODE,
    NACK_ERRORS,
    Command,
    Frame,
    ValueType,
    format_value,
    hexdump,
    parse_value,
)
from labpsu_sci.simulator import SimulatedPort
from labpsu_sci.transport import (
    ConnectionEvent,
    Link,
    LogEvent,
    ResultEvent,
    SerialConfig,
    Status,
    open_serial,
)

SIM_PORT = "<simulator>"
BAUDRATES = ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"]
BYTESIZES = {"5": serial.FIVEBITS, "6": serial.SIXBITS, "7": serial.SEVENBITS, "8": serial.EIGHTBITS}
PARITIES = {
    "None": serial.PARITY_NONE,
    "Even": serial.PARITY_EVEN,
    "Odd": serial.PARITY_ODD,
    "Mark": serial.PARITY_MARK,
    "Space": serial.PARITY_SPACE,
}
STOPBITS = {"1": serial.STOPBITS_ONE, "1.5": serial.STOPBITS_ONE_POINT_FIVE, "2": serial.STOPBITS_TWO}

GREEN = (110, 220, 110)
ORANGE = (240, 180, 80)
RED = (255, 100, 100)
GREY = (170, 170, 170)
LOG_COLORS = {"tx": (120, 180, 255), "rx": GREEN, "info": GREY, "warn": ORANGE, "error": RED}
LOG_PREFIX = {"tx": "TX ", "rx": "RX ", "info": "-- ", "warn": "!! ", "error": "ERR"}
STATUS_COLORS = {Status.ACK: GREEN, Status.NACK: ORANGE, Status.TIMEOUT: RED, Status.ERROR: RED}
MAX_LOG_LINES = 2000
COMMAND_ITEMS = [f"{c.name} (0x{c.value:04X})" for c in Command]
# Log lines produced by polling, hidden unless "Log polling" is checked
POLL_LOG_PREFIXES = tuple(
    f"{cmd} {kd.name}" for kd in KEYS if not kd.writable for cmd in ("READ", "READ_RESP")
)


def list_ports() -> list[str]:
    return [p.device for p in sorted(serial.tools.list_ports.comports())] + [SIM_PORT]


def open_port(cfg: SerialConfig):
    if cfg.port == SIM_PORT:
        return SimulatedPort(cfg)
    return open_serial(cfg)


class App:
    def __init__(self, port: str | None, baudrate: int) -> None:
        self.events: queue.Queue = queue.Queue()
        self.link = Link(self.events, port_factory=open_port)
        self._initial_port = port
        self._initial_baud = str(baudrate)
        self._log_items: deque[int] = deque()
        self._scroll_frames = 0
        self._last_poll = 0.0

    # ----------------------------------------------------------------- layout

    def build(self) -> None:
        with dpg.window(tag="main"):
            with dpg.group(horizontal=True):
                with dpg.child_window(width=300, height=380):
                    self._build_connection()
                with dpg.child_window(width=-1, height=380):
                    self._build_keys()
            with dpg.child_window(width=-1, height=62):
                self._build_raw()
            self._build_log()

    def _build_connection(self) -> None:
        dpg.add_text("UART settings")
        dpg.add_separator()
        ports = list_ports()
        port = self._initial_port if self._initial_port in ports else ports[0]
        with dpg.group(horizontal=True):
            dpg.add_combo(ports, default_value=port, tag="port", width=190)
            dpg.add_button(label="Refresh", callback=self._refresh_ports)
        bauds = BAUDRATES if self._initial_baud in BAUDRATES else BAUDRATES + [self._initial_baud]
        dpg.add_combo(bauds, default_value=self._initial_baud, label="Baud rate", tag="baud", width=120)
        dpg.add_combo(list(BYTESIZES), default_value="8", label="Data bits", tag="bytesize", width=120)
        dpg.add_combo(list(PARITIES), default_value="None", label="Parity", tag="parity", width=120)
        dpg.add_combo(list(STOPBITS), default_value="1", label="Stop bits", tag="stopbits", width=120)
        dpg.add_input_int(label="Timeout [ms]", default_value=200, min_value=10, min_clamped=True, tag="timeout", width=120)
        dpg.add_input_int(label="Retries", default_value=0, min_value=0, max_value=10, min_clamped=True, max_clamped=True, tag="retries", width=120)
        dpg.add_spacer(height=4)
        dpg.add_button(label="Connect", tag="connect_btn", width=-1, callback=self._toggle_connection)
        dpg.add_text("Disconnected", tag="conn_status", color=GREY, wrap=280)
        dpg.add_spacer(height=4)
        dpg.add_separator()
        dpg.add_button(label="Read all keys", width=-1, callback=self._read_all)
        dpg.add_checkbox(label="Poll measurements", tag="poll")
        dpg.add_input_int(label="Interval [ms]", default_value=500, min_value=50, min_clamped=True, tag="poll_ms", width=120)

    def _build_keys(self) -> None:
        dpg.add_text("Keys")
        dpg.add_separator()
        with dpg.table(header_row=True, borders_innerH=True, borders_outerH=True, row_background=True,
                       policy=dpg.mvTable_SizingStretchProp):
            dpg.add_table_column(label="Key", width_fixed=True, init_width_or_weight=60)
            dpg.add_table_column(label="Name", width_fixed=True, init_width_or_weight=150)
            dpg.add_table_column(label="Unit", width_fixed=True, init_width_or_weight=35)
            dpg.add_table_column(label="Value", init_width_or_weight=1.2)
            dpg.add_table_column(label="Ack", init_width_or_weight=1.5)
            dpg.add_table_column(label="New value", width_fixed=True, init_width_or_weight=120)
            dpg.add_table_column(label="", width_fixed=True, init_width_or_weight=100)
            for kd in KEYS:
                with dpg.table_row():
                    dpg.add_text(f"0x{kd.code:04X}")
                    dpg.add_text(kd.name)
                    if kd.hint:
                        with dpg.tooltip(dpg.last_item()):
                            dpg.add_text(kd.hint)
                    dpg.add_text(kd.unit or "-")
                    dpg.add_text("-", tag=f"value_{kd.code}")
                    dpg.add_text("", tag=f"ack_{kd.code}")
                    if kd.writable:
                        dpg.add_input_text(tag=f"input_{kd.code}", width=-1, hint=kd.hint or kd.vtype.value,
                                           on_enter=True, callback=self._write_key, user_data=kd.code)
                    else:
                        dpg.add_text("read only", color=GREY)
                    with dpg.group(horizontal=True):
                        dpg.add_button(label="Read", callback=self._read_key, user_data=kd.code)
                        if kd.writable:
                            dpg.add_button(label="Write", callback=self._write_key, user_data=kd.code)

    def _build_raw(self) -> None:
        dpg.add_text("Raw frame")
        with dpg.group(horizontal=True):
            dpg.add_combo(COMMAND_ITEMS, default_value=COMMAND_ITEMS[0], tag="raw_cmd", width=170,
                          callback=self._update_raw_preview)
            dpg.add_input_text(label="Key", default_value="0x0001", tag="raw_key", width=70,
                               callback=self._update_raw_preview)
            dpg.add_input_text(label="Value", default_value="0", tag="raw_value", width=100,
                               callback=self._update_raw_preview)
            dpg.add_combo([v.value for v in ValueType], default_value=ValueType.FLOAT32.value, tag="raw_type",
                          width=80, callback=self._update_raw_preview)
            dpg.add_button(label="Send", callback=self._send_raw)
            dpg.add_text("", tag="raw_preview", color=GREY)
        self._update_raw_preview()

    def _build_log(self) -> None:
        with dpg.group(horizontal=True):
            dpg.add_text("Log")
            dpg.add_button(label="Clear", callback=self._clear_log)
            dpg.add_checkbox(label="Autoscroll", default_value=True, tag="autoscroll")
            dpg.add_checkbox(label="Show hex", default_value=True, tag="show_hex")
            dpg.add_checkbox(label="Log polling", default_value=False, tag="log_poll")
        dpg.add_child_window(tag="log", width=-1, height=-1, horizontal_scrollbar=True)

    # -------------------------------------------------------------- callbacks

    def _refresh_ports(self) -> None:
        ports = list_ports()
        dpg.configure_item("port", items=ports)
        if dpg.get_value("port") not in ports:
            dpg.set_value("port", ports[0])

    def _toggle_connection(self) -> None:
        if self.link.is_open:
            self.link.close()
            return
        cfg = SerialConfig(
            port=dpg.get_value("port"),
            baudrate=int(dpg.get_value("baud")),
            bytesize=BYTESIZES[dpg.get_value("bytesize")],
            parity=PARITIES[dpg.get_value("parity")],
            stopbits=STOPBITS[dpg.get_value("stopbits")],
            response_timeout=dpg.get_value("timeout") / 1000.0,
            retries=dpg.get_value("retries"),
        )
        try:
            self.link.open(cfg)
        except (serial.SerialException, OSError, ValueError) as exc:
            self._log("error", f"Cannot open {cfg.port}: {exc}")
            dpg.set_value("conn_status", f"Error: {exc}")
            dpg.configure_item("conn_status", color=RED)

    def _submit(self, frame: Frame) -> None:
        if not self.link.is_open:
            self._log("error", "Not connected")
            return
        self.link.submit(frame)

    def _read_key(self, sender, app_data, code: int) -> None:
        self._submit(Frame(Command.READ, code))

    def _write_key(self, sender, app_data, code: int) -> None:
        kd = KEYS_BY_CODE[code]
        text = dpg.get_value(f"input_{code}")
        try:
            raw = parse_value(kd.vtype, text)
        except ValueError:
            self._log("error", f"{kd.name}: invalid {kd.vtype.value} value {text!r}")
            return
        self._submit(Frame(Command.WRITE, code, raw))

    def _read_all(self) -> None:
        for kd in KEYS:
            self._submit(Frame(Command.READ, kd.code))

    def _raw_frame(self) -> Frame:
        command = Command[dpg.get_value("raw_cmd").split(" ")[0]]
        key = int(dpg.get_value("raw_key"), 0)
        if not 0 <= key <= 0xFFFF:
            raise ValueError("key does not fit into uint16")
        raw = parse_value(ValueType(dpg.get_value("raw_type")), dpg.get_value("raw_value"))
        return Frame(command, key, raw)

    def _update_raw_preview(self) -> None:
        try:
            dpg.set_value("raw_preview", hexdump(self._raw_frame().encode()))
        except ValueError:
            dpg.set_value("raw_preview", "invalid input")

    def _send_raw(self) -> None:
        try:
            frame = self._raw_frame()
        except ValueError as exc:
            self._log("error", f"Raw frame: {exc}")
            return
        self._submit(frame)

    def _clear_log(self) -> None:
        dpg.delete_item("log", children_only=True)
        self._log_items.clear()

    # ------------------------------------------------------------- main loop

    def _log(self, kind: str, text: str, raw: bytes = b"") -> None:
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        line = f"{ts}  {LOG_PREFIX[kind]}  {text}"
        if raw and dpg.get_value("show_hex"):
            line += f"    [{hexdump(raw)}]"
        self._log_items.append(dpg.add_text(line, parent="log", color=LOG_COLORS[kind]))
        while len(self._log_items) > MAX_LOG_LINES:
            dpg.delete_item(self._log_items.popleft())
        if dpg.get_value("autoscroll"):
            self._scroll_frames = 2  # scroll max is only known after layout

    def _show_result(self, ev: ResultEvent) -> None:
        kd = KEYS_BY_CODE.get(ev.request.key)
        if kd is None:
            return
        if ev.status is Status.ACK:
            dpg.set_value(f"value_{kd.code}", format_value(kd.vtype, ev.response.value))
            text = Command(ev.response.command).name
        elif ev.status is Status.NACK:
            text = f"NACK {ev.response.value}: {NACK_ERRORS.get(ev.response.value, 'unknown error')}"
        else:
            text = ev.status.value
        dpg.set_value(f"ack_{kd.code}", f"{text}  {datetime.now():%H:%M:%S}")
        dpg.configure_item(f"ack_{kd.code}", color=STATUS_COLORS[ev.status])

    def _is_poll_noise(self, ev: LogEvent) -> bool:
        return (
            self._polling_active
            and not dpg.get_value("log_poll")
            and ev.kind in ("tx", "rx")
            and ev.text.startswith(POLL_LOG_PREFIXES)
        )

    def _process_events(self) -> None:
        while True:
            try:
                ev = self.events.get_nowait()
            except queue.Empty:
                return
            if isinstance(ev, LogEvent):
                if not self._is_poll_noise(ev):
                    self._log(ev.kind, ev.text, ev.raw)
            elif isinstance(ev, ResultEvent):
                self._show_result(ev)
            elif isinstance(ev, ConnectionEvent):
                self._log("info", ev.text)
                dpg.configure_item("connect_btn", label="Disconnect" if ev.connected else "Connect")
                dpg.set_value("conn_status", ev.text)
                dpg.configure_item("conn_status", color=GREEN if ev.connected else GREY)

    @property
    def _polling_active(self) -> bool:
        return dpg.get_value("poll") and self.link.is_open

    def _poll(self) -> None:
        if not self._polling_active or self.link.pending:
            return
        now = time.monotonic()
        if now - self._last_poll < dpg.get_value("poll_ms") / 1000.0:
            return
        self._last_poll = now
        for kd in KEYS:
            if not kd.writable:
                self.link.submit(Frame(Command.READ, kd.code))

    def run(self) -> None:
        dpg.create_context()
        self.build()
        dpg.create_viewport(title=f"LabPSU SCI tool {__version__}", width=1200, height=850)
        dpg.setup_dearpygui()
        dpg.show_viewport()
        dpg.set_primary_window("main", True)
        try:
            while dpg.is_dearpygui_running():
                self._process_events()
                self._poll()
                dpg.render_dearpygui_frame()
                if self._scroll_frames:
                    self._scroll_frames -= 1
                    dpg.set_y_scroll("log", dpg.get_y_scroll_max("log"))
        finally:
            self.link.close()
            dpg.destroy_context()


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="LabPSU CPU2 SCI protocol tool (doc/sci_protocol.md)")
    parser.add_argument("--port", help="serial port to preselect, e.g. /dev/ttyUSB0 or COM5")
    parser.add_argument("--baud", type=int, default=115200, help="baud rate to preselect (default 115200)")
    parser.add_argument("--sim", action="store_true", help="preselect the built-in CPU2 simulator")
    args = parser.parse_args(argv)
    App(SIM_PORT if args.sim else args.port, args.baud).run()


if __name__ == "__main__":
    main()
