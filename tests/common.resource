*** Variables ***
${REPL}     ${CURDIR}/../platform/f4_bringup.repl
${ELF}      ${CURDIR}/../build/f4-bringup.elf
${UART}     sysbus.usart2

*** Keywords ***
Boot Firmware
    Execute Command    mach create "f4-bringup"
    Execute Command    machine LoadPlatformDescription @${REPL}
    Execute Command    sysbus LoadELF @${ELF}
    Execute Command    sysbus.cpu VectorTableOffset 0x08000000
    Create Terminal Tester    ${UART}    defaultPauseEmulation=True
    Start Emulation
