/* Adobe DNG SDK host bridge for darktable's C-based LibRaw loader. */

#include <libraw/libraw.h>
#include "common/darktable.h"
#include "dng_area_task.h"
#include "dng_file_stream.h"
#include "dng_host.h"
#include "dng_image_writer.h"
#include "dng_info.h"
#include "dng_jxl.h"
#include "dng_negative.h"
#include "dng_preview.h"
#include "dng_render.h"
#include "dng_color_space.h"
#include "dng_tag_values.h"

#include <jxl/encode.h>

#include <algorithm>
#include <exception>
#include <memory>

extern "C" void *dt_libraw_dngsdk_create_host(void)
{
  return new dng_host;
}

extern "C" void dt_libraw_dngsdk_set_host(libraw_data_t *raw, void *host)
{
  static_cast<LibRaw *>(raw->parent_class)->set_dng_host(host);
}

extern "C" void dt_libraw_dngsdk_destroy_host(void *host)
{
  delete static_cast<dng_host *>(host);
}

namespace
{
class dt_dng_jxl_host final : public dng_host
{
public:
  dt_dng_jxl_host(const uint32 quality, const uint32 effort)
    : fQuality(quality), fEffort(effort)
  {
  }

  uint32 PerformAreaTaskThreads() override
  {
#ifdef _OPENMP
    if(omp_in_parallel()) return 1;
#endif
    return (uint32)dt_get_num_threads();
  }

  void PerformAreaTask(dng_area_task &task, const dng_rect &area,
                       dng_area_task_progress *progress = nullptr) override
  {
    const dng_point tile = task.FindTileSize(area);
    const uint32 rows = (area.H() + tile.v - 1) / tile.v;
    const uint32 threads = std::min({PerformAreaTaskThreads(), task.MaxThreads(), rows,
        std::max<uint32>(1, (uint64)area.W() * area.H() / std::max<uint32>(1, task.MinTaskArea()))});
    if(threads <= 1 || progress)
    {
      dng_host::PerformAreaTask(task, area, progress);
      return;
    }
    task.Start(threads, area, tile, &Allocator(), Sniffer());
    std::exception_ptr error;
#pragma omp parallel for num_threads(threads) schedule(static)
    for(uint32 index = 0; index < threads; index++)
    {
      try
      {
        dng_rect part = area;
        part.t = area.t + (rows * index / threads) * tile.v;
        part.b = std::min(area.b, area.t + (int32)(rows * (index + 1) / threads) * tile.v);
        task.ProcessOnThread(index, part, tile, Sniffer(), nullptr);
      }
      catch(...)
      {
#pragma omp critical(dngio_task_error)
        { if(!error) error = std::current_exception(); }
      }
    }
    task.Finish(threads);
    if(error) std::rethrow_exception(error);
  }

  dng_jxl_encode_settings *MakeJXLEncodeSettings(
      use_case_enum use_case, const dng_image &,
      const dng_negative * = nullptr) const override
  {
    const bool lossless = use_case == use_case_LosslessMosaic
                          || use_case == use_case_LosslessMainImage
                          || use_case == use_case_LosslessEnhancedImage
                          || use_case == use_case_LosslessTransparency
                          || use_case == use_case_LosslessDepth
                          || use_case == use_case_LosslessSemanticMask
                          || use_case == use_case_LosslessGainMap;
    dng_jxl_encode_settings *settings = new dng_jxl_encode_settings;
    settings->SetDistance(lossless ? 0.0f
                                   : JxlEncoderDistanceFromQuality((float)fQuality));
    if(lossless) settings->SetFastest();
    settings->SetEffort(fEffort);
    settings->SetUseOriginalColorEncoding(true);
    return settings;
  }

  int32 LossyJPEGQuality() const override
  {
    return (int32)fQuality;
  }

private:
  uint32 fQuality;
  uint32 fEffort;
};

uint32 target_dng_version(const int target_version)
{
  switch(target_version)
  {
    case 0: return dngVersion_1_1_0_0;
    case 1: return dngVersion_1_4_0_0;
    default: return dngVersion_1_7_1_0;
  }
}
}

static int _dngsdk_transcode(const char *input,
                                    const char *output,
                                    int target_version,
                                    int compression,
                                    int quality,
                                    int effort,
                                    int preview_size, const void *data, size_t length)
{
  if((!input && !data) || !output || length > UINT32_MAX) return 1;

  try
  {
    const uint32 dng_version = target_dng_version(target_version);
    const bool lossless_jxl = compression == 1 && target_version == 2;
    const bool lossy_mosaic = compression == 2;
    const bool lossy_linear = compression == 3;
    const bool uncompressed = compression == 0;
    dt_dng_jxl_host host((uint32)std::clamp(quality, 1, 100),
                         (uint32)std::clamp(effort, 1, 9));
    host.SetSaveDNGVersion(dng_version);
    host.SetSaveLinearDNG(false);
    host.SetKeepOriginalFile(false);
    host.SetLossyMosaicJXL(lossy_mosaic);
    host.SetLosslessJXL(lossless_jxl);

    std::unique_ptr<dng_stream> source(data
      ? new dng_stream(data, (uint32)length) : new dng_file_stream(input));
    dng_info info;
    info.Parse(host, *source);
    info.PostParse(host);
    if(!info.IsValidDNG()) return 2;

    AutoPtr<dng_negative> negative;
    negative.Reset(host.Make_dng_negative());
    negative->Parse(host, *source, info);
    negative->PostParse(host, *source, info);
    negative->ReadStage1Image(host, *source, info);
    negative->ValidateRawImageDigest(host);
    negative->SynchronizeMetadata();

    if(negative->Stage1Image()) negative->BuildStage2Image(host);
    // Stage 3 is only needed for rendering or when the SDK must bake stage-3 opcodes.
    if(negative->Stage2Image()
       && (preview_size > 0 || lossy_linear
           || negative->RawImageStage() >= dng_negative::rawImageStagePreOpcode3))
      negative->BuildStage3Image(host);

    dng_image_writer writer;
    if(lossless_jxl)
      negative->LosslessCompressJXL(host, writer);
    else if(lossy_mosaic)
      negative->LossyCompressMosaicJXL(host, writer);
    else if(lossy_linear)
      negative->ConvertToProxy(host, writer, 0, 0);

    dng_preview_list previews;
    if(preview_size > 0 && negative->Stage3Image())
    {
      const uint32 long_edge = std::max(negative->DefaultFinalWidth(),
                                        negative->DefaultFinalHeight());
      const uint32 requested_edge = std::max<uint32>(
          1, (long_edge * (uint32)std::min(preview_size, 100) + 50) / 100);

      auto append_preview = [&](const uint32 maximum_size)
      {
        dng_render render(host, *negative);
        render.SetFinalSpace(negative->IsMonochrome()
                             ? dng_space_GrayGamma22::Get()
                             : dng_space_sRGB::Get());
        render.SetFinalPixelType(ttByte);
        render.SetMaximumSize(maximum_size);
        AutoPtr<dng_image> image(render.Render());
        AutoPtr<dng_preview> preview(new dng_jpeg_preview);
        preview->fInfo.fApplicationName.Set("darktable");
        preview->fInfo.fSettingsName.Set("DNG export");
        preview->fInfo.fColorSpace = image->Planes() == 1
                                    ? previewColorSpace_GrayGamma22
                                    : previewColorSpace_sRGB;
        dng_jpeg_preview *jpeg = dynamic_cast<dng_jpeg_preview *>(preview.Get());
        writer.EncodeJPEGPreview(host, *image, *jpeg, 9);
        previews.Append(preview);
      };

      const uint32 thumbnail_edge = std::min<uint32>(256, requested_edge);
      append_preview(thumbnail_edge);
      if(requested_edge > thumbnail_edge) append_preview(requested_edge);
    }

    dng_file_stream destination(output, true);
    writer.WriteDNG(host, destination, *negative.Get(),
                    previews.Count() ? &previews : nullptr,
                    dng_version, uncompressed);
    destination.Flush();
    return 0;
  }
  catch(...)
  {
    return 3;
  }
}

extern "C" int dt_dngsdk_transcode(const char *input, const char *output,
                                    int target_version, int compression, int quality,
                                    int effort, int preview_size)
{
  return _dngsdk_transcode(input, output, target_version, compression, quality,
                           effort, preview_size, nullptr, 0);
}

extern "C" int dt_dngsdk_transcode_memory(const void *data, size_t length, const char *output,
                                           int target_version, int compression, int quality,
                                           int effort, int preview_size)
{
  return _dngsdk_transcode(nullptr, output, target_version, compression, quality,
                           effort, preview_size, data, length);
}
