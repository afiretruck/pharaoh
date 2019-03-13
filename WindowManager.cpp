/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/


#include <X11/Xutil.h>
#include "WindowManager.h"
#include <iostream>
#include <functional>

using namespace Pharaoh;
using namespace std;


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
        default:
            cout << "Event ignored." << endl;
	    break;
        }

    }


    return 0;
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e)
{
    cout << "OnCreateNotify ignored" << endl;
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

    // grant request
    XConfigureWindow(m_pXDisplay, e.window, e.value_mask, &changes);

    // log
    cout << "Resize " << e.window << " to " << e.width << ", " << e.height << endl;
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e)
{
    Frame(e.window);
    XMapWindow(m_pXDisplay, e.window);
}

//--------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------

void WindowManager::Frame(Window w)
{
    // Visual properties
    const unsigned int BORDER_WIDTH = 3;
    const unsigned int BORDER_COLOUR = 0xff0000;
    const unsigned int BG_COLOUR = 0x0000ff;

    // Retrieve attributes of the window to frame
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(m_pXDisplay, w, &windowAttributes); // TODO: check return codes!

    // TODO: framing existing top-level windows

    // Create frame
    const Window frame = XCreateSimpleWindow(
	m_pXDisplay,
	m_RootWindow,
	windowAttributes.x,
	windowAttributes.y,
	windowAttributes.width,
	windowAttributes.height,
	BORDER_WIDTH,
	BORDER_COLOUR,
	BG_COLOUR);

    // select events on the frame
    XSelectInput(
	m_pXDisplay,
	frame,
	SubstructureRedirectMask | SubstructureNotifyMask);

    // Add client to save set, so that it will be restored and kept alive if we crash
    XAddToSaveSet(m_pXDisplay, w);

    // reparent the client window to the frame
    XReparentWindow(
	m_pXDisplay,
	w,
	frame,
	0, 0); // offset of client window within the frame

    // map the frame
    XMapWindow(m_pXDisplay, frame);

    // save the frame handle
    m_Clients[w] = frame;

    // grab universal window management actions on the client window
    //   a. Move windows with alt + left button.
    XGrabButton(
        m_pXDisplay,
        Button1,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    //   b. Resize windows with alt + right button.
    XGrabButton(
        m_pXDisplay,
        Button3,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    //   c. Kill windows with alt + f4.
    XGrabKey(
        m_pXDisplay,
        XKeysymToKeycode(m_pXDisplay, XK_F4),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);
    //   d. Switch windows with alt + tab.
    XGrabKey(
        m_pXDisplay,
        XKeysymToKeycode(m_pXDisplay, XK_Tab),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync); 
}

//--------------------------------------------------------------------------------
// Error handling
//--------------------------------------------------------------------------------

int WindowManager::OnXError(Display* pDisplay, XErrorEvent* pEvent)
{
    // TODO: log the error
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
