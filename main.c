#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define ZIP_HEADER_LEN 4

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


#define CDR_LEN_FIXED               46
#define CDR_OFF_SIGNATURE           0x02014b50
#define CDR_OFF_SIGNATURE_LEN       4
#define CDR_OFF_FILENAME_LEN        28
#define CDR_OFF_EXTRA_FIELD_LEN     30
#define CDR_OFF_FILE_COMMENT_LEN    32
#define CDR_OFF_FILE_HEADER         42
#define CDR_OFF_FILENAME            46



// ZIP Notes
// ref: https://www.vadeen.com/posts/the-zip-file-format/
// ref: https://users.cs.jmu.edu/buchhofp/forensics/formats/pkzip.html
//
// - All multi-bytes numbers are little-endian

uint16_t read_le16(const unsigned char *p) {
    return p[0] | (p[1] << 8);
}

uint32_t read_le32(const unsigned char *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

typedef struct {
    uint16_t disk_num;
    uint16_t start_cent_dir_disk;
    uint16_t num_of_entries_disk;
    uint16_t num_of_entries;
    uint32_t size_cent_dir;
    uint32_t cent_dir_offset;
} eocdr_header_t;

typedef struct {
    char filename[1024];
    size_t filename_len;
    size_t file_offset;
} zip_entry_t;

/// All values of header are initialized on success.
/// Return 1 on success, 0 otherwise.
int read_end_of_central_directory_record(FILE *fp, eocdr_header_t *header) {
    // Bytes | Description
    // ------+-------------------------------------------------------------------------
    //     4 | Signature (0x06054b50)
    //     2 | Number of this disk
    //     2 | Disk where central directory starts
    //     2 | Numbers of central directory records on this disk
    //     2 | Total number of central directory records
    //     4 | Size of central directory in bytes
    //     4 | Offset to start of central directory
    //     2 | Comment length (n)
    //     n | Comment
    // ------+--------------------
    //  22+n + Total length

    const int BUFFER_SIZE = 2048;
    unsigned char buffer[BUFFER_SIZE];

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    long position = filesize > BUFFER_SIZE ? filesize - BUFFER_SIZE : 0;
    long signature_start_position = -1;

    while (position >= 0) {
        fseek(fp, position, SEEK_SET);
        size_t bytes_read = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, fp);

        // find eocd logic and return if found
        for (size_t i = 0; i < bytes_read - EOCDR_SIGNATURE_LEN + 1; i++) {
            uint32_t found_signature = *(uint32_t *)(buffer + i);
            if (found_signature == EOCDR_SIGNATURE) {
                signature_start_position = position + i;
                printf("Found signature at: %zu\n", position + i);
                break;
            }
        }

        position -= (BUFFER_SIZE - EOCDR_SIGNATURE_LEN);
    }

    if (signature_start_position == -1) {
        printf("Signature not found\n");
        return 0;
    }

    fseek(fp, signature_start_position, SEEK_SET);
    fread(buffer, sizeof(unsigned char), EOCDR_LEN_NO_COMMENT, fp);

    header->disk_num            = read_le16(&buffer[EOCDR_OFF_DISK_NUM]);
    header->start_cent_dir_disk = read_le16(&buffer[EOCDR_OFF_START_CDIR_DISK]);
    header->num_of_entries_disk = read_le16(&buffer[EOCDR_OFF_ENTRIES_DISK]);
    header->num_of_entries      = read_le16(&buffer[EOCDR_OFF_TOTAL_ENTRIES]);
    header->size_cent_dir       = read_le32(&buffer[EOCDR_OFF_CDIR_SIZE]);
    header->cent_dir_offset     = read_le32(&buffer[EOCDR_OFF_CDIR_OFFSET]);
    return 1;
}

zip_entry_t* read_central_directory(FILE *fp, eocdr_header_t header) {
    // Bytes | Description
    // ------+-------------------------------------------------------------------------
    //     4 | Signature (0x02014b50)
    //     2 | Version made by
    //     2 | Minimum version needed to extract
    //     2 | Bit flag
    //     2 | Compression method
    //     2 | File last modification time (MS-DOS format)
    //
    //     2 | File last modification date (MS-DOS format)
    //     4 | CRC-32 of uncompressed data
    //     4 | Compressed size
    //     4 | Uncompressed size
    //     2 | File name length (n)
    //     2 | Extra field length (m)
    //     2 | File comment length (k)
    //     2 | Disk number where file starts
    //     2 | Internal file attributes
    //
    //     4 | External file attributes
    //     4 | Offset of local file header (from start of disk)
    //     n | File name
    //     m | Extra field
    //     k | File comment

    zip_entry_t *entries = malloc(sizeof(zip_entry_t) * header.num_of_entries);
    if (!entries) return NULL;

    size_t offset = header.cent_dir_offset;

    for (int i = 0; i < header.num_of_entries; i++) {
        unsigned char buffer[CDR_LEN_FIXED];
        fseek(fp, offset, SEEK_SET);
        fread(buffer, sizeof(unsigned char), CDR_LEN_FIXED, fp);

        uint16_t filename_len = read_le16(&buffer[CDR_OFF_FILENAME_LEN]);
        uint16_t extra_len = read_le16(&buffer[CDR_OFF_EXTRA_FIELD_LEN]);
        uint16_t comment_len = read_le16(&buffer[CDR_OFF_FILE_COMMENT_LEN]);

        zip_entry_t entry;
        entry.file_offset = read_le32(&buffer[CDR_OFF_FILE_HEADER]);
        fread(entry.filename, sizeof(char), filename_len, fp);
        entry.filename[filename_len] = 0;
        entry.filename_len = filename_len;

        entries[i] = entry;

        offset += CDR_LEN_FIXED + filename_len + extra_len + comment_len;
    }

    return entries;
}


void parse_metadata(FILE *fp) {
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <epub_filename>\nExiting...", argv[0]);
        return 1;
    }

    clock_t t0 = clock();

    FILE *epub_fp = fopen(argv[1], "rb");
    if (epub_fp == NULL) {
        printf("Error opening %s, error code: %d\n", argv[1], errno);
        return 1;
    }

    unsigned char header_buffer[ZIP_HEADER_LEN];
    fread(header_buffer, sizeof(unsigned char), ZIP_HEADER_LEN, epub_fp);

    if (!(header_buffer[0] == 0x50 && header_buffer[1] == 0x4b && header_buffer[2] == 0x03 && header_buffer[3] == 0x04)) {
        printf("Invalid zip header file. Exiting...\n");
        fclose(epub_fp);
        return 1;
    }
    printf("Valid zip file: %f seconds\n", (double)(clock() - t0) / CLOCKS_PER_SEC);


    t0 = clock();
    eocdr_header_t header;
    int success = read_end_of_central_directory_record(epub_fp, &header);
    if (!success) {
        printf("Can't find End Of Central Directory Record\n");
        fclose(epub_fp);
        return 1;
    }

    printf("EOCDR found: %f seconds\n", (double)(clock() - t0) / CLOCKS_PER_SEC);

    zip_entry_t *entries = read_central_directory(epub_fp, header);
    if (entries == NULL) {
        printf("Error reading Central Directory Record\n");
        fclose(epub_fp);
        return 1;
    }

    for (int i = 0; i < header.num_of_entries; i++) {
        if (entries[i].filename_len == 0) continue;
        if (entries[i].filename[entries[i].filename_len - 1] == '/') continue;
        printf("%d. filename: %s, offset: %zu\n", i, entries[i].filename, entries[i].file_offset);
    }

    fclose(epub_fp);
    free(entries);

    printf("Total time: %f seconds\n", (double)(clock() - t0) / CLOCKS_PER_SEC);
    return 0;
}
