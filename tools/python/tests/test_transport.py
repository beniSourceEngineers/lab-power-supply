import queue

import pytest

from labpsu_sci.protocol import Command, Frame, ValueType, decode_value, encode_value
from labpsu_sci.simulator import SimulatedPort
from labpsu_sci.transport import ConnectionEvent, Link, LogEvent, ResultEvent, SerialConfig, Status


class SilentPort:
    """Accepts writes, never answers."""

    in_waiting = 0

    def read(self, size=1):
        return b""

    def write(self, data):
        return len(data)

    def close(self):
        pass


def next_result(events: queue.Queue, timeout=2.0) -> ResultEvent:
    while True:
        ev = events.get(timeout=timeout)
        if isinstance(ev, ResultEvent):
            return ev


@pytest.fixture
def sim_link():
    events = queue.Queue()
    link = Link(events, port_factory=SimulatedPort)
    link.open(SerialConfig(port="sim"))
    yield link, events
    link.close()


def test_write_is_acknowledged(sim_link):
    link, events = sim_link
    link.submit(Frame(Command.WRITE, 0x0001, encode_value(ValueType.FLOAT32, 12.0)))
    result = next_result(events)
    assert result.status is Status.ACK
    assert result.response.command == Command.WRITE_ACK
    assert decode_value(ValueType.FLOAT32, result.response.value) == 12.0


def test_write_to_read_only_key_is_nacked(sim_link):
    link, events = sim_link
    link.submit(Frame(Command.WRITE, 0x0101, 0))
    result = next_result(events)
    assert result.status is Status.NACK
    assert result.response.value == 3


def test_measurement_follows_setpoint(sim_link):
    link, events = sim_link
    link.submit(Frame(Command.WRITE, 0x0001, encode_value(ValueType.FLOAT32, 5.0)))
    link.submit(Frame(Command.WRITE, 0x0002, encode_value(ValueType.FLOAT32, 2.0)))
    link.submit(Frame(Command.WRITE, 0x0003, 1))
    link.submit(Frame(Command.READ, 0x0101))
    results = [next_result(events) for _ in range(4)]
    assert [r.status for r in results] == [Status.ACK] * 4
    assert decode_value(ValueType.FLOAT32, results[-1].response.value) == pytest.approx(5.0, abs=0.01)


def test_timeout_with_retries():
    events = queue.Queue()
    link = Link(events, port_factory=lambda cfg: SilentPort())
    link.open(SerialConfig(port="silent", response_timeout=0.05, retries=2))
    try:
        link.submit(Frame(Command.READ, 0x0001))
        seen = []
        while not isinstance(ev := events.get(timeout=2.0), ResultEvent):
            seen.append(ev)
    finally:
        link.close()
    assert ev.status is Status.TIMEOUT
    assert ev.response is None
    assert sum(isinstance(e, LogEvent) and e.kind == "tx" for e in seen) == 3
    assert isinstance(events.get_nowait(), ConnectionEvent)
