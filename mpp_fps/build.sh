#!/bin/bash
gcc ctest.c `pkg-config --cflags --libs gstreamer-1.0 libavcodec libavformat libavutil rockchip_mpp rockchip_vpu libavresample`
#
#mpp related and avdec