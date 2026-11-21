*** Settings ***
Resource          ${CURDIR}/common.resource
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
TMP108 Reads A Plausible Temperature
    [Documentation]    Milestone day 2. Remove the Skip once the firmware prints the line.
    Skip    TODO day 2
    Boot Firmware
    # Wait For Line On Uart    tmp108 mC=  (regex on a sane range)
