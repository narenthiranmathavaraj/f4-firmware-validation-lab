*** Settings ***
Documentation     Does the firmware come up at all, and do all four tasks get started?
Resource          ${CURDIR}/common.resource
Resource          ${RENODEKEYWORDS}
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown

*** Test Cases ***
Board Boots And Prints Its Banner
    Power On
    Firmware Should Say    f4-bringup boot
    Firmware Should Say    BOOT_OK

Both Buses Come Up On A Healthy Board
    Power On
    Firmware Should Say    i2c_init=0x00000000
    Firmware Should Say    tmp108_init=0x00000000
    Firmware Should Say    spi_init=0x00000000
    Firmware Should Say    lsm6dso_init=0x00000000

All Tasks Are Created And The Scheduler Starts
    Power On
    Firmware Should Say    ALL TASKS CREATED
    Firmware Should Say    Starting FreeRTOS scheduler

Telemetry Starts Flowing With Valid Sensors
    Power On    25
    Firmware Should Say    TEMP valid=1 temp_mC=25000
    Firmware Should Say    IMU valid=1

Nothing Fatal Is Printed After Startup
    Power On
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FATAL    5