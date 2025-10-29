#include <stdio.h>
#include <time.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include "zip.h"

uint16_t read_le16(const unsigned char *p) {
    return p[0] | (p[1] << 8);
}

uint32_t read_le32(const unsigned char *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

// public
int zip_valid_header(FILE *fp) {
    unsigned char header_buffer[ZIP_HEADER_LEN];
    fread(header_buffer, sizeof(unsigned char), ZIP_HEADER_LEN, fp);
    return header_buffer[0] == 0x50 && header_buffer[1] == 0x4b && header_buffer[2] == 0x03 && header_buffer[3] == 0x04;
}

int zip_read_end_of_central_directory_record(FILE *fp, ZipEocdrHeader *header) {
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

ZipEntry* zip_read_central_directory(FILE *fp, ZipEocdrHeader header) {
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

    ZipEntry *entries = malloc(sizeof(ZipEntry) * header.num_of_entries);
    if (!entries) return NULL;

    size_t offset = header.cent_dir_offset;

    for (int i = 0; i < header.num_of_entries; i++) {
        unsigned char buffer[CDR_LEN_FIXED];
        fseek(fp, offset, SEEK_SET);
        fread(buffer, sizeof(unsigned char), CDR_LEN_FIXED, fp);

        uint16_t filename_len = read_le16(&buffer[CDR_OFF_FILENAME_LEN]);
        uint16_t extra_len = read_le16(&buffer[CDR_OFF_EXTRA_FIELD_LEN]);
        uint16_t comment_len = read_le16(&buffer[CDR_OFF_FILE_COMMENT_LEN]);

        ZipEntry entry;
        entry.file_offset = read_le32(&buffer[CDR_OFF_FILE_HEADER]);
        fread(entry.filename, sizeof(char), filename_len, fp);
        entry.filename[filename_len] = 0;
        entry.filename_len = filename_len;

        entry.compression_method = read_le16(&buffer[CDR_OFF_COMPRESSION_METHOD]);
        entry.compressed_size = read_le32(&buffer[CDR_OFF_COMPRESSED_SIZE]);
        entry.uncompressed_size = read_le32(&buffer[CDR_OFF_UNCOMPRESSED_SIZE]);
        entry.extra_field_len = extra_len;

        entries[i] = entry;
        offset += CDR_LEN_FIXED + filename_len + extra_len + comment_len;
    }

    return entries;
}

char *zip_uncompress_entry(FILE *fp, ZipEntry *entry) {
    // clock_t t0 = clock();
    fseek(fp, LFH_LEN_FIXED + entry->file_offset + entry->filename_len + entry->extra_field_len, SEEK_SET);

    unsigned char *compressed_data = malloc(entry->compressed_size);
    if (compressed_data == NULL) return NULL;

    fread(compressed_data, sizeof(unsigned char), entry->compressed_size, fp);
    char *output = malloc(entry->uncompressed_size);
    if (output == NULL) return NULL;

    if (entry->compression_method == 0) {
        memcpy(output, compressed_data, entry->uncompressed_size);
    } else if (entry->compression_method == 8) {
        z_stream strm = {0};
        strm.next_in = compressed_data;
        strm.avail_in = entry->compressed_size;
        strm.next_out = (Bytef *)output;
        strm.avail_out = entry->uncompressed_size;

        if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
            free(compressed_data);
            free(output);
            printf("error inflateInit2\n");
            return NULL;
        }

        int ret;
        if ((ret = inflate(&strm, Z_FINISH)) != Z_STREAM_END) {
            free(compressed_data);
            free(output);
            printf("error: inflate\n");
            return NULL;
        }

        inflateEnd(&strm);
    }

    free(compressed_data);
    // printf("[INFO] %.*s uncompressed in %0.2fms\n", entry->filename_len, entry->filename, 1000 * (double)(clock() - t0) / CLOCKS_PER_SEC);
    return output;
}


ZipEntry* zip_find_entry_by_filename(ZipEntry *entries, uint16_t num_of_entries, char *filename) {
    for (int i = 0; i < num_of_entries; i++) {
        if (entries[i].filename_len == 0) continue;
        if (entries[i].filename[entries[i].filename_len - 1] == '/') continue;

        if (strcmp(entries[i].filename, filename) == 0) {
            return &entries[i];
        }
    }

    return NULL;
}
