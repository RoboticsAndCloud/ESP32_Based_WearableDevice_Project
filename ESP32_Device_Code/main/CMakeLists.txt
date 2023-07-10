idf_component_register(SRCS  "app_main.c" "app_adxl345.c" "app_tcp.c" "app_tcp_wifi.c" "app_camera.c" "connect.c"
                    INCLUDE_DIRS "." "include"
                    )

set(COMPONENT_REQUIRES
    esp32-camera
    nvs_flash
    mdns
    )

set(COMPONENT_EMBED_FILES
        "www/index_ov2640.html.gz"
        "www/index_ov3660.html.gz"
        "www/index_ov5640.html.gz"
        "www/monitor.html.gz")


