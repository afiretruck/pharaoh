/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_image.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <memory>

#include "Button.h"

using namespace std;
using namespace Emperor;

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

	// define an event mask for the window & the background colour
	uint32_t masks = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t mainWindowMask[2] =
	{
			0x00999999,
			XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY
	};

	// create a window - create window ID, create window, map window, flush commands to server
	xcb_window_t window = xcb_generate_id(pConnection);
	xcb_create_window(
			pConnection,					// xcb connection
			XCB_COPY_FROM_PARENT,			// depth
			window,							// window id
			pScreenData->root,				// parent window
			0, 0, 							// x, y
			300, 300,						// width, height
			10,								// border width
			XCB_WINDOW_CLASS_INPUT_OUTPUT,	// class
			pScreenData->root_visual,		// visual
			masks, 							// masks bitmap
			&mainWindowMask);				// masks value array



	// get some handy atoms
	const string wmProtocolsStr = "WM_PROTOCOLS";
	const string wmDeleteWindowStr = "WM_DELETE_WINDOW";
	xcb_intern_atom_cookie_t protocolAtomCookie = xcb_intern_atom(pConnection, 1, wmProtocolsStr.length(), wmProtocolsStr.c_str());
	xcb_intern_atom_cookie_t deleteWindowAtomCookie = xcb_intern_atom(pConnection, 0, wmDeleteWindowStr.length(), wmDeleteWindowStr.c_str());

	xcb_intern_atom_reply_t* pProtocolAtomReply = xcb_intern_atom_reply(pConnection, protocolAtomCookie, nullptr);
	xcb_intern_atom_reply_t* pDeleteWindowAtomReply = xcb_intern_atom_reply(pConnection, deleteWindowAtomCookie, nullptr);

	// tell the x server we want to participate in the window delete protocol
	xcb_change_property(pConnection, XCB_PROP_MODE_REPLACE, window, pProtocolAtomReply->atom, XCB_ATOM_ATOM, 32, 1, &(pDeleteWindowAtomReply->atom));

	// map the window and flush
	xcb_map_window(pConnection, window);
	xcb_flush(pConnection);


	// create an encapsulated button
	unique_ptr<Button> xButton(new Button(
			31, 17,
			0, 20,
			window,
			pConnection,
			pScreenData,
			"X-normal.png",
			"X-highlighted.png",
			"X-clicked.png",
			[window, pConnection]()
			{
				xcb_void_cookie_t ck = xcb_destroy_window_checked(pConnection, window);
				xcb_generic_error_t* pError = xcb_request_check(pConnection, ck);

				if(pError != nullptr)
				{
					cout << "Failed to delete the window!" << endl;
					free(pError);
				}
			}));


	// event loop
	xcb_generic_event_t* pEv = nullptr;
	bool keepGoing = true;
	while(keepGoing == true && (pEv = xcb_wait_for_event(pConnection)) != nullptr)
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
			
			if(pExpose->window == xButton->GetWindow())
			{
				xButton->ExposeEvent(pExpose);
			}

			cout << "Exposed! (gasp!)" << endl;
			break;
		}
		case XCB_BUTTON_PRESS:
		{
			// handle the button press event
			xcb_button_press_event_t* pButtonPress = (xcb_button_press_event_t*)pEv;

			if(pButtonPress->event == window)
			{
				cout << "button pressed on main window" << endl;
			}
			else if(pButtonPress->event == xButton->GetWindow())
			{
				xButton->ButtonPressEvent(pButtonPress);
			}
			
			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			// handle the button release event
			xcb_button_release_event_t* pButtonRelease = (xcb_button_release_event_t*)pEv;

			if(pButtonRelease->event == xButton->GetWindow())
			{
				xButton->ButtonReleaseEvent(pButtonRelease);
			}

			break;
		}
		case XCB_ENTER_NOTIFY:
		{
			// pointer entered window
			xcb_enter_notify_event_t* pEntered = (xcb_enter_notify_event_t*)pEv;

			if(pEntered->event == xButton->GetWindow())
			{
				xButton->MouseEnterEvent(pEntered);
			}

			break;
		}
		case XCB_LEAVE_NOTIFY:
		{
			// pointer left window
			xcb_leave_notify_event_t* pLeft = (xcb_leave_notify_event_t*)pEv;

			if(pLeft->event == xButton->GetWindow())
			{
				xButton->MouseLeaveEvent(pLeft);
			}


			break;
		}
		case XCB_CLIENT_MESSAGE:
		{
			xcb_client_message_event_t* pClientMessage = (xcb_client_message_event_t*)pEv;
			if(pClientMessage->data.data32[0] == pDeleteWindowAtomReply->atom)
			{
				if(pClientMessage->window == window)
				{
					cout << "Window delete requested!" << endl;

					// tell the server to delete the window
					xcb_void_cookie_t ck = xcb_destroy_window_checked(pConnection, pClientMessage->window);
					xcb_generic_error_t* pError = xcb_request_check(pConnection, ck);

					if(pError != nullptr)
					{
						cout << "Failed to delete the window!" << endl;
						free(pError);
					}
				}
			}
		 	break;
		}
		case XCB_DESTROY_NOTIFY:
		{
			xcb_destroy_notify_event_t* pDestroyNotify = (xcb_destroy_notify_event_t*)pEv;
			if(pDestroyNotify->window == window)
			{
				keepGoing = false;
				cout << "Window destroyed!" << endl;
			}
		}
		}

		free(pEv);
	}

	// free some resources
	//free(pFirstCRTC);
	free(pProtocolAtomReply);
	free(pDeleteWindowAtomReply);
	for(xcb_randr_get_crtc_info_reply_t* pCRTCInfo : crtcResReplies)
	{
		free(pCRTCInfo);
	}


    // finished, disconnect
    xcb_disconnect(pConnection);
}
