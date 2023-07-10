// Load an image and save it in PNG and JPG format using stb_image libraries
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define WIDTH 640
#define HEIGHT 320
#define BUFFER_LEN 100 * 1024

// clang -std=c17 -Wall -pedantic t0.c -o t0

// clang -std=c17 -Wall -pedantic Image.c t2.c -o t2



int main(void) {

//    int width, height, channels;
//    //unsigned char *img = stbi_load("sky.jpg", &width, &height, &channels, 0);
//    unsigned char *img = stbi_load("esp.jpeg", &width, &height, &channels, 0);
//    if(img == NULL) {
//        printf("Error in loading the image\n");
//        exit(1);
//    }
//    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
//
// // Convert the input image to gray
//    size_t img_size = width * height * channels;
//    int gray_channels = channels == 4 ? 2 : 1;
//    size_t gray_img_size = width * height * gray_channels;
//
//    unsigned char *gray_img = malloc(gray_img_size);
//    if(gray_img == NULL) {
//        printf("Unable to allocate memory for the gray image.\n");
//        exit(1);
//    }
//
//    for(unsigned char *p = img, *pg = gray_img; p != img + img_size; p += channels, pg += gray_channels) {
//        *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
//        if(channels == 4) {
//            *(pg + 1) = *(p + 3);
//        }
//    }


    int width1, height1, channels1;
    // typedef unsigned char stbi_uc;
    stbi_uc *img_buffer;
    img_buffer = (stbi_uc *)malloc(BUFFER_LEN*sizeof(stbi_uc));

    //uint8_t *frame_buffer = (uint8_t *)malloc(IMAGE_NUM*30000*sizeof(uint8_t)); //Allocated enough space for 10 frames @ 61kB size

    stbi_uc buffer; // Buffer to store data
    FILE * stream;
    stream = fopen("esp.jpeg", "r");
    int counter = 0;
    while (fread(&buffer, sizeof(stbi_uc), 1, stream)==1) {
        img_buffer[counter] = buffer;
        counter ++ ;
    }

    fclose(stream);
    // Printing data to check validity
    printf("Elements read: %d \n", counter);

    //unsigned char *img = stbi_load("esp.jpeg", &width, &height, &channels, 0);

    unsigned char *img1 = stbi_load_from_memory(img_buffer, counter, &width1, &height1, &channels1, 0 );
    //stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)

    if(img1 == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
    printf("Memory Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width1, height1, channels1);
    printf("Memory Loaded image with a size of data of %d\n", img1[1]);
    //printf("Loaded image with a size of data of %d\n", img[1][1]);//not work


     // Convert the input image to gray
    size_t img_size1 = width1 * height1 * channels1;
    int gray_channels1 = channels1 == 4 ? 2 : 1;
    size_t gray_img_size1 = width1 * height1 * gray_channels1;

    unsigned char *gray_img1 = malloc(gray_img_size1);
    if(gray_img1 == NULL) {
        printf("Unable to allocate memory for the gray image1.\n");
        exit(1);
    }

    for(unsigned char *p = img1, *pg = gray_img1; p != img1 + img_size1; p += channels1, pg += gray_channels1) {
        *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
        if(channels1 == 4) {
            *(pg + 1) = *(p + 3);
        }
    }


   // stbi_write_png("esp_gray.png", width, height, gray_channels, gray_img, width * gray_channels);

    stbi_write_png("esp_gray2.png", width1, height1, gray_channels1, gray_img1, width1 * gray_channels1);


    //stbi_write_png("sky.png", width, height, channels, img, width * channels);
    //stbi_write_jpg("sky2.jpg", width, height, channels, img, 100);

    stbi_image_free(img1);
}
