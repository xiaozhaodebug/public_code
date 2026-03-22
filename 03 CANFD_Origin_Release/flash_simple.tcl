# S32K144 Flash script
adapter driver jlink
transport select swd
adapter speed 4000

# Create target
target create s32k144 cortex_m -dap [dap create s32k144.dap -chain-position s32k144.cpu]

# Detect using SWD
swd newdap s32k144 cpu -expected-id 0x2ba01477

init
halt

# Write flash
flash write_image erase build/S32K144_CANFD_UDS.bin 0x00000000 bin

# Verify
verify_image build/S32K144_CANFD_UDS.bin 0x00000000 bin

# Reset and run
reset run
shutdown
