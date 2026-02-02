## DRM PNGVideo Player

使用DRM驱动来播放一连串的PNG图像(来源于gif动图切割)

从而呈现出视频播放器的效果

### 环境依赖

1. drm-dev (需要linux内核使能drm驱动)

2. libpng-dev

> 使用 `sudo apt-get install xxx` 即可安装上述依赖

### 使用方法

在main中声明好PNG文件所在路径,即可编译运行

#### 编译方法

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake -DCMAKE_BUILD_TYPE=Release
make
```

### 附录:使用ffmpeg切割gif动图为PNG图片

```bash
ffmpeg -i haavk.gif \
-vf "select=not(mod(n\,1))" \
gif_frame_%02d.png
```

效果为生成一系列的png图片