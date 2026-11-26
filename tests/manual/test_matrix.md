## System-Level Testing & Validation

The firmware is validated using a combination of manual fault injection in Renode and automated system-level testing with Robot Framework.

The testing strategy focuses on verifying normal peripheral operation, fault detection, error handling, recovery, real-time task behaviour, and thermal management.

### Testing Approach

Testing is performed in two stages:

1. **Test planning and manual validation:** Define test scenarios, inject faults using Renode, and verify expected firmware behaviour.
2. **Automated validation:** Implement repeatable Robot Framework test cases to execute scenarios and verify UART logs and system responses.

### Test Matrix

| Test ID | Test scenario | Expected behaviour |
|---|---|---|
| T01 | Boot smoke test | Firmware boots, tasks start, UART logging works |
| T02 | I2C normal communication | TMP108 responds and temperature is decoded correctly |
| T03 | I2C wrong address | NACK detected without blocking execution |
| T04 | I2C missing device | Sensor marked invalid and communication failure detected |
| T05 | I2C recovery | Valid temperature readings resume after device restoration |
| T06 | SPI IMU communication | IMU identity and sensor data are read correctly |
| T07 | SPI missing device | IMU initialization or communication failure handled |
| T08 | UART DMA transmission | Telemetry transmitted correctly and DMA completion handled |
| T09 | Thermal threshold validation | Correct fan duty and operating modes at temperature boundaries |
| T10 | Thermal throttling | IMU acquisition frequency reduced under high temperature |
| T11 | Critical thermal mode | Nonessential acquisition paused and cooling set to maximum |
| T12 | Health monitoring | Sensor timeouts detected without false failures during intentional suspension |

### Fault Injection

Renode is used to reproduce peripheral and operating-condition failures.

Examples include:

- Incorrect I2C slave address to generate a NACK.
- Removing the TMP108 peripheral from the simulated I2C bus.
- Changing TMP108 temperature to test thermal operating modes.
- Running the firmware without the IMU peripheral.
- Simulating sensor communication failures and timeout conditions.

Fault injection is used to verify that communication errors are handled without indefinite blocking and that unrelated system functions continue operating.

### Automated Testing with Robot Framework

Robot Framework is used to automate repeatable system-level test scenarios.

```text
tests/
├── manual/
│   ├── test_matrix.md
│   └── fault_injection_notes.md
│
├── robot/
│   ├── smoke.robot
│   ├── i2c_faults.robot
│   ├── spi_faults.robot
│   ├── uart_dma.robot
│   ├── thermal.robot
│   └── health_monitor.robot
│
└── resources/
    └── renode_keywords.robot
```

**Test execution workflow:**

1. Build firmware using ARM GCC.
2. Launch the STM32F4 simulation in Renode.
3. Load the compiled ELF.
4. Execute Robot Framework test cases.
5. Inject peripheral faults and operating-condition changes.
6. Capture UART logs and verify expected system responses.
7. Generate automated test reports.

Run the automated test suite:

```bash
robot tests/robot/
```

Robot Framework generates:

- `report.html` — Overall test results.
- `log.html` — Detailed execution logs.
- `output.xml` — Machine-readable test results.

### Validation Scope and Limitations

Renode simulation validates firmware behaviour against the available peripheral models. It does not replace physical hardware validation of electrical faults, signal integrity, real-time timing, or actual DMA performance.

Physical STM32F407 board validation is a future extension of this project.