#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zip.h"
#include "xml.h"

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringArray;

typedef struct {
    char *title;
    char *language;
    char *description;
    char *subtitle;
    char *publisher;

    StringArray author;
    StringArray identifier;
    StringArray creator;
} EpubMetadata;


static void StringArray_append(StringArray *arr, const char *value) {
    if (!value) return;

    if (arr->count == arr->capacity) {
        // double the current capacity (starts at 4)
        size_t new_capacity = arr->capacity ? arr->capacity * 2 : 4;
        char **new_items = realloc(arr->items, sizeof(char*) * new_capacity);

        // no memory
        if (!new_items) return;

        arr->items = new_items;
        arr->capacity = new_capacity;
    }

    arr->items[arr->count] = strdup(value);
    if (arr->items[arr->count] == NULL) return; // no memory
    arr->count += 1;
}

static void StringArray_free(StringArray *arr) {
    for (int i = 0; i < arr->count; i++) free(arr->items[i]);
    free(arr->items);
}

EpubMetadata *get_epub_info(char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening %s\n", filename);
        return NULL;
    }

    if (!zip_valid_header(fp)) {
        fprintf(stderr, "Invalid zip header\n");
        fclose(fp);
        return NULL;
    }

    ZipEocdrHeader header;
    if (!zip_read_end_of_central_directory_record(fp, &header)) {
        fprintf(stderr, "Can't find End Of Central Directory Record\n");
        fclose(fp);
        return NULL;
    }

    ZipEntry *entries = zip_read_central_directory(fp, header);
    if (!entries) {
        fprintf(stderr, "Error reading Central Directory Record\n");
        fclose(fp);
        return NULL;
    }

    ZipEntry *container = zip_find_entry_by_filename(entries, header.num_of_entries, "META-INF/container.xml");
    if (!container) {
        fprintf(stderr, "Invalid EPUB: container.xml not found\n");
        fclose(fp);
        free(entries);
        return NULL;
    }

    char *container_content = zip_uncompress_entry(fp, container);
    if (!container_content) {
        fprintf(stderr, "Error uncompressing container.xml\n");
        fclose(fp);
        free(entries);
        return NULL;
    }

    XmlParser parser = {0};
    parser.content = container_content;
    Arena arena = {0};
    arena_init(&arena, 1024);

    XmlValue value = {0};
    char *opf_filename = NULL;
    while (1) {
        value = xml_next(&arena, &parser);
        if (value.type == EOF_TAG || value.type == ERROR_TAG) {
            fprintf(stderr, "Invalid epub: rootfile not found\n");
            fclose(fp);
            free(entries);
            free(container_content);
            arena_free(&arena);
            return NULL;
        }

        char *fullpath = xml_tag_get_attribute(&arena, value.content, "full-path");
        if (fullpath) {
            opf_filename = strdup(fullpath);
            break;
        }
        arena_reset(&arena);
    }

    ZipEntry *opf_entry = zip_find_entry_by_filename(entries, header.num_of_entries, opf_filename);
    if (!opf_entry) {
        fprintf(stderr, "opf entry not found\n");
        fclose(fp);
        free(entries);
        free(container_content);
        free(opf_filename);
        arena_free(&arena);
        return NULL;
    }

    char *opf_content = zip_uncompress_entry(fp, opf_entry);
    if (!opf_content) {
        fprintf(stderr, "opf entry uncompression failed\n");
        fclose(fp);
        free(entries);
        free(container_content);
        free(opf_filename);
        arena_free(&arena);
        return NULL;
    }

    arena_reset(&arena);
    parser.cursor = 0;
    parser.content = opf_content;

    int inside_metadata = 0;
    while (!inside_metadata) {
        XmlValue v = xml_next(&arena, &parser);
        if (v.type == OPEN_TAG || v.type == CLOSE_TAG || v.type == SELF_CLOSE_TAG) {
            char *name = xml_tag_get_name(&arena, v.content);
            if (strcmp(name, "metadata") == 0) inside_metadata = 1;
        }
        arena_reset(&arena);
    }

    EpubMetadata *meta = calloc(1, sizeof(EpubMetadata));
    while (1) {
        XmlValue v = xml_next(&arena, &parser);
        if (v.type == CLOSE_TAG) {
            char *tag_name = xml_tag_get_name(&arena, v.content);
            if (strcmp(tag_name, "metadata") == 0) break;
        }

        if (v.type == OPEN_TAG) {
            char *tag_name = xml_tag_get_name(&arena, v.content);
            if (strncmp(tag_name, "dc:", 3) == 0) {
                XmlValue text = xml_next(&arena, &parser);
                if (text.type == TEXT_TAG) {
                    const char *field = &tag_name[3];
                    // -- individual objects
                    if (strcmp(field, "title") == 0)
                        meta->title = strdup(text.content);
                    else if (strcmp(field, "language") == 0)
                        meta->language = strdup(text.content);
                    else if (strcmp(field, "description") == 0)
                        meta->description = strdup(text.content);
                    else if (strcmp(field, "publisher") == 0)
                        meta->publisher = strdup(text.content);
                    else if (strcmp(field, "subject") == 0)
                        meta->subtitle = strdup(text.content);

                    // -- lists/arrays
                    else if (strcmp(field, "creator") == 0)
                        StringArray_append(&meta->creator, text.content);
                    else if (strcmp(field, "identifier") == 0)
                        StringArray_append(&meta->identifier, text.content);
                    else if (strcmp(field, "author") == 0)
                        StringArray_append(&meta->author, text.content);
                }
            }
        }
        arena_reset(&arena);
    }

    fclose(fp);
    free(entries);
    free(container_content);
    free(opf_content);
    arena_free(&arena);
    free(opf_filename);

    return meta;
}

void free_epub_info(EpubMetadata *epub) {
    if (!epub) return;

    free(epub->title);
    free(epub->language);
    free(epub->description);
    free(epub->subtitle);
    free(epub->publisher);

    StringArray_free(&epub->author);
    StringArray_free(&epub->creator);
    StringArray_free(&epub->identifier);

    free(epub);
}

// -- ffi getters

int get_epub_author_count(EpubMetadata *epub) {
    return epub ? epub->author.count : 0;
}

const char *get_epub_author_at(EpubMetadata *epub, int index) {
    if (!epub || index < 0 || index >= epub->author.count) return NULL;
    return epub->author.items[index];
}

int get_epub_creator_count(EpubMetadata *epub) {
    return epub ? epub->creator.count : 0;
}

const char *get_epub_creator_at(EpubMetadata *epub, int index) {
    if (!epub || index < 0 || index >= epub->creator.count) return NULL;
    return epub->creator.items[index];
}

int get_epub_identifier_count(EpubMetadata *epub) {
    return epub ? epub->identifier.count : 0;
}

const char *get_epub_identifier_at(EpubMetadata *epub, int index) {
    if (!epub || index < 0 || index >= epub->identifier.count) return NULL;
    return epub->identifier.items[index];
}

const char *get_epub_title(EpubMetadata *epub) {
    return epub && epub->title ? epub->title : "";
}

const char *get_epub_language(EpubMetadata *epub) {
    return epub && epub->language ? epub->language : "";
}

const char *get_epub_description(EpubMetadata *epub) {
    return epub && epub->description ? epub->description : "";
}

const char *get_epub_publisher(EpubMetadata *epub) {
    return epub && epub->publisher ? epub->publisher : "";
}

const char *get_epub_subtitle(EpubMetadata *epub) {
    return epub && epub->subtitle ? epub->subtitle : "";
}
