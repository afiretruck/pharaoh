/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include <xcb/xcb.h>
#include <iostream>

using namespace std;

// Following basic tutorial. The purpose of this program is to get familiar with XCB 
// by building a client application first:
// - Window moving by clicking and dragging on something
// - Window resizing by the same thingy
// - Setting mouse cursors
// - Creating a titlebar and frames
// - etc

int main(int argc, char** argv)
{
    // connect to the x server with displayname given by the DISPLAY environment variable.
    int screenNum;
    xcb_connection_t* pConnection = xcb_connect(nullptr, &screenNum);

    // check for error
    int error = xcb_connection_has_error(pConnection);
    if(error != 0)
    {
        cout << "Error occurred while connecting to X server. TODO: determine exact error." << endl;
        xcb_disconnect(pConnection);
        return -1;
    }
    cout << "Using screen number " << screenNum << endl;

    // get some details about the x screen
    const xcb_setup_t* pSetup = xcb_get_setup(pConnection);
    xcb_screen_iterator_t screenIter = xcb_setup_roots_iterator(pSetup);

    for(int i = 0; i < screenNum; i++)
    {
        xcb_screen_next(&screenIter);
    }

    xcb_screen_t* pScreenData = screenIter.data;

    // should have the screen information now
    cout << "    size: " << pScreenData->width_in_pixels << "," << pScreenData->height_in_pixels << endl;


    // finished, disconnect
    xcb_disconnect(pConnection);
}