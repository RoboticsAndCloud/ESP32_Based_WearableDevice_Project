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

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "app_camera.h"
#include "app_tcp.h"
//#include "decode_image.h"



//#define HOST_IP_ADDR "192.168.43.58"
#define HOST_IP_ADDR "192.168.1.128"
#define PORT 3333

#define IMAGE_NUM 10
#define Camera_Capture_Task_Delay 200  // ms

static const char *TAG = "app_tcp";

#include "resize_image/img_resize.h"

#define STB_IMAGE_IMPLEMENTATION
#include "resize_image/stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "resize_image/stb_image/stb_image_write.h"


#define BUFFER_LEN_READ 40 * 1024  // 30 KB

#define RESIZED_BUFFER_LEN 10 * 1024 // Resized image size

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define NEW_IMAGE_HEIGHT 224
#define NEW_IMAGE_WIDTH  224

unsigned char **fb_img;
size_t *fs_img;

void camera_capture_wifi(uint8_t *fb, size_t *fs){

	camera_fb_t *single_fb;
	size_t single_fs;
	size_t offset = 0;

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
			memcpy(fs, &single_fb->len, sizeof(size_t));

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

			if (single_fb->format == PIXFORMAT_JPEG) {
				ESP_LOGI(TAG, "Num.%d single_fb->format: PIXFORMAT_JPEG", i);
				// https://github.com/espressif/esp32-camera/blob/master/README.md
			}

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
}

// edit by fei
stbi_uc *resample_image(int width, int height, int channels, const void *img)
{

     // Convert the input image to sepia
    size_t img_size = width * height * channels;
    unsigned char *sepia_img = malloc(img_size);
    if(sepia_img == NULL) {
        printf("Unable to allocate memory for the sepia image.\n");
        exit(1);
    }

    // Sepia filter coefficients from https://stackoverflow.com/questions/1061093/how-is-a-sepia-tone-created
    for(unsigned char *p = img, *pg = sepia_img; p != img + img_size; p += channels, pg += channels) {
        *pg       = (uint8_t)fmin(0.393 * *p + 0.769 * *(p + 1) + 0.189 * *(p + 2), 255.0);         // red
        *(pg + 1) = (uint8_t)fmin(0.349 * *p + 0.686 * *(p + 1) + 0.168 * *(p + 2), 255.0);         // green
        *(pg + 2) = (uint8_t)fmin(0.272 * *p + 0.534 * *(p + 1) + 0.131 * *(p + 2), 255.0);         // blue
        if(channels == 4) {
            *(pg + 3) = *(p + 3);
        }
    }

    return sepia_img;
}



// // todo: resize the image using linear interpolation algorithm
// Develop the image processing algorithm (linear interpolation) deal with decode, resize the image to 224*224
stbi_uc *resize_image_processing(int width, int height, int channels, const void *img, int new_w, int new_h)
{

//Memory Loaded image with a width of 640px, a height of 480px and 3 channels
//Bilinear interpolation to new size 640 480 3
//Set ptr, begin to set 921600 pixels into matrix

     // Convert the input image to
    size_t img_size = new_w * new_h * channels;
    unsigned char *new_img = malloc(img_size);
    if(new_img == NULL) {
        printf("Unable to allocate memory for the sepia image.\n");
        exit(1);
    }

    // malloc memory
    size_t n1 = channels;
    size_t n2 = height;
    size_t n3 = width;

    size_t n1_new = channels;
    size_t n2_new = new_h;
    size_t n3_new = new_w;

    printf("Bilinear interpolation from size [%d, %d, %d ]to new size [ %d %d %d ]\n", width, height, channels, new_w, new_h, channels);

    unsigned char ***ptr = malloc(n1 * sizeof(unsigned char **) + /* level1 pointer */
                          n1 * n2 * sizeof(unsigned char *) + /* level2 pointer */
                          n1 * n2 * n3 * sizeof(unsigned char)); /* data pointer */
    for (size_t i = 0; i < n1; ++i) {
        ptr[i] = (unsigned char **)(ptr + n1) + i * n2;
        for (size_t j = 0; j < n2; ++j)
            ptr[i][j] = (unsigned char *)(ptr + n1 + n1 * n2) + i * n2 * n3 + j * n3;
    }

    unsigned char ***ptr_new = malloc(n1_new * sizeof(unsigned char **) + /* level1 pointer */
                          n1_new * n2_new * sizeof(unsigned char *) + /* level2 pointer */
                          n1_new * n2_new * n3_new * sizeof(unsigned char)); /* data pointer */
    for (size_t i = 0; i < n1_new; ++i) {
        ptr_new[i] = (unsigned char **)(ptr_new + n1_new) + i * n2_new;
        for (size_t j = 0; j < n2_new; ++j)
            ptr_new[i][j] = (unsigned char *)(ptr_new + n1_new + n1_new * n2_new) + i * n2_new * n3_new + j * n3_new;
    }


    printf("Set ptr, begin to set %d pixels into matrix \n", img_size);
    // set original pixel matrix
    unsigned int counter_pixels = 0;
    for (size_t i = 0; i < height; ++i)
        for (size_t j = 0; j < width; ++j)
            for (size_t k = 0; k < channels; ++k) {

                ptr[k][i][j] = ((unsigned char*)img)[counter_pixels];
//                printf("Setting pixel img: %d,   ptr; %d \n", ptr[k][i][j], ((unsigned char*)img)[counter_pixels]);
                counter_pixels ++;
            }
    printf("Set total %d pixels into the matrix \n", counter_pixels);

    // linear interpolation algorithm
    double scale_x = (height)*1.0/ new_h;
    double scale_y = (width)*1.0 /new_w;


    float x, y, pixel;
    float x_diff, y_diff;
    unsigned char a, b, c, d;
    int x_int, y_int;

    printf("scale_x: %f, scale_y: %f \n", scale_x, scale_y);
    for (size_t k = 0; k < channels; ++k) {
        for (size_t i = 0; i < new_h; ++i) {
            for (size_t j = 0; j < new_w; ++j) {

                y = scale_y * j;
                x = scale_x * i;

                x_int = (int)(x);
                y_int = (int)(y);

                // Prevent crossing
                x_int = MIN(x_int, height-2);
                y_int = MIN(y_int, width-2);

                x_diff = x - x_int;
                y_diff = y - y_int;

                a = ptr[k][x_int][y_int];
                b = ptr[k][x_int][y_int+1];
                c = ptr[k][x_int+1][y_int];
                d = ptr[k][x_int+1][y_int+1];

                pixel = a*(1-x_diff)*(1-y_diff) + b*(1-x_diff)
                        * (y_diff) + c*(x_diff) * (1-y_diff) + d*x_diff*y_diff;

                ptr_new[k][i][j] = (unsigned char)pixel;

            } // height
         } // for width
    } // for channel

    // set new pixels into new matrix
    counter_pixels = 0;
    for (size_t i = 0; i < new_h; ++i)
        for (size_t j = 0; j < new_w; ++j)
            for (size_t k = 0; k < channels; ++k) {
                new_img[counter_pixels++] = ptr_new[k][i][j];
            }

    printf("Set %d pixels into the new_img \n", counter_pixels);

    free(ptr);
    free(ptr_new);

    return new_img;
}


int socket_send_wifi(uint8_t *fb, size_t *fs, int sock)
{
	int64_t fr_start, fr_end;
	int res = -1;

	ESP_LOGI(TAG, "Preparing to send JPG frames");

	for(int i = 0; i < IMAGE_NUM; i++) {

	    fr_start = esp_timer_get_time();

	    res = send(sock, &fs_img[i], sizeof(int32_t), 0);
//        vTaskDelay(10 / portTICK_PERIOD_MS); //Block for 2000 ms
        ESP_LOGI(TAG, "Sent JPG Num.%d byte size", i);
		ESP_LOGI(TAG, "Byte size (decimal/hex): %u/0x%08x", (uint32_t)fs[i], (uint32_t)fs_img[i]);

	    res = send(sock, fb_img[i], fs_img[i], 0);
//	    ESP_LOGI(WIFI_TAG, "Sent %d bytes", res);
		if(res < 0){
			ESP_LOGE(TAG, "Error occurred during image sending: errno %d", errno);
			break;
		}
		fr_end = esp_timer_get_time();
		ESP_LOGI(TAG, "JPG Num.%d: %u Bytes @ %u ms, speed %f Bytes/s", i, (uint32_t)fs_img[i], (uint32_t)((fr_end - fr_start) / 1000),
		(size_t)fs_img[i] * 1.0 / ((uint32_t)((fr_end - fr_start) *1.0/ 1000/1000) ) );
	} // for

	return res;
}


int get_resized_image(uint8_t *fb, size_t *fs)
{
	int64_t fr_start, fr_end;
	int res = 0;
	uint32_t offset = 0;

	ESP_LOGI(TAG, "Preparing to get_resized_image JPG frames");

    int width, height, channels;
    unsigned char *img;

    stbi_uc *img_buffer;
    img_buffer = (stbi_uc *)malloc(BUFFER_LEN_READ*sizeof(stbi_uc));

    void *resize_img = NULL;
    int resize_img_size = -1;


    int new_w = NEW_IMAGE_WIDTH; // 640 ,224
    int new_h = NEW_IMAGE_HEIGHT; // 480, 224


    fb_img = (unsigned char **)malloc(sizeof(unsigned char *) * IMAGE_NUM);// Allocate memmory for the resized images
    for(int i = 0; i < IMAGE_NUM; i++){
        fb_img[i] = (unsigned char *)malloc(sizeof(unsigned char) * (RESIZED_BUFFER_LEN));
    }

	fs_img = (size_t *)malloc(sizeof(size_t)*IMAGE_NUM); //Allocated space for 10 size_t that are 2 bytes each


	for(int i = 0; i < IMAGE_NUM; i++){
		fr_start = esp_timer_get_time();

		// notice: the value of fs[i] should be 4 Bytes, so the length to send is sizeof(int32_t)
		//ESP_LOGI(TAG, "Sent (%d bytes) JPG Num.%d byte size fs=%u", res, i, (uint32_t)fs[i]);
		//ESP_LOGI(TAG, "Byte size (decimal/hex): %u/0x%08x", (uint32_t)fs[i], (uint32_t)fs[i]);

        if (i == 0) {

			int64_t fr_start, fr_end;
            fr_start = esp_timer_get_time();

			memset(img_buffer, 0, BUFFER_LEN_READ);
			memcpy(img_buffer, fb, fs[i]);
			img = stbi_load_from_memory(img_buffer, &fs[i], &width, &height, &channels, 0 );

			if(img == NULL) {
			    printf("Error in loading the image\n");

			}
		    printf("Memory Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
            fr_end = esp_timer_get_time();
            ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, Memory Loaded  Total Time: %u ms", i, (uint32_t)((fr_end - fr_start) / 1000))

            unsigned char *bit_processing_img = resize_image_processing(width, height, channels, img, new_w, new_h);
            fr_end = esp_timer_get_time();
            ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, resize_image_processing Total Time: %u ms", i, (uint32_t)((fr_end - fr_start) / 1000));


            if (bit_processing_img == NULL) {
                printf("resample_image failed\n");

            } // end if

            resize_img = NULL;
            resize_img_size = -1;

            // use new_w, new_h
            resize_img = stbi_write_jpg_to_mem("test.jpg", new_w, new_h, channels, bit_processing_img, 50,  &resize_img_size);


//            printf("resize_image_size: %d\n", resize_img_size);

            ESP_LOGI(TAG, "resize_image_size: %d\n", resize_img_size);

            // Fail to get the image
            if (resize_img_size == 0) {
                res = -1;
                break;
            }

            if (img != NULL) {
                free(img);
            }

            if (bit_processing_img != NULL) {
                free(bit_processing_img);
            }

            fr_end = esp_timer_get_time();
            ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, Resizing Total Time: %u ms", i, (uint32_t)((fr_end - fr_start) / 1000));


            fb_img[i] = resize_img;
            fs_img[i] = resize_img_size; // todo check this value

		}
		else{
			offset += (uint32_t)fs[i-1];

			memset(img_buffer, 0, BUFFER_LEN_READ);
			memcpy(img_buffer, fb+offset, fs[i]);
			img = stbi_load_from_memory(img_buffer, &fs[i], &width, &height, &channels, 0 );

			if(img == NULL) {
			    printf("Error in loading the image\n");
			    res = -1;

			}
		    printf("Memory Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
            fr_end = esp_timer_get_time();
            ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, Memory Loaded  Total Time: %u ms", i, (uint32_t)((fr_end - fr_start) / 1000));

            unsigned char *bit_processing_img = resize_image_processing(width, height, channels, img, new_w, new_h);
            fr_end = esp_timer_get_time();
            ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, resize_image_processing Total Time: %u ms", i, (uint32_t)((fr_end - fr_start) / 1000));


            if (bit_processing_img == NULL) {
                printf("resample_image failed\n");
            } // end if

            resize_img = NULL;
            resize_img_size = -1;

            // use new_w, new_h
            resize_img = stbi_write_jpg_to_mem("test.jpg", new_w, new_h, channels, bit_processing_img, 50,  &resize_img_size);
            ESP_LOGI(TAG, "resize_image_size: %d\n", resize_img_size);

            // Fail to get the image
            if (resize_img_size == 0) {
                res = -1;
                continue;
            }

            if (img != NULL) {
                free(img);
            }

            if (bit_processing_img != NULL) {
                free(bit_processing_img);
            }

            fr_end = esp_timer_get_time();
            ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, Resizing Total Time: %u ms", i, (uint32_t)((fr_end - fr_start) / 1000));

            fb_img[i] = resize_img;
            fs_img[i] = resize_img_size;

//			break;
		} //else

		if(res < 0){
			ESP_LOGE(TAG, "Error occurred during image sending: errno %d", errno);
			break;
		}

		fr_end = esp_timer_get_time();
		ESP_LOGI(TAG, "JPG Num.%d: %u Bytes @ %u ms, speed %f Bytes/s", i, (uint32_t)fs[i], (uint32_t)((fr_end - fr_start) / 1000),
		(uint32_t)fs[i] * 1.0 / ((uint32_t)((fr_end - fr_start) *1.0/ 1000/1000) ) );
	} // for

	return res;
}


static void tcp_client_task_wifi(uint8_t *fb, size_t *fs)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    if (get_resized_image(fb, fs) < 0 ) {
        ESP_LOGI(TAG, "Get_resized_image Failed");
    }

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");



        while (1) {
            int err = socket_send_wifi(fb, fs, sock);
            if(err < 0)
                break;

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "Received %s", rx_buffer);

                if(strstr(rx_buffer, "End") != NULL)
					break;
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS); //Block for 2000 ms
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket....");
            shutdown(sock, 0);
            close(sock);
            break;
        }
    }
}

void client_task_send_by_wifi(uint8_t *fb, size_t *fs) {
   // ESP_ERROR_CHECK(example_connect());

    tcp_client_task_wifi(fb, fs);
}

void app_tcp_wifi_main(int *flag)
{
//	app_camera_main();

	uint8_t *frame_buffer = (uint8_t *)malloc(IMAGE_NUM*40000*sizeof(uint8_t)); //Allocated enough space for 10 frames @ 61kB size
	size_t *frame_size = (size_t *)malloc(sizeof(size_t)*IMAGE_NUM); //Allocated space for 10 size_t that are 2 bytes each

	if(!frame_buffer)
		ESP_LOGE(TAG, "Memory allocation for frame buffer returned NULL pointer");

	printf("frame_buffer address %p \n", frame_buffer);

	if(!frame_size)
		ESP_LOGE(TAG, "Memory allocation for frame size buffer returned NULL pointer");

	memset(frame_buffer, 0 , IMAGE_NUM * 30000 * sizeof(uint8_t));
	memset(frame_size, 0, sizeof(size_t) * IMAGE_NUM);

    int64_t fr_start, fr_end;
    fr_start = esp_timer_get_time();

	camera_capture_wifi(frame_buffer, frame_size);
	fr_end = esp_timer_get_time();
	ESP_LOGI(TAG, "Camera_capture IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
     // for wifi https://github.com/espressif/esp-idf/blob/master/examples/protocols/README.md
//    ESP_ERROR_CHECK(example_connect());
    fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "Wifi connect IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

    tcp_client_task_wifi(frame_buffer, frame_size);
    //client_task_send_by_bt(frame_buffer, frame_size);
    fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "Task end IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

//    ESP_ERROR_CHECK(example_disconnect());
//    fr_end = esp_timer_get_time();
//    ESP_LOGI(TAG, "Wifi disconnect IMAGE_Num:.%d, Total Time: %u ms", IMAGE_NUM, (uint32_t)((fr_end - fr_start) / 1000));

    free(frame_buffer);
    free(frame_size);
}
