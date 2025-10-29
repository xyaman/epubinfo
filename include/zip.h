#ifndef ZIP_H
#define ZIP_H

#include <stdint.h>
#include <stdio.h>

// ZIP Notes
// ref: https://www.vadeen.com/posts/the-zip-file-format/
// ref: https://users.cs.jmu.edu/buchhofp/forensics/formats/pkzip.html
//
// - All multi-bytes numbers are little-endian

#define ZIP_HEADER_LEN 4

// End of Central Directory Record
#define EOCDR_SIGNATURE              0x06054b50
#define EOCDR_SIGNATURE_LEN          4
#define EOCDR_LEN_NO_COMMENT         22    // bytes before optional comment
#define EOCDR_OFF_SIGNATURE          0x00  // 4 bytes
#define EOCDR_OFF_DISK_NUM           0x04  // 2 bytes
#define EOCDR_OFF_START_CDIR_DISK    0x06  // 2 bytes
#define EOCDR_OFF_ENTRIES_DISK       0x08  // 2 bytes
#define EOCDR_OFF_TOTAL_ENTRIES      0x0A  // 2 bytes
#define EOCDR_OFF_CDIR_SIZE          0x0C  // 4 bytes
#define EOCDR_OFF_CDIR_OFFSET        0x10  // 4 bytes
#define EOCDR_OFF_COMMENT_LEN        0x14  // 2 bytes
#define EOCDR_OFF_COMMENT            0x16  // n bytes

// Central Directory Record
#define CDR_LEN_FIXED               46
#define CDR_OFF_SIGNATURE           0x02014b50
#define CDR_OFF_SIGNATURE_LEN       4
#define CDR_OFF_COMPRESSION_METHOD  10
#define CDR_OFF_COMPRESSED_SIZE     20
#define CDR_OFF_UNCOMPRESSED_SIZE   24
#define CDR_OFF_FILENAME_LEN        28
#define CDR_OFF_EXTRA_FIELD_LEN     30
#define CDR_OFF_FILE_COMMENT_LEN    32
#define CDR_OFF_FILE_HEADER         42
#define CDR_OFF_FILENAME            46

// Local File Header
#define LFH_LEN_FIXED               30

typedef struct {
    uint16_t disk_num;
    uint16_t start_cent_dir_disk;
    uint16_t num_of_entries_disk;
    uint16_t num_of_entries;
    uint32_t size_cent_dir;
    uint32_t cent_dir_offset;
} ZipEocdrHeader;

typedef struct {
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint32_t file_offset;
    uint16_t filename_len;
    uint16_t extra_field_len;
    uint16_t compression_method;
    char filename[1024];
} ZipEntry;


int zip_valid_header(FILE *fp);

/// All values of header are initialized on success.
/// Return 1 on success, 0 otherwise.
int zip_read_end_of_central_directory_record(FILE *fp, ZipEocdrHeader *header);

/// Returns an `allocated` array of entries
/// Array length is same as `header.num_of_entries`
ZipEntry* zip_read_central_directory(FILE *fp, ZipEocdrHeader header);

/// Returns an `allocated` string with the entry content
/// Returns `NULL` on error.
char *zip_uncompress_entry(FILE *fp, ZipEntry *entry);

/// `filename` should be a null terminated string
/// Return value is a reference to `entries`
ZipEntry* zip_find_entry_by_filename(ZipEntry *entries, uint16_t num_of_entries, char *filename);

#endif
