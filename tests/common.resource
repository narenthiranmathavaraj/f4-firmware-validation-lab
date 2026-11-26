*** Settings ***
Documentation     Shared steps for the f4-bringup firmware tests.

*** Variables ***
${PLATFORMS}        ${CURDIR}/../platform
${ELF}              ${CURDIR}/../build/f4-bringup.elf
${UART}             sysbus.usart2
${NORMAL_BOARD}     ${PLATFORMS}/f4_bringup.repl
${BOARD_NO_TEMP}    ${PLATFORMS}/f4_no_tmp108.repl
${BOARD_NO_IMU}     ${PLATFORMS}/f4_no_imu.repl
${LOG_DIR}          ${CURDIR}/../logs

*** Keywords ***
Prepare The Board
    [Arguments]    ${board}=${NORMAL_BOARD}
    Execute Command    mach create "f4-bringup"
    Execute Command    machine LoadPlatformDescription @${board}
    # nice find
    Execute Script    ${PLATFORMS}/reset_macro.resc
    Execute Command    sysbus LoadELF @${ELF}
    Execute Command    sysbus.cpu VectorTableOffset 0x08000000
    #Execute Command    logFile @${LOG_DIR}/renode_run.log
    #Execute Command    ${UART} CreateFileBackend @${LOG_DIR}/uart.log true
    Create Terminal Tester    ${UART}    defaultPauseEmulation=True

Set Room Temperature To
    [Arguments]    ${celsius}
    Execute Command    sysbus.i2c1.tmp108 Temperature ${celsius}

Start The Clock
    Start Emulation

Power On
    [Documentation]    Healthy board by default, at the given room temperature.
    [Arguments]    ${celsius}=25    ${board}=${NORMAL_BOARD}
    Prepare The Board    ${board}
    Set Room Temperature To    ${celsius}
    Start The Clock

Firmware Should Say
    [Arguments]    ${text}    ${within_seconds}=20
    ${pattern}=    Replace String    ${text}    =    .
    Wait For Line On Uart    ${pattern}    treatAsRegex=true    timeout=${within_seconds}

Firmware Should Match
    [Documentation]    Same, but the text is a regular expression.
    [Arguments]    ${pattern}    ${within_seconds}=20
    ${safe}=    Replace String    ${pattern}    =    .
    Wait For Line On Uart    ${safe}    treatAsRegex=true    timeout=${within_seconds}

Firmware Should Stay Quiet About
    [Arguments]    ${text}    ${for_seconds}=10
    Should Not Be On Uart    ${text}    timeout=${for_seconds}

# --- kept so the older test files still work ---
Prepare Machine
    Prepare The Board

Boot Firmware
    Prepare The Board
    Start The Clock