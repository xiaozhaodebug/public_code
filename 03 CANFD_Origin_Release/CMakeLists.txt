cmake_minimum_required(VERSION 3.16)
project(S32K144_CANFD_UDS C ASM)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")

# Source files
set(SOURCES
    # Main application
    Sources/main.c
    Sources/can_fd_wrapper.c
    Sources/delay.c
    Sources/key.c
    Sources/sys_cfg.c
    Sources/uart.c
    
    # UDS Stack
    Sources/uds/uds_core.c
    Sources/uds/uds_dtc.c
    Sources/uds/uds_nrc.c
    Sources/uds/uds_pending.c
    Sources/uds/uds_routine.c
    Sources/uds/uds_security.c
    Sources/uds/uds_session.c
    Sources/uds/uds_tp.c
    Sources/uds/uds_user_config_example.c
    Sources/uds/services/uds_svc_10.c
    Sources/uds/services/uds_svc_19.c
    Sources/uds/services/uds_svc_22.c
    Sources/uds/services/uds_svc_27.c
    Sources/uds/services/uds_svc_2e.c
    Sources/uds/services/uds_svc_31.c
    
    # Generated Code (Processor Expert)
    Generated_Code/can_pal0.c
    Generated_Code/clockMan1.c
    Generated_Code/Cpu.c
    Generated_Code/dmaController1.c
    Generated_Code/lpit1.c
    Generated_Code/lpuart1.c
    Generated_Code/pin_mux.c
    
    # SDK - Device
    SDK/platform/devices/S32K144/startup/system_S32K144.c
    SDK/platform/devices/startup.c
    
    # SDK - Drivers
    SDK/platform/drivers/src/clock/S32K1xx/clock_S32K1xx.c
    SDK/platform/drivers/src/edma/edma_driver.c
    SDK/platform/drivers/src/edma/edma_hw_access.c
    SDK/platform/drivers/src/edma/edma_irq.c
    SDK/platform/drivers/src/flexcan/flexcan_driver.c
    SDK/platform/drivers/src/flexcan/flexcan_hw_access.c
    SDK/platform/drivers/src/flexcan/flexcan_irq.c
    SDK/platform/drivers/src/interrupt/interrupt_manager.c
    SDK/platform/drivers/src/lpit/lpit_driver.c
    SDK/platform/drivers/src/lpuart/lpuart_driver.c
    SDK/platform/drivers/src/lpuart/lpuart_hw_access.c
    SDK/platform/drivers/src/lpuart/lpuart_irq.c
    SDK/platform/drivers/src/pins/pins_driver.c
    SDK/platform/drivers/src/pins/pins_port_hw_access.c
    
    # SDK - PAL
    SDK/platform/pal/src/can/can_pal.c
    
    # SDK - RTOS
    SDK/rtos/osif/osif_baremetal.c
    
    # Startup
    Project_Settings/Startup_Code/startup_S32K144.S
)

# Include directories
set(INCLUDE_DIRS
    include
    Sources
    Sources/uds
    Sources/uds/services
    Generated_Code
    SDK/platform/devices/common
    SDK/platform/devices/S32K144/include
    SDK/platform/devices/S32K144/startup
    SDK/platform/devices
    SDK/platform/drivers/inc
    SDK/platform/drivers/src/clock/S32K1xx
    SDK/platform/drivers/src/edma
    SDK/platform/drivers/src/flexcan
    SDK/platform/drivers/src/interrupt
    SDK/platform/drivers/src/lpit
    SDK/platform/drivers/src/lpuart
    SDK/platform/drivers/src/pins
    SDK/platform/pal/inc
    SDK/rtos/osif
)

# Executable
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INCLUDE_DIRS})

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Project_Settings/Linker_Files/S32K144_64_flash.ld)
target_link_options(${PROJECT_NAME}.elf PRIVATE 
    -T${LINKER_SCRIPT}
    -Wl,-Map=${PROJECT_NAME}.map
)

# Post-build commands
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating HEX and BIN files..."
)

# Print memory usage
add_custom_target(size
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    DEPENDS ${PROJECT_NAME}.elf
)

# Flash target using JLink
add_custom_target(flash
    COMMAND ${CMAKE_SOURCE_DIR}/flash.sh
    DEPENDS ${PROJECT_NAME}.elf
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Flashing firmware to S32K144 via JLink..."
)

# Alternative: Direct JLink command
add_custom_target(flash-direct
    COMMAND JLinkExe -device S32K144 -if swd -speed 4000 -autoconnect 1 
            -CommanderScript ${CMAKE_SOURCE_DIR}/flash.jlink
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing firmware via JLink..."
)
