#include <iostream>
#include <istream>
#include <iterator>
#include <ostream>
#include <vector>

#include "sod_img_writer.h"
#include "sod.h"

// camera intrinsic param
constexpr int16_t fx = 931;
constexpr int16_t fy = 931;
constexpr int16_t cx = 965;
constexpr int16_t cy = 548;

int main(int argc, char **argv)
{
	// use stream iterators to copy the stdin stream into a buffer
	// std::cin >> std::noskipws;
	// std::istream_iterator<char> it(std::cin);
	// std::istream_iterator<char> end;
	// std::vector<uint8_t> image_buf{it, end};


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

	// load into an sod_img
	sod_img depth = sod_depth_img_load_from_mem((const unsigned char *)depth_buf, depth_buf_cap, 1);
	sod_img color = sod_img_load_from_mem((const unsigned char *)rgb_buf, rgb_buf_cap, 3);
	// std::cerr << "depth " << depth.w << "x" << depth.h << "x" << depth.c << std::endl;

	// xyz_image stores the output image, two 1080p image horizontally stitched side by side
	// image resolution is (1920*2) * 1080
	int stitched_image_w = 3 * depth.w;
	sod_img xyz_rgb_image = sod_make_image(stitched_image_w, depth.h, 3);

	// loop through every pixel location to process entire image
	for (int16_t v = 0; v < depth.h; v++)
	{
		for (int16_t u = 0; u < depth.w; u++)
		{
			// set xyz
			uint16_t z_16 = sod_img_get_pixel(depth, u, v, 0);
			int16_t x_16 = (z_16 * (u - cx) / fx) + 1000*20;
			int16_t y_16 = (z_16 * (v - cy) / fy) + 1000*20;
			uint8_t z_first_8 = z_16 & 0xFF;
			uint8_t z_second_8 = (z_16 & 0xFF00) >> 8;
			uint8_t x_first_8 = x_16 & 0xFF;
			uint8_t x_second_8 = (x_16 & 0xFF00) >> 8;
			uint8_t y_first_8 = y_16 & 0xFF;
			uint8_t y_second_8 = (y_16 & 0xFF00) >> 8;

			sod_img_set_pixel(xyz_rgb_image, u, v, 0, x_first_8);
			sod_img_set_pixel(xyz_rgb_image, u, v, 1, y_first_8);
			sod_img_set_pixel(xyz_rgb_image, u, v, 2, z_first_8);
			sod_img_set_pixel(xyz_rgb_image, u + depth.w, v, 0, x_second_8);
			sod_img_set_pixel(xyz_rgb_image, u + depth.w, v, 1, y_second_8);
			sod_img_set_pixel(xyz_rgb_image, u + depth.w, v, 2, z_second_8);

			// set color
			float r = sod_img_get_pixel(color, u, v, 0);
			float g = sod_img_get_pixel(color, u, v, 1);
			float b = sod_img_get_pixel(color, u, v, 2);
			sod_img_set_pixel(xyz_rgb_image, u + 2 * depth.w, v, 0, r);
			sod_img_set_pixel(xyz_rgb_image, u + 2 * depth.w, v, 1, g);
			sod_img_set_pixel(xyz_rgb_image, u + 2 * depth.w, v, 2, b);
		}
	}

	// Convert the result to a blob
	auto zPng = sod_image_to_blob(xyz_rgb_image);
	if (zPng == nullptr)
	{
		return SOD_OUTOFMEM;
	}

	// Encode the blob as a PNG and write to STDOUT
	auto rc = stbi_write_png_to_func([](auto context, auto data, auto size)
									 { std::cout.write((const char *)data, size); },
									 NULL, xyz_rgb_image.w, xyz_rgb_image.h, xyz_rgb_image.c,
									 (const void *)zPng, xyz_rgb_image.w * xyz_rgb_image.c);

	// Cleanup
	sod_image_free_blob(zPng);
	sod_free_image(depth);
	sod_free_image(xyz_rgb_image);
	return rc ? SOD_OK : SOD_IOERR;
}
