#include "devices/tmp108.h"
#include "drivers/i2c.h"

/* TODO (day 2): TI TMP108 datasheet: pointer register, temperature register format,
 * configuration register (conversion mode, rate), shutdown / one-shot. Cite pages in
 * docs/datasheet_to_code.md. */
status_t tmp108_init(void)
{
    uint8_t config_data[3] = {TMP108_REG_CONFIG,TMP108_CR_DEFAULT_MSB, TMP108_CR_DEFAULT_LSB};
    return i2c_write(TMP108_ADDR,config_data,3);
}

status_t tmp108_read_raw(int16_t *raw) //reading raw temp
{
    uint8_t reg = TMP108_REG_TEMP;
    status_t status;
    uint8_t buf[2] = {0};
    status = i2c_write(TMP108_ADDR,&reg,1);
    if(status != ERR_OK){
        return status;
    }
    status = i2c_read(TMP108_ADDR,buf,2);
    if(status != ERR_OK){
        return status;
    }
    *raw = (int16_t)((uint16_t)buf[0] << 8 | buf[1]) >> 4;
}
status_t tmp108_read_mC(int32_t *m) {
    int16_t raw =0;
    status_t status;

    status = tmp108_read_raw(&raw);

    if(status != ERR_OK){
        return status;
    }
    raw = (raw>>4);
    /*Shifting right by 4 drops the 4 unused padding bits on the right. Because raw is a signed int16_t and
    its leftmost sign bit is 1, the CPU fills the 4 new spots on the left with 1s (sign extension): */
    /* 1 LSB = 0.0625 C = 62.5 mC = 125 / 2 mC (pure integer math) */
    *m = ((int32_t)raw * 125) / 2;
    *m = ((int32_t)raw * 125) / 2; //Correct! 25000mC to -25.000°C
    return ERR_OK;
}

    status_t tmp108_shutdown(void) {
        status_t status;

        uint8_t config_data[3] = {TMP108_REG_CONFIG,TMP108_CR_DEFAULT_MSB, TMP108_CR_DEFAULT_LSB};
        // config_data[1] is a variable in RAM -> Writable (like I2C_CR1)
        // TMP108_CR_M1 is a constant bitmask -> Read-only (like CR1_POS)
        config_data[1] &= ~(TMP108_CR_M1 | TMP108_CR_M0);
        status = i2c_write(TMP108_ADDR,&config_data,1);
        if(status != ERR_OK){
            return status;
        }
        return ERR_OK;
     }
