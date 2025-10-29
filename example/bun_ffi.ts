import { dlopen, FFIType, read, ptr as Ptr } from "bun:ffi";

const lib = dlopen("./bin/libepubinfo.so", {
    get_epub_info: { args: [FFIType.cstring], returns: FFIType.ptr },
    free_epub_info: { args: [FFIType.ptr], returns: FFIType.void },

    get_epub_title: { args: [FFIType.ptr], returns: FFIType.cstring },
    get_epub_language: { args: [FFIType.ptr], returns: FFIType.cstring },
    get_epub_description: { args: [FFIType.ptr], returns: FFIType.cstring },
    get_epub_publisher: { args: [FFIType.ptr], returns: FFIType.cstring },
    get_epub_subtitle: { args: [FFIType.ptr], returns: FFIType.cstring },

    get_epub_author_count: { args: [FFIType.ptr], returns: FFIType.i32 },
    get_epub_author_at: { args: [FFIType.ptr, FFIType.i32], returns: FFIType.cstring },

    get_epub_creator_count: { args: [FFIType.ptr], returns: FFIType.i32 },
    get_epub_creator_at: { args: [FFIType.ptr, FFIType.i32], returns: FFIType.cstring },

    get_epub_identifier_count: { args: [FFIType.ptr], returns: FFIType.i32 },
    get_epub_identifier_at: { args: [FFIType.ptr, FFIType.i32], returns: FFIType.cstring },
});

function readArray(count: number, getter: (i: number) => string | null): string[] {
    const arr: string[] = [];
    for (let i = 0; i < count; i++) {
        const val = getter(i);
        if (val) arr.push(val);
    }
    return arr;
}


export class EpubMetadata {
    ptr: bigint;

    constructor(ptr: bigint) {
        this.ptr = ptr;
    }

    static fromFile(filename: string) {
        const cstr = Buffer.from(filename + "\0")
        const ptr = lib.symbols.get_epub_info(cstr);
        if (ptr === 0n) throw new Error("Failed to load EPUB metadata");
        return new EpubMetadata(ptr);
    }

    get title() {
        return lib.symbols.get_epub_title(this.ptr);
    }

    get language() {
        return lib.symbols.get_epub_language(this.ptr);
    }

    get description() {
        return lib.symbols.get_epub_description(this.ptr);
    }

    get publisher() {
        return lib.symbols.get_epub_publisher(this.ptr);
    }

    get subtitle() {
        return lib.symbols.get_epub_subtitle(this.ptr);
    }

    get authors() {
        const count = lib.symbols.get_epub_author_count(this.ptr);
        return readArray(count, i => lib.symbols.get_epub_author_at(this.ptr, i));
    }

    get creators() {
        const count = lib.symbols.get_epub_creator_count(this.ptr);
        return readArray(count, i => lib.symbols.get_epub_creator_at(this.ptr, i));
    }

    get identifiers() {
        const count = lib.symbols.get_epub_identifier_count(this.ptr);
        return readArray(count, i => lib.symbols.get_epub_identifier_at(this.ptr, i));
    }

    free() {
        lib.symbols.free_epub_info(this.ptr);
        this.ptr = 0n;
    }
}

if (import.meta.main) {
    const t0 = Date.now();
    const epub = EpubMetadata.fromFile("/home/xyaman/Downloads/the_value_of_others.epub");
    console.log({
        title: epub.title,
        language: epub.language,
        description: epub.description,
        publisher: epub.publisher,
        subtitle: epub.subtitle,
        authors: epub.authors,
        creators: epub.creators,
        identifiers: epub.identifiers,
    });
    epub.free();

    console.log(`Total time: ${Date.now() - t0}ms`);
}

