# STM32F4 Embedded Systems Integration & Fault Management

A real-time embedded system built around the STM32F407 (ARM Cortex-M4), demonstrating system-level firmware design using FreeRTOS, register-level peripheral interfaces, DMA-based telemetry, sensor integration, thermal management, and fault handling.

The project focuses on how multiple hardware peripherals and software tasks work together as a reliable embedded system. It is developed in C without vendor HAL libraries and validated using Renode simulation.

## System Overview

The system integrates multiple peripherals and real-time software components:

- **SPI1:** IMU communication (LSM6DSO)
- **I2C1:** Temperature monitoring (TMP108)
- **USART2 + DMA:** Asynchronous UART telemetry and system logging
- **TIM2 PWM:** Temperature-controlled fan output
- **FreeRTOS:** Concurrent sensor acquisition, telemetry, and system health monitoring

## System Architecture

```text
                    STM32F407
                 ARM Cortex-M4
                        |
              +---------+---------+
              |     FreeRTOS      |
              +---------+---------+
                        |
         +--------------+---------------+
         |              |               |
     IMU Task        Temp Task      Telemetry Task
      100 Hz           1 Hz            10 Hz
         |              |               |
       SPI1           I2C1         Log Formatter
         |              |               |
      LSM6DSO         TMP108        UART TX Buffer
                                         |
                                    DMA Transfer
                                         |
                                       USART2
                                         |
                                    Serial Output

                        |
                 Thermal Manager
                        |
                 Operating Modes
                        |
                    TIM2 PWM
                        |
                    Fan Control

                  Health Monitor
                        |
              Timeout / Fault Detection
```

## Key System Features

### 1. Multi-peripheral integration

- Integrated SPI, I2C, UART, DMA, and timer-based PWM peripherals.
- Implemented register-level peripheral interfaces using the STM32F4 reference manual and device datasheets.
- Separated hardware communication drivers from device-specific logic and application-level functionality.
- Implemented bounded communication waits and explicit error reporting.

### 2. Real-time task architecture

FreeRTOS is used to separate system responsibilities into periodic tasks.

| Task | Frequency | Responsibility |
|---|---:|---|
| IMU acquisition | 100 Hz | Read accelerometer data over SPI |
| Temperature acquisition | 1 Hz | Read temperature over I2C |
| Telemetry | 10 Hz | Format and transmit system status |
| Health monitor | 10 Hz | Detect sensor failures and timeouts |

- Used periodic scheduling with `vTaskDelayUntil()`.
- Protected shared sensor state using FreeRTOS mutexes.
- Separated sensor acquisition from telemetry and health monitoring.
- Used asynchronous DMA-based UART transmission to avoid blocking application execution during data transfer.

### 3. DMA-based UART telemetry

Implemented DMA-based UART transmission using USART2 and the STM32F4 DMA controller.

- Configured DMA and USART registers directly without vendor HAL libraries.
- Transferred formatted telemetry from memory to the USART data register using DMA.
- Allowed the CPU to continue executing other tasks while UART transmission was in progress.
- Used DMA transfer completion handling to track transmission status.
- Separated telemetry formatting, transmit-buffer management, and hardware-level data transfer.

### 4. Thermal management and workload adaptation

Implemented temperature-dependent operating modes to manage system workload and cooling.

| Temperature | Operating mode | System behaviour |
|---|---|---|
| Below 50°C | Normal | Full sensor acquisition |
| 50–59°C | Low cooling | Increased fan duty |
| 60–69°C | Active cooling | Increased cooling |
| 70–79°C | Throttling | Reduced nonessential acquisition frequency |
| 80°C and above | Critical | Pause nonessential sensor activity and maximize cooling |

- Generated PWM using the STM32 hardware timer.
- Implemented temperature-based fan control.
- Adapted sensor acquisition frequency according to thermal state.
- Kept temperature monitoring active during critical operation to detect recovery.

### 5. Fault detection and recovery

- Implemented sensor validity tracking and last-success timestamps.
- Added timeout-based detection for unresponsive peripherals.
- Handled I2C NACK conditions and communication errors without indefinite blocking.
- Distinguished intentional task suspension during thermal operation from unexpected sensor failure.
- Implemented system health checks and recovery behaviour.

### 6. Simulation and system validation

Used [Renode](https://renode.io/) to simulate the STM32F4 platform and connected peripherals.

- Validated sensor communication and firmware execution without physical hardware.
- Injected peripheral faults, including incorrect I2C addresses and communication failures.
- Inspected UART logs and peripheral behaviour during execution.
- Used Robot Framework for automated system-level tests.

For detailed test scenarios, fault-injection procedures, expected results,
and execution status, see:

- [System Test Matrix](docs/test_matrix.md)
- [Fault Injection Notes](docs/fault_injection_notes.md)
- [Automated Robot Framework Tests](docs/automated_system_level_testing.md)

## Software Architecture

```text
src/
├── main.c
│
├── startup.c
│
├── drivers/
│   ├── i2c.c
│   ├── i2c.h
│   ├── spi.c
│   ├── spi.h
│   ├── uart_dma.c
│   └── uart_dma.h
│
├── devices/
│   ├── tmp108.c
│   ├── tmp108.h
│   ├── lsm6dso.c
│   ├── lsm6dso.h
│   ├── fan.c
│   └── fan.h
│
└── common/
│   ├── status.c
│   ├── status.h
│   ├── log.c
│   ├── log.h
│   ├── rtos_hook.c
│   ├── mem.c
│   ├── timer.c
│   └── timer.h
│
platform/
│  ├── f4_bringup.repl
│  ├── f4_no_imu.repl
│  ├── f4_no_tmp108.repl
│  ├── f4_bringup.resc
│  ├── reset_macro.resc
│  ├── run_headless.resc
│  └── f4_bringup.resc
│
tests/
│  ├──common.resource
│  ├── smoke.robot
│  ├── spi_faults.robot
│  ├── i2c_faults.robot
│  ├── uart_dma.robot
│  ├── health_monitor.robot
│  └── thermal.robot
│
docs/
│  ├── datasheet_to_code.md
│  ├── fault_injection_notes.md
│  ├── protocol.md
│  └── automated_system_level_testing
```

*Adjust this tree to match the actual repository structure.*

## Design Decisions

| Design area | Approach |
|---|---|
| Firmware architecture | Layered drivers, device interfaces, and application tasks |
| Peripheral access | Direct register-level programming |
| Task scheduling | Periodic FreeRTOS tasks |
| Shared data | Mutex-protected sensor state |
| UART transmission | DMA-based asynchronous TX |
| Telemetry | Buffered messages and transfer completion handling |
| Communication errors | Explicit status codes and bounded timeouts |
| Thermal response | Operating modes and workload adaptation |
| Validation | Renode simulation and automated tests |

### Why register-level programming?

The project avoids vendor HAL libraries to understand the relationship between peripheral registers, communication protocols, timing requirements, and hardware behaviour.

### Why separate FreeRTOS tasks?

Sensor acquisition, telemetry, and health monitoring have different timing requirements. Independent tasks make their responsibilities and execution periods easier to manage.

### Why DMA for UART?

DMA allows telemetry data to be transferred from memory to the UART peripheral without requiring the CPU to transmit every byte. This reduces CPU involvement and allows other real-time tasks to execute concurrently.

### Why fault injection?

Successful communication alone does not demonstrate system reliability. Fault injection helps validate how the firmware behaves when peripherals stop responding or communication fails.

## Build and Run

Requirements:

- ARM GCC toolchain
- CMake / Make
- Renode
- Robot Framework (for automated tests)

```bash
# Build firmware
make

# Run Renode simulation
make run

# Run automated tests
make test
```

Refer to the Makefile and project documentation for the exact commands supported by the repository.

## Future Improvements

- DMA-based SPI acquisition.
- Circular DMA with UART RX.
- Interrupt-driven sensor acquisition.
- Watchdog-based system recovery.
- More comprehensive fault injection and recovery testing.
- Hardware validation on a physical STM32F407 development board.
- Execution-time and CPU-load measurements.

## Technologies

**Languages:** C

**Microcontroller:** STM32F407, ARM Cortex-M4

**RTOS:** FreeRTOS

**Interfaces:** I2C, SPI, UART, PWM, DMA

**Simulation:** Renode

**Testing:** Robot Framework

**Development:** ARM GCC, Make, CMake, Git