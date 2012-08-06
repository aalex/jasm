#!/bin/bash
# WRITE
gst-launch videotestsrc ! ffmpegcolorspace ! theoraenc ! oggmux ! filesink location=foo.ogg
