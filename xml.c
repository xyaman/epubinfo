#include <stdalign.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "xml.h"

// debug
char *xml_string = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                        "<container xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\" version=\"1.0\">"
                        "    <rootfiles>"
                        "        <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>"
                        "    </rootfiles>"
                        "</container>";

/// Gets the value of an attribute in a tag
int tag_get_attribute(char *tag, char *name) {
    const char *res = strstr(tag, name);
    if (res == NULL) {
        return 0;
    }
    int cursor = res - tag + strlen(name);

    if (tag[cursor] != '=') {
        printf("invalid attribute/tag, `=` not found \n");
        return 0;
    }
    cursor += 1;

    const char *equal_sign = strchr(&tag[cursor + 2], '"');
    if (res == NULL) {
        printf("invalid atribute/tag, `\"` not found.\n");
        return 0;
    }

    int start = cursor + 1; // skip "
    int end = equal_sign - tag; // ignore ""
    int len = end - start;

    printf("attribute value: %.*s\n", len, &tag[start]);
    return 1;
}

/// Returns the XML tag type
TagType get_tag_type(char *tag, int tag_len) {
    if (tag[0] == '<' && tag[1] == '/') return CLOSE_TAG;
    if (tag[tag_len - 2] == '/') return SELF_CLOSE_TAG;
    if (tag[1] == '?' && tag[tag_len - 2] == '?') return SELF_CLOSE_TAG;

    return OPEN_TAG;
}

/// Returns the offset to the first occurrence of `c` in `content`
/// Returns -1 if not found
int xml_find_offset(char c, char *content) {
    const char* res = strchr(content, c);
    if (res == NULL) return -1;
    return res - content;
}

void xml_parse(XmlParser parser, char *content) {
    int cursor = 0;
    int inside_tag = 0;

    Arena arena;
    arena_init(&arena, sizeof(char) * 1024);

    while (content[cursor]) {
        char c = content[cursor];
        if (c == '\n' || c == '\r') {
            cursor += 1;
            continue;
        }

        switch (c) {
            case '<': {
                int offset = xml_find_offset('>', &content[cursor]);
                int strlen = offset + 1;
                char *str = arena_alloc(&arena, sizeof(char)*strlen, alignof(char));
                strncpy(str, &content[cursor], offset + 1);
                str[offset + 1] = 0;

                parser.open_tag(str);
                cursor += offset;
            } break;

            default:
                cursor += 1;
        }

        arena_reset(&arena);
    }

    arena_free(&arena);
}

void open_tag(char *tag) {
    int found = tag_get_attribute(tag, "full-path");
}
