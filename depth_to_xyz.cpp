#include <stdio.h>
#include "sod.h"
#include <iostream>
#include "unistd.h"
// #include <opencv2/opencv.hpp>

#include "sod_img_writer.h"

// #define pow_2_32 4294967296
// #define pow_2_24 16777216

void stbi_write_to_stdout(void *context, void *data, int size)
{
    write(STDOUT_FILENO, data, size);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << argv[0] << " <rgb_size_bytes> <depth_size_bytes>\n";
        exit(EXIT_FAILURE);
    }

    long rgb_buf_cap = strtol(argv[1], NULL, 10);
    long depth_buf_cap = strtol(argv[2], NULL, 10);

    // Read stdin into in-memory buffers for color and depth
    char *rgb_buf = (char *)calloc(rgb_buf_cap + 1, sizeof(char));
    char *depth_buf = (char *)calloc(depth_buf_cap + 1, sizeof(char));

    std::cin.read(rgb_buf, rgb_buf_cap);
    std::cin.read(depth_buf, depth_buf_cap);

    // load color and depth images
    // test image files are located in:
    // ./kinect_image_proc/images_whiteboard/
    //
    // file names are in the form: n_rgb.png/n_depth.png, where n represents
    // synchronized frame number

    sod_img color = sod_img_load_from_mem((const unsigned char *)rgb_buf, rgb_buf_cap, 3);
    sod_img depth = sod_img_load_from_mem((const unsigned char *)depth_buf, depth_buf_cap, 3);

    // sod_img color = sod_img_load_from_file("../images_whiteboard/0_rgb.png", 3);
    // sod_img depth = sod_img_load_from_depth_file("../images_whiteboard/0_depth.png", 1);
    // sod_img depth = sod_img_load_grayscale("../images_whiteboard/0_depth.png");
    std::cerr << "color " << color.w << "x" << color.h << "x" << color.c << std::endl;
    std::cerr << "depth " << depth.w << "x" << depth.h << "x" << depth.c << std::endl;

    // return 0;

    // camera intrinsic param
    float fx = 931.44895319;
    float fy = 931.76842975;
    float cx = 965.9112793;
    float cy = 548.42700032;

    // load color and depth images
    // test image files are located in:
    // ./kinect_image_proc/images_whiteboard/
    //
    // file names are in the form: n_rgb.png/n_depth.png, where n represents
    // synchronized frame number
    // sod_img color = sod_img_load_from_file("../images_whiteboard/0_rgb.png", 3);
    // sod_img depth = sod_img_load_from_depth_file("../images_whiteboard/0_depth.png", 1);
    // sod_img depth = sod_img_load_grayscale("../images_whiteboard/0_depth.png");
    // std::cout << "color " << color.w << "x" << color.h << "x" << color.c << std::endl;
    // std::cout << "depth " << depth.w << "x" << depth.h << "x" << depth.c << std::endl;

    // cv::Mat depth_cv = cv::imread("../images_whiteboard/0_depth.png", -1);

    /*
    The following code is task of the server function
    */

    // rgb_xyz_image stores the output image, two 1080p image horizontally stitched side by side
    // image resolution is (1920*2) * 1080
    int stitched_image_w = 2 * depth.w;
    sod_img rgb_xyz_image = sod_make_image(stitched_image_w, depth.h, 3);

    std::cerr << "rgb_xyz_image " << rgb_xyz_image.w << "x" << rgb_xyz_image.h << "x" << rgb_xyz_image.c << std::endl;

    // loop through every pixel location to process entire image
    for (int v = 0; v < color.h; v++)
    {
        // copy 32 bit color image to left side of the output image
        for (int u = 0; u < color.w; u++)
        {
            float r = sod_img_get_pixel(color, u, v, 0);
            float g = sod_img_get_pixel(color, u, v, 1);
            float b = sod_img_get_pixel(color, u, v, 2);

            // std::cout << u << "x" << v << std::endl;
            // std::cout << r << std::endl;

            sod_img_set_pixel(rgb_xyz_image, u, v, 0, r);
            sod_img_set_pixel(rgb_xyz_image, u, v, 1, g);
            sod_img_set_pixel(rgb_xyz_image, u, v, 2, b);
        }

        for (int u = 0; u < color.w; u++)
        {
            // float z_correct = static_cast<float>(depth_cv.at<uint16_t>(v, u));
            // float x = z * (u - cx) / fx;
            // float y = z * (v - cy) / fy;

            // rgb_xyz_image.at<cv::Vec3f>(v, u + depth.cols)[0] = x;
            // rgb_xyz_image.at<cv::Vec3f>(v, u + depth.cols)[1] = y;
            // rgb_xyz_image.at<cv::Vec3f>(v, u + depth.cols)[2] = z;

            float z = sod_img_get_pixel(depth, u, v, 0);
            // float z = depth.data[0*depth.h*depth.w + v * depth.w + u];
            float x = z * (u - cx) / fx;
            float y = z * (v - cy) / fy;

            // std::cout << "z " << z << std::endl;
            // std::cout << "z correct " << z_correct << std::endl << std::endl;

            sod_img_set_pixel(rgb_xyz_image, u + depth.w, v, 0, x);
            sod_img_set_pixel(rgb_xyz_image, u + depth.w, v, 1, y);
            sod_img_set_pixel(rgb_xyz_image, u + depth.w, v, 2, z);
        }
    }

    // sod_img_save_as_png(rgb_xyz_image, "rgb_xyz_image.png");

    unsigned char *zPng = sod_image_to_blob(rgb_xyz_image);
    int rc;
    if (zPng == 0)
    {
        return SOD_OUTOFMEM;
    }
    stbi_write_png_to_func(stbi_write_to_stdout, NULL, rgb_xyz_image.w, rgb_xyz_image.h, rgb_xyz_image.c, (const void *)zPng, rgb_xyz_image.w * rgb_xyz_image.c);
    // rc = stbi_write_png(zPath, input.w, input.h, input.c, (const void *)zPng, input.w * input.c);
    sod_image_free_blob(zPng);
    return rc ? SOD_OK : SOD_IOERR;

    // sod_img_blob_save_as_png("rgb_xyz_image.png", rgb_xyz_image.data, rgb_xyz_image.w, rgb_xyz_image.h, rgb_xyz_image.c);

    // sod_free_image(color);
    // sod_free_image(depth);
    // sod_free_image(rgb_xyz_image);

    return 0;
}
