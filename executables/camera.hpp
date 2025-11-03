#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <fstream>
#include <functional>
#include <utility>

#include "camera_settings.hpp"

// For development, always 'works' since fully emulated in SW.
static constexpr int X_CAM_TYPE_DUMMY_SW = 0;  // Dummy sw picture
// Manually feed camera data (encoded,rtp) to openhd. Bitrate control and more
// is not working in this mode, making it only valid for development and in
// extreme cases valid for users that want to use a specific ip camera.
static constexpr int X_CAM_TYPE_EXTERNAL = 2;
// For openhd, this is exactly the same as X_CAM_TYPE_EXTERNAL - only file
// start_ip_cam.txt is created Such that the ip cam service can start forwarding
// data to openhd core.
static constexpr int X_CAM_TYPE_EXTERNAL_IP = 3;

// RPIF stands for RPI Foundation (aka original rpi foundation cameras)
static constexpr int X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V1_OV5647 = 30;
static constexpr int X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V2_IMX219 = 31;
static constexpr int X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V3_IMX708 = 32;
static constexpr int X_CAM_TYPE_RPI_LIBCAMERA_RPIF_HQ_IMX477 = 33;

//
// ... rest is reserved for future use
// no camera, only exists to have a default value for secondary camera (which is
// disabled by default). NOTE: The primary camera cannot be disabled !
static constexpr int X_CAM_TYPE_DISABLED = 255;  // Max for uint8_t

static std::string x_cam_type_to_string(int camera_type) {
  switch (camera_type) {
    case X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V1_OV5647:
      return "RPIF_V1_OV5647";
    case X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V2_IMX219:
      return "RPIF_V2_IMX219";
    case X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V3_IMX708:
      return "RPIF_V3_IMX708";
    case X_CAM_TYPE_RPI_LIBCAMERA_RPIF_HQ_IMX477:
      return "RPIF_HQ_IMX477";
    default:
      break;
  }
  std::stringstream ss;
  ss << "UNKNOWN (" << camera_type << ")";
  return ss.str();
}


struct ResolutionFramerate {
  int width_px;
  int height_px;
  int fps;
  std::string as_string() const {
    std::stringstream ss;
    ss << width_px << "x" << height_px << "@" << fps;
    return ss.str();
  }
};


struct XCamera {
  int camera_type = X_CAM_TYPE_DUMMY_SW;
  // 0 for primary camera, 1 for secondary camera
  int index;
  // Only valid if camera is of type USB
  // For CSI camera(s) we in general 'know' from platform and cam type how to
  // tell the pipeline which cam/source to use.
  int usb_v4l2_device_number;

  bool requires_rpi_libcamera_pipeline() const {
    return camera_type >= 30 && camera_type < 60;
  }

  // Returns a list of known supported resolution(s).
  // They should be ordered in ascending resolution / framerate
  // Must always return at least one resolution
  // Might not return all resolutions a camera supports per HW
  std::vector<ResolutionFramerate> get_supported_resolutions() const {
    if (requires_rpi_libcamera_pipeline()) {
      std::vector<ResolutionFramerate> ret;
      if (camera_type == X_CAM_TYPE_RPI_LIBCAMERA_RPIF_HQ_IMX477) {
        ret.push_back(ResolutionFramerate{640, 480, 50});
        ret.push_back(ResolutionFramerate{896, 504, 50});
        ret.push_back(ResolutionFramerate{1280, 720, 50});
        ret.push_back(ResolutionFramerate{1920, 1080, 30});
      } else if (camera_type == X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V2_IMX219) {
        ret.push_back(ResolutionFramerate{640, 480, 47});
        ret.push_back(ResolutionFramerate{896, 504, 47});
        ret.push_back(ResolutionFramerate{1280, 720, 47});
        ret.push_back(ResolutionFramerate{1920, 1080, 30});
      } else if (camera_type == X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V3_IMX708) {
        ret.push_back(ResolutionFramerate{640, 480, 60});
        ret.push_back(ResolutionFramerate{896, 504, 60});
        ret.push_back(ResolutionFramerate{1280, 720, 60});
        ret.push_back(ResolutionFramerate{1920, 1080, 30});
      } else if (camera_type == X_CAM_TYPE_RPI_LIBCAMERA_RPIF_V1_OV5647) {
        ret.push_back(ResolutionFramerate{1280, 720, 30});
        ret.push_back(ResolutionFramerate{1920, 1080, 30});
      } else {
        std::cerr << "Not yet mapped:" << camera_type << std::endl;
        ret.push_back(ResolutionFramerate{1920, 1080, 30});
      }
      return ret;
    }
    return {ResolutionFramerate{640, 480, 30}};
  }

  // We default to the last supported resolution
  [[nodiscard]] ResolutionFramerate get_default_resolution_fps() const {
    auto supported_resolutions = get_supported_resolutions();
    return supported_resolutions.at(supported_resolutions.size() - 1);
  }
};

static bool is_rpi_csi_camera(int cam_type) {
  return cam_type >= 20 && cam_type <= 69;
}

static bool is_valid_primary_cam_type(int cam_type) {
  if (cam_type >= 0 && cam_type < X_CAM_TYPE_DISABLED) return true;
  return false;
}
// Takes a string in the from {width}x{height}@{framerate}
// e.g. 1280x720@30
static std::optional<ResolutionFramerate> parse_video_format(
    const std::string& videoFormat) {
  // 0x0@0 is a valid resolution (omit resolution / fps in the pipeline)
  if (videoFormat == "0x0@0") return ResolutionFramerate{0, 0, 0};
  // Otherwise, we need at least 6 characters (0x0@0 is 5 characters)
  if (videoFormat.size() <= 5) {
    return std::nullopt;
  }
  ResolutionFramerate tmp_video_format{0, 0, 0};
  const std::regex reg{R"((\d*)x(\d*)\@(\d*))"};
  std::smatch result;
  if (std::regex_search(videoFormat, result, reg)) {
    if (result.size() == 4) {
      // openhd::log::get_default()->debug("result[0]=["+result[0].str()+"]");
      tmp_video_format.width_px = atoi(result[1].str().c_str());
      tmp_video_format.height_px = atoi(result[2].str().c_str());
      tmp_video_format.fps = atoi(result[3].str().c_str());
      return tmp_video_format;
    }
  }
  return std::nullopt;
}

//
// Used in QOpenHD UI
//
static std::string get_verbose_string_of_resolution(
    const ResolutionFramerate& resolution_framerate) {
  if (resolution_framerate.width_px == 0 &&
      resolution_framerate.height_px == 0 && resolution_framerate.fps == 0) {
    return "AUTO";
  }
  std::stringstream ss;
  if (resolution_framerate.width_px == 640 &&
      resolution_framerate.height_px == 480) {
    ss << "VGA 4:3";
  } else if (resolution_framerate.width_px == 848 &&
             resolution_framerate.height_px == 480) {
    ss << "VGA 16:9";
  } else if (resolution_framerate.width_px == 896 &&
             resolution_framerate.height_px == 504) {
    ss << "SD 16:9";
  } else if (resolution_framerate.width_px == 1280 &&
             resolution_framerate.height_px == 720) {
    ss << "HD 16:9";
  } else if (resolution_framerate.width_px == 1920 &&
             resolution_framerate.height_px == 1080) {
    ss << "FHD 16:9";
  } else if (resolution_framerate.width_px == 2560 &&
             resolution_framerate.height_px == 1440) {
    ss << "2K 16:9";
  } else {
    ss << resolution_framerate.width_px << "x"
       << resolution_framerate.height_px;
  }
  ss << "\n" << resolution_framerate.fps << "fps";
  return ss.str();
}

static std::string get_v4l2_device_name_string(int value) {
  std::stringstream ss;
  ss << "/dev/video" << value;
  return ss.str();
}

class CameraHolder
{
 public:
  explicit CameraHolder(XCamera camera): m_camera(std::move(camera)) {}
  [[nodiscard]] const XCamera& get_camera() const { return m_camera; }
  // Settings hacky end
 private:
  // Camera info is immutable
  const XCamera m_camera;

 private:
  [[nodiscard]] CameraSettings create_default() const {
    auto ret = CameraSettings{};
    auto default_resolution = m_camera.get_default_resolution_fps();
    ret.streamed_video_format.width = default_resolution.width_px;
    ret.streamed_video_format.height = default_resolution.height_px;
    ret.streamed_video_format.framerate = default_resolution.fps;
    return ret;
  }
};


#endif  // CAMERA_HPP