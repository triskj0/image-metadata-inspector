# Image Metadata Inspector

## Usage
```shell
./executable <path to an image>
```

## Examples
<img src="https://github.com/triskj0/imi/blob/main/screenshots/exif-itxt-screenshot.png" alt="screenshot" width = "700"/>

a good example of textual metadata that can be found in a png image:

<img src="https://github.com/triskj0/imi/blob/main/screenshots/text-itxt-screenshot.png" alt="screenshot" width = "700"/>

jpg metadata example:

<img src="https://github.com/triskj0/imi/blob/main/screenshots/jpg-screenshot.png" alt="screenshot" width = "700"/>


## building
- this is what your build.sh/build.bat script for this project might look like

1) on Linux
```shell
cc -o imi jpg_operations.c png_operations.c inspector.c csv_lookup.c
```

2) on Windows
```shell
@echo off
clang -o imi.exe jpg_operations.c png_operations.c inspector.c csv_lookup.c
```
