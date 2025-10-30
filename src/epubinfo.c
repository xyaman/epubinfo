#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epubinfo/zip.h"
#include "epubinfo/xml.h"
#include "epubinfo/epubinfo.h"

// internal declarations
typedef struct StringArray StringArray;
void StringArray_append(StringArray *arr, const char *value);
void StringArray_free(StringArray *arr);

struct StringArray {
    char **items;
    size_t count;
    size_t capacity;
};

struct EpubMetadata {
    char *title;
    char *subtitle;
    char *language;
    char *description;
    char *publisher;

    StringArray author;
    StringArray creator;
    StringArray identifier;
};

struct EpubDocument {
    char *filename;
    ZipEntry *entries;
    size_t num_of_entries;
    EpubMetadata metadata;
    uint8_t *cover_image;
    size_t cover_image_size;
    char last_error[256];
};


void StringArray_append(StringArray *arr, const char *value) {
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

void StringArray_free(StringArray *arr) {
    for (int i = 0; i < arr->count; i++) free(arr->items[i]);
    free(arr->items);
}

EpubDocument* EpubDocument_from_file(const char *filename) {
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

    EpubMetadata meta = {0};
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
                        meta.title = strdup(text.content);
                    else if (strcmp(field, "language") == 0)
                        meta.language = strdup(text.content);
                    else if (strcmp(field, "description") == 0)
                        meta.description = strdup(text.content);
                    else if (strcmp(field, "publisher") == 0)
                        meta.publisher = strdup(text.content);
                    else if (strcmp(field, "subject") == 0)
                        meta.subtitle = strdup(text.content);

                    // -- lists/arrays
                    else if (strcmp(field, "creator") == 0)
                        StringArray_append(&meta.creator, text.content);
                    else if (strcmp(field, "identifier") == 0)
                        StringArray_append(&meta.identifier, text.content);
                    else if (strcmp(field, "author") == 0)
                        StringArray_append(&meta.author, text.content);
                }
            }
        }
        arena_reset(&arena);
    }

    EpubDocument *doc = calloc(1, sizeof(EpubDocument));
    doc->filename = strdup(filename);
    doc->entries = entries;
    doc->num_of_entries = header.num_of_entries;
    doc->metadata = meta;

    fclose(fp);
    free(container_content);
    free(opf_content);
    arena_free(&arena);
    free(opf_filename);

    return doc;
}

void EpubDocument_free(EpubDocument *doc) {
    if (!doc) return;

    free(doc->entries);
    free(doc->filename);
    free(doc->cover_image);

    free(doc->metadata.title);
    free(doc->metadata.language);
    free(doc->metadata.description);
    free(doc->metadata.subtitle);
    free(doc->metadata.publisher);

    StringArray_free(&doc->metadata.author);
    StringArray_free(&doc->metadata.creator);
    StringArray_free(&doc->metadata.identifier);

    free(doc);
}

const EpubMetadata* EpubDocument_get_metadata(EpubDocument *doc) {
    return doc ? &doc->metadata : NULL;
}

// title
const char* EpubMetadata_get_title(const EpubMetadata *meta) {
    return (meta && meta->title) ? meta->title : "";
}

const char* EpubMetadata_get_subtitle(const EpubMetadata *meta) {
    return (meta && meta->subtitle) ? meta->subtitle : "";
}

const char* EpubMetadata_get_language(const EpubMetadata *meta) {
    return (meta && meta->language) ? meta->language : "";
}


const char* EpubMetadata_get_description(const EpubMetadata *meta) {
    return (meta && meta->description) ? meta->description : "";
}

const char* EpubMetadata_get_publisher(const EpubMetadata *meta) {
    return (meta && meta->publisher) ? meta->publisher : "";
}


int EpubMetadata_get_author_count(const EpubMetadata *meta) {
    return meta ? (int)meta->author.count : 0;
}

const char* EpubMetadata_get_author(const EpubMetadata *meta, int index) {
    return (meta && index >= 0 && index < (int)meta->author.count)
        ? meta->author.items[index]
        : NULL;
}


int EpubMetadata_get_creator_count(const EpubMetadata *meta) {
    return meta ? (int)meta->creator.count : 0;
}

const char* EpubMetadata_get_creator(const EpubMetadata *meta, int index) {
    return (meta && index >= 0 && index < (int)meta->creator.count)
        ? meta->creator.items[index]
        : NULL;
}

int EpubMetadata_get_identifier_count(const EpubMetadata *meta) {
    return meta ? (int)meta->identifier.count : 0;
}

const char* EpubMetadata_get_identifier(const EpubMetadata *meta, int index) {
    return (meta && index >= 0 && index < (int)meta->identifier.count)
        ? meta->identifier.items[index]
        : NULL;
}
