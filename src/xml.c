#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "xml.h"

/// Gets the name of a tag
char *xml_tag_get_name(Arena *arena, char *tag) {
    assert(tag[0] == '<');

    // close tag starts at 2
    // open/self close tag starts at 1
    size_t start_index = tag[1] == '/' ? 2 : 1;

    int end = xml_find_offset(' ', &tag[start_index]);
    if (end == -1) end = xml_find_offset('>', &tag[start_index]);
    if (end == -1) return NULL; // malformed tag

    int len = end;
    char *name = arena_alloc(arena, len + 1, alignof(char));
    memcpy(name, &tag[start_index], len);
    name[len] = '\0';
    return name;
}

/// Gets the value of an attribute in a tag
char *xml_tag_get_attribute(Arena *arena, char *tag, char *name) {
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
    if (equal_sign == NULL) {
        printf("invalid atribute/tag, `\"` not found.\n");
        return 0;
    }

    int start = cursor + 1; // skip "
    int end = equal_sign - tag; // ignore ""
    int len = end - start;

    char *attribute = arena_alloc(arena, sizeof(char) * len + 1, alignof(char));
    memcpy(attribute, &tag[start], len);
    attribute[len] = 0;

    return attribute;
}

/// Returns the XML tag type
TagType get_tag_type(char *tag, int tag_len) {
    if (tag[0] == '<' && tag[1] == '/') return CLOSE_TAG;
    if (tag[tag_len - 2] == '/') return SELF_CLOSE_TAG;
    if (tag[1] == '?' && tag[tag_len - 2] == '?') return DECLARATION_TAG;

    // TODO:
    if (tag[1] == '!' && tag[2] == '-' && tag[3] == '-') return COMMENT_TAG;
    if (tag[1] == '!')  return DECLARATION_TAG;

    return OPEN_TAG;
}

/// Returns the offset to the first occurrence of `c` in `content`
/// Returns -1 if not found
int xml_find_offset(char c, char *content) {
    const char* res = strchr(content, c);
    if (res == NULL) return -1;
    return res - content;
}

/// Doesn't free any resource from the arena.
/// XmlValue.content is allocated in the arena.
XmlValue xml_next(Arena *arena, XmlParser *parser) {

    assert(arena != NULL);
    assert(parser != NULL);
    assert(parser->content != NULL);

    XmlValue value = {0};

    while (parser->content[parser->cursor]) {
        char c = parser->content[parser->cursor];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            parser->cursor++;
            continue;
        }

        switch (c) {
            case '<': {
                int offset = xml_find_offset('>', &parser->content[parser->cursor]);
                if (offset < 0) {
                    value.type = ERROR_TAG;
                    return value;
                }
                int len = offset + 1; // include '>'
                char *str = arena_alloc(arena, len + 1, alignof(char));
                memcpy(str, &parser->content[parser->cursor], len);
                str[len] = '\0';
                parser->cursor += len;

                value.content = str;
                value.type = get_tag_type(str, len);
                return value;
            }


            case '&': {

                printf("unhandled character: &\n");
                parser->cursor++;
                break;
            }


            default: {
                int end = xml_find_offset('<', &parser->content[parser->cursor]);
                if (end < 0) {
                    end = strlen(&parser->content[parser->cursor]);
                }
                int len = end;
                char *str = arena_alloc(arena, len + 1, alignof(char));
                memcpy(str, &parser->content[parser->cursor], len);
                str[len] = '\0';
                parser->cursor += len;

                value.content = str;
                value.type = TEXT_TAG;
                return value;
            }
        }
    }

    value.type = EOF_TAG;
    return value;
}
