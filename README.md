# f4-bringup

Register-level I2C / SPI / UART drivers for an STM32F4 (Cortex-M4), written from the reference
manual and device datasheets (no vendor HAL), run and tested entirely in [Renode](https://renode.io).

> **One-line pitch (edit):** Bare-metal bring-up of an I2C temperature sensor, an SPI NOR flash and
> an SPI IMU on a simulated STM32F4, with fault injection, Robot Framework CI and bus traces.

## Results (fill with real output)

```
f4-bringup boot
[i2c] addr=0x48 W 00 -> ACK
[i2c] addr=0x48 R 19 00
tmp108 raw=0x1900 mC=25000
[spi1] JEDEC: 20 ba 19
flash verify OK (256 B @ 0x000000)
i2c fault: NACK (addr=0x49) in 3 ms, no hang
```

CI badge: `![ci](https://github.com/<you>/f4-bringup/actions/workflows/ci.yml/badge.svg)`

## Architecture

```
main.c
  devices/   tmp108 (I2C) | spi_flash (SPI) | lsm6dso (SPI)      <- datasheets
  drivers/   i2c | spi | uart                                    <- reference manual
  common/    regs.h  status.h (error codes)  log.c (hex/bytes over UART)
platform/    f4_bringup.repl (the "netlist")  f4_bringup.resc
tests/       Robot Framework: smoke, tmp108, flash, faults
```

Add an SVG block diagram here (MCU -> I2C1 -> TMP108, SPI1 -> MT25Q, SPI2 -> LSM6DSO, USART2 -> log).

## Quick start

```
docker build -t f4-bringup .
docker run --rm -it -v "$PWD":/work f4-bringup make test      # build + headless tests
docker run --rm -it -v "$PWD":/work f4-bringup make run       # interactive Renode monitor
```

> Note: Renode's stock STM32F4 platform downloads an SVD file the first time it loads (needs internet once; cached afterwards).

## Design decisions (the part reviewers read)

- **Why no HAL:** ... (what you learned by going to register level)
- **Polling vs interrupt vs DMA:** what you chose per bus and why
- **Error model:** every wait is bounded; `status_t` returned, never a hang
- **Wiring source of truth:** `platform/f4_bringup.repl`; addresses/pins are not magic numbers
- **Power:** TMP108 shutdown / one-shot + `WFI` between samples; measured duty cycle: ...

## Datasheet -> code

See [`docs/datasheet_to_code.md`](docs/datasheet_to_code.md): every register and timing used, with
document + page.

## Debugging evidence

See [`docs/bugs.md`](docs/bugs.md): real bugs hit, the bus trace before/after, root cause, fix.
Traces live in `docs/traces/`.

## Tests

| Test | What it proves |
|---|---|
| smoke | boots, UART works |
| tmp108 | raw bytes decode to a plausible temperature |
| flash | erase -> program -> readback verify |
| faults | wrong address / wrong SPI mode / stuck busy returns an error within a timeout |

## Limitations / next steps

Be honest: simulated peripherals, clocks not modelled, no real-board timing. Next: DMA, a Linux I2C
client driver for the same sensor on QEMU.
