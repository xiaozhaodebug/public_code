#!/bin/bash
# S32K144 Flash script using OpenOCD

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ELF_FILE="$SCRIPT_DIR/build/S32K144_CANFD_UDS.elf"
BIN_FILE="$SCRIPT_DIR/build/S32K144_CANFD_UDS.bin"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "S32K144 Firmware Flash Tool (OpenOCD)"
echo "========================================"

# Check if firmware exists
if [ ! -f "$BIN_FILE" ]; then
    echo -e "${RED}Error: Firmware not found: $BIN_FILE${NC}"
    exit 1
fi

echo ""
echo "Firmware: $BIN_FILE"
echo "Size: $(ls -lh $BIN_FILE | awk '{print $5}')"
echo "Device: S32K144"
echo "Interface: J-Link (SWD)"
echo ""

# Check for JLink
if ! lsusb | grep -q "1366\|J-Link"; then
    echo -e "${YELLOW}Warning: J-Link not detected via USB${NC}"
    echo "Available USB devices:"
    lsusb | grep -i "debug\|jlink\|segger" || echo "  (none detected)"
    echo ""
fi

# Flash using OpenOCD with generic Cortex-M configuration
echo -e "${YELLOW}Starting flash process...${NC}"
echo ""

/usr/bin/openocd -f interface/jlink.cfg -c "transport select swd" \
    -f target/stm32f1x.cfg \
    -c "init" \
    -c "reset halt" \
    -c "flash write_image erase $BIN_FILE 0x00000000" \
    -c "verify_image $BIN_FILE 0x00000000" \
    -c "reset run" \
    -c "shutdown" 2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}Flash successful!${NC}"
    echo ""
    arm-none-eabi-size "$ELF_FILE"
else
    echo ""
    echo -e "${RED}Flash failed!${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Check J-Link USB connection"
    echo "  2. Check SWD wiring (SWDIO, SWCLK, RESET, GND)"
    echo "  3. Ensure target board is powered"
    exit 1
fi
