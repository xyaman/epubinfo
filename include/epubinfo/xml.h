#ifndef XML_H
#define XML_H
#include <stddef.h>
#include "arena.h"

typedef struct {
    size_t cursor;
    char *content;
} XmlParser;

typedef enum {
    OPEN_TAG,
    CLOSE_TAG,
    SELF_CLOSE_TAG,
    COMMENT_TAG,
    DOCTYPE_TAG,
    DECLARATION_TAG,
    TEXT_TAG,
    EOF_TAG,
    ERROR_TAG,
} TagType;

typedef struct {
    TagType type;
    char *content;
} XmlValue;

typedef struct {
    char *text;
    int len;
} XmlValueSlice;

/// Gets the value of an attribute in a tag
char* xml_tag_get_attribute(Arena *arena, char *tag, char *name);

/// Returns the XML tag name
char* xml_tag_get_name(Arena *arena, char *tag);

/// Returns the XML tag type
TagType get_tag_type(char *tag, int tag_len);

/// Returns the offset to the first occurrence of `c` in `content`
/// Returns -1 if not found
int xml_find_offset(char c, char *content);

XmlValue xml_next(Arena *arena, XmlParser *parser);

#endif
