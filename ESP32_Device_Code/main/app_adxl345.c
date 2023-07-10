#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"

#include "driver/i2c.h"

#define I2C_PORT		 0	// Only available I2C channel. SCCB or camera uses I2C channel 1

#define PIN_NUM_SDA		14  // IO14 - HS2_CLK  // different from V1
#define PIN_NUM_SCK 	15  // IO15 - HS2_CMD // different from V1

#define I2C_MAX_FREQ	400000 // Max freq for ADLX345 = 400kHz

#define ACCEL_ADDR					0xa6
#define ACCEL_DATA_RATE_REGISTER	0x2c
#define ACCEL_POWER_CTL_REGISTER	0x2d
#define ACCEL_DATA_FORMAT_REGISTER 	0x31
#define ACCEL_FIFO_CNTRL_REGISTER	0x38

#define ACCEL_DATA_X_REGISTER		0x32
#define ACCEL_DATA_Y_REGISTER		0x34
#define ACCEL_DATA_Z_REGISTER		0x36

#define ACCEL_READ_BIT		0x1
#define ACCEL_WRITE_BIT		0x0

#define ACK_CHECK_EN		0x1
#define ACK_VAL				0x0
#define NACK_VAL			0x1

static const char *TAG = "app_adxl345";

esp_err_t adxl345_read_reg(uint8_t reg, uint8_t *data){
	
	int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ACCEL_ADDR | ACCEL_WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(10 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ACCEL_ADDR | ACCEL_READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t adxl345_write_reg(uint8_t reg, uint8_t comd){
	
	int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ACCEL_ADDR | ACCEL_WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, comd, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void adxl345_read_device_id(){
	
	uint8_t reg = 0x00;
	uint8_t id;
	
	uint8_t flag = 0x1;
	while(flag == 0x1){
		if(ESP_OK == adxl345_read_reg(reg, &id)){
			ESP_LOGI(TAG, "Reading device ID.....");
			if(id == 0xe5){
				ESP_LOGI(TAG, "Correct device ID! Accel device ID: 0x%02x", id);
				flag = 0x0;
			}else{
				//printf("Incorrect device ID! Received accel device ID: 0x%02x\n", id);
				ESP_LOGE(TAG, "Incorrect device ID! Received accel device ID: 0x%02x", id);
			}
		}else{
			ESP_LOGE(TAG, "Error: Could not complete read of device id register\n");
		}
	}
}

void convert2float(uint16_t old_data, float_t *data){
	
	float_t temp1;
	uint16_t temp2 = old_data & 0x1fff;
	
	if((temp2 >> 12) == 0x0001){
		temp1 = -16.0;
	}else{
		temp1 = 0.0;
	}
	
	temp2 = old_data & 0x0fff;
	
	*data = temp1 + (temp2 * 0.004);
	
//	printf("temp1 float value: %f\ttemp2 hex value: %04x\n", temp1, temp2);
//	printf("Hex value of axis: %04x\nFloat value of axis: %f\n\n", old_data, *data);
}

esp_err_t adxl345_read_axes(float_t *data){

	//Reading all data registers
	uint8_t buf[6];
		
	//Start read from x-axis
	int ret = ESP_FAIL;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ACCEL_ADDR | ACCEL_WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ACCEL_DATA_X_REGISTER, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(10 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ACCEL_ADDR | ACCEL_READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &buf[0], ACK_VAL);
    i2c_master_read_byte(cmd, &buf[1], ACK_VAL);
    i2c_master_read_byte(cmd, &buf[2], ACK_VAL);
    i2c_master_read_byte(cmd, &buf[3], ACK_VAL);
    i2c_master_read_byte(cmd, &buf[4], ACK_VAL);
    i2c_master_read_byte(cmd, &buf[5], NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    
    convert2float(((buf[1] << 8) | buf[0]), &data[0]);
    convert2float(((buf[3] << 8) | buf[2]), &data[1]);
    convert2float(((buf[5] << 8) | buf[4]), &data[2]);
    
    return ret;
}

void magnitude(float_t xdata, float_t ydata, float_t zdata, float_t *mag){
	
	*mag = sqrt((xdata*xdata) + (ydata*ydata) + (zdata*zdata));
//	printf("Axis X: %02.3f\tY: %02.3f\tZ: %02.3f\tMag: %02.3f\n\n", xdata, ydata, zdata, *mag);
}

void mag_and_dir(double_t *alpha, double_t *beta, double_t *gamma, float_t *mag){
	
	float_t axes[3];
	if(ESP_OK != adxl345_read_axes(axes)){
		ESP_LOGE(TAG, "Error: Could not read axis data for calculation of magnitude and direction");
	}
	
	magnitude(axes[0], axes[1], axes[2], mag);
	
	double_t xnorm = axes[0] / *mag;
	double_t ynorm = axes[1] / *mag;
	double_t znorm = axes[2] / *mag;
	
//	printf("Normalized axis X: %02.3f\tY: %02.3f\tZ: %02.3f\tMag: %02.3f\n\n", xnorm, ynorm, znorm, *mag);
	
	*alpha = acos(xnorm);
	*beta = acos(ynorm);
	*gamma = acos(znorm);
}

void adxl345_init(){

	//Initilize ADXL345 for full functionality
	esp_err_t ret;
	
    i2c_config_t conf = {
		
		.mode = I2C_MODE_MASTER,
		.sda_io_num = PIN_NUM_SDA,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = PIN_NUM_SCK,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = I2C_MAX_FREQ
	};
	
    ret = i2c_param_config(I2C_PORT, &conf);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Completed parameter configuration");
    
    ret = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "Completed driver installation");
	
	// Reading device ID for ADXL345
	adxl345_read_device_id();
	
	// Setting data format
	ESP_LOGI(TAG, "Setting data format on ADXL345 to 0x0f....");
	if(ESP_OK == adxl345_write_reg(ACCEL_DATA_FORMAT_REGISTER, 0x0b)){
		ESP_LOGI(TAG, "Write complete!");
	}else{
		ESP_LOGE(TAG, "Error: Write of data format register incomplete");
	}

	// Setting data rate for ADXL345
	ESP_LOGI(TAG, "Setting data rate register to 0x0d...");
	if(ESP_OK == adxl345_write_reg(ACCEL_DATA_RATE_REGISTER, 0x0d)){
		ESP_LOGI(TAG, "Finished write of data rate register! Checking data rate register...");
		uint8_t data_rate;
		adxl345_read_reg(ACCEL_DATA_RATE_REGISTER, &data_rate);
		if(data_rate == 0x0d){
			ESP_LOGI(TAG, "Correct value for data rate register! Data rate register holds: 0x%02x", data_rate);
		}else{
			ESP_LOGE(TAG, "Error: Incorrect value for data rate register! Data rate register holds: 0x%02x", data_rate);
		}
	}else{
		ESP_LOGE(TAG, "Error: Could not complete write of data rate register\n");
	}
	
	// Setting FIFO mode to stream on ADXL345
	ESP_LOGI(TAG, "Setting FIFO control register to 0x80...");
	if(ESP_OK == adxl345_write_reg(ACCEL_FIFO_CNTRL_REGISTER, 0x80)){
		ESP_LOGI(TAG, "Finished write of FIFO control register! Checking FIFO control register...");
		uint8_t fifo_cntrl;
		adxl345_read_reg(ACCEL_FIFO_CNTRL_REGISTER, &fifo_cntrl);
		if(fifo_cntrl == 0x80){
			ESP_LOGI(TAG, "Correct value for FIFO control register! FIFO control register holds: 0x%02x", fifo_cntrl);
		}else{
			ESP_LOGE(TAG, "Error: Incorrect value for FIFO control register! FIFO control register holds: 0x%02x", fifo_cntrl);
		}
	}else{
		ESP_LOGE(TAG, "Error: Could not complete write of FIFO control register\n");
	}
	
	//Setting ADXL345 in Measurement Mode
	ESP_LOGI(TAG, "Setting ADXL345 to measurement mode.....");
	if(ESP_OK == adxl345_write_reg(ACCEL_POWER_CTL_REGISTER, 0x08)){
		ESP_LOGI(TAG, "Finished write of power control register! Checking register for 0x08...");
		uint8_t power_ctl;
		adxl345_read_reg(ACCEL_POWER_CTL_REGISTER, &power_ctl);
		if(power_ctl == 0x08){
			ESP_LOGI(TAG, "Correct value for power control register! Register holds: 0x%02x", power_ctl);
		}else{
			ESP_LOGE(TAG, "Error: Incorrect value for power control register! Register holds: 0x%02x", power_ctl);
		}
	}else{
		ESP_LOGE(TAG, "Error: Could not complete write of power control register\n");
	}
}
