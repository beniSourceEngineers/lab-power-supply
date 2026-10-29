# LabPSU SCI tool

PC tool for the CPU2 SCI protocol in [`doc/sci_protocol.md`](../../doc/sci_protocol.md):
12-byte frames `A5 5A | command | key | value | 5A A5`, little-endian.

- UART settings: port, baud rate, data bits, parity, stop bits, response
  timeout and retries.
- Key table: read/write every key from §5, shows the last value and the
  acknowledge (READ_RESP / WRITE_ACK / NACK with error text / TIMEOUT).
- Raw frame: send any command/key/value, with a hex preview.
- Log: timestamped TX/RX frames with decoded text and hex bytes, plus
  timeouts, unexpected frames and discarded bytes.
- Polling of the measurement keys at a fixed interval.
- Built-in simulator (`<simulator>` port) that answers like CPU2, for trying
  the tool without hardware.

## Run

Needs [uv](https://docs.astral.sh/uv/). From this folder:

```sh
uv run labpsu-sci                       # pick the port in the GUI
uv run labpsu-sci --port /dev/ttyACM0   # preselect a port (COM5 on Windows)
uv run labpsu-sci --sim                 # preselect the simulator
```

On Linux the user needs access to the serial device (group `dialout` or
`uucp`, depending on the distribution).

## Tests

```sh
uv run pytest
```

## Layout

| File | Contents |
|---|---|
| `src/labpsu_sci/protocol.py` | Frame, commands, keys, value encoding, frame assembler (sliding window, §7.2) |
| `src/labpsu_sci/transport.py` | Serial link thread: one request at a time, waits for the matching response |
| `src/labpsu_sci/simulator.py` | Simulated CPU2 behind a `serial.Serial`-like interface |
| `src/labpsu_sci/gui.py` | Dear PyGui front end |

When keys or commands change in `doc/sci_protocol.md`, update `KEYS` /
`Command` in `protocol.py`. The simulator limits (30 V, 5 A, 10 Ω load) are
placeholders until §9 defines the real ones.
