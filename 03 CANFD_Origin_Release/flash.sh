#!/bin/bash
# S32K144 Flash script using JLink

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ELF_FILE="$SCRIPT_DIR/build/S32K144_CANFD_UDS.elf"
BIN_FILE="$SCRIPT_DIR/build/S32K144_CANFD_UDS.bin"
JLINK_SCRIPT="$SCRIPT_DIR/flash.jlink"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "S32K144 Firmware Flash Tool"
echo "========================================"

# Check if JLinkExe exists
if ! command -v JLinkExe &> /dev/null; then
    echo -e "${RED}Error: JLinkExe not found${NC}"
    echo "Please install J-Link software from:"
    echo "https://www.segger.com/downloads/jlink/"
    exit 1
fi

# Check if firmware exists
if [ ! -f "$BIN_FILE" ]; then
    echo -e "${RED}Error: Firmware not found: $BIN_FILE${NC}"
    echo "Please build the project first:"
    echo "  cd build && make"
    exit 1
fi

echo ""
echo "Firmware: $BIN_FILE"
echo "Device: S32K144"
echo "Interface: JTAG/SWD"
echo ""

# Flash firmware
echo -e "${YELLOW}Flashing firmware...${NC}"
JLinkExe -device S32K144 -if swd -speed 4000 -autoconnect 1 \
    -CommanderScript "$JLINK_SCRIPT"

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}Flash successful!${NC}"
    echo ""
    # Show firmware size
    arm-none-eabi-size "$ELF_FILE"
else
    echo ""
    echo -e "${RED}Flash failed!${NC}"
    exit 1
fi
