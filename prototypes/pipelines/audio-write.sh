#!/bin/bash
# WRITE
# SRC=alsasrc
SRC=audiotestsrc
gst-launch $SRC ! vorbisenc ! oggmux ! filesink location=out-audio.ogg

