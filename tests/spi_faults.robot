*** Settings ***
Documentation     IMU identity check and what the firmware does when the IMU is wrong or gone.
Resource          ${CURDIR}/common.resource
Resource          ${RENODEKEYWORDS}
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown

*** Test Cases ***
Healthy IMU Passes The Identity Check
    Power On
    Firmware Should Say    spi_init=0x00000000
    Firmware Should Say    lsm6dso_init=0x00000000
    Firmware Should Say    accel_config=0x00000000

Missing IMU Fails The Identity Check
    Prepare The Board    ${BOARD_NO_IMU}
    Start The Clock
    Firmware Should Say    spi_init=0x00000000
    Firmware Should Match    lsm6dso_init=0x(?!00000000)[0-9a-f]{8}

Accelerometer Is Not Configured When The IMU Was Not Identified
    Prepare The Board    ${BOARD_NO_IMU}
    Start The Clock
    Firmware Should Match    lsm6dso_init=0x(?!00000000)[0-9a-f]{8}
    Firmware Should Match    accel_config=0x(?!00000000)[0-9a-f]{8}

Missing IMU Is Declared A Fault After The Timeout
    Prepare The Board    ${BOARD_NO_IMU}
    Start The Clock
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FAULT: IMU    3
    Firmware Should Say    FAULT: IMU communication timeout    10

Board Resets After A Declared Sensor Fault
    Prepare The Board    ${BOARD_NO_IMU}
    Start The Clock
    Firmware Should Say    f4-bringup boot
    Firmware Should Say    FAULT: IMU communication timeout    10
    Firmware Should Say    f4-bringup boot    5

IMU Dropping Out While Running Is Detected
    [Documentation]    The Renode SPI IMU model cannot be unplugged mid-run, and it only
    ...                decodes the first SPI transaction (see docs/protocols.md, G7).
    [Tags]    needs-fault-injector
    Skip    Not testable in Renode 1.15.3.