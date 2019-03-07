/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

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
        default:
            cout << "Event ignored.";
        }

    }


    return 0;
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