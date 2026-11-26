# Fault Injection Notes

## Overview

This document describes the fault-injection scenarios used to validate the STM32F407 embedded system in Renode.

The objective is to evaluate how the firmware responds to peripheral failures, communication errors, abnormal sensor readings, and thermal conditions.

Each experiment documents:

- Fault being injected
- Injection method
- Expected firmware behaviour
- Observed behaviour
- Pass/fail criteria

## Test Environment

| Component | Configuration |
|---|---|
| MCU | STM32F407 |
| CPU | ARM Cortex-M4 |
| RTOS | FreeRTOS |
| Simulation | Renode |
| Temperature sensor | TMP108 |
| IMU | LSM6DSO |
| I2C | I2C1 |
| SPI | SPI1 |
| UART | USART2 with DMA |
| Testing | Robot Framework |

## 1. I2C Fault Injection

### 1.1 Incorrect I2C Slave Address

**Objective:** Verify that the firmware handles an I2C NACK without blocking execution.

**Initial configuration:**

The TMP108 is connected at address `0x48` in the Renode platform description.

**Fault injection:**

Temporarily configure the firmware to use an incorrect address:

```c
#define TMP108_I2C_ADDRESS (0x49U << 1)
```

Rebuild and execute the firmware.

**Expected behaviour:**

- I2C transaction returns a NACK/error.
- Temperature acquisition reports failure.
- Temperature validity is cleared.
- Health monitoring continues.
- Firmware does not hang.

**Expected UART output (illustrative):**

```text
[i2c] addr=0x49 NACK
[temp] read failed
[health] temperature sensor communication failure
```

**Pass criteria:**

The communication error is detected, reported, and handled within the configured timeout.

### 1.2 Missing I2C Peripheral

**Objective:** Verify behaviour when the temperature sensor is physically unavailable.

**Fault injection:**

Remove the TMP108 from the simulated I2C bus:

```text
sysbus.i2c1 Unregister sysbus.i2c1.tmp108
```

**Expected behaviour:**

- Temperature transactions fail.
- `temp_valid` becomes false.
- Health monitor detects the timeout.
- Other tasks continue executing.

**Recovery:**

Restart the simulation with the TMP108 attached and verify that valid readings resume.

**Pass criteria:**

The missing peripheral does not cause indefinite blocking or unintended system reset.

## 2. SPI Fault Injection

### 2.1 IMU Identity Verification

**Objective:** Verify that the firmware correctly identifies the LSM6DSO.

The `WHO_AM_I` register is:

```text
Register: 0x0F
Expected: 0x6C
```

**Expected behaviour:**

- Correct identity is detected during initialization.
- An unexpected identity is reported as an initialization failure.

### 2.2 Missing SPI Peripheral

**Objective:** Verify behaviour when the IMU is unavailable.

**Fault injection:**

Use a separate Renode platform configuration without the LSM6DSO attached to SPI1.

**Expected behaviour:**

- IMU initialization fails or communication times out.
- `imu_valid` becomes false.
- Health monitoring detects the failure.
- Temperature acquisition and telemetry continue.

## 3. UART DMA Validation

### 3.1 Continuous Telemetry

**Objective:** Verify repeated DMA-based UART transmissions.

**Procedure:**

1. Start the firmware.
2. Allow telemetry to execute periodically.
3. Capture USART2 output.
4. Verify message integrity and sequence progression.

**Expected output (illustrative):**

```text
[telemetry] seq=1 temp_mC=25000 imu_valid=1
[telemetry] seq=2 temp_mC=25000 imu_valid=1
[telemetry] seq=3 temp_mC=25000 imu_valid=1
```

### 3.2 DMA Buffer Ownership

**Objective:** Ensure that the transmit buffer is not overwritten while DMA is using it.

**Procedure:**

1. Start a DMA transmission.
2. Attempt to queue another telemetry message.
3. Verify that the active DMA buffer remains unchanged.
4. Verify that the next message is transmitted after the previous transfer completes.

**Pass criteria:**

- No corrupted or interleaved messages.
- DMA completion is handled correctly.
- Subsequent transmissions succeed.

## 4. Thermal Fault Injection

The TMP108 Renode model allows its temperature property to be changed at runtime.

Example:

```text
sysbus.i2c1.tmp108 Temperature 85
```

### 4.1 Temperature Threshold Validation

Inject different temperatures:

```text
sysbus.i2c1.tmp108 Temperature 45
sysbus.i2c1.tmp108 Temperature 55
sysbus.i2c1.tmp108 Temperature 65
sysbus.i2c1.tmp108 Temperature 75
sysbus.i2c1.tmp108 Temperature 85
```

**Expected behaviour:**

| Temperature | Operating mode |
|---|---|
| 45°C | Normal |
| 55°C | Low cooling |
| 65°C | Active cooling |
| 75°C | Throttling |
| 85°C | Critical |

Verify the actual fan duty and acquisition period associated with each mode.

### 4.2 Thermal Throttling

**Objective:** Verify workload adaptation under elevated temperature.

**Expected behaviour:**

| Parameter | Normal | Throttling |
|---|---:|---:|
| IMU period | 10 ms | 50 ms |
| IMU frequency | 100 Hz | 20 Hz |
| Temperature monitoring | Active | Active |
| Fan duty | Temperature-dependent | Increased |

Verify the actual acquisition frequency using timestamps or counters rather than relying only on log messages.

### 4.3 Critical Thermal Mode

**Objective:** Verify the system's response to critical temperature.

**Injection:**

```text
sysbus.i2c1.tmp108 Temperature 85
```

**Expected behaviour:**

- Fan duty reaches 100%.
- Nonessential sensor acquisition is paused.
- Temperature monitoring remains active.
- Health monitoring does not interpret intentional IMU suspension as a failure.

### 4.4 Thermal Recovery

**Objective:** Verify that the system can leave critical mode safely.

**Procedure:**

1. Raise temperature to 85°C.
2. Verify critical mode.
3. Lower temperature below the configured recovery threshold.
4. Verify transition to the appropriate operating mode.
5. Verify IMU acquisition resumes.

Use the implemented hysteresis thresholds when evaluating transitions.

## 5. Health Monitoring

### 5.1 Sensor Timeout

**Objective:** Verify detection of sensors that stop providing successful readings.

**Procedure:**

1. Start the firmware.
2. Interrupt sensor communication.
3. Wait for the configured timeout.
4. Verify health-monitor behaviour.

**Pass criteria:**

- Sensor timeout is detected.
- Invalid sensor data is not treated as valid.
- Recovery or system reset follows the documented fault policy.

### 5.2 Intentional Suspension

**Objective:** Prevent false fault detection during thermal critical mode.

**Procedure:**

1. Enter critical thermal mode.
2. Verify IMU acquisition is paused.
3. Allow the health monitor to execute beyond the normal IMU timeout.
4. Verify that no false IMU timeout is reported.

## 6. Test Results

Record actual experimental results here.

| Test | Result | Evidence |
|---|---|---|
| I2C wrong address | PASS | UART log |
| I2C missing peripheral | PASS | UART log |
| SPI identity verification | PASS | UART log |
| SPI missing peripheral | PASS | UART log |
| UART DMA transmission | PASS | UART capture |
| DMA buffer ownership | PASS | UART capture |
| Thermal thresholds | PASS | Mode and duty logs |
| Thermal throttling | PASS | Acquisition timestamps |
| Critical thermal mode | PASS | UART log |
| Thermal recovery | PASS | UART log |
| Sensor timeout | PASS | Health-monitor log |
| Intentional suspension | PASS | Health-monitor log |

Only change a result to PASS after the corresponding behaviour has been verified.

## Notes

Renode peripheral models may not reproduce every electrical or hardware-level failure. Missing-device, wrong-address, and sensor-property tests validate firmware behaviour against the simulation model. Physical board testing is required to validate electrical faults, signal integrity, and actual DMA timing.