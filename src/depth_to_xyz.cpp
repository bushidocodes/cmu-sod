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
	std::cin >> std::noskipws;
	std::istream_iterator<char> it(std::cin);
	std::istream_iterator<char> end;
	std::vector<uint8_t> image_buf{it, end};

	// load into an sod_img
	sod_img depth = sod_img_load_from_mem((const unsigned char *)image_buf.data(), image_buf.size(), 1);
	// std::cerr << "depth " << depth.w << "x" << depth.h << "x" << depth.c << std::endl;

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

	// Convert the result to a blob
	auto zPng = sod_image_to_blob(xyz_image);
	if (zPng == nullptr)
	{
		return SOD_OUTOFMEM;
	}

	// Encode the blob as a PNG and write to STDOUT
	auto rc = stbi_write_png_to_func([](auto context, auto data, auto size)
									 { std::cout.write((const char *)data, size); },
									 NULL, xyz_image.w, xyz_image.h, xyz_image.c,
									 (const void *)zPng, xyz_image.w * xyz_image.c);

	// Cleanup
	sod_image_free_blob(zPng);
	sod_free_image(depth);
	sod_free_image(xyz_image);
	return rc ? SOD_OK : SOD_IOERR;
}
