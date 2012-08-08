#!/bin/bash
# WRITE
# SRC=v4l2src
SRC=videotestsrc
gst-launch -e $SRC ! ffmpegcolorspace ! theoraenc ! oggmux ! filesink location=out-video.ogg
