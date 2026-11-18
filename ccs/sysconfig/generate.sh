#!/bin/sh
# Generate the SysConfig output (board.c/.h, pinmux.csv, ...) for both cores.
#
# Both scripts run in one SysConfig call (multi-core mode), so CPU1 also
# generates the pin mux for the peripherals CPU2 uses (SCIA on GPIO42/43).
#
# Usage: ccs/sysconfig/generate.sh [output_dir]   (default: ccs/sysconfig/generated)
#
# Environment (defaults match toolchain.md / doc/toolchain.md):
#   SYSCONFIG_CLI   SysConfig CLI shipped with CCS 20.0.1
#   C2000WARE_ROOT  full C2000Ware 5.02.00.00 install (the copy in ccs/ext/c2000ware
#                   has no SysConfig metadata)
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-"$HERE/generated"}
SYSCONFIG_CLI=${SYSCONFIG_CLI:-"$HOME/ti/ccs2001/ccs/utils/sysconfig_1.22.0/sysconfig_cli.sh"}
C2000WARE_ROOT=${C2000WARE_ROOT:-"$HOME/ti/c2000/C2000Ware_5_02_00_00"}

"$SYSCONFIG_CLI" --product "$C2000WARE_ROOT/.metadata/sdk.json" \
    --script "$HERE/labpsu_cpu1.syscfg" --context CPU1 --output "$OUT/cpu1" \
    --script "$HERE/labpsu_cpu2.syscfg" --context CPU2 --output "$OUT/cpu2"
