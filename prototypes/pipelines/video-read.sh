#!/bin/bash
# READ
gst-launch filesrc location=out-video.ogg ! decodebin ! ffmpegcolorspace ! xvimagesink
