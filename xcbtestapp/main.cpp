/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include <xcb/xcb.h>
#include <xcb/randr.h>
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

    // create a dummy window
    xcb_window_t windowDummy = xcb_generate_id(pConnection);
	xcb_create_window(pConnection, 0, windowDummy, pScreenData->root,
                          0, 0, 1, 1, 0, 0, 0, 0, 0);
	xcb_flush(pConnection); // make sure requests are flushed to the x server

    // get output layout information via randr extensions

	// Send a request for screen resources to the X server
	xcb_randr_get_screen_resources_cookie_t screenResCookie = {};
	screenResCookie = xcb_randr_get_screen_resources(pConnection,
			windowDummy);

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

	// free some resources
	//free(pFirstCRTC);
	for(xcb_randr_get_crtc_info_reply_t* pCRTCInfo : crtcResReplies)
	{
		free(pCRTCInfo);
	}


    // finished, disconnect
    xcb_disconnect(pConnection);
}
