#!/bin/bash

rm -f "$1.wixobj" "$1.msi"
wine ~/Programme/wix/candle.exe "$1.wxs"
wine ~/Programme/wix/light.exe "$1.wixobj" -sval -v
