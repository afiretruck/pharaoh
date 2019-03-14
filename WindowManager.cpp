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
		Frame(pTopLevelWindows[i], true);
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

    // if the window is already mapped, re-configure the parent frame
    auto frameIt = m_Clients.find(e.window);
    if(frameIt != m_Clients.end())
    {
	// configure the parent frame
	const Window frame = frameIt->second;
	XConfigureWindow(m_pXDisplay, frame, e.value_mask, &changes);
	cout << "Resize " << e.window << " to " << e.width << ", " << e.height << endl;
    }

    // grant request
    XConfigureWindow(m_pXDisplay, e.window, e.value_mask, &changes);

    // log
    cout << "Resize " << e.window << " to " << e.width << ", " << e.height << endl;
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e)
{
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e)
{
    Frame(e.window, false);
    XMapWindow(m_pXDisplay, e.window);
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
    Unframe(e.window);
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e)
{
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
	const Window frame = frameIt->second;
	m_DragCursorStartX = e.x;
	m_DragCursorStartY = e.y;

	// save the initial window information
	Window returnedRoot;
	int x, y;
	unsigned int width, height, borderWidth, depth;
	XGetGeometry(
	    m_pXDisplay,
	    frame,
	    &returnedRoot,
	    &x, &y,
	    &width, &height,
	    &borderWidth,
	    &depth);
	m_DragFrameStartX = x;
	m_DragFrameStartY = y;
	m_DragFrameStartWidth = width;
	m_DragFrameStartHeight = height;

	// raise the window to the top
	XRaiseWindow(m_pXDisplay, frame);
    }
}

void WindowManager::OnButtonRelease(const XButtonEvent& e)
{
}

void WindowManager::OnMotionNotify(const XMotionEvent& e)
{
}

void WindowManager::OnKeyPress(const XKeyEvent& e)
{
}

void WindowManager::OnKeyRelease(const XKeyEvent& e)
{
}

//--------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------

void WindowManager::Frame(Window w, bool createdBeforeWindowManager)
{
    // Visual properties
    const unsigned int BORDER_WIDTH = 3;
    const unsigned int BORDER_COLOUR = 0xff0000;
    const unsigned int BG_COLOUR = 0x0000ff;

    // Retrieve attributes of the window to frame
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(m_pXDisplay, w, &windowAttributes); // TODO: check return codes!

    // framing existing top-level windows - only frame if visible and doesn't set override_redirect
    if(true == createdBeforeWindowManager)
    {
	if(windowAttributes.override_redirect || windowAttributes.map_state != IsViewable)
	{
	    return;
	}
    }

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

void WindowManager::Unframe(Window w)
{
    // reverse the steps taken in Frame()
    const Window frame = m_Clients[w];

    // unmap the frame
    XUnmapWindow(m_pXDisplay, frame);

    // reprent client window back to root window
    XReparentWindow(
	m_pXDisplay,
	w,
	m_RootWindow,
	0, 0); // offset of client window within root.

    // remove client window from save set, as it is now unrelated to us.
    XRemoveFromSaveSet(m_pXDisplay, w);

    // destroy the frame
    XDestroyWindow(m_pXDisplay, frame);
    m_Clients.erase(w);

    // log
    cout << "Unframed window " << w << " [" << frame << "]" << endl;
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
