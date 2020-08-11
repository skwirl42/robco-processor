#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _graphics_mode
{
    uint8_t enabled : 1;

    // 000 - 1bpp
    // 001 - 2bpp
    // 010 - 4bpp
    // 011 - 8bpp
    // 1xx - reserved
    uint8_t depth : 3;

    // 0 - black border
    // 1 - green border
    uint8_t border : 1;

    // 000 - 120x80
    // 001 - 192x128
    // 010 - 240x160
    // 011 - 320x240
    // 100 - 480x320
    // 101 - reserved
    // 110 - reserved
    // 111 - reserved
    uint8_t resolution : 3;
} graphics_mode_t;

typedef enum _graphics_depth
{
    ONE_BIT_PER_PIXEL       = 0b000,
    TWO_BITS_PER_PIXEL      = 0b001,
    FOUR_BITS_PER_PIXEL     = 0b010,
    EIGHT_BITS_PER_PIXEL    = 0b011,
} graphics_depth_t;

typedef enum _graphics_resolution
{
    RES_120x80              = 0b000,
    RES_192x128             = 0b001,
    RES_240x160             = 0b010,
    RES_320x240             = 0b011,
    RES_480x320             = 0b100,
} graphics_resolution_t;

typedef enum _graphics_error
{
    GRAPHICS_ERROR_OK                       = 0,
    GRAPHICS_ERROR_UNSUPPORTED_MODE         = 1,
    GRAPHICS_ERROR_SPACE_TOO_SMALL          = 2,
} graphics_error_t;

typedef struct _1_bpp
{
    uint8_t pixel0 : 1;
    uint8_t pixel1 : 1;
    uint8_t pixel2 : 1;
    uint8_t pixel3 : 1;
    uint8_t pixel4 : 1;
    uint8_t pixel5 : 1;
    uint8_t pixel6 : 1;
    uint8_t pixel7 : 1;
} one_bit_per_pixel_t;

typedef struct _2_bpp
{
    uint8_t pixel0 : 2;
    uint8_t pixel1 : 2;
    uint8_t pixel2 : 2;
    uint8_t pixel3 : 2;
} two_bits_per_pixel_t;

typedef struct _4_bpp
{
    uint8_t pixel0 : 4;
    uint8_t pixel1 : 4;
} four_bits_per_pixel_t;

typedef union _pixel_byte
{
    one_bit_per_pixel_t one_bit;
    two_bits_per_pixel_t two_bit;
    four_bits_per_pixel_t four_bit;
    uint8_t eight_bit;
} pixel_byte_t;

// A return value of 0 means the mode isn't supported
int graphics_mem_size_for_mode(graphics_mode_t mode);
int graphics_get_bit_divisor(graphics_mode_t mode);
uint16_t graphics_bytes_per_line(graphics_mode_t mode);
uint16_t graphics_pixels_per_line(graphics_mode_t mode);
uint16_t graphics_get_row_count(graphics_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // __GRAPHICS_H__