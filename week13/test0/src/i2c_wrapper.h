
#ifndef I2C_WRAPPER_H
#define I2C_WRAPPER_H

int i2c_write_dt_wrapper(
    void *spec,
    const uint8_t *buf,
    uint32_t num_bytes
);

#endif