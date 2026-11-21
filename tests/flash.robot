*** Settings ***
Resource          ${CURDIR}/common.resource
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Flash Erase Program Verify Round Trip
    [Documentation]    Milestone day 3. Remove the Skip once the firmware prints the line.
    Skip    TODO day 3
    Boot Firmware
    # Wait For Line On Uart    flash verify OK
