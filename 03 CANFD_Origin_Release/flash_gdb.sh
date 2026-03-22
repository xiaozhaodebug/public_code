#!/bin/bash
# Flash S32K144 using OpenOCD + GDB

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ELF_FILE="$SCRIPT_DIR/build/S32K144_CANFD_UDS.elf"

echo "========================================"
echo "S32K144 GDB Flash Tool"
echo "========================================"

# Start OpenOCD in background
/usr/bin/openocd -s /usr/share/openocd/scripts \
    -f interface/jlink.cfg \
    -c "transport select swd" \
    -c "adapter speed 2000" \
    -c "source [find target/swj-dp.tcl]" \
    -c "swj_newdap s32k144 cpu -irlen 4 -ircapture 0x1 -irmask 0xf -expected-id 0x2ba01477" \
    -c "dap create s32k144.dap -chain-position s32k144.cpu" \
    -c "target create s32k144.cpu cortex_m -dap s32k144.dap" \
    -c "init" \
    -c "halt" \
    > /tmp/openocd.log 2>&1 &

OPENOCD_PID=$!
sleep 3

echo "OpenOCD started (PID: $OPENOCD_PID)"
echo "Connecting GDB..."

# Use GDB to flash
gdb-multiarch "$ELF_FILE" -batch \
    -ex "target remote localhost:3333" \
    -ex "monitor reset halt" \
    -ex "load" \
    -ex "monitor reset run" \
    -ex "quit"

GDB_RESULT=$?

# Kill OpenOCD
kill $OPENOCD_PID 2>/dev/null
wait $OPENOCD_PID 2>/dev/null

if [ $GDB_RESULT -eq 0 ]; then
    echo ""
    echo "[+] Flash successful!"
else
    echo ""
    echo "[-] Flash failed!"
    echo "OpenOCD log:"
    cat /tmp/openocd.log
fi
