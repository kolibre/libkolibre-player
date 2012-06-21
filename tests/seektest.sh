#!/bin/sh

toppkgdir=${srcdir:-.}/..

#--gst-debug=neonhttpsrc:5
#--gst-debug-no-color
#gst_params="--gst-debug-level=3 --gst-debug=typefind:5"

# test seek in mp3 file
mp3_file=$toppkgdir/tests/testdata/mp3/dtb_full.mp3
./seektest $mp3_file $gst_params

# test seek in ogg file
ogg_file=$toppkgdir/tests/testdata/ogg/dtb_full.ogg
./seektest $ogg_file $gst_params

# test seek in wav file
wav_file=$toppkgdir/tests/testdata/wav/dtb_full.wav
./seektest $wav_file $gst_params
