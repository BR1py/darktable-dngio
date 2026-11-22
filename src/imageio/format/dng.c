/*
    Raw DNG export with Adobe DNG SDK JPEG XL compression.

    The original Bayer or X-Trans mosaic is exported; the rendered pixelpipe is ignored.
*/

#include "bauhaus/bauhaus.h"
#include "common/darktable.h"
#include "common/exif.h"
#include "common/image.h"
#include "common/image_cache.h"
#include "common/mipmap_cache.h"
#include "control/conf.h"
#include "control/control.h"
#include "develop/develop.h"
#include "develop/imageop.h"
#include "develop/pixelpipe_hb.h"
#include "gui/gtk.h"
#include "imageio/format/imageio_format_api.h"
#include "imageio/imageio_dng.h"

#include <glib/gstdio.h>
#include <math.h>
#include <unistd.h>

DT_MODULE(2)

typedef enum dt_dng_target_version_t
{
  DT_DNG_VERSION_1_1 = 0,
  DT_DNG_VERSION_1_4 = 1,
  DT_DNG_VERSION_1_7_1 = 2
} dt_dng_target_version_t;

typedef enum dt_dng_compression_t
{
  DT_DNG_UNCOMPRESSED = 0,
  DT_DNG_LOSSLESS = 1,
  DT_DNG_LOSSY_MOSAIC = 2,
  DT_DNG_LOSSY_LINEAR = 3
} dt_dng_compression_t;

typedef struct dt_imageio_dng_jxl_t
{
  dt_imageio_module_data_t global;
  int target_version;
  int compression;
  int quality;
  int effort;
  int consider_crop;
  int crop_extension;
  int preview_size;
  int clone_xmp;
  int import_to_catalog;
} dt_imageio_dng_jxl_t;

typedef struct dt_imageio_dng_jxl_gui_t
{
  GtkWidget *target_version;
  GtkWidget *compression;
  GtkWidget *quality;
  GtkWidget *effort;
  GtkWidget *consider_crop;
  GtkWidget *crop_extension;
  GtkWidget *preview_size;
  GtkWidget *clone_xmp;
  GtkWidget *clone_xmp_partial;
  GtkWidget *import_to_catalog;
  gboolean updating_gui;
} dt_imageio_dng_jxl_gui_t;

int dt_dngsdk_transcode(const char *input, const char *output,
                        int target_version, int compression, int quality, int effort,
                        int preview_size);
int dt_dngsdk_transcode_memory(const void *data, size_t length, const char *output,
                                int target_version, int compression, int quality,
                                int effort, int preview_size);


const char *name(void)
{
  return _("DNG");
}

const char *mime(dt_imageio_module_data_t *data)
{
  return "image/x-adobe-dng";
}

const char *extension(dt_imageio_module_data_t *data)
{
  return "dng";
}

int bpp(dt_imageio_module_data_t *data)
{
  return 0;
}

int flags(dt_imageio_module_data_t *data)
{
  const dt_imageio_dng_jxl_t *params = (const dt_imageio_dng_jxl_t *)data;
  return FORMAT_FLAGS_NO_GLOBAL_OPTIONS | FORMAT_FLAGS_RAW_EXPORT
    | (params && params->import_to_catalog ? FORMAT_FLAGS_IMPORT_TO_CATALOG : 0);
}

size_t params_size(dt_imageio_module_format_t *self)
{
  return sizeof(dt_imageio_dng_jxl_t);
}

void *legacy_params(dt_imageio_module_format_t *self,
                    const void *const old_params,
                    const size_t old_params_size,
                    const int old_version,
                    int *new_version,
                    size_t *new_size)
{
  if(old_version == 1
     && old_params_size == G_STRUCT_OFFSET(dt_imageio_dng_jxl_t, import_to_catalog))
  {
    dt_imageio_dng_jxl_t *params = g_malloc0(sizeof(*params));
    memcpy(params, old_params, old_params_size);
    *new_version = 2;
    *new_size = sizeof(*params);
    return params;
  }
  return NULL;
}

void *get_params(dt_imageio_module_format_t *self)
{
  dt_imageio_dng_jxl_t *d = g_malloc0(sizeof(*d));
  if(!d) return NULL;
  d->target_version = dt_conf_get_int("plugins/imageio/format/dng_jxl/target_version");
  d->compression = dt_conf_get_int("plugins/imageio/format/dng_jxl/compression");
  d->quality = dt_conf_get_int("plugins/imageio/format/dng_jxl/quality");
  d->effort = dt_conf_get_int("plugins/imageio/format/dng_jxl/effort");
  d->consider_crop = dt_conf_get_bool("plugins/imageio/format/dng_jxl/consider_crop");
  d->crop_extension = dt_conf_get_int("plugins/imageio/format/dng_jxl/crop_extension");
  d->preview_size = dt_conf_get_int("plugins/imageio/format/dng_jxl/preview_size");
  d->clone_xmp = dt_conf_get_bool("plugins/imageio/format/dng_jxl/clone_xmp");
  d->import_to_catalog = dt_conf_get_bool("plugins/imageio/format/dng_jxl/import_to_catalog");
  return d;
}

void free_params(dt_imageio_module_format_t *self, dt_imageio_module_data_t *params)
{
  g_free(params);
}

static gboolean _compression_supported(const int target_version, const int compression)
{
  if(compression == DT_DNG_UNCOMPRESSED || compression == DT_DNG_LOSSLESS)
    return TRUE;
  if(compression == DT_DNG_LOSSY_LINEAR)
    return target_version >= DT_DNG_VERSION_1_4;
  return compression == DT_DNG_LOSSY_MOSAIC
         && target_version >= DT_DNG_VERSION_1_7_1;
}

static void _update_lossy_controls(dt_imageio_dng_jxl_gui_t *g,
                                   const int target_version,
                                   const int compression)
{
  const gboolean lossy = compression == DT_DNG_LOSSY_MOSAIC
                         || compression == DT_DNG_LOSSY_LINEAR;
  gtk_widget_set_visible(g->quality, lossy);
  gtk_widget_set_visible(g->effort, lossy && target_version == DT_DNG_VERSION_1_7_1);
}

static void _populate_compression(dt_imageio_dng_jxl_gui_t *g,
                                  const int target_version,
                                  int compression)
{
  if(!_compression_supported(target_version, compression))
    compression = target_version >= DT_DNG_VERSION_1_4
                  ? DT_DNG_LOSSY_LINEAR : DT_DNG_LOSSLESS;

  g->updating_gui = TRUE;
  dt_bauhaus_combobox_clear(g->compression);
  dt_bauhaus_combobox_add_full(g->compression, _("uncompressed"),
                               DT_BAUHAUS_COMBOBOX_ALIGN_RIGHT,
                               GINT_TO_POINTER(DT_DNG_UNCOMPRESSED), NULL, TRUE);
  dt_bauhaus_combobox_add_full(g->compression, _("lossless"),
                               DT_BAUHAUS_COMBOBOX_ALIGN_RIGHT,
                               GINT_TO_POINTER(DT_DNG_LOSSLESS), NULL, TRUE);
  if(target_version >= DT_DNG_VERSION_1_7_1)
    dt_bauhaus_combobox_add_full(g->compression, _("lossy, preserve mosaic"),
                                 DT_BAUHAUS_COMBOBOX_ALIGN_RIGHT,
                                 GINT_TO_POINTER(DT_DNG_LOSSY_MOSAIC), NULL, TRUE);
  if(target_version >= DT_DNG_VERSION_1_4)
    dt_bauhaus_combobox_add_full(g->compression, _("lossy, linear raw"),
                                 DT_BAUHAUS_COMBOBOX_ALIGN_RIGHT,
                                 GINT_TO_POINTER(DT_DNG_LOSSY_LINEAR), NULL, TRUE);

  for(int row = 0; row < dt_bauhaus_combobox_length(g->compression); row++)
  {
    dt_bauhaus_combobox_set(g->compression, row);
    if(GPOINTER_TO_INT(dt_bauhaus_combobox_get_data(g->compression)) == compression)
      break;
  }
  g->updating_gui = FALSE;
  dt_conf_set_int("plugins/imageio/format/dng_jxl/compression", compression);
  _update_lossy_controls(g, target_version, compression);
}

int set_params(dt_imageio_module_format_t *self, const void *params, const int size)
{
  if(size != (int)sizeof(dt_imageio_dng_jxl_t)) return 1;
  const dt_imageio_dng_jxl_t *d = params;
  dt_imageio_dng_jxl_gui_t *g = self->gui_data;
  dt_bauhaus_combobox_set(g->target_version, d->target_version);
  _populate_compression(g, d->target_version, d->compression);
  dt_bauhaus_slider_set(g->quality, d->quality);
  dt_bauhaus_slider_set(g->effort, d->effort);
  dt_bauhaus_combobox_set(g->consider_crop, d->consider_crop != 0);
  dt_bauhaus_slider_set(g->crop_extension, d->crop_extension);
  dt_bauhaus_slider_set(g->preview_size, d->preview_size);
  dt_bauhaus_combobox_set(g->clone_xmp, d->clone_xmp != 0);
  dt_bauhaus_combobox_set(g->clone_xmp_partial, d->clone_xmp != 0);
  dt_bauhaus_combobox_set(g->import_to_catalog, d->import_to_catalog != 0);
  _update_lossy_controls(g, d->target_version, d->compression);
  gtk_widget_set_visible(g->crop_extension, d->consider_crop != 0);
  gtk_widget_set_visible(g->clone_xmp, d->consider_crop == 0);
  gtk_widget_set_visible(g->clone_xmp_partial, d->consider_crop != 0);
  return 0;
}

// Map the rendered crop boundary back through all active geometric modules.
// The enclosing rectangle stays in the sensor's original orientation, and
// even coordinates preserve the phase of the 2x2 Bayer pattern.
static gboolean _crop_bounds_from_pipe(dt_dev_pixelpipe_t *pipe,
                                       const int raw_width,
                                       const int raw_height,
                                       const int cfa_repeat,
                                       int *x0, int *y0, int *x1, int *y1)
{
  if(!pipe || pipe->processed_width <= 0 || pipe->processed_height <= 0)
    return FALSE;

  enum { EDGE_SAMPLES = 33, POINTS = EDGE_SAMPLES * 4 };
  float points[POINTS * 2];
  for(int i = 0; i < EDGE_SAMPLES; i++)
  {
    const float t = (float)i / (float)(EDGE_SAMPLES - 1);
    points[(0 * EDGE_SAMPLES + i) * 2 + 0] = t * pipe->processed_width;
    points[(0 * EDGE_SAMPLES + i) * 2 + 1] = 0.0f;
    points[(1 * EDGE_SAMPLES + i) * 2 + 0] = pipe->processed_width;
    points[(1 * EDGE_SAMPLES + i) * 2 + 1] = t * pipe->processed_height;
    points[(2 * EDGE_SAMPLES + i) * 2 + 0] = (1.0f - t) * pipe->processed_width;
    points[(2 * EDGE_SAMPLES + i) * 2 + 1] = pipe->processed_height;
    points[(3 * EDGE_SAMPLES + i) * 2 + 0] = 0.0f;
    points[(3 * EDGE_SAMPLES + i) * 2 + 1] = (1.0f - t) * pipe->processed_height;
  }

  GList *modules = g_list_last(pipe->iop);
  GList *pieces = g_list_last(pipe->nodes);
  while(modules && pieces)
  {
    dt_iop_module_t *module = modules->data;
    dt_dev_pixelpipe_iop_t *piece = pieces->data;
    if(piece->enabled && piece->data && module->distort_backtransform)
      module->distort_backtransform(module, piece, points, POINTS);
    modules = g_list_previous(modules);
    pieces = g_list_previous(pieces);
  }
  if(modules || pieces) return FALSE;

  float min_x = points[0], max_x = points[0];
  float min_y = points[1], max_y = points[1];
  for(int i = 1; i < POINTS; i++)
  {
    if(!isfinite(points[i * 2]) || !isfinite(points[i * 2 + 1]))
      return FALSE;
    min_x = fminf(min_x, points[i * 2]);
    max_x = fmaxf(max_x, points[i * 2]);
    min_y = fminf(min_y, points[i * 2 + 1]);
    max_y = fmaxf(max_y, points[i * 2 + 1]);
  }

  int left = CLAMP((int)floorf(min_x), 0, raw_width);
  int top = CLAMP((int)floorf(min_y), 0, raw_height);
  int right = CLAMP((int)ceilf(max_x), 0, raw_width);
  int bottom = CLAMP((int)ceilf(max_y), 0, raw_height);
  left -= left % cfa_repeat;
  top -= top % cfa_repeat;
  right = MIN(raw_width, ((right + cfa_repeat - 1) / cfa_repeat) * cfa_repeat);
  bottom = MIN(raw_height, ((bottom + cfa_repeat - 1) / cfa_repeat) * cfa_repeat);
  if(right - left < cfa_repeat || bottom - top < cfa_repeat) return FALSE;

  *x0 = left;
  *y0 = top;
  *x1 = right;
  *y1 = bottom;
  return TRUE;
}

static int _write_cloned_xmp(const char *filename, const dt_imgid_t imgid,
                             const gboolean reset_geometry, const int compression)
{
  char *xmpfile = g_strconcat(filename, ".xmp", NULL);
  dt_image_t source;
  const gboolean normalized = compression == DT_DNG_LOSSY_MOSAIC
                              || compression == DT_DNG_LOSSY_LINEAR;
  if(normalized)
  {
    const dt_image_t *cached = dt_image_cache_get(imgid, 'r');
    if(!cached) { g_free(xmpfile); return 1; }
    source = *cached;
    dt_image_cache_read_release(cached);
  }
  const gboolean error = dt_exif_xmp_write_dng(imgid, xmpfile, reset_geometry,
                                              normalized ? &source : NULL);
  if(error)
  {
    g_unlink(xmpfile);
    dt_control_log(_("DNG export: cannot write cloned XMP sidecar"));
  }
  g_free(xmpfile);
  return error ? 1 : 0;
}

int write_image(dt_imageio_module_data_t *data, const char *filename,
                const void *in, dt_colorspaces_color_profile_type_t over_type,
                const char *over_filename, void *exif, int exif_len,
                dt_imgid_t imgid, int num, int total,
                dt_dev_pixelpipe_t *pipe, const gboolean export_masks)
{
  const dt_imageio_dng_jxl_t *params = (const dt_imageio_dng_jxl_t *)data;
  int result = 1;
  const int target_version = CLAMP(params->target_version,
                                   DT_DNG_VERSION_1_1, DT_DNG_VERSION_1_7_1);
  const int compression = CLAMP(params->compression,
                                DT_DNG_UNCOMPRESSED, DT_DNG_LOSSY_LINEAR);
  if(!_compression_supported(target_version, compression))
  {
    dt_control_log(_("DNG export: compression is not supported by the selected target version"));
    return 1;
  }
  const int quality = CLAMP(params->quality, 1, 100);
  const int effort = CLAMP(params->effort, 1, 9);
  const int preview_size = CLAMP(params->preview_size, 0, 100);

  // A DNG already contains the authoritative raw geometry, CFA phase,
  // calibration matrices, opcodes and private metadata. Transcode it directly
  // instead of rebuilding it from darktable's normalized mipmap.
  char sourcefile[PATH_MAX];
  gboolean from_cache = TRUE;
  dt_image_full_path(imgid, sourcefile, sizeof(sourcefile), &from_cache);
  const char *source_ext = g_strrstr(sourcefile, ".");
  if(!params->consider_crop && source_ext && !g_ascii_strcasecmp(source_ext, ".dng"))
  {
    result = dt_dngsdk_transcode(sourcefile, filename,
                                 target_version, compression, quality, effort,
                                 preview_size);
    if(result != 0)
    {
      g_unlink(filename);
      dt_control_log(_("DNG export: Adobe DNG transcoding failed"));
    }
    else if(params->clone_xmp)
      result = _write_cloned_xmp(filename, imgid, FALSE, compression);
    return result;
  }

  dt_mipmap_buffer_t mbuf;
  dt_mipmap_cache_get(&mbuf, imgid, DT_MIPMAP_FULL, DT_MIPMAP_BLOCKING, 'r');
  if(!mbuf.buf)
  {
    dt_control_log(_("DNG export: cannot load the original raw image"));
    dt_mipmap_cache_release(&mbuf);
    return 1;
  }

  const dt_image_t *cached = dt_image_cache_get(imgid, 'r');
  if(!cached)
  {
    dt_mipmap_cache_release(&mbuf);
    return 1;
  }
  dt_image_t image = *cached;
  dt_image_cache_read_release(cached);

  if(!(image.flags & DT_IMAGE_RAW)
     || image.buf_dsc.channels != 1
     || image.buf_dsc.filters == 0
     || image.buf_dsc.datatype != TYPE_UINT16)
  {
    dt_control_log(_("DNG export currently supports 16-bit Bayer and X-Trans raw files only"));
    dt_mipmap_cache_release(&mbuf);
    return 1;
  }

  dt_print(DT_DEBUG_IMAGEIO,
           "[dng_jxl] img=%dx%d p=%dx%d crop=%d,%d,%d,%d "
           "mipmap=%dx%d filters=0x%x channels=%u datatype=%d",
           image.width, image.height, image.p_width, image.p_height,
           image.crop_x, image.crop_y, image.crop_right, image.crop_bottom,
           mbuf.width, mbuf.height, image.buf_dsc.filters,
           image.buf_dsc.channels, image.buf_dsc.datatype);

  const uint16_t *raw = (const uint16_t *)mbuf.buf;
  uint16_t *cropped = NULL;
  int output_width = mbuf.width;
  int output_height = mbuf.height;
  if(params->consider_crop)
  {
    const int cfa_repeat = image.buf_dsc.filters == 9u ? 6 : 2;
    int x0, y0, x1, y1;
    if(!_crop_bounds_from_pipe(pipe, mbuf.width, mbuf.height,
                               cfa_repeat,
                               &x0, &y0, &x1, &y1))
    {
      dt_control_log(_("DNG export: cannot map the current crop to raw coordinates"));
      dt_mipmap_cache_release(&mbuf);
      return 1;
    }
    const float extension = CLAMP(params->crop_extension, 0, 99) * 0.01f;
    if(extension > 0.0f)
    {
      x0 = CLAMP((int)floorf(x0 * (1.0f - extension)), 0, mbuf.width);
      y0 = CLAMP((int)floorf(y0 * (1.0f - extension)), 0, mbuf.height);
      x0 -= x0 % cfa_repeat;
      y0 -= y0 % cfa_repeat;
      x1 = MIN(mbuf.width,
               ((int)ceilf(x1 + (mbuf.width - x1) * extension)
                + cfa_repeat - 1) / cfa_repeat * cfa_repeat);
      y1 = MIN(mbuf.height,
               ((int)ceilf(y1 + (mbuf.height - y1) * extension)
                + cfa_repeat - 1) / cfa_repeat * cfa_repeat);
    }
    output_width = x1 - x0;
    output_height = y1 - y0;
    cropped = g_try_malloc_n((size_t)output_width * output_height,
                             sizeof(uint16_t));
    if(!cropped)
    {
      dt_mipmap_cache_release(&mbuf);
      return 1;
    }
    for(int y = 0; y < output_height; y++)
      memcpy(cropped + (size_t)y * output_width,
             raw + (size_t)(y0 + y) * mbuf.width + x0,
             (size_t)output_width * sizeof(uint16_t));
    raw = cropped;

    image.width = image.p_width = output_width;
    image.height = image.p_height = output_height;
    image.crop_x = image.crop_y = 0;
    image.crop_right = image.crop_bottom = 0;
    dt_print(DT_DEBUG_IMAGEIO,
             "[dng_jxl] considering crop: raw rectangle %d,%d %dx%d",
             x0, y0, output_width, output_height);
  }

  GBytes *intermediate = dt_imageio_dng_write_cfa_mosaic_memory(
      raw, output_width, output_height, &image, exif, exif_len);
  g_free(cropped);
  dt_mipmap_cache_release(&mbuf);

  if(intermediate)
  {
    gsize length;
    const void *buffer = g_bytes_get_data(intermediate, &length);
    result = dt_dngsdk_transcode_memory(buffer, length, filename,
                                       target_version, compression, quality, effort,
                                       preview_size);
    g_bytes_unref(intermediate);
  }
  if(result != 0)
  {
    g_unlink(filename);
    dt_control_log(_("DNG export: Adobe DNG compression failed"));
  }
  else if(params->clone_xmp)
    result = _write_cloned_xmp(filename, imgid, params->consider_crop, compression);
  return result;
}

static void _compression_changed(GtkWidget *widget, dt_imageio_dng_jxl_gui_t *g)
{
  if(g->updating_gui) return;
  const int compression = GPOINTER_TO_INT(dt_bauhaus_combobox_get_data(widget));
  const int target_version = dt_bauhaus_combobox_get(g->target_version);
  dt_conf_set_int("plugins/imageio/format/dng_jxl/compression", compression);
  _update_lossy_controls(g, target_version, compression);
}

static void _target_version_changed(GtkWidget *widget, dt_imageio_dng_jxl_gui_t *g)
{
  if(g->updating_gui) return;
  const int target_version = dt_bauhaus_combobox_get(widget);
  const int compression = dt_conf_get_int("plugins/imageio/format/dng_jxl/compression");
  dt_conf_set_int("plugins/imageio/format/dng_jxl/target_version", target_version);
  _populate_compression(g, target_version, compression);
}

static void _quality_changed(GtkWidget *widget, gpointer user_data)
{
  dt_conf_set_int("plugins/imageio/format/dng_jxl/quality",
                  (int)dt_bauhaus_slider_get(widget));
}

static void _effort_changed(GtkWidget *widget, gpointer user_data)
{
  dt_conf_set_int("plugins/imageio/format/dng_jxl/effort",
                  (int)dt_bauhaus_slider_get(widget));
}

static void _crop_extension_changed(GtkWidget *widget, gpointer user_data)
{
  dt_conf_set_int("plugins/imageio/format/dng_jxl/crop_extension",
                  (int)dt_bauhaus_slider_get(widget));
}

static void _preview_size_changed(GtkWidget *widget, gpointer user_data)
{
  dt_conf_set_int("plugins/imageio/format/dng_jxl/preview_size",
                  (int)dt_bauhaus_slider_get(widget));
}

static void _import_to_catalog_changed(GtkWidget *widget, gpointer user_data)
{
  dt_conf_set_bool("plugins/imageio/format/dng_jxl/import_to_catalog",
                   dt_bauhaus_combobox_get(widget) != 0);
}

static void _clone_xmp_changed(GtkWidget *widget, gpointer user_data)
{
  dt_conf_set_bool("plugins/imageio/format/dng_jxl/clone_xmp",
                   dt_bauhaus_combobox_get(widget) != 0);
}

static void _consider_crop_changed(GtkWidget *widget,
                                   dt_imageio_dng_jxl_gui_t *g)
{
  const gboolean active = dt_bauhaus_combobox_get(widget) != 0;
  dt_conf_set_bool("plugins/imageio/format/dng_jxl/consider_crop", active);
  gtk_widget_set_visible(g->crop_extension, active);
  GtkWidget *clone_xmp = active ? g->clone_xmp_partial : g->clone_xmp;
  dt_bauhaus_combobox_set(
      clone_xmp, dt_conf_get_bool("plugins/imageio/format/dng_jxl/clone_xmp") ? 1 : 0);
  gtk_widget_set_visible(g->clone_xmp, !active);
  gtk_widget_set_visible(g->clone_xmp_partial, active);
}

void gui_init(dt_imageio_module_format_t *self)
{
  dt_imageio_dng_jxl_gui_t *g = g_malloc0(sizeof(*g));
  self->gui_data = g;

  const int target_version = CLAMP(
      dt_conf_get_int("plugins/imageio/format/dng_jxl/target_version"),
      DT_DNG_VERSION_1_1, DT_DNG_VERSION_1_7_1);
  const int compression = dt_conf_get_int("plugins/imageio/format/dng_jxl/compression");

  DT_BAUHAUS_COMBOBOX_NEW_FULL(g->target_version, self, NULL,
      N_("DNG target version"),
      _("minimum DNG version required to read the exported file"),
      target_version, _target_version_changed, g,
      N_("1.1"), N_("1.4"), N_("1.7.1"));

  g->compression = dt_bauhaus_combobox_new((dt_iop_module_t *)self);
  dt_bauhaus_widget_set_label(g->compression, NULL, N_("compression"));
  gtk_widget_set_tooltip_text(
      g->compression,
      _("available modes depend on the selected DNG target version"));
  g_signal_connect(G_OBJECT(g->compression), "value-changed",
                   G_CALLBACK(_compression_changed), g);

  g->quality = dt_bauhaus_slider_new_with_range(
      (dt_iop_module_t *)self, 1, 100, 1, 90, 0);
  // The export panel calls gtk_widget_show_all() after initializing a format.
  // Keep lossy-only controls out of that operation so their explicit
  // visibility below remains authoritative on application startup.
  gtk_widget_set_no_show_all(g->quality, TRUE);
  dt_bauhaus_slider_set(g->quality,
                        dt_conf_get_int("plugins/imageio/format/dng_jxl/quality"));
  dt_bauhaus_widget_set_label(g->quality, NULL, N_("quality"));
  gtk_widget_set_tooltip_text(
      g->quality,
      _("higher quality preserves more detail and creates larger files"));
  g_signal_connect(G_OBJECT(g->quality), "value-changed",
                   G_CALLBACK(_quality_changed), NULL);

  g->effort = dt_bauhaus_slider_new_with_range(
      (dt_iop_module_t *)self, 1, 9, 1, 7, 0);
  gtk_widget_set_no_show_all(g->effort, TRUE);
  dt_bauhaus_slider_set(g->effort,
                        dt_conf_get_int("plugins/imageio/format/dng_jxl/effort"));
  dt_bauhaus_widget_set_label(g->effort, NULL, N_("encoding effort"));
  gtk_widget_set_tooltip_text(
      g->effort,
      _("higher effort can reduce file size but takes longer to encode"));
  g_signal_connect(G_OBJECT(g->effort), "value-changed",
                   G_CALLBACK(_effort_changed), NULL);
  _populate_compression(g, target_version, compression);

  DT_BAUHAUS_COMBOBOX_NEW_FULL(
      g->consider_crop, self, NULL, N_("consider crop"),
      _("export only the current crop, mapped back to an enclosing rectangle in the original sensor orientation"),
      dt_conf_get_bool("plugins/imageio/format/dng_jxl/consider_crop") ? 1 : 0,
      _consider_crop_changed, g, N_("no"), N_("yes"));

  g->crop_extension = dt_bauhaus_slider_new_with_range(
      (dt_iop_module_t *)self, 0, 99, 1, 0, 0);
  gtk_widget_set_no_show_all(g->crop_extension, TRUE);
  dt_bauhaus_slider_set(g->crop_extension,
                        dt_conf_get_int("plugins/imageio/format/dng_jxl/crop_extension"));
  dt_bauhaus_slider_set_format(g->crop_extension, "%");
  dt_bauhaus_widget_set_label(g->crop_extension, NULL, N_("extend crop"));
  gtk_widget_set_tooltip_text(
      g->crop_extension,
      _("extend the crop toward the full raw image; 0% exports only the crop"));
  gtk_widget_set_visible(
      g->crop_extension,
      dt_conf_get_bool("plugins/imageio/format/dng_jxl/consider_crop"));
  g_signal_connect(G_OBJECT(g->crop_extension), "value-changed",
                   G_CALLBACK(_crop_extension_changed), NULL);

  g->preview_size = dt_bauhaus_slider_new_with_range(
      (dt_iop_module_t *)self, 0, 100, 1, 0, 0);
  dt_bauhaus_slider_set(g->preview_size,
                        dt_conf_get_int("plugins/imageio/format/dng_jxl/preview_size"));
  dt_bauhaus_slider_set_format(g->preview_size, "%");
  dt_bauhaus_widget_set_label(g->preview_size, NULL, N_("preview size"));
  gtk_widget_set_tooltip_text(
      g->preview_size,
      _("preview size relative to the visible raw image; 0% embeds no preview"));
  g_signal_connect(G_OBJECT(g->preview_size), "value-changed",
                   G_CALLBACK(_preview_size_changed), NULL);

  const gboolean consider_crop =
    dt_conf_get_bool("plugins/imageio/format/dng_jxl/consider_crop");
  DT_BAUHAUS_COMBOBOX_NEW_FULL(
      g->clone_xmp, self, NULL, N_("clone XMP sidecar"),
      NULL, dt_conf_get_bool("plugins/imageio/format/dng_jxl/clone_xmp") ? 1 : 0,
      _clone_xmp_changed, NULL, N_("no"), N_("yes"));
  gtk_widget_set_no_show_all(g->clone_xmp, TRUE);
  gtk_widget_set_visible(g->clone_xmp, !consider_crop);

  DT_BAUHAUS_COMBOBOX_NEW_FULL(
      g->clone_xmp_partial, self, NULL, N_("partly clone XMP sidecar"),
      NULL, dt_conf_get_bool("plugins/imageio/format/dng_jxl/clone_xmp") ? 1 : 0,
      _clone_xmp_changed, NULL, N_("no"), N_("yes"));
  gtk_widget_set_no_show_all(g->clone_xmp_partial, TRUE);
  gtk_widget_set_visible(g->clone_xmp_partial, consider_crop);

  DT_BAUHAUS_COMBOBOX_NEW_FULL(
      g->import_to_catalog, self, NULL, N_("import to catalog"),
      _("add the completed DNG to the catalog when exporting to file on disk"),
      dt_conf_get_bool("plugins/imageio/format/dng_jxl/import_to_catalog") ? 1 : 0,
      _import_to_catalog_changed, NULL, N_("no"), N_("yes"));

  self->widget = dt_gui_vbox(g->target_version, g->compression,
                             g->quality, g->effort,
                             g->preview_size, g->consider_crop,
                             g->crop_extension, g->clone_xmp,
                             g->clone_xmp_partial, g->import_to_catalog);
}

void gui_cleanup(dt_imageio_module_format_t *self)
{
  g_free(self->gui_data);
}

void gui_reset(dt_imageio_module_format_t *self)
{
  dt_imageio_dng_jxl_gui_t *g = self->gui_data;
  dt_bauhaus_combobox_set(g->target_version, DT_DNG_VERSION_1_7_1);
  _populate_compression(g, DT_DNG_VERSION_1_7_1, DT_DNG_LOSSLESS);
  dt_bauhaus_slider_set(g->quality, 90);
  dt_bauhaus_slider_set(g->effort, 7);
  dt_bauhaus_combobox_set(g->consider_crop, 0);
  dt_bauhaus_slider_set(g->crop_extension, 0);
  dt_bauhaus_slider_set(g->preview_size, 50);
  dt_bauhaus_combobox_set(g->clone_xmp, 0);
  dt_bauhaus_combobox_set(g->clone_xmp_partial, 0);
  dt_bauhaus_combobox_set(g->import_to_catalog, 0);
}

void init(dt_imageio_module_format_t *self)
{
#ifdef USE_LUA
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, target_version, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, compression, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, quality, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, effort, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, consider_crop, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, crop_extension, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, preview_size, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, clone_xmp, int);
  dt_lua_register_module_member(darktable.lua_state.state, self,
                                dt_imageio_dng_jxl_t, import_to_catalog, int);
#endif
}

void cleanup(dt_imageio_module_format_t *self)
{
}
