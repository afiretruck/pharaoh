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

#define cimg_use_jpeg
#define cimg_use_png
#include "CImg.h"

using namespace std;
using namespace cimg_library;

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

	// create a button. This button will close the window.
	// how to do this? create a small child window and handle mouse-over and stuff?
	uint32_t buttonMask[2] =
	{
			0x00ffffff,
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_EXPOSURE
	};
	xcb_window_t buttonWindow = xcb_generate_id(pConnection);
	xcb_create_window(
			pConnection,					// xcb connection
			XCB_COPY_FROM_PARENT,			// depth
			buttonWindow,					// window id
			window,							// parent window
			0, 0, 							// x, y
			31, 17,							// width, height
			0,								// border width
			XCB_WINDOW_CLASS_INPUT_OUTPUT,	// class
			pScreenData->root_visual,		// visual
			masks, 							// masks bitmap
			&buttonMask);					// masks value array

	// find the right pixmap format
	xcb_format_t* pFormats = xcb_setup_pixmap_formats(pSetup);
	xcb_format_t* pFormat = nullptr;
	uint32_t numFormats = xcb_setup_pixmap_formats_length(pSetup);
	for(uint32_t i = 0; i < numFormats; i++)
	{
		// 24 bit depth with 32 bits per pixel
		if(pFormats[i].bits_per_pixel == 32 && pFormats[i].depth == 24)
		{
			pFormat = &pFormats[i];
		}
	}

	// show the button
	xcb_map_window(pConnection, buttonWindow);
	xcb_flush(pConnection);



	// read in PNG with Cimg
	CImg<unsigned char> buttonImageReader("X-normal.png");
	CImg<unsigned char> buttonHighlightedImageReader("X-highlighted.png");
	CImg<unsigned char> buttonClickedImageReader("X-clicked.png");

	unsigned char *image32=(unsigned char *)malloc(buttonImageReader.width()*buttonImageReader.height()*4);
	unsigned char *image32Highlighted=(unsigned char *)malloc(buttonImageReader.width()*buttonImageReader.height()*4);
	unsigned char *image32Clicked=(unsigned char *)malloc(buttonImageReader.width()*buttonImageReader.height()*4);
	for(int y = 0; y < buttonImageReader.height(); y++)
	{
		for(int x = 0; x < buttonImageReader.width(); x++)
		{
			image32[(y*buttonImageReader.width() + x)*4] = buttonImageReader(x, y, 0, 2);
			image32[(y*buttonImageReader.width() + x)*4 + 1] = buttonImageReader(x, y, 0, 1);
			image32[(y*buttonImageReader.width() + x)*4 + 2] = buttonImageReader(x, y, 0, 0);
			image32[(y*buttonImageReader.width() + x)*4 + 3] = 0x00;

			image32Highlighted[(y*buttonImageReader.width() + x)*4] = buttonHighlightedImageReader(x, y, 0, 2);
			image32Highlighted[(y*buttonImageReader.width() + x)*4 + 1] = buttonHighlightedImageReader(x, y, 0, 1);
			image32Highlighted[(y*buttonImageReader.width() + x)*4 + 2] = buttonHighlightedImageReader(x, y, 0, 0);
			image32Highlighted[(y*buttonImageReader.width() + x)*4 + 3] = 0x00;

			image32Clicked[(y*buttonImageReader.width() + x)*4] = buttonClickedImageReader(x, y, 0, 2);
			image32Clicked[(y*buttonImageReader.width() + x)*4 + 1] = buttonClickedImageReader(x, y, 0, 1);
			image32Clicked[(y*buttonImageReader.width() + x)*4 + 2] = buttonClickedImageReader(x, y, 0, 0);
			image32Clicked[(y*buttonImageReader.width() + x)*4 + 3] = 0x00;
		}
	}

	xcb_image_t* pButtonImage = xcb_image_create(
		buttonImageReader.width(),
		buttonImageReader.height(),
		XCB_IMAGE_FORMAT_Z_PIXMAP,
		pFormat->scanline_pad,
		pFormat->depth,
		pFormat->bits_per_pixel,
		0,
		(xcb_image_order_t)pSetup->image_byte_order,
		XCB_IMAGE_ORDER_LSB_FIRST,
		image32,
		buttonImageReader.width()*buttonImageReader.height()*4,
		image32);


	xcb_image_t* pButtonImageHighlighted = xcb_image_create(
		buttonHighlightedImageReader.width(),
		buttonHighlightedImageReader.height(),
		XCB_IMAGE_FORMAT_Z_PIXMAP,
		pFormat->scanline_pad,
		pFormat->depth,
		pFormat->bits_per_pixel,
		0,
		(xcb_image_order_t)pSetup->image_byte_order,
		XCB_IMAGE_ORDER_LSB_FIRST,
		image32Highlighted,
		buttonHighlightedImageReader.width()*buttonHighlightedImageReader.height()*4,
		image32Highlighted);


	xcb_image_t* pButtonImageClicked = xcb_image_create(
		buttonClickedImageReader.width(),
		buttonClickedImageReader.height(),
		XCB_IMAGE_FORMAT_Z_PIXMAP,
		pFormat->scanline_pad,
		pFormat->depth,
		pFormat->bits_per_pixel,
		0,
		(xcb_image_order_t)pSetup->image_byte_order,
		XCB_IMAGE_ORDER_LSB_FIRST,
		image32Clicked,
		buttonClickedImageReader.width()*buttonClickedImageReader.height()*4,
		image32Clicked);

	xcb_pixmap_t buttonPixmap = xcb_generate_id(pConnection);
	xcb_create_pixmap(pConnection, 24, buttonPixmap, buttonWindow, buttonImageReader.width(), buttonImageReader.height());

	xcb_pixmap_t buttonPixmapHighlighted = xcb_generate_id(pConnection);
	xcb_create_pixmap(pConnection, 24, buttonPixmapHighlighted, buttonWindow, buttonHighlightedImageReader.width(), buttonHighlightedImageReader.height());

	xcb_pixmap_t buttonPixmapClicked = xcb_generate_id(pConnection);
	xcb_create_pixmap(pConnection, 24, buttonPixmapClicked, buttonWindow, buttonClickedImageReader.width(), buttonClickedImageReader.height());

	uint32_t buttonPixmapMask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
 	uint32_t buttonPixmapValues[2] =
	{
		0,
		0x00ffffff
	};
	xcb_gcontext_t buttonContext = xcb_generate_id(pConnection);
	xcb_create_gc(pConnection, buttonContext, buttonPixmap, buttonPixmapMask, buttonPixmapValues);

	xcb_image_put(pConnection, buttonPixmap, buttonContext, pButtonImage, 0, 0, 0);
	xcb_image_put(pConnection, buttonPixmapHighlighted, buttonContext, pButtonImageHighlighted, 0, 0, 0);
	xcb_image_put(pConnection, buttonPixmapClicked, buttonContext, pButtonImageClicked, 0, 0, 0);

	xcb_flush(pConnection);

	// some button control variables
	bool buttonHeld = false;
	bool mouseOver = false;


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

			if(pExpose->window == buttonWindow)
			{
				cout << "Button exposed!" << endl;
				xcb_copy_area(
					pConnection, buttonPixmap, buttonWindow, buttonContext,
					pExpose->x,
					pExpose->y,
					pExpose->x,
					pExpose->y,
					pExpose->width,
					pExpose->height);
				xcb_flush(pConnection);
			}

			cout << "Exposed! (gasp!)" << endl;
			break;
		}
		case XCB_BUTTON_PRESS:
		{
			// handle the button press event
			xcb_button_press_event_t* pButtonPress = (xcb_button_press_event_t*)pEv;

			if(pButtonPress->event == buttonWindow )
			{
				cout << "button pressed on button window" << endl;

				buttonHeld = true;

				xcb_copy_area(
					pConnection, buttonPixmapClicked, buttonWindow, buttonContext,
					0,
					0,
					0,
					0,
					31,
					17);
				xcb_flush(pConnection);
			}
			else if(pButtonPress->event == window)
			{
				cout << "button pressed on main window" << endl;
			}
			
			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			// handle the button release event
			xcb_button_release_event_t* pButtonRelease = (xcb_button_release_event_t*)pEv;

			if(pButtonRelease->event == buttonWindow)
			{
				cout << "button released on button window" << endl;

				buttonHeld = false;

				xcb_copy_area(
					pConnection, buttonPixmapHighlighted, buttonWindow, buttonContext,
					0,
					0,
					0,
					0,
					31,
					17);
				xcb_flush(pConnection);
			}

			break;
		}
		case XCB_ENTER_NOTIFY:
		{
			// pointer entered window
			xcb_enter_notify_event_t* pEntered = (xcb_enter_notify_event_t*)pEv;

			if(pEntered->event == buttonWindow)
			{
				cout << "mouse over button!" << endl;

				mouseOver = true;

				if(buttonHeld)
				{
					xcb_copy_area(
						pConnection, buttonPixmapClicked, buttonWindow, buttonContext,
						0,
						0,
						0,
						0,
						31,
						17);
					xcb_flush(pConnection);
				}
				else
				{
					xcb_copy_area(
						pConnection, buttonPixmapHighlighted, buttonWindow, buttonContext,
						0,
						0,
						0,
						0,
						31,
						17);
					xcb_flush(pConnection);
				}
			}

			break;
		}
		case XCB_LEAVE_NOTIFY:
		{
			// pointer left window
			xcb_leave_notify_event_t* pLeft = (xcb_leave_notify_event_t*)pEv;

			if(pLeft->event == buttonWindow)
			{
				cout << "mouse left button!" << endl;

				mouseOver = false;

				xcb_copy_area(
					pConnection, buttonPixmap, buttonWindow, buttonContext,
					0,
					0,
					0,
					0,
					31,
					17);
				xcb_flush(pConnection);
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
