#!/bin/bash
# READ
gst-launch filesrc location=foo.ogg ! decodebin ! ffmpegcolorspace ! xvimagesink
