#include <cstdint>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <mupdf/fitz.h>

#define ALIGNMENT ((size_t) 16)
#define KBYTE ((size_t) 1024)
#define MBYTE (1024 * KBYTE)
#define GBYTE (1024 * MBYTE)
#define MAX_ALLOCATION (1 * GBYTE)

static size_t used;

static void *fz_limit_reached_ossfuzz(size_t oldsize, size_t size)
{
  if (oldsize == 0)
    fprintf(stderr, "limit: %zu Mbyte used: %zu Mbyte allocation: %zu: limit reached\n", MAX_ALLOCATION / MBYTE, used / MBYTE, size);
  else
    fprintf(stderr, "limit: %zu Mbyte used: %zu Mbyte reallocation: %zu -> %zu: limit reached\n", MAX_ALLOCATION / MBYTE, used / MBYTE, oldsize, size);
  fflush(0);
  return NULL;
}

static void *fz_malloc_ossfuzz(void *opaque, size_t size)
{
  char *ptr = NULL;

  if (size == 0)
    return NULL;
  if (size > SIZE_MAX - ALIGNMENT)
    return NULL;
  if (size + ALIGNMENT > MAX_ALLOCATION - used)
    return fz_limit_reached_ossfuzz(0, size + ALIGNMENT);

  ptr = (char *) malloc(size + ALIGNMENT);
  if (ptr == NULL)
    return NULL;

  memcpy(ptr, &size, sizeof(size));
  used += size + ALIGNMENT;

  return ptr + ALIGNMENT;
}

static void fz_free_ossfuzz(void *opaque, void *ptr)
{
  size_t size;

  if (ptr == NULL)
    return;
  if (ptr < (void *) ALIGNMENT)
    return;

  ptr = (char *) ptr - ALIGNMENT;
  memcpy(&size, ptr, sizeof(size));

  used -= size + ALIGNMENT;
  free(ptr);
}

static void *fz_realloc_ossfuzz(void *opaque, void *old, size_t size)
{
  size_t oldsize;
  char *ptr;

  if (old == NULL)
    return fz_malloc_ossfuzz(opaque, size);
  if (old < (void *) ALIGNMENT)
    return NULL;

  if (size == 0) {
    fz_free_ossfuzz(opaque, old);
    return NULL;
  }
  if (size > SIZE_MAX - ALIGNMENT)
    return NULL;

  old = (char *) old - ALIGNMENT;
  memcpy(&oldsize, old, sizeof(oldsize));

  if (size + ALIGNMENT > MAX_ALLOCATION - used + oldsize + ALIGNMENT)
    return fz_limit_reached_ossfuzz(oldsize + ALIGNMENT, size + ALIGNMENT);

  ptr = (char *) realloc(old, size + ALIGNMENT);
  if (ptr == NULL)
    return NULL;

  used -= oldsize + ALIGNMENT;
  memcpy(ptr, &size, sizeof(size));
  used += size + ALIGNMENT;

  return ptr + ALIGNMENT;
}

static fz_alloc_context fz_alloc_ossfuzz =
{
  NULL,
  fz_malloc_ossfuzz,
  fz_realloc_ossfuzz,
  fz_free_ossfuzz
};

int FuzzerEntryOneInput(const uint8_t *data, size_t size) {
  fz_context *ctx;
  fz_stream *stream;
  fz_document *doc;
  fz_pixmap *pix;

  used = 0;

  ctx = fz_new_context(&fz_alloc_ossfuzz, nullptr, FZ_STORE_DEFAULT);
  stream = NULL;
  doc = NULL;
  pix = NULL;

  fz_var(stream);
  fz_var(doc);
  fz_var(pix);

  fz_try(ctx) {
    fz_register_document_handlers(ctx);
    stream = fz_open_memory(ctx, data, size);
    doc = fz_open_document_with_stream(ctx, "pdf", stream);

    for (int i = 0; i < fz_count_pages(ctx, doc); i++) {
      pix = fz_new_pixmap_from_page_number(ctx, doc, i, fz_identity, fz_device_rgb(ctx), 0);
      fz_drop_pixmap(ctx, pix);
      pix = NULL;
    }
  }
  fz_always(ctx) {
    fz_drop_pixmap(ctx, pix);
    fz_drop_document(ctx, doc);
    fz_drop_stream(ctx, stream);
  }
  fz_catch(ctx) {
    fz_report_error(ctx);
    fz_log_error(ctx, "error rendering pages");
  }

  fz_flush_warnings(ctx);
  fz_drop_context(ctx);

  return 0;
}

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

void processFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::cout << "Running on file: " << filePath << std::endl;

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!buffer.empty()) {
        FuzzerEntryOneInput(buffer.data(), buffer.size());
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <corpus_directory>" << std::endl;
        return 1;
    }

    fs::path corpusDir = argv[1];

    if (!fs::exists(corpusDir) || !fs::is_directory(corpusDir)) {
        std::cerr << "Invalid corpus directory: " << corpusDir << std::endl;
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(corpusDir)) {
        if (fs::is_regular_file(entry)) {
            processFile(entry.path());
        }
    }

    return 0;
}
