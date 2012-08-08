#!/bin/bash
# WRITE
# SRC=alsasrc
SRC=audiotestsrc
gst-launch -e $SRC ! vorbisenc ! oggmux ! filesink location=out-audio.ogg

