*** Settings ***
Documentation     The sensor watchdog: timeouts, resets, and the intentional IMU pause.
Resource          ${CURDIR}/common.resource
Resource          ${RENODEKEYWORDS}
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown

*** Test Cases ***
Healthy System Never Declares A Fault
    Power On
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FAULT    15

Missing IMU Trips The Five Second Timeout
    Prepare The Board    ${BOARD_NO_IMU}
    Start The Clock
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FAULT: IMU    3
    Firmware Should Say    FAULT: IMU communication timeout    10

Missing Temperature Sensor Trips The Ten Second Timeout
    Prepare The Board    ${BOARD_NO_TEMP}
    Start The Clock
    Firmware Should Say    Starting FreeRTOS scheduler
    Firmware Should Stay Quiet About    FAULT: TMP108    8
    Firmware Should Say    FAULT: TMP108 communication timeout    10

Board Restarts After A Declared Fault
    Prepare The Board    ${BOARD_NO_IMU}
    Start The Clock
    Firmware Should Say    f4-bringup boot
    Firmware Should Say    FAULT: IMU communication timeout    15
    Firmware Should Say    f4-bringup boot    5

IMU Pause In Critical Mode Is Not Treated As A Fault
    [Documentation]    Fails on the current code: pausing the IMU stops last_imu_success,
    ...                so the health task resets the board about 5 s later.
    Power On    85
    Firmware Should Say    thermal mode=3 duty=100
    Firmware Should Stay Quiet About    FAULT: IMU    12

IMU Resumes After The Board Cools Down
    Power On    85
    Firmware Should Say    thermal mode=3 duty=100
    Firmware Should Say    IMU valid=0
    Set Room Temperature To    25
    Firmware Should Say    thermal mode=0 duty=0
    Firmware Should Say    IMU valid=1    10


Leaving Critical Mode After A Long Pause Does Not Cause A Timeout
    [Documentation]    Regression test for the gap between "temperature dropped" and
    ...                "IMU task refreshed its timestamp".
    Power On    85
    Firmware Should Say    thermal mode=3 duty=100
    Firmware Should Stay Quiet About    FAULT: IMU    8
    Set Room Temperature To    25
    Firmware Should Say    thermal mode=0 duty=0
    Firmware Should Stay Quiet About    FAULT: IMU    3