/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include <stdio.h>
#include <stdlib.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "app_camera.h"
#include "app_bluetooth.h"
#include "app_tcp_wifi.h"

#define HOST_IP_ADDR "192.168.1.103"
#define PORT 3333

#define IMAGE_NUM 10
#define Camera_Capture_Task_Delay 200  // ms

static const char *TAG = "app_tcp";


void camera_capture(uint8_t *fb, uint32_t *fs){

	camera_fb_t *single_fb;
	uint32_t single_fs;
	uint32_t offset = 0;

	if(!fb)
		ESP_LOGE(TAG, "fb pointer is NULL");

	if(!fs)
		ESP_LOGE(TAG, "fs pointer is NULL");

	for(int i = 0; i < IMAGE_NUM; i++){

		if(i == 0){
			single_fb = esp_camera_fb_get();
			if(!single_fb)
				ESP_LOGE(TAG, "Camera capture failed");

			ESP_LOGI(TAG, "Num.%d single_fb->len: %zu", i, single_fb->len);
			memcpy(fb, single_fb->buf, single_fb->len);
			memcpy(fs, &single_fb->len, sizeof(uint32_t));

			ESP_LOGI(TAG, "Num.%d fb address: %p", i, fb);
			ESP_LOGI(TAG, "Num.%d fs address/contents: %p-%zu", i, fs, *fs);
			ESP_LOGI(TAG, "Num.%d fb+offset address: %p, fs[i]=%d", i, (fb + fs[i]), fs[i]);
		}
		else{
			single_fs = fs[i-1];
			offset += single_fs;

			single_fb = esp_camera_fb_get();
			if(!single_fb)
				ESP_LOGE(TAG, "Camera capture failed");

			ESP_LOGI(TAG, "Num.%d single_fb->len: %zu", i, single_fb->len);

			memcpy((fb + offset), single_fb->buf, single_fb->len);

			ESP_LOGI(TAG, "Num.%d fb+offset address: %p, fb=%p, offset=%u", i, (fb + offset), fb, offset);
			fs[i] = single_fb->len;

			ESP_LOGI(TAG, "Num.%d fs address/contents: %p-%zu", i, &fs[i], fs[i]);

		}

		if(single_fb){
            esp_camera_fb_return(single_fb);
            single_fb = NULL;
        }

	} // end for


//	 ESP_ERROR_CHECK(example_connect());
//	 tcp_client_task(fb, fs);
}


int socket_send(uint8_t *fb, uint32_t *fs, int sock)
{
	int64_t fr_start, fr_end;
	int res = -1;
	uint32_t offset = 0;

	ESP_LOGI(TAG, "Preparing to send JPG frames");

	for(int i = 0; i < IMAGE_NUM; i++){

		fr_start = esp_timer_get_time();

		// notice: the value of fs[i] should be 4 Bytes, so the length to send is sizeof(int32_t)
		res = send_image_data(sizeof(int32_t), &fs[i]);

		ESP_LOGI(TAG, "Sent (%d bytes) JPG Num.%d byte size fs=%u", res, i, (uint32_t)fs[i]);
		ESP_LOGI(TAG, "Byte size (decimal/hex): %u/0x%08x", (uint32_t)fs[i], (uint32_t)fs[i]);

		if(i == 0){
			res = send_image_data(fs[i], fb);
			ESP_LOGI(TAG, "Sent %d bytes", res);
			ESP_LOGI(TAG, "Num.0 fb address: %p", fb);
		}
		else{
			offset += (uint32_t)fs[i-1];
			res = send_image_data(fs[i], (fb + offset));
			ESP_LOGI(TAG, "Sent %d bytes", res);
			ESP_LOGI(TAG, "Num.%d fb address: %p, fs=%u", i, (fb + offset), (uint32_t)fs[i]);
		}

		if(res < 0){
			ESP_LOGE(TAG, "Error occurred during image sending: errno %d", errno);
			break;
		}

		fr_end = esp_timer_get_time();
		ESP_LOGI(TAG, "JPG Num.%d: %u Bytes @ %u ms", i, (uint32_t)fs[i], (uint32_t)((fr_end - fr_start) / 1000));
	}

	return res;
}


static void tcp_client_task_bt(uint8_t *fb, uint32_t *fs)
{
    extern uint8_t extern_end_flag; // defined in app_bluetooth.c

    while (1) {
        while (1) {
            int err = socket_send(fb, fs, 0);

            // todo if sending images by Bluetooth failed, try to use wifi
            if(err < 0)
                break;

            vTaskDelay(2000 / portTICK_PERIOD_MS); //Block for 2000 ms

            if (extern_end_flag == 1) {
                ESP_LOGI(TAG, "Received extern_end_flag signal");
                extern_end_flag = 0;
                break;
            }
            break;
        }
        break;
    }
}

void client_task_send_by_bt(uint8_t *fb, uint32_t *fs) {
    tcp_client_task_bt(fb, fs);
}

// todo get the speed by testing the wifi speed
double get_wifi_speed() {
    double wifi_speed = 100;
    double speeds[] = {10, 20, 30, 40, 50.0, 60.0, 70.0, 80, 90, 100};
    time_t t;
    srand((unsigned) time(&t));
    int random_index = rand() % 10;

    wifi_speed = speeds[random_index];

    ESP_LOGI(TAG, "Task wifi_speed:.%f", wifi_speed);

    return wifi_speed;
}

double get_bt_speed() {
    double bt_speed = 40; // KB/s
    ESP_LOGI(TAG, "Task bt_speed:.%f", bt_speed);
    return bt_speed;
}

void app_tcp_main(int *flag)
{	
//	app_camera_main();
	
	uint8_t *frame_buffer = (uint8_t *)malloc(IMAGE_NUM*30000*sizeof(uint8_t)); //Allocated enough space for 10 frames @ 61kB size
	uint32_t *frame_size = (uint32_t *)malloc(sizeof(uint32_t)*IMAGE_NUM); //Allocated space for 10 uint32_t that are 2 bytes each

	if(!frame_buffer)
		ESP_LOGE(TAG, "Memory allocation for frame buffer returned NULL pointer");

	printf("frame_buffer address %p \n", frame_buffer);

	if(!frame_size)
		ESP_LOGE(TAG, "Memory allocation for frame size buffer returned NULL pointer");

	memset(frame_buffer, 0 , IMAGE_NUM * 30000 * sizeof(uint8_t));
	memset(frame_size, 0, sizeof(frame_size) * IMAGE_NUM);

    int64_t fr_start, fr_end;
    fr_start = esp_timer_get_time();

    camera_capture(frame_buffer, frame_size);
	fr_end = esp_timer_get_time();
	ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
     // for wifi https://github.com/espressif/esp-idf/blob/master/examples/protocols/README.md
    fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "Wifi connect IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

    // send the images via wifi or bt
    fr_end = esp_timer_get_time();

    // by bluetooth
    tcp_client_task_bt(frame_buffer, frame_size);

    /*
    if (get_wifi_speed() <= get_bt_speed ()) {
        tcp_client_task_bt(frame_buffer, frame_size);
//        client_task_send_by_wifi(frame_buffer, frame_size);
    } else {
       // todo if WIFI_Speed > BT_Speed ()
        client_task_send_by_wifi(frame_buffer, frame_size);
    }
    */

    ESP_LOGI(TAG, "Task end IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

    free(frame_buffer);
    free(frame_size);
}
