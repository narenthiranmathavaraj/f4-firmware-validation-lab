# Automated System-Level Testing

## Overview

This directory contains Robot Framework test cases for validating the STM32F407 embedded system in Renode.

The tests automate system-level scenarios involving:

- Firmware startup
- I2C and SPI communication
- Peripheral failure handling
- UART DMA telemetry
- Thermal management
- Sensor health monitoring

The objective is to make system validation repeatable and to detect regressions when firmware changes.

## Test Architecture

```text
              Robot Framework
                     |
              Test Cases (.robot)
                     |
              Reusable Keywords
                     |
                Renode CLI
                     |
             STM32F407 Platform
                     |
              Firmware (.elf)
                     |
             UART Output Capture
                     |
             Test Assertions
                     |
              PASS / FAIL
```

## Directory Structure

```text
tests/
├── smoke.robot
├── common.resource
├── i2c_faults.robot
├── spi_faults.robot
├── uart_dma.robot
├── thermal.robot
└── health_monitor.robot
```

## Test Suites

| Test suite | Validation |
|---|---|
| `smoke.robot` | Firmware boot and task startup |
| `i2c_faults.robot` | I2C NACK, missing sensor, recovery |
| `spi_faults.robot` | IMU identity and communication failures |
| `uart_dma.robot` | DMA transmission and buffer handling |
| `thermal.robot` | Temperature thresholds and operating modes |
| `health_monitor.robot` | Sensor timeout and intentional suspension |

## Test Execution

### Requirements

- ARM GNU Toolchain
- Renode
- Python 3
- Robot Framework


### Build Firmware

From the project root:

```bash
make
```

### Run Individual Test Suites

```bash
renode-test tests/smoke.robot

renode-test tests/robot/i2c_faults.robot

renode-test tests/robot/spi_faults.robot

renode-test tests/robot/uart_dma.robot

renode-test tests/robot/thermal.robot

renode-test tests/robot/health_monitor.robot
```

### Run All Tests

```bash
make test
```

### Generate Reports

Robot Framework automatically generates:

```text
report.html
log.html
output.xml
```

These contain the overall test results, detailed execution logs, and machine-readable test output.

## Test Design Principles

### 1. Independent execution

Each test should begin with a known firmware and simulation state.

### 2. Deterministic behaviour

Use controlled peripheral inputs and bounded timeouts rather than arbitrary delays wherever possible.

### 3. Observable results

Verify behaviour using UART output, sensor status, acquisition timestamps, and other measurable indicators.

### 4. Failure isolation

A failure in one test should not contaminate the next test. Restart the simulation or reload a clean platform configuration when necessary.

### 5. Explicit pass criteria

A test passes only when the expected system behaviour is observed within its configured timeout.

## Limitations

Robot Framework coordinates test execution and evaluates results; it does not itself simulate STM32 peripherals.

Renode provides the simulated hardware environment, while the firmware implements the actual fault detection and recovery logic.

The automated results therefore validate firmware behaviour against the supported Renode models. Physical hardware testing is still required for electrical characteristics and real-world peripheral timing.

## Related Documentation
- [Fault Injection Notes](docs/fault_injection_notes.md)
- [Automated Robot Framework Tests](docs/automated_system_level_testing.md)