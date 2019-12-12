/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#pragma once

#include "Logger.h"
#include <xcb/xcb.h>

// represents the base class for any reparenting windows
// this window class has no decorations and doesn't respond to dragging, etc directly.

namespace Emperor
{
    class ReparentingWindow : public Logger
    {
    public:
        ReparentingWindow(xcb_window_t* pWindow);

    private:
    };
}