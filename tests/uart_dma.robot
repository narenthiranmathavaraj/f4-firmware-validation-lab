*** Settings ***
Documentation     DMA transmit: messages arrive whole, in order, and the buffer is not reused early.
...               Needs the boot-time self-test below, and a platform where USART2 has a
...               DMA request line (not the case in the stock Renode STM32F4 platform).
Resource          ${CURDIR}/common.resource
Resource          ${RENODEKEYWORDS}
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Force Tags        dma    needs-dma-wiring

*** Test Cases ***


Self Test Finishes After Waiting For Every Transfer
    Power On
    Firmware Should Say    DMA: self-test done