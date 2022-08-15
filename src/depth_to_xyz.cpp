#include <stdio.h>
#include "sod.h"
#include <iostream>
#include "unistd.h"

#include "sod_img_writer.h"

void
stbi_write_to_stdout(void *context, void *data, int size)
{
	write(STDOUT_FILENO, data, size);
}

int
main(int argc, char **argv)
{
	// for (auto i{ 0 }; i < argc; i++) { std::cerr << i << ": " << argv[i] << "\n"; }

	if (argc < 3) {
		std::cout << argv[0] << " <rgb_size_bytes> <depth_size_bytes>\n";
		exit(EXIT_FAILURE);
	}

	// camera intrinsic param
	float fx = 931.44895319;
	float fy = 931.76842975;
	float cx = 965.9112793;
	float cy = 548.42700032;

	/*
	The following code is task of the server function
	*/

	// rgb_xyz_image stores the output image, two 1080p image horizontally stitched side by side
	// image resolution is (1920*2) * 1080

	long rgb_buf_cap   = strtol(argv[1], NULL, 10);
	long depth_buf_cap = strtol(argv[2], NULL, 10);

	// Read stdin into in-memory buffers for color and depth
	char *rgb_buf   = (char *)calloc(rgb_buf_cap + 1, sizeof(char));
	char *depth_buf = (char *)calloc(depth_buf_cap + 1, sizeof(char));

	std::cin.read(rgb_buf, rgb_buf_cap);
	std::cin.read(depth_buf, depth_buf_cap);

	sod_img color = sod_img_load_from_mem((const unsigned char *)rgb_buf, rgb_buf_cap, 3);
	sod_img depth = sod_img_load_from_mem((const unsigned char *)depth_buf, depth_buf_cap, 3);

	// std::cerr << "color " << color.w << "x" << color.h << "x" << color.c << std::endl;
	// std::cerr << "depth " << depth.w << "x" << depth.h << "x" << depth.c << std::endl;

	sod_img rgb_xyz_image = sod_make_image(2 * depth.w, depth.h, 3);

	// std::cerr << "rgb_xyz_image " << rgb_xyz_image.w << "x" << rgb_xyz_image.h << "x" << rgb_xyz_image.c <<
	// std::endl;

	// loop through every pixel location to process entire image
	for (int v = 0; v < color.h; v++) {
		// copy 32 bit color image to left side of the output image
		for (int u = 0; u < color.w; u++) {
			float r = sod_img_get_pixel(color, u, v, 0);
			float g = sod_img_get_pixel(color, u, v, 1);
			float b = sod_img_get_pixel(color, u, v, 2);

			sod_img_set_pixel(rgb_xyz_image, u, v, 0, r);
			sod_img_set_pixel(rgb_xyz_image, u, v, 1, g);
			sod_img_set_pixel(rgb_xyz_image, u, v, 2, b);
		}

		for (int u = 0; u < color.w; u++) {
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

	unsigned char *zPng = sod_image_to_blob(rgb_xyz_image);
	if (zPng == nullptr) { return SOD_OUTOFMEM; }

	int rc = stbi_write_png_to_func(stbi_write_to_stdout, NULL, rgb_xyz_image.w, rgb_xyz_image.h, rgb_xyz_image.c,
	                                (const void *)zPng, rgb_xyz_image.w * rgb_xyz_image.c);
	sod_image_free_blob(zPng);

	sod_free_image(color);
	sod_free_image(depth);
	sod_free_image(rgb_xyz_image);

	return rc ? SOD_OK : SOD_IOERR;
}
