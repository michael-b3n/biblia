# OCR test images

Screenshots dropped here are the source of the OCR data the `reference_ocr` test replays.

- The capture runs each image through the real tesseract engine once and writes the recognized words, lines and paragraphs next to it as a `.ocr` file.
- The test only reads the `.ocr` files, so it is fast and independent of the installed tesseract version.
- Neither the images nor the `.ocr` files are committed, `.gitignore` keeps this folder empty except for this file.
- Without `.ocr` files the test skips its capture driven part and still checks its own designed data.

## A good image

- A bible reference inside running text, not one standing on its own.
- Two to four lines of the surrounding paragraph, so the paragraph recognition has neighbouring lines to widen to.
- Best of all a reference broken across a line break, e.g. "... steht in Johannes 3," at the end of one line and "16 und wurde ..." at the start of the next.
- A crop around the paragraph, not a whole desktop.
- Text rendered the way the app sees it: the scaling and font smoothing of the screen it was captured from.

## Capturing

Add the images, then run the capture. It reads every png, bmp, jpg and tif in this folder and writes a `<name>.ocr` next to each:

```bash
build/bibstd_test/bibstd_test.exe "[.capture]"
```

- The capture is hidden from ctest, it needs the real tesseract engine.
- This folder is the `BIBSTD_TEST_OCR_DIR` define of `bibstd_test/CMakeLists.txt`.
- The tessdata folder is found at runtime by `ocr_engine_tesseract::tessdata_folder_finder`.
