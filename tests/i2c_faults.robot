*** Settings ***
Documentation     What happens when the temperature sensor is not answering on I2C.
Resource          ${CURDIR}/common.resource
Resource          ${RENODEKEYWORDS}
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown

*** Test Cases ***
Missing Sensor Is Reported During Init
    [Documentation]    Assumes tmp108_init() talks to the bus, so a NACK shows up as an error.
    Prepare The Board    ${BOARD_NO_TEMP}
    Start The Clock
    Firmware Should Say    i2c_init=0x00000000
    Firmware Should Match    tmp108_init=0x(?!00000000)[0-9a-f]{8}

System Keeps Running Without The Sensor
    Prepare The Board    ${BOARD_NO_TEMP}
    Start The Clock
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FATAL    5

Lost Sensor Is Declared A Fault After The Timeout
    Prepare The Board    ${BOARD_NO_TEMP}
    Start The Clock
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FAULT: TMP108    8
    Firmware Should Say    FAULT: TMP108 communication timeout    10

Board Resets After A Declared Sensor Fault
    Prepare The Board    ${BOARD_NO_TEMP}
    Start The Clock
    Firmware Should Say    f4-bringup boot
    Firmware Should Say    FAULT: TMP108 communication timeout    10
    Firmware Should Say    f4-bringup boot    5

Fan Stays At Full Speed When The Sensor Is Missing
    [Documentation]    No temperature reading means no reason to trust a cool board.
    Prepare The Board    ${BOARD_NO_TEMP}
    Start The Clock
    Firmware Should Say    thermal mode=0 duty=100

Sensor Coming Back Is Picked Up Again
    [Documentation]    Needs a way to unplug and replug the sensor while the clock runs.
    ...                There is no verified Renode command for that, so this stays skipped
    ...                until the firmware has a fault-injection mailbox.
    [Tags]    needs-fault-injector
    Skip    Cannot unplug the sensor mid-run yet.