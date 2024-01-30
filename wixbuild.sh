#!/bin/bash

WIXPATH=${WIXPATH:-/opt/wix}

rm -f "$1.wixobj" "$1.msi"
wine $WIXPATH/candle.exe "$1.wxs"
wine $WIXPATH/light.exe "$1.wixobj" -sval -v
