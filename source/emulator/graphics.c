#include "graphics.h"

int graphics_get_bit_divisor(graphics_mode_t mode)
{
	int bit_divisor = 1;
	switch (mode.depth)
	{
	case ONE_BIT_PER_PIXEL:
		bit_divisor = 8;
		break;

	case TWO_BITS_PER_PIXEL:
		bit_divisor = 4;
		break;

	case FOUR_BITS_PER_PIXEL:
		bit_divisor = 2;
		break;
	}

	return bit_divisor;
}

int graphics_mem_size_for_mode(graphics_mode_t mode)
{
	int dimensions = 0;
	switch (mode.resolution)
	{
	case RES_120x80:
		dimensions = 120 * 80;
		break;

	case RES_192x128:
		dimensions = 192 * 128;
		break;

	case RES_240x160:
		dimensions = 240 * 160;
		break;

	case RES_320x240:
		dimensions = 320 * 240;
		break;

	case RES_480x320:
		dimensions = 480 * 320;
		break;
	}

	if (dimensions > 0)
	{
		return dimensions / graphics_get_bit_divisor(mode);
	}

	return 0;
}

uint16_t graphics_bytes_per_line(graphics_mode_t mode)
{
	return graphics_pixels_per_line(mode) / graphics_get_bit_divisor(mode);
}

uint16_t graphics_pixels_per_line(graphics_mode_t mode)
{
	int pixel_width = 0;
	switch (mode.resolution)
	{
	case RES_120x80:
		pixel_width = 120;
		break;

	case RES_192x128:
		pixel_width = 192;
		break;

	case RES_240x160:
		pixel_width = 240;
		break;

	case RES_320x240:
		pixel_width = 320;
		break;

	case RES_480x320:
		pixel_width = 480;
		break;
	}

	return pixel_width;
}

uint16_t graphics_get_row_count(graphics_mode_t mode)
{
	int row_count = 0;
	switch (mode.resolution)
	{
	case RES_120x80:
		row_count = 80;
		break;

	case RES_192x128:
		row_count = 128;
		break;

	case RES_240x160:
		row_count = 160;
		break;

	case RES_320x240:
		row_count = 240;
		break;

	case RES_480x320:
		row_count = 320;
		break;
	}

	return row_count;
}