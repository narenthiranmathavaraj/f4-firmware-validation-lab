
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "devices/tmp108.h"
#include "devices/lsm6dso.h"
#include "devices/fan.h"

#include "drivers/uart.h"
#include "drivers/uart_dma.h"
#include "drivers/i2c.h"
#include "drivers/spi.h"

#include "common/status.h"
#include "common/log.h"
#include "common/timer.h"

/* =========================================================
 * Scheduling periods
 * ========================================================= */

#define IMU_PERIOD_MS          10U
#define TEMP_PERIOD_MS         1000U
#define TELEMETRY_PERIOD_MS    1000U
#define HEALTH_PERIOD_MS       100U

/* =========================================================
 * Sensor communication timeouts
 * ========================================================= */

#define IMU_TIMEOUT_MS         5000U
#define TEMP_TIMEOUT_MS        10000U

/* =========================================================
 * Task configuration
 * ========================================================= */

#define TASK_STACK_WORDS       (configMINIMAL_STACK_SIZE * 3U)

#define IMU_PRIORITY           3U
#define TEMP_PRIORITY          2U
#define HEALTH_PRIORITY        2U
#define TELEMETRY_PRIORITY     1U

/* =========================================================
 * Sensor configuration
 * ========================================================= */

static spi_cfg_t spi1 = {
    .instance = 1,
    .cpol = 1,
    .cpha = 1,
    .baud_prescaler = 3
};

/* =========================================================
 * Shared sensor state
 * ========================================================= */

typedef struct
{
    int32_t accel[3];
    int32_t temperature_mC;

    bool imu_valid;
    bool temp_valid;

    TickType_t last_imu_success;
    TickType_t last_temp_success;

} sensor_state_t;

static sensor_state_t sensors = {0};

/* Protect shared sensor data */
static SemaphoreHandle_t sensor_mutex = NULL;

/* Initialization status */
static bool imu_ready = false;
static bool temp_ready = false;

/* UART Test */

static const char dma_msg_1[]    = "DMA: first message\n";
static const char dma_msg_2[]    = "DMA: second message\n";
static const char dma_msg_long[] =
"DMA: long 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 END\n";


typedef enum
{
    THERMAL_NORMAL = 0,
    THERMAL_ACTIVE_COOL,
    THERMAL_THROTTLE,
    THERMAL_CRITICAL
} thermal_mode_t;


#define FAN_TEMP_THRESHOLD_NORMAL       50000
#define FAN_TEMP_THRESHOLD_ACTIVE_COOL  60000
#define FAN_TEMP_THRESHOLD_THROTTLING   70000
#define FAN_TEMP_THRESHOLD_CRITICAL    80000
static volatile thermal_mode_t thermal_mode = THERMAL_NORMAL;
static volatile uint32_t       fan_duty     = 100U;   /* matches fan_set_duty(100U) in main */

const char* thermal_mode_to_string(thermal_mode_t s) {
    switch (s) {
        case THERMAL_NORMAL:      return "0";
        case THERMAL_ACTIVE_COOL:   return "1";
        case THERMAL_THROTTLE: return "2";
        case THERMAL_CRITICAL: return "3";
        default:             return "-1";
    }
}

void int_to_string(size_t num, char *str)
{
    BaseType_t i = 0;
    BaseType_t negative = 0;

    if (num < 0)
    {
        negative = 1;
        num = -num;
    }

    do
    {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    if (negative)
        str[i++] = '-';

    str[i] = '\0';

    // Reverse the string
    for (BaseType_t j = 0; j < i / 2; j++)
    {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

static void apply_fan(thermal_mode_t mode, uint32_t duty)
{
    thermal_mode = mode;
    fan_duty     = duty;
    fan_set_duty(duty);
}

static void fan_control(int32_t temperature_mC)
{
    if      (temperature_mC >= FAN_TEMP_THRESHOLD_CRITICAL)    apply_fan(THERMAL_CRITICAL,    100U);
    else if (temperature_mC >= FAN_TEMP_THRESHOLD_THROTTLING)  apply_fan(THERMAL_THROTTLE,     80U);
    else if (temperature_mC >= FAN_TEMP_THRESHOLD_ACTIVE_COOL) apply_fan(THERMAL_ACTIVE_COOL,  60U);
    else if (temperature_mC >= FAN_TEMP_THRESHOLD_NORMAL)      apply_fan(THERMAL_NORMAL,       30U);
    else                                                       apply_fan(THERMAL_NORMAL,        0U);
}


/* =========================================================
 * Task declarations
 * ========================================================= */

static void vReadIMU(void *argument);
static void vReadTemp(void *argument);
static void vTelemetry(void *argument);
static void vSensorCheck(void *argument);

/* =========================================================
 * IMU acquisition task
 *
 * LSM6DSO over SPI
 * 100 Hz = every 10 ms
 * ========================================================= */

static void vReadIMU(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();

    int32_t local_accel[3];
    static bool imu_was_paused = false;
    for (;;)
    {
        uint32_t period_ms;

        if (thermal_mode == THERMAL_CRITICAL)
        {
            /* Stop IMU acquisition during critical temperature */
            xSemaphoreTake(sensor_mutex, portMAX_DELAY);
            sensors.imu_valid = false;
            xSemaphoreGive(sensor_mutex);
            imu_was_paused = true;
            last_wake = xTaskGetTickCount();

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (imu_was_paused)
        {
            xSemaphoreTake(sensor_mutex, portMAX_DELAY);

            sensors.last_imu_success = xTaskGetTickCount();

            xSemaphoreGive(sensor_mutex);

            imu_was_paused = false;
        }
        /* Select sampling frequency */
        if (thermal_mode == THERMAL_THROTTLE)
        {
            period_ms = 50U;  // 20 Hz
        }
        else
        {
            period_ms = 10U;  // 100 Hz
        }

        if (imu_ready)
        {
            status_t status =
                lsm6dso_read_accel_mg(&spi1, local_accel);

            if (status == ERR_OK)
            {
                xSemaphoreTake(sensor_mutex, portMAX_DELAY);

                sensors.accel[0] = local_accel[0];
                sensors.accel[1] = local_accel[1];
                sensors.accel[2] = local_accel[2];

                sensors.imu_valid = true;
                sensors.last_imu_success = xTaskGetTickCount();

                xSemaphoreGive(sensor_mutex);
            }
            else
            {
                xSemaphoreTake(sensor_mutex, portMAX_DELAY);
                sensors.imu_valid = false;
                xSemaphoreGive(sensor_mutex);
            }
        }

        vTaskDelayUntil(
            &last_wake,
            pdMS_TO_TICKS(period_ms)
        );
    }
}

/* =========================================================
 * Temperature acquisition task
 *
 * TMP108 over I2C
 * 1 Hz = every 1000 ms
 * ========================================================= */
static void vReadTemp(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();

    int32_t local_temperature;

    uint32_t consecutive_failures = 0;
    for (;;)
    {
        uint32_t period_ms;

        /* Select temperature sampling interval */
        if (thermal_mode == THERMAL_THROTTLE)
        {
            period_ms = 500U;   // 2 Hz
        }
        else
        {
            period_ms = 1000U;  // 1 Hz
        }

        if (temp_ready)
        {
            status_t status =
                tmp108_read_mC(&local_temperature);

            if (status == ERR_OK)
            {
                xSemaphoreTake(sensor_mutex, portMAX_DELAY);

                sensors.temperature_mC = local_temperature;
                sensors.temp_valid = true;
                sensors.last_temp_success = xTaskGetTickCount();

                xSemaphoreGive(sensor_mutex);
            }
            else
            {
                xSemaphoreTake(sensor_mutex, portMAX_DELAY);

                sensors.temp_valid = false;

                xSemaphoreGive(sensor_mutex);
            }
        }
        if (temp_ready)
        {

            fan_control(local_temperature);            /* only act on a fresh reading */
        }
        else
        {
            /* ... your existing temp_valid = false block ... */
            if (++consecutive_failures >= 3U)
                apply_fan(thermal_mode, 100U);         /* cannot see the temperature: cool hard */
        }

        vTaskDelayUntil(
            &last_wake,
            pdMS_TO_TICKS(period_ms)
        );
    }
}

/* =========================================================
 * Telemetry task
 *
 * UART DMA
 * 10 Hz = every 100 ms
 * ========================================================= */

static void vTelemetry(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();

    sensor_state_t snapshot;

    for (;;)
    {
        /* Obtain a consistent sensor-data snapshot */

        xSemaphoreTake(sensor_mutex, portMAX_DELAY);

        snapshot = sensors;

        xSemaphoreGive(sensor_mutex);

        /* Temperature telemetry */

        log_str("TEMP valid=");
        log_dec(snapshot.temp_valid);

        log_str(" temp_mC=");
        log_dec(snapshot.temperature_mC);

        /* IMU telemetry */

        log_str(" | IMU valid=");
        log_dec(snapshot.imu_valid);

        log_str(" accel_mg=");

        log_dec(snapshot.accel[0]);
        log_str(",");

        log_dec(snapshot.accel[1]);
        log_str(",");

        log_dec(snapshot.accel[2]);



        log_str("\n");
       /* Fan Status */
        log_str("thermal mode=");
        log_str(thermal_mode_to_string(thermal_mode));
        log_str(" duty=");
        char str[12]; // Enough space for a 32-bit int (-2147483648 + '\0')
        int_to_string(fan_duty, str);
        log_str(str);
        log_str("\n");
        vTaskDelayUntil(
            &last_wake,
            pdMS_TO_TICKS(TELEMETRY_PERIOD_MS)
        );
    }
}

/* =========================================================
 * Sensor health-monitor task
 *
 * Checks time since last successful acquisition.
 *
 * IMU timeout:  5 seconds
 * TEMP timeout: 10 seconds
 *
 * On timeout: system reset.
 * ========================================================= */

static void vSensorCheck(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();

    sensor_state_t snapshot;

    const TickType_t imu_timeout =
        pdMS_TO_TICKS(IMU_TIMEOUT_MS);

    const TickType_t temp_timeout =
        pdMS_TO_TICKS(TEMP_TIMEOUT_MS);

    for (;;)
    {
        TickType_t now = xTaskGetTickCount();

        xSemaphoreTake(sensor_mutex, portMAX_DELAY);

        snapshot = sensors;

        xSemaphoreGive(sensor_mutex);

        /* Check IMU communication */

        bool imu_paused = (thermal_mode == THERMAL_CRITICAL);

        if (!imu_paused &&
            (now - snapshot.last_imu_success) > imu_timeout)
        {
            log_str("FAULT: IMU communication timeout\n");
            vTaskDelay(pdMS_TO_TICKS(50));
            NVIC_SystemReset();
        }

        /* Check temperature communication */

        if ((now - snapshot.last_temp_success) > temp_timeout)
        {
            log_str("FAULT: TMP108 communication timeout\n");

            vTaskDelay(pdMS_TO_TICKS(50));

            NVIC_SystemReset();
        }

        vTaskDelayUntil(
            &last_wake,
            pdMS_TO_TICKS(HEALTH_PERIOD_MS)
        );
    }
}
void uart_dma_selftest(void)
{
    log_str(dma_msg_1);
    log_str(dma_msg_2);
    log_str(dma_msg_long);
    log_str("DMA: self-test done\n");
}
/* =========================================================
 * MAIN
 * ========================================================= */

int main(void)
{

    /* -----------------------------------------------------
     * 1. System initialization
     * ----------------------------------------------------- */

    timebase_init();
    uart_dma_init();

    uart_dma_selftest();
    fan_init();
    log_str("\n");
    log_str("f4-bringup boot\n");
    log_str("BOOT_OK\n");

    /* -----------------------------------------------------
     * 2. I2C + TMP108 initialization
     * ----------------------------------------------------- */

    status_t i2c_status = i2c_init();

    status_t tmp_status = ERR_NINITIALIZED;

    if (i2c_status == ERR_OK)
    {
        tmp_status = tmp108_init();
    }

    log_kv("i2c_init", (uint32_t)i2c_status);
    log_kv("tmp108_init", (uint32_t)tmp_status);

    temp_ready =
        (i2c_status == ERR_OK) &&
        (tmp_status == ERR_OK);

    /* -----------------------------------------------------
     * 3. SPI + LSM6DSO initialization
     * ----------------------------------------------------- */

    status_t spi_status = spi_init(&spi1);
    status_t imu_status = ERR_NINITIALIZED;
    status_t accel_cfg_status = ERR_NINITIALIZED;

    if (spi_status == ERR_OK)
    {
        imu_status = lsm6dso_init(&spi1);
    }

    /* Configure accelerometer:
     * ODR = 104 Hz
     * Full scale = +/- 2g
     */

    if (imu_status == ERR_OK)
    {
        uint8_t ctrl1_xl[2] = {0x10, 0x40};
        uint8_t dummy[2];
        accel_cfg_status =
            spi_transfer(&spi1, ctrl1_xl, dummy, 2);
    }

    log_kv("spi_init", (uint32_t)spi_status);
    log_kv("lsm6dso_init", (uint32_t)imu_status);
    log_kv("accel_config", (uint32_t)accel_cfg_status);

    imu_ready =
        (spi_status == ERR_OK) &&
        (imu_status == ERR_OK) &&
        (accel_cfg_status == ERR_OK);

    /* -----------------------------------------------------
     * 4. Initialize shared state
     * ----------------------------------------------------- */

    sensors.imu_valid = false;
    sensors.temp_valid = false;

    sensors.last_imu_success = xTaskGetTickCount();
    sensors.last_temp_success = xTaskGetTickCount();

    /* -----------------------------------------------------
     * 5. Create mutex
     * ----------------------------------------------------- */
    log_str("Free heap before mutex: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");
    sensor_mutex = xSemaphoreCreateMutex();
    log_str("Free heap after mutex: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");
    if (sensor_mutex == NULL)
    {
        log_str("FATAL: sensor mutex creation failed\n");

        NVIC_SystemReset();
    }

    /* -----------------------------------------------------
     * 6. Create FreeRTOS tasks
     * ----------------------------------------------------- */
    BaseType_t result;

    log_str("Free heap before tasks: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");

    /* IMU */

    result = xTaskCreate(
        vReadIMU,
        "IMU",
        TASK_STACK_WORDS,
        NULL,
        IMU_PRIORITY,
        NULL
    );

    log_str("Create IMU: ");
    log_dec((int32_t)result);
    log_str("\n");

    log_str("Heap remaining: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");

    if (result != pdPASS)
        goto task_creation_failed;

    /* Temperature */

    result = xTaskCreate(
        vReadTemp,
        "Temperature",
        TASK_STACK_WORDS,
        NULL,
        TEMP_PRIORITY,
        NULL
    );

    log_str("Create Temperature: ");
    log_dec((int32_t)result);
    log_str("\n");

    log_str("Heap remaining: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");

    if (result != pdPASS)
        goto task_creation_failed;

    /* Telemetry */

    result = xTaskCreate(
        vTelemetry,
        "Telemetry",
        TASK_STACK_WORDS,
        NULL,
        TELEMETRY_PRIORITY,
        NULL
    );

    log_str("Create Telemetry: ");
    log_dec((int32_t)result);
    log_str("\n");

    log_str("Heap remaining: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");

    if (result != pdPASS)
        goto task_creation_failed;

    /* Health monitor */

    result = xTaskCreate(
        vSensorCheck,
        "Health",
        TASK_STACK_WORDS,
        NULL,
        HEALTH_PRIORITY,
        NULL
    );

    log_str("Create Health: ");
    log_dec((int32_t)result);
    log_str("\n");

    log_str("Heap remaining: ");
    log_dec((int32_t)xPortGetFreeHeapSize());
    log_str("\n");

    if (result != pdPASS)
        goto task_creation_failed;

    log_str("ALL TASKS CREATED\n");

    log_str("Starting FreeRTOS scheduler\n");

    vTaskStartScheduler();

    log_str("FATAL: scheduler failed\n");

    for (;;)
    {
        __asm volatile("wfi");
    }

    task_creation_failed:

    log_str("FATAL: task creation failed\n");

    for (;;)
    {
        __asm volatile("wfi");
    }
}
