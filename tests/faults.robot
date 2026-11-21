*** Settings ***
Resource          ${CURDIR}/common.resource
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Wrong Address Returns NACK Not A Hang
    [Documentation]    Milestone day 4. Remove the Skip once the firmware prints the line.
    Skip    TODO day 4
    Boot Firmware
    # Wait For Line On Uart    i2c fault: NACK
