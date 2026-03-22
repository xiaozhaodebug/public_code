# Minimal S32K144 flash script
adapter driver jlink
transport select swd
adapter speed 4000

# SWD DAP
swd newdap s32k144 cpu -expected-id 0x2ba01477
target create s32k144.cpu cortex_m -dap s32k144.dap

init
halt

# Write binary directly to flash memory
load_image build/S32K144_CANFD_UDS.bin 0x00000000 bin

# Reset and run
reset run
shutdown
