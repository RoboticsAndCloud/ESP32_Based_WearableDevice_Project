#ifndef _APP_TCP_H_
#define _APP_TCP_H_

#ifdef __cplusplus
extern "C" {
#endif

//static void tcp_client_task(void *pvParameters);
void app_tcp_main(int *flag);

void client_task_send_by_bt(uint8_t *fb, uint32_t *fs);

#ifdef __cplusplus
}
#endif

#endif /* _APP_TCP_H_ */
