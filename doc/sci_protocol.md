# SCI protocol — LabPSU CPU02 (draft v0.2)

Status: **draft v0.2 for review**, no implementation yet. Items marked **[OPEN]**
need a decision; the proposed default is given so implementation can start.

## 1. Scope

Simple binary request/response protocol on SCI (UART) of CPU2 to:

- write setpoints, enable and operating mode,
- read back setpoints and poll measurements/status.

The host (PC) is always the master. CPU2 only answers; it never sends
unsolicited frames.

Note: AGENTS.md §3.5 mentions "SLIP/COBS + CRC16". This draft follows the
simpler fixed-length start/stop framing requested here instead. AGENTS.md
should be updated once this is agreed.

## 2. Physical layer

| Item | Value |
|---|---|
| Interface | SCIA (owned by CPU2), via LaunchPad XDS100/110 virtual COM or external USB-UART |
| Format | 8N1 |
| Baud rate | 115200 **[OPEN]** |
| Flow control | none |

## 3. Frame format

Fixed length, **12 bytes**:

| Offset | Size | Field | Value |
|---|---|---|---|
| 0 | 2 | Start | `0xA5 0x5A` |
| 2 | 2 | Command | see §4 |
| 4 | 2 | Key | see §5 |
| 6 | 4 | Value | see §6 |
| 10 | 2 | Stop | `0x5A 0xA5` |

**Assumption:** the original request listed both "command(2) + key(2) +
value(4)" between start/stop and a frame "start(2) + key(2) + value(4) +
stop(2)" without command. This draft assumes the command field is present,
giving 12 bytes. **[OPEN] confirm.**

- **Byte order:** little-endian for command, key and value (matches C28x and
  x86). **[OPEN]**
- **No escaping.** Because the frame length is fixed, start/stop bytes may
  appear inside command/key/value. The receiver only accepts a frame when start
  is at offset 0 *and* stop is at offset 10 (§7.2), which resynchronises after
  a lost or extra byte.
- **No checksum** (decided 2026-10-29). Start+stop catch misalignment but
  not bit errors in the payload. A CRC can be added later if needed.

## 4. Commands

Request (host → CPU2):

| Code | Name | Value field |
|---|---|---|
| `0x0001` | READ | ignored, send 0 |
| `0x0002` | WRITE | new value |

Response (CPU2 → host), same frame format, same key as the request:

| Code | Name | Value field |
|---|---|---|
| `0x8001` | READ_RESP | current value of the key |
| `0x8002` | WRITE_ACK | value actually applied (after limiting, if any) |
| `0x80FF` | NACK | error code (§8) |

Every valid request gets exactly one response. **[OPEN]** Is a WRITE_ACK
wanted, or should writes be silent? Proposal: always answer, it makes the
host simple (send, wait for response with timeout, retry).

## 5. Keys

| Key | Name | Access | Type | Unit | IPC signal (CPU02 buses) |
|---|---|---|---|---|---|
| `0x0001` | VOLTAGE_SETPOINT | R/W | float32 | V | `com2ctrl.voltageSetpoint` |
| `0x0002` | CURRENT_SETPOINT | R/W | float32 | A | `com2ctrl.currentSetpoint` |
| `0x0003` | ENABLE | R/W | uint32 | 0 = off, 1 = on | `com2ctrl.enable` |
| `0x0004` | OPMODE | R/W | uint32 | 1 = closed loop, 2 = open loop, 3 = reserved, others = don't care | `com2ctrl.opmode` |
| `0x0101` | VOLTAGE_MEASURED | R | float32 | V | `ctrl2com.voltageMeasured` |
| `0x0102` | CURRENT_MEASURED | R | float32 | A | `ctrl2com.currentMeasured` |
| `0x0103` | STATUS | R | uint32 | bit field **[OPEN]** | `ctrl2com.status` (added, exists on the bus) |

- Range `0x0001–0x00FF`: writable parameters. Range `0x0100–0x01FF`: read-only
  measurements/status. Further ranges free for extensions (limits, calibration,
  firmware version, ...).
- Reading a setpoint returns the value last written by the host (held on CPU2),
  not a value echoed from CPU1. **[OPEN]**
- The key table is intentionally "object dictionary"-like (read/write by
  index) so it maps 1:1 onto the later CANopen object dictionary (AGENTS.md
  §3.5).

## 6. Value encoding

The value field is 4 bytes, interpreted per key (column *Type* in §5):

- `float32`: IEEE 754 single, little-endian (same as `single` on C28x).
- `uint32`: unsigned integer, little-endian; 16-bit IPC signals use the low
  word, high word must be 0.

**[OPEN]** Alternative: all values as int32 scaled (e.g. mV, mA) — avoids float
handling on the host side, but needs scaling on CPU2. Proposal: float32, since
the IPC buses are already `single`.

## 7. Receive path design

```
SCI RX ISR ──bytes──▶ RX ring buffer ──▶ receive context (1 ms task / bg loop)
                                           ├─ frame assembler (sliding window)
                                           ├─ dispatcher (key table)
                                           │    ├─ write → com2ctrl → IPC to CPU1
                                           │    └─ read  ← ctrl2com (latest IPC)
                                           └─ response → TX (FIFO / TX ring)
```

### 7.1 SCI RX interrupt

- SCI FIFO mode, RX FIFO interrupt level e.g. 4 bytes **[OPEN]** (trade-off
  ISR rate vs. latency), plus RX error interrupt.
- ISR: read all bytes currently in the RX FIFO (`SCI_readCharNonBlocking`,
  keep low 8 bits), push into a single-producer/single-consumer ring buffer
  (e.g. 64 entries of `uint16_t`, one byte per entry because `char` is 16 bit).
- On ring buffer overflow: drop byte, increment `rxOverflowCount`.
- On RX error (framing, parity, overrun, break): reset the SCI RX path
  (`SCI_performSoftwareReset`, clear FIFO), increment `rxErrorCount`.
- ISR does no parsing. Clear the FIFO interrupt flag and ACK PIE group 9.

### 7.2 Receive context: frame assembler

Runs outside the ISR (1 ms com task or background loop). Pops all available
bytes from the ring buffer. Uses a 12-byte sliding window:

```
for each byte b popped:
    shift window left by 1, window[11] = b
    if window[0..1] == START and window[10..11] == STOP:
        handle_frame(window)
        clear window            // no overlap between frames
```

- Resynchronises by itself after lost, extra or corrupted bytes; no explicit
  states and no escaping needed.
- Optional inter-byte timeout (e.g. 5 ms without bytes → clear window)
  **[OPEN]**; not strictly needed with the sliding window.

### 7.3 Dispatcher

- Decode command, key, value (assemble little-endian bytes into `uint16_t` /
  `uint32_t`, then reinterpret as `float` for float keys).
- Look up key in a static table `{key, access, type, min, max, read(), write()}`.
- WRITE: check access and range, store into the local `com2ctrl` copy, which is
  posted to CPU1 by the existing 1 ms IPC write (`Com_ipcTs`). Reply WRITE_ACK.
- READ: read from local `com2ctrl` copy or the latest received `ctrl2com`.
  Reply READ_RESP.
- Errors → NACK (§8).

### 7.4 Transmit

A response is 12 bytes and fits into the 16-byte SCI TX FIFO, so it can be
written directly from the receive context as long as only one request is
outstanding. If the host may pipeline requests, use a TX ring buffer plus TX
FIFO interrupt. **[OPEN]** Proposal: direct FIFO write, host waits for each
response.

### 7.5 Implementation target

Decided: first implementation in **Project A** (`simulink_full/LabPSU_CPU02.slx`).
Project B follows later.
Built by `simulink_full/scripts/add_sci_protocol.m`.


- **Project A (`simulink_full/LabPSU_CPU02.slx`):** C2000 Hardware Interrupt
  block on SCIA RX → function-call subsystem with SCI Receive block writing to
  the ring buffer (Data Store / MATLAB Function), parser and dispatcher as a
  MATLAB Function in the 1 ms rate, SCI Transmit block for responses.
- **Project B (`ccs/cpu2/`, C++03):** `hal::Sci` (driverlib wrapper, behind
  `hal::ISerialPort`), `comm::RingBuffer`, `comm::FrameAssembler`,
  `comm::IParameterAccess` (key table, later reused by CANopen),
  `comm::SciProtocol`; each with mock and host unit tests per AGENTS.md §5.

## 8. Error codes (NACK value)

| Code | Meaning |
|---|---|
| 1 | Unknown command |
| 2 | Unknown key |
| 3 | Key is read-only |
| 4 | Value out of range |
| 5 | Not allowed in current state (e.g. opmode change while enabled) **[OPEN]** |

## 9. Safety behaviour

- Communication watchdog: not part of this protocol. A general watchdog is
  added separately later.
- Default after reset: ENABLE = 0, setpoints = 0.
- Setpoint limits (min/max per key) to be defined from hardware ratings. **[OPEN]**

## 10. Examples

Write voltage setpoint 12.0 V (`12.0f` = `0x41400000`):

```
Host → A5 5A  02 00  01 00  00 00 40 41  5A A5
CPU2 → A5 5A  02 80  01 00  00 00 40 41  5A A5   (WRITE_ACK)
```

Read measured current, answer 1.5 A (`0x3FC00000`):

```
Host → A5 5A  01 00  02 01  00 00 00 00  5A A5
CPU2 → A5 5A  01 80  02 01  00 00 C0 3F  5A A5   (READ_RESP)
```

Write to read-only key:

```
Host → A5 5A  02 00  01 01  00 00 00 00  5A A5
CPU2 → A5 5A  FF 80  01 01  03 00 00 00  5A A5   (NACK, key is read-only)
```

## 11. Open points (summary)

1. Command field in frame (12 bytes) — confirm.
2. Byte order: little-endian.
3. Value encoding: float32 vs. scaled int32.
4. Response to writes (ACK) and NACK format as above.
5. STATUS bits.
6. Baud rate, RX FIFO level, inter-byte timeout.
7. Setpoint limits.

Decided (2026-10-29): no CRC; OPMODE 1 = closed loop, 2 = open loop,
3 = reserved, others don't care; no comm watchdog in the protocol (general
watchdog later); first implementation in Simulink CPU02 (Project A).
