#ifndef XML_H
#define XML_H

/// The pointer is only valid during the function scope.
/// Allocate a new string if you want to store it.
/// `tag` is a null-terminated string
typedef void (*open_tag_cb)(char *tag);

typedef struct {
    open_tag_cb open_tag;
} XmlParser;

typedef enum {
    OPEN_TAG,
    CLOSE_TAG,
    SELF_CLOSE_TAG,
} TagType;

/// Gets the value of an attribute in a tag
int tag_get_attribute(char *tag, char *name);

/// Returns the XML tag type
TagType get_tag_type(char *tag, int tag_len);

/// Returns the offset to the first occurrence of `c` in `content`
/// Returns -1 if not found
int xml_find_offset(char c, char *content);

void xml_parse(XmlParser parser, char *content);

#endif
