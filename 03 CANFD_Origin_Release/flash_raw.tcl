# Generic Cortex-M flash script for S32K144
interface jlink
transport select swd
adapter speed 4000

# Create SWD DP and DAP
set CPUTAPID 0x2ba01477
swj_newdap s32k144 cpu -irlen 4 -ircapture 0x1 -irmask 0xf -expected-id $CPUTAPID
dap create s32k144.dap -chain-position s32k144.cpu

# Create target
target create s32k144.cpu cortex_m -dap s32k144.dap

# Initialize
init

# Halt the target
halt

# Flash the binary (raw write to flash)
flash write_image erase build/S32K144_CANFD_UDS.bin 0x00000000

# Verify
verify_image build/S32K144_CANFD_UDS.bin 0x00000000

# Reset and run
reset run

# Shutdown
shutdown
