/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Utils.h"
#include "WindowManager.h"
#include <iostream>
#include <functional>
#include <algorithm>
#include <cstring>

using namespace Pharaoh;
using namespace std;

//--------------------------------------------------------------------------------
// stores information about the current window drag operation
//--------------------------------------------------------------------------------
struct WindowManager::DragOperation
{
    enum DragType
    {
        DragType_Move,
        DragType_ResizeHorizonal,
        DragType_ResizeVertical,
        DragType_ResizeAll
    };

    int cursorStartX;
    int cursorStartY;
    int frameStartX;
    int frameStartY;
    unsigned int frameStartWidth;
    unsigned int frameStartHeight;
    unsigned int frameMininumWidth;
    unsigned int frameMininumHeight;

    DragType dragType;
};

//--------------------------------------------------------------------------------
// static data
//--------------------------------------------------------------------------------

WindowManager* WindowManager::m_pInstance = nullptr;
bool WindowManager::m_WMDetected = false;

//--------------------------------------------------------------------------------
// ctor & dtor
//--------------------------------------------------------------------------------

WindowManager::WindowManager(int argc, char** argv)
    : m_argc(argc)
    , m_argv(argv)
{
    m_pInstance = this;
}

WindowManager::~WindowManager()
{

}

//--------------------------------------------------------------------------------
// main entrypoint
//--------------------------------------------------------------------------------

int WindowManager::Run()
{
    // open the X display to use. By passing nullptr (not specifying a
    // display name) X will use the DISPLAY environment variable value.
    m_pXDisplay = XOpenDisplay(nullptr);
    if(m_pXDisplay == nullptr)
    {
        // failed to open X display
        cerr << "Failed to open X display" << endl;
        return -1;
    }

    // get the root window for the X display.
    m_RootWindow = DefaultRootWindow(m_pXDisplay);

    // set some protocol things
    WM_PROTOCOLS = XInternAtom(m_pXDisplay, "WM_PROTOCOLS", false);
    WM_DELETE_WINDOW = XInternAtom(m_pXDisplay, "WM_DELETE_WINDOW", false);

    // attempt to initialise the window manager with X
    // we require special permissions that only a single
    // Window manager can get. Error-out if we're not the
    // only one. Set the temporary error handler to catch
    // this event during init.
    int returnCode = -2;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(m_pXDisplay, m_RootWindow, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(m_pXDisplay, false);

    // was another window manager detected on this display?
    if(false == m_WMDetected)
    {
        // initialisation successful, set the real error handler.
        XSetErrorHandler(&WindowManager::OnXError);

        // frame any existing top-level windows
        XGrabServer(m_pXDisplay);

        Window returnedRoot, returnedParent;
        Window* pTopLevelWindows = nullptr;
        unsigned int numberOfTopLevelWindows = -1;
        XQueryTree(
            m_pXDisplay,
            m_RootWindow,
            &returnedRoot,
            &returnedParent,
            &pTopLevelWindows,
            &numberOfTopLevelWindows);

        if(returnedRoot == m_RootWindow)
        {
            for(unsigned int i = 0; i < numberOfTopLevelWindows; i++)
            {
                // Retrieve attributes of the window to frame
                XWindowAttributes windowAttributes;
                XGetWindowAttributes(m_pXDisplay, pTopLevelWindows[i], &windowAttributes); // TODO: check return codes!

                // create a window
                PharaohWindow* pNewWindow = new PharaohWindow(
                    m_pXDisplay, 
                    m_RootWindow, 
                    pTopLevelWindows[i],
                    windowAttributes.x,
                    windowAttributes.y,
                    windowAttributes.width,
                    windowAttributes.height);
                m_Clients[pTopLevelWindows[i]] = unique_ptr<PharaohWindow>(pNewWindow);

                // framing existing top-level windows - only frame if visible and doesn't set override_redirect
                // TODO: override_redirect check should be moved to PharaohWindow::Map
                if(windowAttributes.override_redirect || windowAttributes.map_state != IsViewable)
                {
                    continue;
                }

                pNewWindow->Map(m_DecorationWindows);
                m_FramesToClients[pNewWindow->GetFrameWindow()] = pNewWindow;
            }
        }
        XFree(pTopLevelWindows);

        XUngrabServer(m_pXDisplay);

        // enter main even loop
        returnCode = EventLoop();
    }
    else
    {
        cerr << "Detected another window manager on display " << XDisplayString(m_pXDisplay) << endl;
    }
    

    // shutdown
    XCloseDisplay(m_pXDisplay);

    return returnCode;
}


//--------------------------------------------------------------------------------
// main loop
//--------------------------------------------------------------------------------

int WindowManager::EventLoop()
{
    while(1)
    {
        // wait for the next XEvent
        XEvent event;
        XNextEvent(m_pXDisplay, &event);

        // dispatch
        switch(event.type)
        {
    case CreateNotify:
        OnCreateNotify(event.xcreatewindow);
        break;
        case ConfigureRequest:
        OnConfigureRequest(event.xconfigurerequest);
        break;
    case MapRequest:
        OnMapRequest(event.xmaprequest);
        break;
    case ReparentNotify:
        OnReparentNotify(event.xreparent);
        break;
    case MapNotify:
        OnMapNotify(event.xmap);
        break;
    case ConfigureNotify:
        OnConfigureNotify(event.xconfigure);
        break;
    case UnmapNotify:
        OnUnmapNotify(event.xunmap);
        break;
    case DestroyNotify:
        OnDestroyNotify(event.xdestroywindow);
        break;
    case ButtonPress:
        OnButtonPress(event.xbutton);
            break;
    case ButtonRelease:
        OnButtonRelease(event.xbutton);
            break;
    case MotionNotify:
        // skip any already pending motion events
        while(XCheckTypedWindowEvent(
        m_pXDisplay, event.xmotion.window, MotionNotify, &event)){}
        OnMotionNotify(event.xmotion);
            break;
    case KeyPress:
        OnKeyPress(event.xkey);
            break;
    case KeyRelease:
        OnKeyRelease(event.xkey);
        break;
        default:
            cout << "Event ignored." << endl;
        break;
        }

    }


    return 0;
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e)
{
    // we need to ignore this if it's a result of framing a window
    if(m_DecorationWindows.find(e.window) == m_DecorationWindows.end() &&
        m_Clients.find(e.window) == m_Clients.end())
    {
        PharaohWindow* pNewWindow = new PharaohWindow(m_pXDisplay, m_RootWindow, e.window);
        m_Clients[e.window] = unique_ptr<PharaohWindow>(pNewWindow);
    }
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e)
{
    XWindowChanges changes;

    // copy fields from e to changes
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    auto it = m_Clients.find(e.window);
    if(it != m_Clients.end())
    {
        it->second->Configure(changes, e.value_mask);
    }
    else
    {
        XConfigureWindow(m_pXDisplay, e.window, e.value_mask, &changes);
    }
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e)
{
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e)
{
    auto it = m_Clients.find(e.window);
    if(it != m_Clients.end())
    {
        it->second->Map(m_DecorationWindows);
        m_FramesToClients[it->second->GetFrameWindow()] = it->second.get();
    }
}

void WindowManager::OnReparentNotify(const XReparentEvent& e)
{
}

void WindowManager::OnMapNotify(const XMapEvent& e)
{
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& e)
{
    // ignore if we don't manage this window
    auto frameIt = m_Clients.find(e.window);
    if(frameIt == m_Clients.end())
    {
        cout << "Ignore UnmapNotify for non-client window " << e.window << endl;
        return;
    }

    // ignore the event if it was triggered by reparenting a window that was mapped
    // before the window manager started.
    if(e.event == m_RootWindow)
    {
        cout << "Ignore UnmapNotify for reparented pre-existing window " << e.window << endl;
        return;
    }

    // unframe the window if we do manage it
    m_FramesToClients.erase(frameIt->second->GetFrameWindow());
    frameIt->second->Unmap(m_DecorationWindows);
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e)
{
    auto it = m_Clients.find(e.window);
    if(it != m_Clients.end())
    {
        if(it->second->IsMapped())
        {
            m_FramesToClients.erase(it->second->GetFrameWindow());
            it->second->Unmap(m_DecorationWindows);
        }
        m_Clients.erase(it);
    }
}

//--------------------------------------------------------------------------------
// User input events
//--------------------------------------------------------------------------------

void WindowManager::OnButtonPress(const XButtonEvent& e)
{
    auto frameIt = m_Clients.find(e.window);
    if(frameIt != m_Clients.end())
    {
        // get the frame & save the start position
        m_DragCursorStartX = e.x_root;
        m_DragCursorStartY = e.y_root;

        cout << "Mouse start pos (x, y) = " << to_string(m_DragCursorStartX) << ", " << to_string(m_DragCursorStartY) << endl;

        // save the initial window information
        int x, y;
        unsigned int width, height;

        frameIt->second->GetLocation(x, y);
        frameIt->second->GetSize(width, height);

        m_DragFrameStartX = x;
        m_DragFrameStartY = y;
        m_DragFrameStartWidth = width;
        m_DragFrameStartHeight = height;
        cout << "    (x, y, width, height) = " << x << ", " << y << ", " << width << ", " << height << endl;

        cout << "Mouse pressed on client" << endl;

        // raise the window to the top
        frameIt->second->RaiseAndSetFocus();
    }
    else
    {
        auto frameWindowIt = m_FramesToClients.find(e.window);
        if(frameWindowIt != m_FramesToClients.end())
        {
            PharaohWindow::LocationInFrame cursorLocation = frameWindowIt->second->GetPositionInFrame(e.x, e.y);
            if(cursorLocation != PharaohWindow::LocationInFrame_None)
            {
                // save the drag start position
                // start a drag operation on this window
                int x, y;
                unsigned int width, height;

                frameWindowIt->second->GetLocation(x, y);
                frameWindowIt->second->GetSize(width, height);

                DragOperation::DragType theDragType;

                switch(cursorLocation)
                {
                case PharaohWindow::LocationInFrame_DragBar:
                    cout << "button press on client frame drag bar" << endl;
                    theDragType = DragOperation::DragType_Move;
                    break;
                case PharaohWindow::LocationInFrame_ResizeFrameLeft:
                case PharaohWindow::LocationInFrame_ResizeFrameRight:
                    cout << "button press on client frame resize frame horizontal" << endl;
                    theDragType = DragOperation::DragType_ResizeHorizonal;
                    break;
                case PharaohWindow::LocationInFrame_ResizeFrameTop:
                case PharaohWindow::LocationInFrame_ResizeFrameBottom:
                    cout << "button press on client frame resize frame vertical" << endl;
                    theDragType = DragOperation::DragType_ResizeVertical;
                    break;
                case PharaohWindow::LocationInFrame_ResizeFrameTopRight:
                case PharaohWindow::LocationInFrame_ResizeFrameBottomLeft:
                    cout << "button press on client frame resize frame diagonal top right/bottom left" << endl;
                    theDragType = DragOperation::DragType_ResizeAll;
                    break;
                case PharaohWindow::LocationInFrame_ResizeFrameTopLeft:
                case PharaohWindow::LocationInFrame_ResizeFrameBottomRight:
                    cout << "button press on client frame resize frame diagonal top left/bottom right" << endl;
                    theDragType = DragOperation::DragType_ResizeAll;
                    break;
                default:
                    break;
                }

                m_xCurrentDragOperation.reset(new DragOperation
                {
                   e.x_root,
                   e.y_root,
                   x,
                   y,
                   width,
                   height,
                   10,
                   10,
                   theDragType
                });

                // raise the window to the top
                frameWindowIt->second->RaiseAndSetFocus();
            }
        }
    }
}

void WindowManager::OnButtonRelease(const XButtonEvent& e)
{
    auto frameWindowIt = m_FramesToClients.find(e.window);
    if(frameWindowIt != m_FramesToClients.end())
    {
        cout << "mouse released on client window" << endl;
        m_xCurrentDragOperation.release();

        frameWindowIt->second->RaiseAndSetFocus();
    }
}

void WindowManager::OnMotionNotify(const XMotionEvent& e)
{
    auto frameIt = m_Clients.find(e.window);
    if(frameIt != m_Clients.end())
    {
        // int dragPosX = e.x_root;
        // int dragPosY = e.y_root;
        // int deltaX = dragPosX - m_DragCursorStartX;
        // int deltaY = dragPosY - m_DragCursorStartY;

        // cout << "Mouse delta (x, y) = " << to_string(deltaX) << ", " << to_string(deltaY) << endl;

        // if(e.state & Button1Mask) 
        // {
        //     // alt + left button: Move window.
        //     const int destFramePosX = m_DragFrameStartX + deltaX;
        //     const int destFramePosY = m_DragFrameStartY + deltaY;
        //     frameIt->second->SetLocation(destFramePosX, destFramePosY);
        // } 
        // else if (e.state & Button3Mask) 
        // {
        //     // alt + right button: Resize window.
        //     // Window dimensions cannot be negative.
        //     const int sizeDeltaX = max(deltaX, -m_DragFrameStartWidth);
        //     const int sizeDeltaY = max(deltaY, -m_DragFrameStartHeight);
        //     const int destFrameSizeWidth = m_DragFrameStartWidth + sizeDeltaX;
        //     const int destFrameSizeHeight = m_DragFrameStartHeight + sizeDeltaY;
        //     cout << "    Resize window to (x, y) = " << destFrameSizeWidth << ", " << destFrameSizeHeight << endl;
        //     frameIt->second->SetSize(destFrameSizeWidth, destFrameSizeHeight);        
        // }
    }
    else
    {
        auto frameWindowIt = m_FramesToClients.find(e.window);
        if(frameWindowIt != m_FramesToClients.end())
        {
            // is a drag operation in progress?
            if((e.state & Button1Mask) > 0 && m_xCurrentDragOperation.get() != nullptr)
            {
                // get mouse delta since the drag started
                const int deltaX = e.x_root - m_xCurrentDragOperation->cursorStartX;
                const int deltaY = e.y_root - m_xCurrentDragOperation->cursorStartY;

                // what is the drag operation?
                switch(m_xCurrentDragOperation->dragType)
                {
                case DragOperation::DragType_Move:
                    frameWindowIt->second->SetLocation(
                        m_xCurrentDragOperation->frameStartX + deltaX, 
                        m_xCurrentDragOperation->frameStartY + deltaY);
                    break;
                case DragOperation::DragType_ResizeAll:
                    frameWindowIt->second->SetSize(
                        (unsigned int)max((int)m_xCurrentDragOperation->frameStartWidth + deltaX, (int)m_xCurrentDragOperation->frameMininumWidth),
                        (unsigned int)max((int)m_xCurrentDragOperation->frameStartHeight + deltaY, (int)m_xCurrentDragOperation->frameMininumHeight));
                    break;
                case DragOperation::DragType_ResizeHorizonal:
                    break;
                case DragOperation::DragType_ResizeVertical:
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& e)
{
    if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(m_pXDisplay, XK_F4))) 
    {
        // alt + f4: Close window.
        //
        // There are two ways to tell an X window to close. The first is to send it
        // a message of type WM_PROTOCOLS and value WM_DELETE_WINDOW. If the client
        // has not explicitly marked itself as supporting this more civilized
        // behavior (using XSetWMProtocols()), we kill it with XKillClient().
        Atom* supported_protocols;
        int num_supported_protocols;
        if (XGetWMProtocols(m_pXDisplay, e.window, &supported_protocols, &num_supported_protocols) &&
                (::std::find(supported_protocols, supported_protocols + num_supported_protocols,
                        WM_DELETE_WINDOW) !=
            supported_protocols + num_supported_protocols)) 
        {
            cout << "Gracefully deleting window " << e.window << endl;

            // 1. Construct message.
            XEvent msg;
            memset(&msg, 0, sizeof(msg));
            msg.xclient.type = ClientMessage;
            msg.xclient.message_type = WM_PROTOCOLS;
            msg.xclient.window = e.window;
            msg.xclient.format = 32;
            msg.xclient.data.l[0] = WM_DELETE_WINDOW;

            // 2. Send message to window to be closed.
            XSendEvent(m_pXDisplay, e.window, false, 0, &msg);
        } 
        else 
        {
            cout << "Killing window " << e.window << endl;;
            XKillClient(m_pXDisplay, e.window);
        }
    } 
    else if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(m_pXDisplay, XK_Tab))) 
    {
        // alt + tab: Switch window.
        // 1. Find next window.
        auto i = m_Clients.find(e.window);
        if(i != m_Clients.end())
        {
            ++i;
            if (i == m_Clients.end()) 
            {
                i = m_Clients.begin();
            }
            // 2. Raise and set focus.
            i->second->RaiseAndSetFocus();
        }
    }
}

void WindowManager::OnKeyRelease(const XKeyEvent& e)
{
}

//--------------------------------------------------------------------------------
// Error handling
//--------------------------------------------------------------------------------

int WindowManager::OnXError(Display* pDisplay, XErrorEvent* e)
{
    const int MAX_ERROR_TEXT_LENGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LENGTH];

    XGetErrorText(pDisplay, e->error_code, error_text, sizeof(error_text));
    cout << "Received X error:\n"
         << "    Request: " << int(e->request_code)
         << " - " << XRequestCodeToString(e->request_code) << "\n"
         << "    Error code: " << int(e->error_code)
         << " - " << error_text << "\n"
         << "    Resource ID: " << e->resourceid << endl;

    // The return value is ignored.
    return 0;
}

int WindowManager::OnWMDetected(Display* pDisplay, XErrorEvent* pEvent)
{
    if(pEvent->error_code == BadAccess)
    {
        m_WMDetected = true;
    }

    return 0;
}
