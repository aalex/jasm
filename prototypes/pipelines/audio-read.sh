#!/bin/bash
# READ
gst-launch filesrc location=out-audio.ogg ! oggdemux ! vorbisdec ! autoaudiosink

