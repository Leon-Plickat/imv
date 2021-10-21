#include "backend.h"
#include "bitmap.h"
#include "image.h"
#include "source.h"
#include "source_private.h"

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

struct private {
  fz_context *context;
  fz_document *document;
  fz_page *page;
  int width, height;
};

static void free_private (void *raw_private)
{
  if (!raw_private) {
    return;
  }
  struct private *private = raw_private;
  if (!private->page) {
    fz_drop_page(private->context, private->page);
  }
  if (!private->document) {
    fz_drop_document(private->context, private->document);
  }
  if (!private->context) {
    fz_drop_context(private->context);
  }
}

static void load_image(void *raw_private, struct imv_image **image, int *frametime)
{
  // TODO
  *image = NULL;
  *frametime = 0;

  struct private *private = raw_private;

  fz_rect rect = { .x1 = private->width, .y1 = private->height };
  fz_display_list *disp_list = fz_new_display_list(private->context, rect);
  fz_device *device = fz_new_list_device(private->context, disp_list);
}

static const struct imv_source_vtable vtable = {
  .load_first_frame = load_image,
  .free = free_private,
};

static enum backend_result open_path(const char *path, struct imv_source **src)
{
  struct private private;

  private.context = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  if (!private.context) {
    return BACKEND_UNSUPPORTED;
  }

  fz_try(private.context) {
    fz_register_document_handlers(private.context);
    private.document = fz_open_document(private.context, path);
  } fz_catch(private.context) {
    free_private(&private);
    return BACKEND_UNSUPPORTED;
  }

  if (!private.document) {
    free_private(&private);
    return BACKEND_UNSUPPORTED;
  }

  // TODO password input
  if (fz_needs_password(private.context, private.document) != 0) {
    free_private(&private);
    return BACKEND_UNSUPPORTED;
  }

  fz_try(private.context) {
    // TODO support setting the page index
    private.page = fz_load_page(private.context, private.document, 1);
  } fz_catch(private.context) {
    free_private(&private);
    return BACKEND_UNSUPPORTED;
  }

  fz_rect bbox = fz_bound_page(private.context, private.page);
  private.width = bbox.x1 - bbox.x0;
  private.height = bbox.y1 - bbox.y0;

  struct private *new_private = malloc(sizeof(private));
  memcpy(new_private, &private, sizeof(private));

  *src = imv_source_create(&vtable, new_private);
  return BACKEND_SUCCESS;
}

static enum backend_result open_memory(void *data, size_t len, struct imv_source **src)
{
  // TODO
  return BACKEND_UNSUPPORTED;
}

const struct imv_backend imv_backend_mupdf = {
  .name = "mupdf",
  .description = "MuPDF is a lightweight PDF, XPS, and E-book viewer",
  .website = "https://mupdf.com",
  .license = "AGPLv3",
  .open_path = &open_path,
  .open_memory = &open_memory,
};
