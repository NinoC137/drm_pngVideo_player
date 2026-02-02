#ifndef __DRM_DISPLAY_H__
#define __DRM_DISPLAY_H__

#include <cstdint>
#include <string>
#include <xf86drm.h>
#include <xf86drmMode.h>

class DrmDisplay {
public:
    DrmDisplay(const std::string &drm_dev="/dev/dri/card0");
    ~DrmDisplay();

    bool init(uint32_t width, uint32_t height);

    bool load_png(const std::string &filename);

    void display();

    void play_frames(const std::string &path_prefix, int start, int end, int fps);

private:
    int drm_fd_;
    uint32_t width_;
    uint32_t height_;

    struct DrmBuffer {
        uint32_t fb_id;
        uint32_t handle;
        uint32_t pitch;
        uint8_t *map;
    } buffer_;

    uint32_t connector_id_;
    uint32_t crtc_id_;
    drmModeModeInfo mode_;
    void *map_addr_;

    bool create_drm_buffer();
    bool find_connector_crtc();
};

#endif // __DRM_DISPLAY_H__