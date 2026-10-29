"""Frame encoding and decoding for the LabPSU CPU2 SCI protocol.

Frame (12 bytes, little-endian), see doc/sci_protocol.md:

    A5 5A | command (2) | key (2) | value (4) | 5A A5
"""

from __future__ import annotations

import math
import struct
from dataclasses import dataclass
from enum import Enum, IntEnum

START = bytes((0xA5, 0x5A))
STOP = bytes((0x5A, 0xA5))
FRAME_LEN = 12
RESPONSE_FLAG = 0x8000

_PAYLOAD = struct.Struct("<HHI")


class Command(IntEnum):
    READ = 0x0001
    WRITE = 0x0002
    READ_RESP = 0x8001
    WRITE_ACK = 0x8002
    NACK = 0x80FF


def command_name(code: int) -> str:
    try:
        return Command(code).name
    except ValueError:
        return f"0x{code:04X}"


class ValueType(str, Enum):
    FLOAT32 = "float32"
    UINT32 = "uint32"


@dataclass(frozen=True)
class KeyDef:
    code: int
    name: str
    writable: bool
    vtype: ValueType
    unit: str = ""
    hint: str = ""


KEYS = (
    KeyDef(0x0001, "VOLTAGE_SETPOINT", True, ValueType.FLOAT32, "V"),
    KeyDef(0x0002, "CURRENT_SETPOINT", True, ValueType.FLOAT32, "A"),
    KeyDef(0x0003, "ENABLE", True, ValueType.UINT32, "", "0 = off, 1 = on"),
    KeyDef(0x0004, "OPMODE", True, ValueType.UINT32, "", "1 = closed loop, 2 = open loop"),
    KeyDef(0x0101, "VOLTAGE_MEASURED", False, ValueType.FLOAT32, "V"),
    KeyDef(0x0102, "CURRENT_MEASURED", False, ValueType.FLOAT32, "A"),
    KeyDef(0x0103, "STATUS", False, ValueType.UINT32, "", "bit field"),
)
KEYS_BY_CODE = {k.code: k for k in KEYS}

NACK_ERRORS = {
    1: "Unknown command",
    2: "Unknown key",
    3: "Key is read-only",
    4: "Value out of range",
    5: "Not allowed in current state",
}


def encode_value(vtype: ValueType, value: float | int) -> int:
    """Convert a physical value to the raw 32-bit value field."""
    if vtype is ValueType.FLOAT32:
        return struct.unpack("<I", struct.pack("<f", float(value)))[0]
    v = int(value)
    if not 0 <= v <= 0xFFFFFFFF:
        raise ValueError(f"{v} does not fit into uint32")
    return v


def decode_value(vtype: ValueType, raw: int) -> float | int:
    """Convert the raw 32-bit value field to a physical value."""
    if vtype is ValueType.FLOAT32:
        return struct.unpack("<f", struct.pack("<I", raw))[0]
    return raw


def parse_value(vtype: ValueType, text: str) -> int:
    """Parse user input ("12.5", "1", "0x10") to the raw 32-bit value field."""
    text = text.strip()
    if vtype is ValueType.FLOAT32:
        value = float(text)
        if not math.isfinite(value):
            raise ValueError(f"{text!r} is not a finite number")
        return encode_value(vtype, value)
    return encode_value(vtype, int(text, 0))


def format_value(vtype: ValueType, raw: int) -> str:
    if vtype is ValueType.FLOAT32:
        return f"{decode_value(vtype, raw):.6g}"
    return f"{raw} (0x{raw:08X})"


def hexdump(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)


@dataclass(frozen=True)
class Frame:
    command: int
    key: int
    value: int = 0  # raw 32-bit value field

    def encode(self) -> bytes:
        return START + _PAYLOAD.pack(self.command, self.key, self.value) + STOP

    @classmethod
    def decode(cls, data: bytes) -> Frame:
        if len(data) != FRAME_LEN or data[:2] != START or data[10:] != STOP:
            raise ValueError(f"not a valid frame: {hexdump(data)}")
        return cls(*_PAYLOAD.unpack(data[2:10]))

    def answers(self, request: Frame) -> bool:
        """True if this frame is the response to `request`."""
        return self.key == request.key and self.command in (
            request.command | RESPONSE_FLAG,
            Command.NACK,
        )

    def describe(self) -> str:
        kd = KEYS_BY_CODE.get(self.key)
        key_txt = kd.name if kd else f"key 0x{self.key:04X}"
        if self.command == Command.NACK:
            val = f"error {self.value}: {NACK_ERRORS.get(self.value, 'unknown error')}"
        elif self.command == Command.READ:
            val = ""
        elif kd:
            val = f"= {format_value(kd.vtype, self.value)} {kd.unit}".rstrip()
        else:
            val = f"= 0x{self.value:08X}"
        return f"{command_name(self.command)} {key_txt} {val}".rstrip()


class FrameAssembler:
    """Sliding 12-byte window, resynchronises after lost or extra bytes (§7.2)."""

    def __init__(self) -> None:
        self._window = bytearray()
        self.discarded = 0  # bytes dropped while searching for a frame

    def reset(self) -> None:
        self._window.clear()

    def feed(self, data: bytes) -> list[Frame]:
        frames = []
        for b in data:
            self._window.append(b)
            if len(self._window) > FRAME_LEN:
                del self._window[0]
                self.discarded += 1
            if self._window[:2] == START and self._window[10:] == STOP:
                frames.append(Frame.decode(bytes(self._window)))
                self._window.clear()
        return frames
