#!/bin/bash
# WRITE
# SRC=v4l2src
SRC=videotestsrc
gst-launch $SRC ! ffmpegcolorspace ! theoraenc ! oggmux ! filesink location=out-video.ogg
