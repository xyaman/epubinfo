import { dlopen, FFIType } from "bun:ffi";

const lib = dlopen("./out/libepubinfo.so", {
  EpubDocument_from_file: { args: [FFIType.cstring], returns: FFIType.ptr },
  EpubDocument_free: { args: [FFIType.ptr], returns: FFIType.void },
  EpubDocument_get_metadata: { args: [FFIType.ptr], returns: FFIType.ptr },

  EpubMetadata_get_title: { args: [FFIType.ptr], returns: FFIType.cstring },
  EpubMetadata_get_subtitle: { args: [FFIType.ptr], returns: FFIType.cstring },
  EpubMetadata_get_language: { args: [FFIType.ptr], returns: FFIType.cstring },
  EpubMetadata_get_description: { args: [FFIType.ptr], returns: FFIType.cstring },
  EpubMetadata_get_publisher: { args: [FFIType.ptr], returns: FFIType.cstring },

  EpubMetadata_get_author_count: { args: [FFIType.ptr], returns: FFIType.i32 },
  EpubMetadata_get_author: { args: [FFIType.ptr, FFIType.i32], returns: FFIType.cstring },

  EpubMetadata_get_creator_count: { args: [FFIType.ptr], returns: FFIType.i32 },
  EpubMetadata_get_creator: { args: [FFIType.ptr, FFIType.i32], returns: FFIType.cstring },

  EpubMetadata_get_identifier_count: { args: [FFIType.ptr], returns: FFIType.i32 },
  EpubMetadata_get_identifier: { args: [FFIType.ptr, FFIType.i32], returns: FFIType.cstring },
});

function readArray(
  count: number,
  getter: (i: number) => string | null,
): string[] {
  const arr: string[] = [];
  for (let i = 0; i < count; i++) {
    const val = getter(i);
    if (val) arr.push(val);
  }
  return arr;
}

export class EpubDocument {
  ptr: bigint;
  metaPtr: bigint;

  constructor(ptr: bigint) {
    this.ptr = ptr;
    this.metaPtr = lib.symbols.EpubDocument_get_metadata(ptr);
  }

  static fromFile(filename: string) {
    const cstr = Buffer.from(filename + "\0");
    const ptr = lib.symbols.EpubDocument_from_file(cstr);
    if (ptr === 0n) throw new Error("Failed to load EPUB document");
    return new EpubDocument(ptr);
  }

  get title() {
    return lib.symbols.EpubMetadata_get_title(this.metaPtr);
  }

  get subtitle() {
    return lib.symbols.EpubMetadata_get_subtitle(this.metaPtr);
  }

  get language() {
    return lib.symbols.EpubMetadata_get_language(this.metaPtr);
  }

  get description() {
    return lib.symbols.EpubMetadata_get_description(this.metaPtr);
  }

  get publisher() {
    return lib.symbols.EpubMetadata_get_publisher(this.metaPtr);
  }

  get authors() {
    const count = lib.symbols.EpubMetadata_get_author_count(this.metaPtr);
    return readArray(count, (i) =>
      lib.symbols.EpubMetadata_get_author(this.metaPtr, i),
    );
  }

  get creators() {
    const count = lib.symbols.EpubMetadata_get_creator_count(this.metaPtr);

    return readArray(count, (i) =>
      lib.symbols.EpubMetadata_get_creator(this.metaPtr, i),
    );
  }

  get identifiers() {
    const count = lib.symbols.EpubMetadata_get_identifier_count(this.metaPtr);
    return readArray(count, (i) =>
      lib.symbols.EpubMetadata_get_identifier(this.metaPtr, i),
    );
  }

  free() {
    lib.symbols.EpubDocument_free(this.ptr);
    this.ptr = 0n;
    this.metaPtr = 0n;
  }
}

if (import.meta.main) {
  const t0 = Date.now();
  const epub = EpubDocument.fromFile("/home/xyaman/Downloads/the_value_of_others.epub");
  console.log({
    title: epub.title,
    subtitle: epub.subtitle,
    language: epub.language,
    description: epub.description,
    publisher: epub.publisher,
    authors: epub.authors,
    creators: epub.creators,
    identifiers: epub.identifiers,
  });
  epub.free();

  console.log(`Total time: ${Date.now() - t0}ms`);
}
