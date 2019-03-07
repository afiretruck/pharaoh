/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#ifndef WINDOWMANAGER_H_INCLUDED
#define WINDOWMANAGER_H_INCLUDED

#include <X11/Xlib.h>

namespace Pharaoh
{
    class WindowManager
    {
    public:
        WindowManager(int argc, char** argv);
        ~WindowManager();

        int Run();

    private:
        static int OnXError(Display* pDisplay, XErrorEvent* pEvent);
        static int OnWMDetected(Display* pDisplay, XErrorEvent* pEvent);
        int EventLoop();

        Display* m_pXDisplay;
        Window m_RootWindow;
        int m_argc;
        char** m_argv;
        static WindowManager* m_pInstance;
        static bool m_WMDetected;
    };
}

#endif