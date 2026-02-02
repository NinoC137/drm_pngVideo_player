#include "drm_display.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <png.h>
#include <cstring>
#include <chrono>
#include <thread>

DrmDisplay::DrmDisplay(const std::string &drm_dev) : drm_fd_(-1), width_(0), height_(0),
    connector_id_(0), crtc_id_(0), map_addr_(nullptr) {
    drm_fd_ = open(drm_dev.c_str(), O_RDWR | O_CLOEXEC);
    if (drm_fd_ < 0) {
        std::cerr << "Failed to open DRM device: " << drm_dev << std::endl;
    }
}

DrmDisplay::~DrmDisplay() {
    if (map_addr_) {
        munmap(map_addr_, buffer_.pitch * height_);
    }
    if (buffer_.fb_id) {
        drmModeRmFB(drm_fd_, buffer_.fb_id);
    }
    if (buffer_.handle) {
        drmIoctl(drm_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &buffer_.handle);
    }
    if (drm_fd_ >= 0) {
        close(drm_fd_);
    }
}

bool DrmDisplay::init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    if (!create_drm_buffer()) {
        return false;
    }
    if (!find_connector_crtc()) {
        return false;
    }
    return true;
}

bool DrmDisplay::create_drm_buffer() {
    struct drm_mode_create_dumb create_req = {};
    create_req.width = width_;
    create_req.height = height_;
    create_req.bpp = 32;

    if (drmIoctl(drm_fd_, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0) {
        std::cerr << "Failed to create dumb buffer" << std::endl;
        return false;
    }

    buffer_.handle = create_req.handle;
    buffer_.pitch = create_req.pitch;

    struct drm_mode_map_dumb map_req = {};
    map_req.handle = buffer_.handle;

    if (drmIoctl(drm_fd_, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
        std::cerr << "Failed to map dumb buffer" << std::endl;
        return false;
    }

    map_addr_ = mmap(0, create_req.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd_, map_req.offset);
    if (map_addr_ == MAP_FAILED) {
        std::cerr << "Failed to mmap dumb buffer" << std::endl;
        return false;
    }
    buffer_.map = static_cast<uint8_t *>(map_addr_);

    if (drmModeAddFB(drm_fd_, width_, height_, 24, 32, buffer_.pitch, buffer_.handle, &buffer_.fb_id) < 0) {
        std::cerr << "Failed to add framebuffer" << std::endl;
        return false;
    }

    return true;
}

bool DrmDisplay::find_connector_crtc() {
    drmModeRes *resources = drmModeGetResources(drm_fd_);
    if (!resources) {
        std::cerr << "Failed to get DRM resources" << std::endl;
        return false;
    }

    for (int i = 0; i < resources->count_connectors; ++i) {
        drmModeConnector *connector = drmModeGetConnector(drm_fd_, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
            connector_id_ = connector->connector_id;

            if(connector->count_modes > 0){
                crtc_id_ = drmModeGetEncoder(drm_fd_, connector->encoder_id)->crtc_id;
                mode_ = connector->modes[0];
                width_ = connector->modes[0].hdisplay;
                height_ = connector->modes[0].vdisplay;
                drmModeFreeConnector(connector);
                drmModeFreeResources(resources);
                return true;
            }
        }
        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);

    if (connector_id_ == 0 || crtc_id_ == 0) {
        std::cerr << "Failed to find suitable connector/crtc" << std::endl;
        return false;
    }

    return true;
}

bool DrmDisplay::load_png(const std::string &filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open PNG file: " << filename << std::endl;
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        std::cerr << "Failed to create PNG read struct" << std::endl;
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        std::cerr << "Failed to create PNG info struct" << std::endl;
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        std::cerr << "Error during PNG read" << std::endl;
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    uint32_t img_width = png_get_image_width(png, info);
    uint32_t img_height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (img_width > width_ || img_height > height_) {
        std::cerr << "PNG dimensions do not match initialized dimensions" << std::endl;
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    png_read_update_info(png, info);

    png_bytep *row_pointers = new png_bytep[img_height];
    for (uint32_t y = 0; y < img_height; y++) {
        row_pointers[y] = new png_byte[png_get_rowbytes(png, info)];
    }
        
    png_read_image(png, row_pointers);

    for(int y = 0; y < img_height; y++) {
        png_bytep row = row_pointers[y];
        for(int x = 0; x < img_width; x++) {
            png_bytep px = &(row[x * 4]);
            uint32_t *row = (uint32_t *)(buffer_.map + y * buffer_.pitch);
            row[x] = (px[3] << 24) | (px[0] << 16) | (px[1] << 8) | px[2]; // ARGB
        }
    }

    for(int y = 0; y < img_height; y++) {
        delete[] row_pointers[y];
    }
    delete[] row_pointers;
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
    return true;
}

void DrmDisplay::display() {
    // std::cout << "Displaying frame on DRM" << std::endl;
    // drmModeModeInfo mode = connector.modes[0];
    drmModeSetCrtc(drm_fd_, crtc_id_, buffer_.fb_id, 0, 0, &connector_id_, 1, &mode_);
}

void DrmDisplay::play_frames(const std::string &path_prefix, int start, int end, int fps) {
    int frame_delay = 1000 / fps;

    for (int i = start; i <= end; ++i) {
        char fname[256];
        snprintf(fname, sizeof(fname), "%s%02d.png", path_prefix.c_str(), i);
        if (!load_png(fname)) {
            std::cerr << "Failed to load frame: " << fname << std::endl;
            continue;
        }
        display();
        std::this_thread::sleep_for(std::chrono::milliseconds(frame_delay));
    }
}