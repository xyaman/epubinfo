## EPUB METADATA CLI

Lightweight and fast CLI tool to extract basic EPUB metadata.
Retrieve author, title, language, and character count.

> Only support for unencrypted epubs

### Usage

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

### Build

```bash
# debug build
make

# release build
make release

# clean
make clean
```

### Dependencies

- zlib
