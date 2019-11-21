/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <unistd.h>
#include <iostream>
#include <vector>

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

    // get output layout information via randr extensions

	// Send a request for screen resources to the X server
	xcb_randr_get_screen_resources_cookie_t screenResCookie = {};
	screenResCookie = xcb_randr_get_screen_resources(pConnection,
			pScreenData->root);

	// Receive reply from X server
	xcb_randr_get_screen_resources_reply_t* pScreenResReply = {};
	pScreenResReply = xcb_randr_get_screen_resources_reply(pConnection,
					 screenResCookie, 0);

	int crtcsNum = 0;
	xcb_randr_crtc_t* pFirstCRTC;

	// Get a pointer to the first CRTC and number of CRTCs
	// It is crucial to notice that you are in fact getting
	// an array with firstCRTC being the first element of
	// this array and crtcs_length - length of this array
	if(pScreenResReply != nullptr)
	{
		crtcsNum = xcb_randr_get_screen_resources_crtcs_length(pScreenResReply);
		pFirstCRTC = xcb_randr_get_screen_resources_crtcs(pScreenResReply);
	}
	else
	{
		return -1;
	}

	// Array of requests to the X server for CRTC info
	vector<xcb_randr_get_crtc_info_cookie_t> crtcResCookies(crtcsNum);
	for(int i = 0; i < crtcsNum; i++)
	{
		crtcResCookies[i] = xcb_randr_get_crtc_info(pConnection, pFirstCRTC[i], 0);
	}

	// Array of pointers to replies from X server
	vector<xcb_randr_get_crtc_info_reply_t*> crtcResReplies(crtcsNum);
	for(int i = 0; i < crtcsNum; i++)
	{
		crtcResReplies[i] = xcb_randr_get_crtc_info_reply(pConnection, crtcResCookies[i], 0);
	}

	// Self-explanatory
	for(int i = 0; i < crtcsNum; i++)
	{
		if(crtcResReplies[i] != nullptr)
		{
			printf("CRTC[%i] INFO:\n", i);
			printf("status\t: %i\n", crtcResReplies[i]->status);
			printf("x-off\t: %i\n", crtcResReplies[i]->x);
			printf("y-off\t: %i\n", crtcResReplies[i]->y);
			printf("width\t: %i\n", crtcResReplies[i]->width);
			printf("height\t: %i\n\n", crtcResReplies[i]->height);
		}
	}

	// define an event mask for the window
	uint32_t masks = XCB_CW_EVENT_MASK;
	uint32_t eventMask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS;

	// create a window - create window ID, create window, map window, flush commands to server
	xcb_window_t window = xcb_generate_id(pConnection);
	xcb_create_window(
			pConnection,					// xcb connection
			XCB_COPY_FROM_PARENT,			// depth
			window,							// window id
			pScreenData->root,				// parent window
			0, 0, 							// x, y
			150, 150,						// width, height
			10,								// border width
			XCB_WINDOW_CLASS_INPUT_OUTPUT,	// class
			pScreenData->root_visual,		// visual
			masks, 							// masks bitmap
			&eventMask);					// masks value array
	xcb_map_window(pConnection, window);
	xcb_flush(pConnection);

	// event loop
	xcb_generic_event_t* pEv = nullptr;
	bool keepRunning = true;
	while(keepRunning == true && (pEv = xcb_wait_for_event(pConnection)) != nullptr)
	{
		switch(pEv->response_type & ~0x80)
		{
		case XCB_EXPOSE:
		{
			// handle the expose event
			// one of this event will be sent for each of the following cases:
			// - A window that covered part of our window has moved away, exposing part (or all) of our window.
    		// - Our window was raised above other windows.
    		// - Our window mapped for the first time.
    		// - Our window was de-iconified. 
			xcb_expose_event_t* pExpose = (xcb_expose_event_t*)pEv;

			cout << "Exposed! (gasp!)" << endl;
			break;
		}
		case XCB_BUTTON_PRESS:
		{
			// handle the button press event
			xcb_button_press_event_t* pButtonPress = (xcb_button_press_event_t*)pEv;
			
			cout << "Button pressed!" << endl;
			break;
		}
		// how to respond to window closing?
		// case :
		// {
		// 	keepRunning = false;
		// 	break;
		// }
		}

		free(pEv);
	}

	// free some resources
	//free(pFirstCRTC);
	for(xcb_randr_get_crtc_info_reply_t* pCRTCInfo : crtcResReplies)
	{
		free(pCRTCInfo);
	}


    // finished, disconnect
    xcb_disconnect(pConnection);
}
