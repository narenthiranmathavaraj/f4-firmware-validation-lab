*** Settings ***
Documentation     Temperature thresholds, fan duty and operating modes (read from telemetry).
Resource          ${CURDIR}/common.resource
Resource          ${RENODEKEYWORDS}
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Test Template     Board At Temperature Should Show

*** Test Cases ***                                      CELSIUS    MODE    DUTY
Cool board keeps the fan off                            25         0       0
Just under the first threshold keeps the fan off        49         0       0
At the first threshold the fan starts gently            50         0       30
Warm board switches to active cooling                   60         1       60
Hot board starts throttling                             70         2       80
Critical temperature runs the fan flat out              80         3       100

Fan Steps Back Down When The Board Cools
    [Template]    NONE
    Power On    85
    Firmware Should Say    thermal mode=3 duty=100
    Set Room Temperature To    25
    Firmware Should Say    thermal mode=0 duty=0

*** Keywords ***
Board At Temperature Should Show
    [Arguments]    ${celsius}    ${mode}    ${duty}
    Power On    ${celsius}
    Firmware Should Say    thermal mode=${mode} duty=${duty}