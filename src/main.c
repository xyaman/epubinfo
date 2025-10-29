#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zlib.h>

#include "xml.h"
#include "arena.h"
#include "zip.h"


int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <epub_filename>\nExiting...", argv[0]);
        return 1;
    }

    clock_t t0 = clock();

    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Error opening %s, error code: %d\n", argv[1], errno);
        return 1;
    }

    if (!zip_valid_header(fp)) {
        printf("Invalid zip header file. Exiting...\n");
        fclose(fp);
        return 1;
    }

    ZipEocdrHeader header;
    int success = zip_read_end_of_central_directory_record(fp, &header);
    if (!success) {
        printf("Can't find End Of Central Directory Record\n");
        fclose(fp);
        return 1;
    }

    ZipEntry *entries = zip_read_central_directory(fp, header);
    if (entries == NULL) {
        printf("Error reading Central Directory Record\n");
        fclose(fp);
        return 1;
    }

    // EPUB: META-INF/container.xml decompression
    ZipEntry *container = zip_find_entry_by_filename(entries, header.num_of_entries, "META-INF/container.xml");
    if (container == NULL) {
        printf("Invalid EPUB: `META-INF/container.xml` not found.\n");
        fclose(fp);
        free(entries);
        return 1;
    }

    char *container_content = zip_uncompress_entry(fp, container);
    if (container_content == NULL) {
        printf("Error uncompressing: META-INF/container.xml\n");
        fclose(fp);
        free(entries);
        return 1;
    }

    // EPUB: find rootfile filename (xml parsing)
    XmlParser parser = {0};
    parser.content = container_content;
    Arena arena = {0};
    arena_init(&arena, 1024);

    XmlValue value = {0};
    char *opf_filename;
    while (1) {
        value = xml_next(&arena, &parser);
        if (value.type == EOF_TAG || value.type == ERROR_TAG) {
            printf("Invalid epub: rootfile[full-path] not found.\n");
            return 1;
        }
        // <rootfile full-path="..." />
        if (value.type != SELF_CLOSE_TAG) {
            arena_reset(&arena);
        }

        char *fullpath = xml_tag_get_attribute(&arena, value.content, "full-path");
        if (fullpath != NULL) {
            opf_filename = fullpath;
            break;
        }
    }

    // EPUB: rootfile decompression
    ZipEntry *opf_entry = zip_find_entry_by_filename(entries, header.num_of_entries, opf_filename);
    if (opf_entry == NULL) {
        printf("opf entry not found.\n");
        return 1;
    }

    char *opf_content = zip_uncompress_entry(fp, opf_entry);
    if (opf_content == NULL) {
        printf("opf entry uncompression failed.\n");
        return 1;
    }

    // EPUB: get book metadata (xml parsing)
    // We are not using opf_filename anymore so it's safe to reset the arena
    arena_reset(&arena);

    // reset parser
    parser.cursor = 0;
    parser.content = opf_content;

    // move parser cursor until metadata
    int inside_metadata = 0;
    while (!inside_metadata) {
        XmlValue value = xml_next(&arena, &parser);
        if (value.type == OPEN_TAG || value.type == CLOSE_TAG || value.type == SELF_CLOSE_TAG) {
            char *name = xml_tag_get_name(&arena, value.content);
            if (strcmp(name, "metadata") == 0) inside_metadata = 1;
        }
        arena_reset(&arena);
    }

    // start reading metadata
    // reading until </metadata> is found
    while (1) {
        XmlValue value = xml_next(&arena, &parser);
        if (value.type == CLOSE_TAG) {
            char *tag_name = xml_tag_get_name(&arena, value.content);
            if (strcmp(tag_name, "metadata") == 0) break;
        }

        if (value.type == OPEN_TAG) {
            char *tag_name = xml_tag_get_name(&arena, value.content);
            if (tag_name[0] == 'd' && tag_name[1] == 'c' && tag_name[2] == ':') {
                XmlValue text = xml_next(&arena, &parser);
                if (text.type != TEXT_TAG) {
                    printf("Malformed epub (%s).\n", &tag_name[3]);
                    return 1;
                }

                printf("%s: %s\n", &tag_name[3], text.content);
            }
        }

        arena_reset(&arena);
    }

    fclose(fp);
    free(entries);
    free(container_content);
    free(opf_content);
    arena_free(&arena);

    printf("\nTotal time: %.02fms\n", 1000 * (double)(clock() - t0) / CLOCKS_PER_SEC);
    return 0;
}
