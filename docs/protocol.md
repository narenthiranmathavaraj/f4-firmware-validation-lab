# Protocol notes and simulator gaps

What the buses do on the wire, what the firmware does about it, and where the simulator
(Renode) differs from real hardware. Each gap is marked **Verified** (seen in a run or
register trace) or **Hypothesis** (explanation not yet confirmed).

Environment: Renode 1.15.3 (portable), stock `stm32f4.repl` platform (STM32F40x SVD),
firmware built with arm-none-eabi-gcc, CMSIS device headers.

Reproduce a register trace (Renode monitor or `.resc`):

```
logLevel -1 sysbus.i2c1
sysbus LogPeripheralAccess sysbus.i2c1 true
```

---

## 1. I2C

### Frames

```
write:       S | addr+W | A | data... | A | P
read:        S | addr+R | A | data... | A.. N | P
write_read:  S | addr+W | A | reg | A | Sr | addr+R | A | data... | N | P
```

`write_read` is the usual register read. The repeated START (`Sr`) keeps the bus between the
pointer write and the data read, and the device keeps its register pointer.

### Annotated trace: TMP108 temperature read at 25 C (Renode)

```
START                           S
DR <- 0x90                      address 0x48 << 1 | 0 (write)
DR <- 0x00                      pointer register = temperature
START                           Sr (repeated START, no STOP before it)
DR <- 0x91                      address 0x48 << 1 | 1 (read)
SR1 -> 0x44                     RXNE | BTF
CR1 <- 0x201                    STOP set, before the data bytes are read
DR -> 0x19                      high byte (25 C)
DR -> 0x00                      low byte: model FIFO empty, see G1
```

Receive sequence by byte count (RM0090, I2C master receiver): 1 byte clears ACK before ADDR is
cleared; 2 bytes use POS and clear ACK after ADDR; 3 or more keep ACK on until N-2.

---

## 2. SPI (LSM6DSO)

### Frame

First byte, sent MSB first:

```
[RW] [AD6 AD5 AD4 AD3 AD2 AD1 AD0]      read  = 0x80 | address
                                         write = 0x00 | address
```

The datasheet numbers the RW bit as "bit 0" because it is the first bit on the wire, which is
bit 7 of the byte (ST DS12140 Rev 2, I2C/SPI interface section; confirm the page in your PDF).
Written as `(addr << 1) | 1`, the byte selects the wrong register and `WHO_AM_I` fails with
`ERR_BADID` (verified in Renode).

Mode: CPOL = 1, CPHA = 1 (mode 3), as used in the datasheet figures. Prescaler 0..7 selects
fPCLK / 2 .. fPCLK / 256.

---

## 3. Simulator gaps

| Warning or symptom in the Renode log | Gap |
|---|---|
| `i2c1: Tried to read from an empty fifo` | G1, G3 |
| `i2c1: Unhandled write ... bits [11] ... 0xC01` | G2 |
| `gpioPortB: Unhandled write to offset 0x4 ... OT6, OT7` | G6 |
| `i2c1.tmp108: Unhandled write ... CONF.conv_rate / CONF.mode` | G5 |
| `spi1: Slave mode is not supported` | G8 |
| `spi1.imu: Unhandled read from offset 0x13 ...` | G7 |

### G1. TMP108: the low byte of a 2-byte read is never delivered (Verified)
- **Symptom:** the second `DR` read logs `Tried to read from an empty fifo` and returns 0.
- **Evidence:** trace in section 1. Setting `Temperature 25.5` still gives raw `0x1900`.
- **Correction:** an earlier note said the model returns whole degrees only. The trace shows the
  second byte is not delivered; for whole-degree temperatures the missing byte reads as 0, which
  looks identical to a real 0.
- **Impact:** fractional temperatures cannot be tested in Renode.
- **Workaround:** keep the raw-to-milli-degree conversion a pure function and test it on the host
  with values from the datasheet's temperature format table.

### G2. I2C `CR1.POS` is ignored (Verified)
- **Symptom:** `Unhandled bits: [11] when writing value 0xC01` (POS | ACK | PE) on each 2-byte read.
- **Impact:** the 2-byte read still returns the first byte. I tried the classic sequence without
  POS; it read raw `0x0000` in my test (cause not investigated), so the driver keeps the POS
  sequence the reference manual specifies.

### G3. Reads of 3 or more bytes time out (Verified)
- **Symptom:** `ERR_TIMEOUT` on the last byte, plus the empty-FIFO warning.
- **Cause:** the model ends the transfer when STOP is set, so the last byte never arrives. The
  driver follows RM0090 (STOP set after reading N-2).
- **Impact:** only 1 and 2 byte reads are testable. The TMP108 needs at most 2.

### G4. I2C error interrupt is not raised on NACK (Verified, prototype)
- **Symptom:** interrupt-driven write to an absent address returned `ERR_TIMEOUT`; the event
  interrupt fired but the error interrupt (IRQ 32) did not.
- **Workaround:** check `SR1.AF` in the event handler too (harmless on real hardware).
- **Also:** the model re-enters the event interrupt hundreds of times per byte, so do not use the
  event count as a metric.

### G5. TMP108 configuration bits are ignored (Verified)
- **Symptom:** `CONF.conv_rate` and `CONF.mode` writes are logged as unhandled.
- **Impact:** shutdown and one-shot modes cannot be verified, so a sensor-side power saving
  cannot be measured in the simulator.

### G6. GPIO output type (open-drain) is ignored (Verified)
- **Symptom:** unhandled `OTYPER` bits for PB6 and PB7.
- **Impact:** the wired-AND behaviour of I2C and the need for pull-ups are not modelled. Real
  hardware needs open-drain pins and about 4.7 kOhm pull-ups.

### G7. SPI IMU: only the first transaction after boot is decoded (Verified symptom, Hypothesis cause)
- **Evidence:**
  - `WHO_AM_I` as the first transaction: OK.
  - `CTRL1_XL` write after it: `IsAccelerometerPoweredOn` stays false, so accel reads return 0.
  - `CTRL1_XL` write first: the accelerometer powers on, but a following `WHO_AM_I` read gives
    `ERR_BADID`.
  - PA4 wired to the IMU in the `.repl` is rejected (`does not implement IGPIOReceiver`).
  - Toggling `SPE` or `SSI` between transactions made no difference.
  - A second transaction produces `Unhandled read from offset 0x13 ...`.
- **Hypothesis:** the SPI controller model never signals chip-select to the device, so the IMU
  model cannot tell where a transaction ends and keeps auto-incrementing its register pointer.
  Not traced to the source.
- **Impact:** SPI framing and `WHO_AM_I` can be verified; live accelerometer data cannot.
- **Options:** verify accel on hardware; use a synthetic IMU source for the control logic; or use
  an IMU model on I2C (`LSM9DS1_IMU`, `ADXL345` and `LIS2DW12` load on an I2C bus; not driven yet).

### G8. `spi1: Slave mode is not supported` (Hypothesis, benign)
- The model supports master only. The warning probably appears while `MSTR` is not yet set during
  `spi_init`; the final configuration is master and transfers work.

### G9. Timing and clocks are not real (Verified)
- The stock platform runs SysTick at 72 MHz, while the drivers assume the 16 MHz HSI reset clock.
  No waveforms and no electrical timing: evidence is register traces and device-level behaviour.

### G10. Environment: SVD download (Verified)
- The stock STM32F4 platform downloads an SVD file the first time it loads, so it needs internet
  once (cached afterwards) and fails in an offline CI without a warmed cache.