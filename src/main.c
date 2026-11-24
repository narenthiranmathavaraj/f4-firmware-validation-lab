#include <stddef.h>
#include "drivers/uart.h"
#include "drivers/i2c.h"
#include "drivers/spi.h"
#include "devices/tmp108.h"
#include "devices/lsm6dso.h"
#include "common/log.h"


#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"

#include "common/timer.h"
#include "common/status.h"
#include "common/log.h"

#include "drivers/uart.h"
#include "drivers/i2c.h"
#include "drivers/spi.h"

#include "devices/tmp108.h"
#include "devices/lsm6dso.h"

/* Scheduling periods */
#define IMU_PERIOD_MS          10U
#define TEMP_PERIOD_MS         1000U
#define TELEMETRY_PERIOD_MS    100U

/* Sensor communication timeouts */
#define IMU_TIMEOUT_MS         5000U
#define TEMP_TIMEOUT_MS        10000U

int main(void)
{
    /* ---- System initialization ---- */

    timebase_init();

    uart_dma_init();

    log_str("f4-bringup boot\n");
    log_str("BOOT_OK\n");

    /* ---- I2C + TMP108 ---- */

    status_t i2c_status = i2c_init();
    status_t tmp_status = tmp108_init();

    log_kv("i2c_init", (uint32_t)i2c_status);
    log_kv("tmp108_init", (uint32_t)tmp_status);

    /* ---- SPI + LSM6DSO ---- */

    spi_cfg_t spi1 = {
        .instance = 1,
        .cpol = 1,
        .cpha = 1,
        .baud_prescaler = 3
    };

    status_t spi_status = spi_init(&spi1);
    status_t imu_status = lsm6dso_init(&spi1);

    log_kv("spi_init", (uint32_t)spi_status);
    log_kv("lsm6dso_init", (uint32_t)imu_status);

    /* Configure accelerometer: 104 Hz, +-2g */

    uint8_t ctrl1_xl[2] = {0x10, 0x40};

    status_t accel_cfg_status = spi_transfer(
        &spi1, ctrl1_xl, NULL, 2
    );

    log_kv("accel_config", (uint32_t)accel_cfg_status);

    bool temp_ready =
        (i2c_status == ERR_OK) &&
        (tmp_status == ERR_OK);

    bool imu_ready =
        (spi_status == ERR_OK) &&
        (imu_status == ERR_OK) &&
        (accel_cfg_status == ERR_OK);

    /* ---- Runtime state ---- */

    int32_t accel[3] = {0};
    int32_t temperature_mC = 0;

    bool imu_seen = false;
    bool temp_seen = false;

    uint32_t now = millis();

    uint32_t last_imu = now;
    uint32_t last_temp = now;
    uint32_t last_telemetry = now;

    uint32_t last_imu_success = 0;
    uint32_t last_temp_success = 0;

    log_str("RUNTIME_START\n");

    /* ---- Main superloop ---- */

    for (;;)
    {
        now = millis();

        /* 1. IMU acquisition: 100 Hz */

        if (imu_ready &&
            (uint32_t)(now - last_imu) >= IMU_PERIOD_MS)
        {
            last_imu = now;

            if (lsm6dso_read_accel_mg(&spi1, accel) == ERR_OK)
            {
                imu_seen = true;
                last_imu_success = now;
            }
        }

        /* 2. Temperature acquisition: 1 Hz */

        if (temp_ready &&
            (uint32_t)(now - last_temp) >= TEMP_PERIOD_MS)
        {
            last_temp = now;

            if (tmp108_read_mC(&temperature_mC) == ERR_OK)
            {
                temp_seen = true;
                last_temp_success = now;
            }
        }

        /* 3. Sensor freshness monitoring */

        bool imu_valid =
            imu_seen &&
            ((uint32_t)(now - last_imu_success) < IMU_TIMEOUT_MS);

        bool temp_valid =
            temp_seen &&
            ((uint32_t)(now - last_temp_success) < TEMP_TIMEOUT_MS);

        /* 4. Telemetry: 10 Hz */

        if ((uint32_t)(now - last_telemetry) >= TELEMETRY_PERIOD_MS)
        {
            if(temperature_mC > 30000){
                last_telemetry = now;
                log_str("Too Hot");
                log_str("\n");

            }
            else{ last_telemetry = now;

            log_str("IMU valid=");
            log_dec(imu_valid);

            log_str(" accel_mg=");
            log_dec(accel[0]);
            log_str(",");
            log_dec(accel[1]);
            log_str(",");
            log_dec(accel[2]);

            log_str(" TEMP valid=");
            log_dec(temp_valid);

            log_str(" temp_mC=");
            log_dec(temperature_mC);

            log_str("\n");}

        }

        /* Sleep until an interrupt occurs */
        __asm volatile("wfi");
    }
}
