#pragma once

typedef enum {
    ERR_OK = 0,
    ERR_NACK = -1,      /* I2C address or data NACK */
    ERR_TIMEOUT = -2,   /* flag never arrived: bounded wait expired */
    ERR_BUSY = -3,
    ERR_BADID = -4,     /* WHO_AM_I / JEDEC ID mismatch */
    ERR_VERIFY = -5,    /* readback != written */
    ERR_NOT_IMPL = -99, /* scaffold stub: you haven't written this yet */
} status_t;
