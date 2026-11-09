/**
 * @file flash_file_status.c
 * @brief Optional, allocation-free flash-file diagnostic strings.
 *
 * Keeping this switch in its own translation unit prevents applications that
 * never print errors from retaining the strings in their firmware image.
 * Returned pointers have static storage duration and must not be modified.
 */
#include "flash_file.h"

const char *pogo_flash_file_status_string(pogo_flash_file_status_t status) {
    /* A default is required because callers may log values read from damaged
     * memory or produced by a newer library version. */
    switch (status) {
    case POGO_FLASH_FILE_OK: return "ok";
    case POGO_FLASH_FILE_INVALID_ARGUMENT: return "invalid argument";
    case POGO_FLASH_FILE_UNFORMATTED: return "flash-file catalog is unformatted";
    case POGO_FLASH_FILE_CORRUPT_CATALOG: return "flash-file catalog is corrupt";
    case POGO_FLASH_FILE_NOT_FOUND: return "flash file not found";
    case POGO_FLASH_FILE_ALREADY_EXISTS: return "flash file ID already exists";
    case POGO_FLASH_FILE_NAME_EXISTS: return "flash file name already exists";
    case POGO_FLASH_FILE_INVALID_SIZE: return "invalid flash file size";
    case POGO_FLASH_FILE_NO_SPACE: return "no contiguous flash space";
    case POGO_FLASH_FILE_BAD_CHECKSUM: return "flash file checksum mismatch";
    case POGO_FLASH_FILE_VERIFY_FAILED: return "flash-file write verification failed";
    case POGO_FLASH_FILE_GENERATION_EXHAUSTED: return "flash-file generation exhausted";
    case POGO_FLASH_FILE_UNSUPPORTED_FORMAT: return "unsupported catalog or payload format";
    default: return "unknown flash-file status";
    }
}
