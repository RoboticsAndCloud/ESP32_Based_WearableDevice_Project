#ifndef _APP_CAM_WIFI_H_
#define _APP_CAM_WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif

void app_tcp_wifi_main(int *flag);


void client_task_send_by_wifi(uint8_t *fb, size_t *fs);

#ifdef __cplusplus
}
#endif

#endif /* _APP_CAM_WIFI_H_ */
