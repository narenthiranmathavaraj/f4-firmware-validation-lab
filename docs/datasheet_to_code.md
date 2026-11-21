# Datasheet -> code

One row per register field / timing / command you actually use. Cite document + page + the
function in this repo.

| Device / block | Document (rev) | Page / section | Register / field / command | Value used | Why | Code |
|---|---|---|---|---|---|---|
| TMP108 | TI TMP108 datasheet | | pointer register | | | `tmp108.c:` |
| TMP108 | | | temperature format / LSB | | | |
| TMP108 | | | config: conversion mode, shutdown | | | |
| I2C1 | RM0090 I2C | | CR2.FREQ / CCR / TRISE | | | `i2c.c:` |
| I2C1 | | | CR1 START/STOP/ACK, SR1 SB/ADDR | | | |
| I2C1 | | | SR1 TXE / RXNE / BTF / AF | | | |
| SPI1 | RM0090 SPI | | CR1 (CPOL, CPHA, MSTR, BR, SPE) | | | `spi.c:` |
| SPI1 | | | SR TXE / RXNE / BSY, DR | | | |
| MT25Q | Micron MT25Q datasheet | | RDID / WREN / RDSR (WIP) | | | `spi_flash.c:` |
| MT25Q | | | READ / PP / SE | | | |
| LSM6DSO | ST LSM6DSO datasheet | | SPI read bit, WHO_AM_I | | | `lsm6dso.c:` |
| LSM6DSO | | | CTRL1_XL, sensitivity | | | |
