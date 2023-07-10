#ifndef _APP_ADXL345_H_
#define _APP_ADXL345_H_

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t adxl345_read_reg(uint8_t reg, uint8_t *data);

esp_err_t adxl345_write_reg(uint8_t reg, uint8_t comd);

void mag_and_dir(double_t *alpha, double_t *beta, double_t *gamma, float_t *mag);

void adxl345_init();

#ifdef __cplusplus
}
#endif

#endif /* _APP_ADXL345_H_ */
