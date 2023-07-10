# ESP32_Based_WearableDevice
## ESP 32 Device
- If want to deploy the code on your ESP 32 Device by using the code, please get familiar with ESP32 and the Development environment. You can follow the links below in ESP32 Product section.
- You can read the **app_main.c** for the main function of device
- If you want to resize your image, you can read resize_image_processing() function in the file app_tcp_wifi.c, linear interpolation algorithm is adopted for resizing the images.

## Backend
- We provide two type of servers to communicate with the esp client, WiFi-based(esp_server_wifi.c) and Bluetooth-based(esp_server_bluetooth.c)
- We also provide how to test the throughput of the connection based on WiFi or Bluethooth protocols.


## Citation

Copyright (c) 2023 Fei Liang. To cite my code, please cite my paper:

Here is the BibTeX citation code: 
```
@inproceedings{liang2021energy,
  title={Energy Consumption in a Collaborative Activity Monitoring System using a Companion Robot and a Wearable Device},
  author={Liang, Fei and Hernandez, Ricardo and Sheng, Weihua and Gu, Ye},
  booktitle={2021 IEEE 11th Annual International Conference on CYBER Technology in Automation, Control, and Intelligent Systems (CYBER)},
  pages={812--817},
  year={2021},
  organization={IEEE}
}
```

## Extra links

### ESP32 products
- [ESP32](https://www.espressif.com/en/products/socs/esp32)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/release-v4.0/get-started/index.html)
- [ESP32-CAM Tutorial](https://lastminuteengineers.com/getting-started-with-esp32-cam/)
- [ESP GitHub](https://github.com/espressif)

### Image Resize
- [Linear Interpolation Algorithm ](https://www.sciencedirect.com/topics/engineering/bilinear-interpolation)
- [C Programming - Reading and writing images with the stb_image libraries](https://solarianprogrammer.com/2019/06/10/c-programming-reading-writing-images-stb_image-libraries/)
