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
#include <map>
#include <memory>

#include "Window.h"

namespace Pharaoh
{
    class WindowManager
    {
    public:
        WindowManager(int argc, char** argv);
        ~WindowManager();

        int Run();

    private:
        void OnCreateNotify(const XCreateWindowEvent& e);
        void OnConfigureRequest(const XConfigureRequestEvent& e);
        void OnConfigureNotify(const XConfigureEvent& e);
        void OnMapRequest(const XMapRequestEvent& e);
        void OnReparentNotify(const XReparentEvent& e);
        void OnMapNotify(const XMapEvent& e);
        void OnUnmapNotify(const XUnmapEvent& e);
        void OnDestroyNotify(const XDestroyWindowEvent& e);
        void Frame(Window w, bool createdBeforeWindowManager);
        void Unframe(Window w);
        void OnButtonPress(const XButtonEvent& e);
        void OnButtonRelease(const XButtonEvent& e);
        void OnMotionNotify(const XMotionEvent& e);
        void OnKeyPress(const XKeyEvent& e);
        void OnKeyRelease(const XKeyEvent& e);

        static int OnXError(Display* pDisplay, XErrorEvent* pEvent);
        static int OnWMDetected(Display* pDisplay, XErrorEvent* pEvent);
        int EventLoop();

        Display* m_pXDisplay;
        Window m_RootWindow;
        int m_argc;
        char** m_argv;

        // map the XWindows to their handler classes 
        // (the Window key is the handle for the un-framed client window)
        //std::map<Window, std::unique_ptr<PharaohWindow>> m_Clients;
        std::map<Window, Window> m_Clients;

        int m_DragCursorStartX = 0;
        int m_DragCursorStartY = 0;
        int m_DragFrameStartX = 0;
        int m_DragFrameStartY = 0;
        int m_DragFrameStartWidth = 0;
        int m_DragFrameStartHeight = 0;
        
        Atom WM_PROTOCOLS;
        Atom WM_DELETE_WINDOW;

        static WindowManager* m_pInstance;
        static bool m_WMDetected;
    };
}

#endif
