#ifndef __EXECUTABLE_FILE_H__
#define __EXECUTABLE_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// An executable file comprises:
// - executable_file_header_t main_header
// - main_header.segment_count times:
//   - executable_segment_header_t segment_header
//   - uint8_t segment_data[segment_header.segment_length - sizeof(executable_segment_header_t)]
// Total size: main_header.total_length

// TODO: Use emulator_word_t for word length fields so that
// they can be saved in big endian
//
// OR, do they need to be? 

typedef struct _executable_segment_header
{
    uint16_t segment_length;
    uint8_t is_code;
} executable_segment_header_t;

typedef struct _executable_file_header
{
    uint16_t total_length;
    uint16_t segment_count;
    uint16_t execution_start_address;
} executable_file_header_t;

#ifdef __cplusplus
}
#endif

#endif // __EXECUTABLE_FILE_H__
