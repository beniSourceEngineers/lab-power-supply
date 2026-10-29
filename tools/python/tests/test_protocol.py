import pytest

from labpsu_sci.protocol import (
    Command,
    Frame,
    FrameAssembler,
    ValueType,
    decode_value,
    parse_value,
)


def frame_bytes(text: str) -> bytes:
    return bytes.fromhex(text)


# Examples from doc/sci_protocol.md §10
WRITE_12V = frame_bytes("A55A 0200 0100 00004041 5AA5")
WRITE_12V_ACK = frame_bytes("A55A 0280 0100 00004041 5AA5")
READ_CURRENT = frame_bytes("A55A 0100 0201 00000000 5AA5")
READ_CURRENT_RESP = frame_bytes("A55A 0180 0201 0000C03F 5AA5")
NACK_READ_ONLY = frame_bytes("A55A FF80 0101 03000000 5AA5")


def test_encode_matches_doc_examples():
    assert Frame(Command.WRITE, 0x0001, parse_value(ValueType.FLOAT32, "12.0")).encode() == WRITE_12V
    assert Frame(Command.READ, 0x0102).encode() == READ_CURRENT


def test_decode_matches_doc_examples():
    resp = Frame.decode(READ_CURRENT_RESP)
    assert resp == Frame(Command.READ_RESP, 0x0102, 0x3FC00000)
    assert decode_value(ValueType.FLOAT32, resp.value) == 1.5
    assert Frame.decode(NACK_READ_ONLY) == Frame(Command.NACK, 0x0101, 3)


def test_decode_rejects_bad_framing():
    with pytest.raises(ValueError):
        Frame.decode(WRITE_12V[:-1] + b"\x00")
    with pytest.raises(ValueError):
        Frame.decode(WRITE_12V[:11])


def test_answers_matches_command_and_key():
    request = Frame.decode(WRITE_12V)
    assert Frame.decode(WRITE_12V_ACK).answers(request)
    assert Frame(Command.NACK, 0x0001, 4).answers(request)
    assert not Frame(Command.READ_RESP, 0x0001, 0).answers(request)
    assert not Frame(Command.WRITE_ACK, 0x0002, 0).answers(request)


def test_parse_value():
    assert parse_value(ValueType.UINT32, "1") == 1
    assert parse_value(ValueType.UINT32, "0x10") == 16
    with pytest.raises(ValueError):
        parse_value(ValueType.UINT32, "-1")
    with pytest.raises(ValueError):
        parse_value(ValueType.FLOAT32, "nan")
    with pytest.raises(ValueError):
        parse_value(ValueType.FLOAT32, "abc")


def test_assembler_splits_back_to_back_frames():
    asm = FrameAssembler()
    assert asm.feed(WRITE_12V_ACK + READ_CURRENT_RESP) == [
        Frame.decode(WRITE_12V_ACK),
        Frame.decode(READ_CURRENT_RESP),
    ]
    assert asm.discarded == 0


def test_assembler_handles_partial_input():
    asm = FrameAssembler()
    assert asm.feed(READ_CURRENT_RESP[:5]) == []
    assert asm.feed(READ_CURRENT_RESP[5:]) == [Frame.decode(READ_CURRENT_RESP)]


def test_assembler_resynchronises_after_garbage():
    asm = FrameAssembler()
    frames = asm.feed(b"\x00\xA5\x5A\x01" + READ_CURRENT_RESP)
    assert frames == [Frame.decode(READ_CURRENT_RESP)]
    assert asm.discarded == 4
