/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include "Window.h"
#include <X11/Xutil.h>

using namespace std;
using namespace Pharaoh;

//--------------------------------------------------------------------------------
// ctor
//--------------------------------------------------------------------------------
PharaohWindow::PharaohWindow(Display* pXDisplay, Window rootWindow, Window clientWindow)
    : m_pXDisplay(pXDisplay)
    , m_RootWindow(rootWindow)
    , m_ClientWindow(clientWindow)
{

}

PharaohWindow::PharaohWindow(
    Display* pXDisplay, 
    Window rootWindow, 
    Window clientWindow, 
    int x, 
    int y, 
    unsigned int width, 
    unsigned int height)
    : m_pXDisplay(pXDisplay)
    , m_RootWindow(rootWindow)
    , m_ClientWindow(clientWindow)
    , m_X(x)
    , m_Y(y)
    , m_Width(width)
    , m_Height(height)
{
    
}

//--------------------------------------------------------------------------------
// Configure
//--------------------------------------------------------------------------------
void PharaohWindow::Configure(XWindowChanges& windowChanges, unsigned int valueMask)
{
    // if the window is already mapped, re-configure the parent frame
    if(true == m_IsMapped)
    {
        XConfigureWindow(m_pXDisplay, m_FrameWindow, valueMask, &windowChanges);
        // TODO: check error codes

        m_X = windowChanges.x;
        m_Y = windowChanges.y;
        m_Width = windowChanges.width;
        m_Height = windowChanges.height;
    }

    // grant request
    XConfigureWindow(m_pXDisplay, m_ClientWindow, valueMask, &windowChanges);

}

//--------------------------------------------------------------------------------
// Map & Unmap
//--------------------------------------------------------------------------------
void PharaohWindow::Map()
{
    if(true == m_IsMapped)
    {
        return;
    }

    // Visual properties
    const unsigned int BORDER_WIDTH = 3;
    const unsigned int BORDER_COLOUR = 0xff0000;
    const unsigned int BG_COLOUR = 0x0000ff;

    // Retrieve attributes of the window to frame
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(m_pXDisplay, m_ClientWindow, &windowAttributes); // TODO: check return codes!

    // Create frame
    m_FrameWindow = XCreateSimpleWindow(
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
        m_FrameWindow,
        SubstructureRedirectMask | SubstructureNotifyMask);

    // Add client to save set, so that it will be restored and kept alive if we crash
    XAddToSaveSet(m_pXDisplay, m_FrameWindow);

    // reparent the client window to the frame
    XReparentWindow(
        m_pXDisplay,
        m_ClientWindow,
        m_FrameWindow,
        0, 0); // offset of client window within the frame

    // map the frame
    XMapWindow(m_pXDisplay, m_FrameWindow);

    // grab universal window management actions on the client window
    //   a. Move windows with alt + left button.
    XGrabButton(
        m_pXDisplay,
        Button1,
        Mod1Mask,
        m_FrameWindow,
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
        m_FrameWindow,
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
        m_FrameWindow,
        false,
        GrabModeAsync,
        GrabModeAsync);
    //   d. Switch windows with alt + tab.
    XGrabKey(
        m_pXDisplay,
        XKeysymToKeycode(m_pXDisplay, XK_Tab),
        Mod1Mask,
        m_FrameWindow,
        false,
        GrabModeAsync,
        GrabModeAsync);

    // finally map the client window
    XMapWindow(m_pXDisplay, m_ClientWindow);

    m_IsMapped = true;
}

void PharaohWindow::Unmap()
{
    if(false == m_IsMapped)
    {
        return;
    }

    // unmap the frame
    XUnmapWindow(m_pXDisplay, m_FrameWindow);

    // reprent client window back to root window
    XReparentWindow(
    m_pXDisplay,
    m_ClientWindow,
    m_RootWindow,
    0, 0); // offset of client window within root.

    // remove client window from save set, as it is now unrelated to us.
    XRemoveFromSaveSet(m_pXDisplay, m_ClientWindow);

    // destroy the frame
    XDestroyWindow(m_pXDisplay, m_FrameWindow);

    m_IsMapped = false;
}


bool PharaohWindow::IsMapped() const
{
    return m_IsMapped;
}

//--------------------------------------------------------------------------------
// Location
//--------------------------------------------------------------------------------
void PharaohWindow::SetLocation(int x, int y)
{
    m_X = x;
    m_Y = y;

    if(true == m_IsMapped)
    {
        XMoveWindow(m_pXDisplay, m_FrameWindow, x, y);
    }
}

void PharaohWindow::GetLocation(int& x, int& y) const
{
    x = m_X;
    y = m_Y;
}

//--------------------------------------------------------------------------------
// Size
//--------------------------------------------------------------------------------
void PharaohWindow::SetSize(const unsigned int width, const unsigned int height)
{
    // TODO: have the client window's minimum & maximum sizes and test them before continuing

    m_Width = width;
    m_Height = height;

    if(true == m_IsMapped)
    {
        // Resize frame.
        XResizeWindow(m_pXDisplay, m_FrameWindow, width, height);

        // Resize client window.
        // TODO: calculate client window size (it should be smaller than the frame)
        XResizeWindow(m_pXDisplay, m_ClientWindow, width, height);
    }
}

void PharaohWindow::GetSize(unsigned int& width, unsigned int& height) const
{
    width = m_Width;
    height = m_Height;
}

//--------------------------------------------------------------------------------
// Others
//--------------------------------------------------------------------------------
void PharaohWindow::RaiseAndSetFocus()
{
    XRaiseWindow(m_pXDisplay, m_FrameWindow);
    XSetInputFocus(m_pXDisplay, m_ClientWindow, RevertToPointerRoot, CurrentTime);
}

void PharaohWindow::Raise()
{
    XRaiseWindow(m_pXDisplay, m_FrameWindow);
}