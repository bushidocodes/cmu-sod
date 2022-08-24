#include <stdio.h>
#include "sod.h"
#include <stdint.h>

int main()
{
    // camera intrinsic param
    int16_t fx = 931;
    int16_t fy = 931;
    int16_t cx = 965;
    int16_t cy = 548;

    // load color and depth images
    // test image files are located in:
    // ./kinect_image_proc/images_whiteboard/
    //
    // file names are in the form: n_rgb.png/n_depth.png, where n represents
    // synchronized frame number
    sod_img depth = sod_img_load_from_depth_file("../images_whiteboard/0_depth.png", 1);

    /*
    The following code is task of the server function
    */
    // xyz_image stores the output image, two 1080p image horizontally stitched side by side
    // image resolution is (1920*2) * 1080
    int stitched_image_w = 2 * depth.w;
    sod_img xyz_image = sod_make_image(stitched_image_w, depth.h, 3);

    // loop through every pixel location to process entire image
    for (int16_t v = 0; v < depth.h; v++)
    {
        for (int16_t u = 0; u < depth.w; u++)
        {
            uint16_t z_16 = sod_img_get_pixel(depth, u, v, 0);
            int16_t x_16 = z_16 * (u - cx) / fx;
            int16_t y_16 = z_16 * (v - cy) / fy;
            uint8_t z_first_8 = z_16 & 0xFF;
            uint8_t z_second_8 = (z_16 & 0xFF00) >> 8;
            uint8_t x_first_8 = x_16 & 0xFF;
            uint8_t x_second_8 = (x_16 & 0xFF00) >> 8;
            uint8_t y_first_8 = y_16 & 0xFF;
            uint8_t y_second_8 = (y_16 & 0xFF00) >> 8;
            sod_img_set_pixel(xyz_image, u, v, 0, x_first_8);
            sod_img_set_pixel(xyz_image, u, v, 1, y_first_8);
            sod_img_set_pixel(xyz_image, u, v, 2, z_first_8);
            sod_img_set_pixel(xyz_image, u + depth.w, v, 0, x_second_8);
            sod_img_set_pixel(xyz_image, u + depth.w, v, 1, y_second_8);
            sod_img_set_pixel(xyz_image, u + depth.w, v, 2, z_second_8);
        }
    }
    sod_img_save_as_png(xyz_image, "xyz_image.png");
    sod_free_image(depth);
    sod_free_image(xyz_image);

    return 0;
}