*** Settings ***
Resource          ${CURDIR}/common.resource
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Should Boot And Print Banner
    Boot Firmware
    Wait For Line On Uart    f4-bringup boot
    Wait For Line On Uart    BOOT_OK
