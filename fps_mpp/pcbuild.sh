#!/bin/bash
gcc ctest.c `pkg-config --cflags --libs gstreamer-1.0 glib-2.0 libavcodec libavformat libavutil`
#
#mpp related and avdec