#!/bin/bash
# READ
gst-launch filesrc location=out-audio.ogg ! vorbisdec ! alsasink

