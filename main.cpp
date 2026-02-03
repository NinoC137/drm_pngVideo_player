#include "drm_display.h"
#include <iostream>

int main() {
    DrmDisplay disp;

    //配置分辨率为480x480
    if(!disp.init(480, 480)) {
        std::cerr << "DRM init failed" << std::endl;
        return -1;
    }

    // 播放1~124编号的png,帧率为60
    disp.play_frames("/root/gif_boot/gif_frame_", 1, 124, 60);

    return 0;
}
