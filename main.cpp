#include "drm_display.h"
#include <iostream>

int main() {
    DrmDisplay disp;

    if(!disp.init(480, 480)) {
        std::cerr << "DRM init failed" << std::endl;
        return -1;
    }

    // 播放 0~120 编号 PNG，帧率 30FPS
    disp.play_frames("/root/gif_boot/gif_frame_", 1, 124, 60);

    return 0;
}
