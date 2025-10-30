# epubinfo

Lightweight and fast CLI tool for extracting basic EPUB metadata,
including author, title, language, and more.

> Note: This library performs minimal validation to maximize speed.
> As a result, it may still attempt to process EPUB files that are invalid or
> malformed. This can sometimes produce unexpected output.
> If you encounter any issues, please feel free to report bugs.

> Only unencrypted EPUB files are supported.

## Usage

```bash
epubinfo <filename.epub>

title: 東京ヴァンパイア・ファイナンス (電撃文庫)
creator: 真藤 順丈
identifier: mobi-asin:B00K7TB5FI
identifier: calibre:13667
identifier: uuid:5f64f6a0-fa65-499c-b6c2-c12f20a6abad
identifier: uuid:8d6e4d30-aa36-411b-a220-b718b064e8d4
language: ja
contributor: calibre (5.11.0) [https://calibre-ebook.com]
date: 2014-02-26T13:00:00+00:00
publisher: 株式会社KADOKAWA
```

## Bun FFI Bindings

Check the examples folder to see how to use it with bun.
Just make sure to have the correct path for the library.

Build the library
```bash
make lib-shared-release
bun examples/bun_ffi.ts
```

Use the library from Bun!
```ts
// main.ts
const epub = EpubMetadata.fromFile("path/to/epub");
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
// dont forget to free!
epub.free();

// output:
// {
//   title: "東京ヴァンパイア・ファイナンス (電撃文庫)",
//   language: "ja",
//   description: "",
//   publisher: "株式会社KADOKAWA",
//   subtitle: "",
//   authors: [],
//   creators: [ "真藤 順丈" ],
//   identifiers: [ "mobi-asin:B00K7TB5FI", "calibre:13667", "uuid:5f64f6a0-fa65-499c-b6c2-c12f20a6abad",
//     "uuid:8d6e4d30-aa36-411b-a220-b718b064e8d4"
//   ],
// }
```

## Build

```bash
# Build both static (.a) and shared (.so/.dylib) libraries
make

# Build CLI
make bin
make bin-release    # release binary

# Build library
make lib-static
make lib-shared

make lib-static-release  # release static library
make lib-shared-release  # release shared library (.so / .dylib)

# Clean build artifacts (out folder & object files)
make clean
```

### Using `epubinfo` library in another C project

```bash
# Clone the repo
git clone https://github.com/xyaman/epubinfo deps/epubinfo

# Build the library
make -C deps/epubinfo lib-static-release   # for static linking in C
make -C deps/epubinfo lib-shared-release   # for dynamic linking / FFI

# Include the headers in your C code
#include "epubinfo/epubinfo.h"

# Link the library
# Static: you need to link to zlib
gcc main.c -Ideps/epubinfo/include deps/epubinfo/out/libepubinfo.a -lz -o myapp

# Dynamic
gcc main.c -Ideps/epubinfo/include deps/epubinfo/out/libepubinfo.so -o myapp
```

### Dependencies

- zlib

## License

MIT License
