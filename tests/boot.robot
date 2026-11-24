
*** Settings ***
Documentation    STM32F4 firmware boot integration test

*** Variables ***
${UART}    sysbus.usart2

*** Test Cases ***
STM32F4 Firmware Boot Test

    # Load platform, firmware and start simulation
    Execute Command    include @/workspaces/f4-bringup/platform/f4_bringup.resc
    # Attach UART tester
    Create Terminal Tester    ${UART}

    # Verify boot messages
    Wait For Line On Uart    f4-bringup boot
    Wait For Line On Uart    BOOT_OK

    # Verify I2C and TMP108 initialization
    Wait For Line On Uart    i2c_init status=0x00000000
    Wait For Line On Uart    tmp108_init status=0x00000000
    Wait For Line On Uart    lsm6dso_init status=0x00000000

    # Verify SPI and IMU initialization
    Wait For Line On Uart    spi_init status=0x00000000
    Wait For Line On Uart    lsm6dso_init status=0x00000000

    # Verify firmware completed initialization
    Wait For Line On Uart    ALL_INIT_TESTS_COMPLETED

    Execute Command    pause
