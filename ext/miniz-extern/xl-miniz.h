#pragma once

#include <cstdint>

extern "C" {

typedef unsigned char mz_uint8;
typedef signed short mz_int16;
typedef unsigned short mz_uint16;
typedef unsigned int mz_uint32;
typedef unsigned int mz_uint;
typedef int64_t mz_int64;
typedef uint64_t mz_uint64;
typedef int mz_bool;

typedef enum {
    MZ_ZIP_MODE_INVALID = 0,
    MZ_ZIP_MODE_READING = 1,
    MZ_ZIP_MODE_WRITING = 2,
    MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED = 3
} mz_zip_mode;

typedef enum {
    MZ_ZIP_TYPE_INVALID = 0,
    MZ_ZIP_TYPE_USER,
    MZ_ZIP_TYPE_MEMORY,
    MZ_ZIP_TYPE_HEAP,
    MZ_ZIP_TYPE_FILE,
    MZ_ZIP_TYPE_CFILE,
    MZ_ZIP_TOTAL_TYPES
} mz_zip_type;

typedef enum {
    MZ_ZIP_NO_ERROR = 0,
    MZ_ZIP_UNDEFINED_ERROR,
    MZ_ZIP_TOO_MANY_FILES,
    MZ_ZIP_FILE_TOO_LARGE,
    MZ_ZIP_UNSUPPORTED_METHOD,
    MZ_ZIP_UNSUPPORTED_ENCRYPTION,
    MZ_ZIP_UNSUPPORTED_FEATURE,
    MZ_ZIP_FAILED_FINDING_CENTRAL_DIR,
    MZ_ZIP_NOT_AN_ARCHIVE,
    MZ_ZIP_INVALID_HEADER_OR_CORRUPTED,
    MZ_ZIP_UNSUPPORTED_MULTIDISK,
    MZ_ZIP_DECOMPRESSION_FAILED,
    MZ_ZIP_COMPRESSION_FAILED,
    MZ_ZIP_UNEXPECTED_DECOMPRESSED_SIZE,
    MZ_ZIP_CRC_CHECK_FAILED,
    MZ_ZIP_UNSUPPORTED_CDIR_SIZE,
    MZ_ZIP_ALLOC_FAILED,
    MZ_ZIP_FILE_OPEN_FAILED,
    MZ_ZIP_FILE_CREATE_FAILED,
    MZ_ZIP_FILE_WRITE_FAILED,
    MZ_ZIP_FILE_READ_FAILED,
    MZ_ZIP_FILE_CLOSE_FAILED,
    MZ_ZIP_FILE_SEEK_FAILED,
    MZ_ZIP_FILE_STAT_FAILED,
    MZ_ZIP_INVALID_PARAMETER,
    MZ_ZIP_INVALID_FILENAME,
    MZ_ZIP_BUF_TOO_SMALL,
    MZ_ZIP_INTERNAL_ERROR,
    MZ_ZIP_FILE_NOT_FOUND,
    MZ_ZIP_ARCHIVE_TOO_LARGE,
    MZ_ZIP_VALIDATION_FAILED,
    MZ_ZIP_WRITE_CALLBACK_FAILED,
    MZ_ZIP_TOTAL_ERRORS
} mz_zip_error;

typedef void* (*mz_alloc_func)(void* opaque, size_t items, size_t size);
typedef void (*mz_free_func)(void* opaque, void* address);
typedef void* (*mz_realloc_func)(void* opaque, void* address, size_t items, size_t size);

typedef size_t (*mz_file_read_func)(void* pOpaque, mz_uint64 file_ofs, void* pBuf, size_t n);
typedef size_t (*mz_file_write_func)(void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n);
typedef mz_bool (*mz_file_needs_keepalive)(void* pOpaque);

struct mz_zip_internal_state_tag;
typedef struct mz_zip_internal_state_tag mz_zip_internal_state;

typedef struct {
    mz_uint64 m_archive_size;
    mz_uint64 m_central_directory_file_ofs;

    /* We only support up to UINT32_MAX files in zip64 mode. */
    mz_uint32 m_total_files;
    mz_zip_mode m_zip_mode;
    mz_zip_type m_zip_type;
    mz_zip_error m_last_error;

    mz_uint64 m_file_offset_alignment;

    mz_alloc_func m_pAlloc;
    mz_free_func m_pFree;
    mz_realloc_func m_pRealloc;
    void* m_pAlloc_opaque;

    mz_file_read_func m_pRead;
    mz_file_write_func m_pWrite;
    mz_file_needs_keepalive m_pNeeds_keepalive;
    void* m_pIO_opaque;

    mz_zip_internal_state* m_pState;

} mz_zip_archive;

extern mz_bool mz_zip_writer_init_heap_v2(mz_zip_archive* pZip, size_t size_to_reserve_at_beginning,
    size_t initial_allocation_size, mz_uint flags);

extern mz_bool mz_zip_writer_add_mem(mz_zip_archive* pZip, const char* pArchive_name,
    const void* pBuf, size_t buf_size, mz_uint level_and_flags);

extern mz_bool mz_zip_writer_finalize_heap_archive(
    mz_zip_archive* pZip, void** ppBuf, size_t* pSize);

extern void mz_free(void* p);

extern mz_bool mz_zip_writer_end(mz_zip_archive* pZip);

} // extern "C"