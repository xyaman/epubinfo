#ifndef EPUBINFO_H
#define EPUBINFO_H

/// @brief An opaque handle representing a loaded EPUB document.
typedef struct EpubDocument EpubDocument;

/// @brief An opaque handle representing the metadata of an EPUB document.
typedef struct EpubMetadata EpubMetadata;

/// @brief Loads an EPUB document from a file.
/// @param filename The path to the .epub file.
/// @return A pointer to a new EpubDocument, or NULL on error.
EpubDocument* EpubDocument_from_file(const char *filename);

/// @brief Frees all memory associated with an EPUB document.
/// @param doc The document to free.
void EpubDocument_free(EpubDocument *doc);

/// @brief Gets a non-owned pointer to the document's metadata.
/// @note Do not free this pointer. It is valid only for the lifetime of the EpubDocument.
/// @param doc The document.
/// @return A const pointer to the metadata.
const EpubMetadata* EpubDocument_get_metadata(EpubDocument *doc);

/// @brief Gets the document's title.
/// @param meta The document metadata.
/// @return The title string, or "" if not available.
const char* EpubMetadata_get_title(const EpubMetadata *meta);

/// @brief Gets the document's subtitle.
/// @param meta The document metadata.
/// @return The subtitle string, or "" if not available.
const char* EpubMetadata_get_subtitle(const EpubMetadata *meta);

/// @brief Gets the document's language code (e.g., "en").
/// @param meta The document metadata.
/// @return The language string, or "" if not available.
const char* EpubMetadata_get_language(const EpubMetadata *meta);

/// @brief Gets the document's description.
/// @param meta The document metadata.
/// @return The description string, or "" if not available.
const char* EpubMetadata_get_description(const EpubMetadata *meta);

/// @brief Gets the document's publisher.
/// @param meta The document metadata.
/// @return The publisher string, or "" if not available.
const char* EpubMetadata_get_publisher(const EpubMetadata *meta);

/// @brief Gets the number of authors.
/// @param meta The document metadata.
int EpubMetadata_get_author_count(const EpubMetadata *meta);

/// @brief Gets a specific author by index.
/// @param meta The document metadata.
/// @param index The index of the author (from 0 to count-1).
/// @return The author string, or NULL if the index is out of bounds.
const char* EpubMetadata_get_author(const EpubMetadata *meta, int index);

/// @brief Gets the number of creators.
/// @param meta The document metadata.
int EpubMetadata_get_creator_count(const EpubMetadata *meta);

/// @brief Gets a specific creator by index.
/// @param meta The document metadata.
/// @param index The index of the creator (from 0 to count-1).
/// @return The creator string, or NULL if the index is out of bounds.
const char* EpubMetadata_get_creator(const EpubMetadata *meta, int index);

/// @brief Gets the number of identifiers.
/// @param meta The document metadata
int EpubMetadata_get_identifier_count(const EpubMetadata *meta);

/// @brief Gets a specific identifier by index.
/// @param meta The document metadata
/// @param index The index of the identifier (from 0 to count-1).
/// @return The identifier string, or NULL if the index is out of bounds.
const char* EpubMetadata_get_identifier(const EpubMetadata *meta, int index);

/// @brief Finds the cover image in the EPUB and saves it to a file.
/// @param doc The document.
/// @param filename The output path to save the image (e.g., "cover.jpg").
/// @return 0 on success, non-zero on failure (e.g., cover not found, can't write file).
int EpubDocument_save_cover(const EpubDocument *doc, const char *filename);

#endif // EPUBINFO_H
