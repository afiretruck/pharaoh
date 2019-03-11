#/*******************************************************************************
#
# WARNING This file is subject to the terms and conditions defined in the file
# 'LICENSE.txt', which is part of this source code package. Please ensure you
# have read the 'LICENSE.txt' file before using this software.
#
#********************************************************************************/

# stop on errors
set -e

# taken from basic_wm
# We need to specify the full path to Xephyr, as otherwise xinit will not
# interpret it as an argument specifying the X server to launch and will launch
# the default X server instead.
XEPHYR=$(whereis -b Xephyr | cut -f2 -d' ')
xinit ./xinitrc -- \
    "$XEPHYR" \
        :100 \
        -ac \
        -screen 800x600 \
-host-cursor
