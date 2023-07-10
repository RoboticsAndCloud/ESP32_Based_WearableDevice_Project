// Load an image and save it in PNG and JPG format using stb_image libraries
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define WIDTH 640
#define HEIGHT 320
#define BUFFER_LEN_READ 100 * 1024

// clang -std=c17 -Wall -pedantic t0.c -o t0

// clang -std=c17 -Wall -pedantic Image.c t2.c -o t2

#define FILE_PRE "labframe";


int main(void) {
      FILE *f_test = NULL;

      if((void *)f_test) {
        printf("void*f");
      }
//    int width, height, channels;
//    //unsigned char *img = stbi_load("sky.jpg", &width, &height, &channels, 0);
//    unsigned char *img = stbi_load("esp.jpeg", &width, &height, &channels, 0);
//    if(img == NULL) {
//        printf("Error in loading the image\n");
//        exit(1);
//    }
//    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
//
//    stbi_write_jpg("esp_old1_1.jpg", width, height, channels, img, 100);
//
//    return 0;

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


    int width, height, channels;
    // typedef unsigned char stbi_uc;
    stbi_uc *img_buffer;
    img_buffer = (stbi_uc *)malloc(BUFFER_LEN_READ*sizeof(stbi_uc));

    //uint8_t *frame_buffer = (uint8_t *)malloc(IMAGE_NUM*30000*sizeof(uint8_t)); //Allocated enough space for 10 frames @ 61kB size

    stbi_uc buffer; // Buffer to store data
    FILE * stream;
    char filename[20] = FILE_PRE;
//    strcpy(filename, FILE_PRE);

    strcat(filename, ".jpg");

//    stream = fopen("esp.jpeg", "r");

    stream = fopen(filename, "r");


    int counter = 0;
    while (fread(&buffer, sizeof(stbi_uc), 1, stream)==1) {
        img_buffer[counter] = buffer;
        counter ++ ;
    }

    fclose(stream);
    // Printing data to check validity
    printf("Elements read: %d \n", counter);

    //unsigned char *img = stbi_load("esp.jpeg", &width, &height, &channels, 0);

    unsigned char *img = stbi_load_from_memory(img_buffer, counter, &width, &height, &channels, 0 );
    //stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)

    if(img == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
    printf("Memory Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    //printf("Memory Loaded image with a size of data of %d\n", img[1]);
    //printf("Loaded image with a size of data of %d\n", img[1][1]);//not work


//    stbi_write_jpg("esp_old1.jpg", width, height, channels, img, 100); // 70
//
    // todo check the function stbi_write_jpg, import errors

//    return 0;
//

     // Convert the input image to gray
//    size_t img_size = width * height * channels;
//    int gray_channels = channels == 4 ? 2 : 1;
//    size_t gray_img_size = width * height * gray_channels;
//
//    unsigned char *gray_img = malloc(gray_img_size);
//    if(gray_img == NULL) {
//        printf("Unable to allocate memory for the gray image1.\n");
//        exit(1);
//    }
//
//    for(unsigned char *p = img, *pg = gray_img; p != img + img_size; p += channels, pg += gray_channels) {
//        *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
//        if(channels == 4) {
//            *(pg + 1) = *(p + 3);
//        }
//    }

// todo image resize using liner interpolation

//    stbi_write_png("esp_gray.png", width, height, gray_channels, gray_img, width * gray_channels);

    //stbi_write_png("esp_gray2.png", width, height, gray_channels, gray_img, width * gray_channels);


    //stbi_write_png("sky.png", width, height, channels, img, width * channels);
//    stbi_write_jpg("esp_gray_new.jpg", width, height, gray_channels, gray_img, 100);
//    stbi_write_jpg("esp_old.jpg", width, height, channels, img, 100);


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



    void *resize_img = NULL;
    int resize_img_size = -1;

    //stbi_write_jpg("esp_resize_img_new_by_stbi_write_jpg.jpg", width, height, channels, img, 100);

//    resize_img = stbi_write_jpg_to_mem("test.jpg", width, height, channels, img, 100, resize_img,  &resize_img_size);
    resize_img = stbi_write_jpg_to_mem("test.jpg", width, height, channels, sepia_img, 100,  &resize_img_size);

    printf("resize_image_size: %d\n", resize_img_size);
    FILE * pFile;
    // char buffer[] = { 'x' , 'y' , 'z' };
    char resize_filename[20] = FILE_PRE;

    strcat(resize_filename, "_resize.jpg");
    printf("resize_filename: %s\n", resize_filename);

//    pFile = fopen ("esp_resize_img_new.jpg", "wb");
    pFile = fopen (resize_filename, "wb");

    fwrite (resize_img , sizeof(unsigned char), resize_img_size, pFile);
    fclose (pFile);

    stbi_image_free(img);

    return 0;

}
