#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "driver/i2s.h"

#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// Pin numbers for ESP-EYE
#define I2S_WS  	2
#define I2S_SD 		13 // different from V1 13
#define I2S_SCK 	4
#define I2S_PORT 	I2S_NUM_1

//max dma buffer parameters allowed by driver
#define I2S_DMA_BUF_COUNT 22
#define I2S_DMA_BUF_LEN   512

#define I2S_SAMPLE_RATE   41000
#define I2S_SAMPLE_BITS   32
#define I2S_CHANNEL_NUM   1
#define I2S_READ_LEN	  90024

#define RECORD_TIME       10 //Seconds
#define RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS/ 8 * RECORD_TIME)

#define TAG "app_mic"

void i2s_init(void)
{
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,			
        .sample_rate = I2S_SAMPLE_RATE,                 
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,   
        .bits_per_sample = I2S_SAMPLE_BITS,             
        .communication_format = I2S_COMM_FORMAT_I2S,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .intr_alloc_flags = 0,
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,  
        .ws_io_num = I2S_WS,   
        .data_out_num = -1,
        .data_in_num = I2S_SD  
    };
    
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_PORT);
}

void i2s_data_scale(uint8_t *buff, uint32_t len)
{
	uint32_t dac_value = 0;
	
	for(int i=0; i<len; i+=2)
	{
		dac_value = (((uint16_t)(buff[i+1] & 0xf) << 8) | buff[i]);
		buff[i] = 0;
		buff[i+1] = dac_value * 256 / 2048;
	}
}

int i2s_socket_send(int sock, char *buf)
{   
    int res = -1;
    uint32_t size = RECORD_SIZE;
    uint32_t bytes_sent = 0;
    char *mic_tag = "app_mic";

    ESP_LOGI(TAG, " *** Starting WAV Transmission *** ");
    
    res = send(sock, mic_tag, 7, 0);
    if (res < 0){
			ESP_LOGE(TAG, "Failed to send microphone tag");
			return res;
	}
    ESP_LOGI(TAG, "Sent %d bytes for microphone tag \"%s\"", res, mic_tag);
    
    res = send(sock, &size, sizeof(int), 0);
    if (res < 0){
			ESP_LOGE(TAG, "Failed to send size of WAV data");
			return res;
	}
    ESP_LOGI(TAG, "Sent %d bytes for recording size of %d", res, size);
    
    while(bytes_sent < RECORD_SIZE){
		
		res = send(sock, buf, size, 0);
		
        if (res < 0){
			ESP_LOGE(TAG, "Failed to send WAV data");
			break;
		}
        
        bytes_sent += res;
        size -= res;
        
        if(size > 0)
			buf += res;
    }
    ESP_LOGI(TAG, " *** WAV Transmission Complete *** ");
    
    return res;
}

esp_err_t i2s_record(char *data, char *buf)
{
	esp_err_t res = ESP_FAIL;
	size_t data_wr_size = 0;
	size_t bytes_read = 0;
	uint32_t diff = RECORD_SIZE;
	uint32_t read_len = I2S_READ_LEN;
	
	ESP_LOGI(TAG, " *** Recording Start *** ");
	while(data_wr_size < RECORD_SIZE)
	{
		
		ESP_LOGI(TAG, "Reading data.....");
		if(diff > I2S_READ_LEN)
		{
			ESP_LOGI(TAG, "I2S transfer of %d bytes into %dB buffer.....", read_len, I2S_READ_LEN * sizeof(char));
			res = i2s_read(I2S_PORT, (void *)buf, read_len, &bytes_read, portMAX_DELAY);
		}
		else
		{
			ESP_LOGI(TAG, "I2S transfer of %d bytes into %dB buffer.....", diff, I2S_READ_LEN * sizeof(char));
			res = i2s_read(I2S_PORT, (void *)buf, diff, &bytes_read, portMAX_DELAY);
		}
			
		if(res != ESP_OK)
			break;
		
		ESP_LOGI(TAG, "Scaling data.....");
		i2s_data_scale((uint8_t *)buf, (uint32_t)bytes_read);
		
		ESP_LOGI(TAG, "Copying data.....");
		memcpy(data, buf, bytes_read);
		
		data += (uint32_t)bytes_read;
		data_wr_size += bytes_read;
		ESP_LOGI(TAG, "---------------Sound recording %u%%---------------", data_wr_size * 100 / RECORD_SIZE);
		
		diff = RECORD_SIZE - data_wr_size;
	}
	ESP_LOGI(TAG, " *** Recording End *** ");
	
	return res;
}

void app_mic(int sock)
{	
	char rx_buffer[128];
	
	i2s_init();
	
	char *i2s_read_buf  = (char *)heap_caps_malloc(I2S_READ_LEN * sizeof(char), MALLOC_CAP_8BIT);
	char *i2s_read_data = (char *)heap_caps_malloc(RECORD_SIZE * sizeof(char), MALLOC_CAP_SPIRAM);
	
	if(!i2s_read_buf)
		ESP_LOGE(TAG, "Memory allocation for i2s buffer failed");
	if(!i2s_read_data)
		ESP_LOGE(TAG, "Memory allocation for i2s data failed");
	
	ESP_LOGI(TAG, "Starting to record in 3 seconds....");
	for(int i=3; i>0; i--)
	{
		ESP_LOGI(TAG, "....%d....", i);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	
	int res = i2s_record(i2s_read_data, i2s_read_buf);
	if(res != ESP_OK)
		ESP_LOGE(TAG, "Error during recording");
	
	res = i2s_socket_send(sock, i2s_read_data);
	if(res < 0)
		ESP_LOGE(TAG, "Error during transmission of audio");
		
	int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
		
	if (len < 0) {
		ESP_LOGE(TAG, "recv failed: errno %d", errno);
	}
	else {
		while(1) {
			rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
			ESP_LOGI(TAG, "Received %d bytes from server", len);
			ESP_LOGI(TAG, "Response: %s", rx_buffer);
			
			if(strstr(rx_buffer, "False") != NULL){
				ESP_LOGI(TAG, "A fall was NOT DETECTED");
				break;
			}
			else if(strstr(rx_buffer, "True") != NULL){
				ESP_LOGI(TAG, "FALL DETECTED");
				break;
			}
			else{
				ESP_LOGE(TAG, "Received unsatisfactory response from server");
				break;
			}
				
			vTaskDelay(10 / portTICK_PERIOD_MS); //Block for 10 ms
		}
	}
	
	free(i2s_read_buf);
	free(i2s_read_data);
}
