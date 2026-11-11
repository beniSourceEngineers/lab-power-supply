# On-target tests

CCS test projects that run on the LAUNCHXL-F28379D (AGENTS.md §5). They use the
production code from `ccs/common/` and the linker command files of `ccs/cpu*/`.

## IPC test (`ipc_cpu1`, `ipc_cpu2`)

Runs both cores against each other with the real IPC hardware (Phase 5):

| Channel | Direction | How it is used |
|---|---|---|
| `Mailbox` (`Pattern`) | CPU1 → CPU2 | Written at 10 kHz in a timer ISR, read in the IPC1 interrupt on CPU2; checks for torn messages and wrong order |
| `Mailbox` (`Cpu2Report`) | CPU2 → CPU1 | CPU2's counters, polled by CPU1 |
| `Queue` (`Event`) | CPU2 → CPU1 | Running numbers as fast as possible; checks for lost, doubled or wrong elements |
| `CommandLink` | CPU1 → CPU2 | CPU2 answers with `data + 1`; CPU1 checks the response |

Both cores also check that the channels are at the addresses from
`ipc/MsgRamSections.hpp`.

### Run

1. *File → Import Projects…*, select `test/target/`, then import
   `labpsu_ipc_test_cpu1` and `labpsu_ipc_test_cpu2` without copying them.
2. Build both projects (configurations `CPU1_RAM` and `CPU2_RAM`).
3. In the *Run and Debug* view, start `IPC test RAM (CPU1 + CPU2)`. It is
   committed in `ipc_cpu1/.theia/launch.json` and `ipc_cpu2/.theia/launch.json`
   (same content), uses `ipc_cpu1/targetConfigs/TMS320F28379D.ccxml`, and loads
   `labpsu_ipc_test_cpu1.out` on CPU1 and `labpsu_ipc_test_cpu2.out` on CPU2.
   Reset both cores, then run both. See `ccs/README.md` (*Open in CCS*) if CCS
   reports that `launch.json` is not valid.

### Result

| LED1 | `ipcTestResult.verdict` | Meaning |
|---|---|---|
| Toggles every 100 ms | 0 | Running |
| Toggles every 1 s | 1 | **Passed**: at least 100000 exchanges of each kind and no errors |
| Steady on | 2 | **Failed**: see the error counters in `ipcTestResult` |

The pattern mailbox needs the longest, about 10 s at 10 kHz. If LED1 keeps
toggling every 100 ms for much longer, watch the counters in `ipcTestResult` on
CPU1 to see which exchange has stopped.
