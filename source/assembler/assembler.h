#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ASSEMBLER_SUCCESS,
    ASSEMBLER_NOOUTPUT,
    ASSEMBLER_IO_ERROR,
    ASSEMBLER_INTERNAL_ERROR,
    ASSEMBLER_WARNINGS,
    ASSEMBLER_ERRORS,
    ASSEMBLER_ALLOC_FAILED,
    ASSEMBLER_INPUT_ERROR,
    ASSEMBLER_SYMBOL_ERROR,
} assembler_status_t;

typedef struct _assembler_result
{
    assembler_status_t status;
    int warning_count;
    int error_count;
    const char *error;
} assembler_result_t;

assembler_result_t assemble(const char *filename, const char **search_paths);

#ifdef __cplusplus
}
#endif

#endif // __ASSEMBLER_H__
