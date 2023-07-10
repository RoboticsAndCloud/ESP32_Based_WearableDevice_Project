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
#include "esp_event.h"
#include "protocol_examples_common.h"

#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "app_adxl345.h"
//#include "app_cam_wifi.h"
#include "app_tcp.h"
#include "app_tcp_wifi.h"
#include "app_camera.h"
#include "app_bluetooth.c"

// ./main/CMakeLists.txt:set(COMPONENT_SRCS "app_main.c" "app_adxl345.c" "app_cam_wifi.c" "app_mic.c" "app_camera.c" "connect.c")

#define ACCEL_POWER_CTL_REGISTER	0x2d

static const char *TAG = "app_adxl345";

static void adxl345_task(void *pvParameters){
	
	adxl345_init();
	
	// Buffer of angles and magnitude for future use
	double_t alpha[100], beta[100], gamma[100];
	float_t mag[100];
	
	int flag = 1;
	int i = 0;
	while(flag == 1){
		mag_and_dir(&alpha[i], &beta[i], &gamma[i], &mag[i]);
		ESP_LOGI(TAG, "Alpha: %02.2f\tBeta: %02.2f\tGamma: %02.2f\tMagnitude: %02.2f", alpha[i], beta[i], gamma[i], mag[i]);
		if(mag[i] > 3){
			
			// Initialize camera and run camera code
			// app_cam_wifi(&flag);

            // bluetooth
			// app_tcp_main(&flag);
			// sleep 4000 ms
			//vTaskDelay(5000 / portTICK_RATE_MS);
			// wifi
			 app_tcp_wifi_main(&flag);
		}
		
		// Periodic blocking for lower priority tasks to run
		if(i == 50){
			vTaskDelay(10 / portTICK_RATE_MS);
		}

		// delay 60 s, test wifi_speed every 1 minute
		// 5 seconds is good for testing the bluetooth speed
		// vTaskDelay(1000 * 5 / portTICK_RATE_MS);
		//app_tcp_wifi_main(&flag);  // wifi channel
//        app_tcp_main(&flag);  // bluetooth channel

		i = (i + 1) % 100;
	}
	
	//Setting ADXL345 in Standby Mode
	ESP_LOGI(TAG, "Setting ADXL345 back to standby mode.....");
	if(ESP_OK == adxl345_write_reg(ACCEL_POWER_CTL_REGISTER, 0x00)){
		ESP_LOGI(TAG, "Finished write of power control register! Checking register for 0x00...");
		uint8_t power_ctl;
		adxl345_read_reg(ACCEL_POWER_CTL_REGISTER, &power_ctl);
		if(power_ctl == 0x00){
			ESP_LOGI(TAG, "Correct value for power control register! Register holds: 0x%02x", power_ctl);
		}else{
			ESP_LOGE(TAG, "Error: Incorrect value for power control register! Register holds: 0x%02x", power_ctl);
		}
	}
}

void app_main(){
	
	app_camera_main();
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
//    app_main_bluetooth();

    // connect the wifi first, for long connection to wifi
    ESP_ERROR_CHECK(example_connect()); // test

	xTaskCreate(adxl345_task, "adxl345_task", 8192, NULL, 5, NULL);
}
