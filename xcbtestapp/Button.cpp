/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include "Button.h"
#include <vector>
#include <iostream>

#define cimg_use_png
#include "CImg.h"

using namespace std;
using namespace cimg_library;
using namespace Emperor;

// TODO: add error checking

//---------------------------------------------------------------------------------
// Ctor & Dtor
//---------------------------------------------------------------------------------

Button::Button(
    int width,
    int height,
    int x,
    int y,
    xcb_window_t parentWindow,
    xcb_connection_t* pConnection,
    xcb_screen_t* pScreenData,
    const string& normalImage,
    const string& highlightedImage,
    const string& clickedImage,
    const function<void()>& onClick)
	: m_pConnection(pConnection)
	, m_ButtonHeld(false)
	, m_MouseOver(false)
	, m_Width(width)
	, m_Height(height)
    , OnClick(onClick)
{
    // create the button window
    uint32_t masks = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t buttonMask[2] =
	{
			0x00ffffff,
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_EXPOSURE
	};
	m_ButtonWindow = xcb_generate_id(pConnection);
	xcb_create_window(
			pConnection,					// xcb connection
			XCB_COPY_FROM_PARENT,			// depth
			m_ButtonWindow,					// window id
			parentWindow,					// parent window
			x, y, 							// x, y
			width, height,					// width, height
			0,								// border width
			XCB_WINDOW_CLASS_INPUT_OUTPUT,	// class
			pScreenData->root_visual,		// visual
			masks, 							// masks bitmap
			&buttonMask);					// masks value array
    
	xcb_map_window(pConnection, m_ButtonWindow);
	xcb_flush(pConnection);

    // find the right pixmap format to use
    const xcb_setup_t* pSetup = xcb_get_setup(pConnection);
	xcb_format_t* pFormats = xcb_setup_pixmap_formats(pSetup);
	xcb_format_t* pFormat = nullptr;
	uint32_t numFormats = xcb_setup_pixmap_formats_length(pSetup);
	for(uint32_t i = 0; i < numFormats; i++)
	{
		// 24 bit depth with 32 bits per pixel is ideal
		if(pFormats[i].bits_per_pixel == 32 && pFormats[i].depth == 24)
		{
			pFormat = &pFormats[i];
		}
	}

    // get the bytes per pixel and the scanline padding values
    uint32_t bytesPerPixel = pFormat->bits_per_pixel / 8;
    uint32_t linePadBytes = (bytesPerPixel * width) % (pFormat->scanline_pad / 8);

    // temporary storage
    uint32_t bytesPerLine = (width * bytesPerPixel) + linePadBytes;
    vector<uint8_t> imageBuffer(bytesPerLine * height);

    array<string, 3> names = 
    {
        normalImage,
        highlightedImage,
        clickedImage
    };

    // generate each image
    for(int i = 0; i < 3; i++)
    {
        // read in the image
        CImg<unsigned char> imageReader(names[i].c_str());

        // copy to temp buffer
        if(bytesPerPixel > 3)
        {
            for(int y = 0; y < height; y++)
            {
                for(int x = 0; x < width; x++)
                {
                    uint32_t base = (y * width + x) * bytesPerPixel + (y * linePadBytes);

                    imageBuffer[base] = imageReader(x, y, 0, 2);
                    imageBuffer[base + 1] = imageReader(x, y, 0, 1);
                    imageBuffer[base + 2] = imageReader(x, y, 0, 0);
                    imageBuffer[base + 3] = 0x00;
                }
            }
        }
        else
        {
            for(int y = 0; y < height; y++)
            {
                for(int x = 0; x < width; x++)
                {
                    uint32_t base = (y * width + x) * bytesPerPixel + (y * linePadBytes);

                    imageBuffer[base] = imageReader(x, y, 0, 2);
                    imageBuffer[base + 1] = imageReader(x, y, 0, 1);
                    imageBuffer[base + 2] = imageReader(x, y, 0, 0);
                }
            }
        }

        // create xlib image
        m_ButtonImages[i] = xcb_image_create(
            width,
            height,
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            pFormat->scanline_pad,
            pFormat->depth,
            pFormat->bits_per_pixel,
            0,
            (xcb_image_order_t)pSetup->image_byte_order,
            XCB_IMAGE_ORDER_LSB_FIRST,
            imageBuffer.data(),
            imageBuffer.size(),
            imageBuffer.data());

        // create pixmap
        m_ButtonPixmaps[i] = xcb_generate_id(pConnection);
        xcb_create_pixmap(pConnection, pFormat->depth, m_ButtonPixmaps[i], m_ButtonWindow, width, height);

        // create graphics context if first image
        if(i == 0)
        {
            uint32_t buttonPixmapMask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
            uint32_t buttonPixmapValues[2] =
            {
                0,
                0x00ffffff
            };
            m_GraphicsContext = xcb_generate_id(pConnection);
            xcb_create_gc(pConnection, m_GraphicsContext, m_ButtonPixmaps[i], buttonPixmapMask, buttonPixmapValues);
            xcb_flush(pConnection);
        }

        // load the images to the pixmaps
        xcb_image_put(pConnection, m_ButtonPixmaps[i], m_GraphicsContext, m_ButtonImages[i], 0, 0, 0);
    }

    xcb_flush(pConnection);
}

Button::~Button()
{
	xcb_void_cookie_t eck = xcb_free_gc_checked(m_pConnection, m_GraphicsContext);
	xcb_generic_error_t* pError = xcb_request_check(m_pConnection, eck);
	if(pError != nullptr)
	{
		// failed to free the graphics context
		free(pError);
	}

	for(size_t i = 0; i < m_ButtonPixmaps.size(); i++)
	{
		eck = xcb_free_pixmap_checked(m_pConnection, m_ButtonPixmaps[i]);
		pError = xcb_request_check(m_pConnection, eck);
		if(pError != nullptr)
		{
			// failed to free pixmap
			free(pError);
		}

		free(m_ButtonImages[i]);
	}
}

//---------------------------------------------------------------------------------
// Getters
//---------------------------------------------------------------------------------

xcb_window_t Button::GetWindow() const
{
    return m_ButtonWindow;
}

//---------------------------------------------------------------------------------
// Event handlers
//---------------------------------------------------------------------------------

void Button::ExposeEvent(xcb_expose_event_t* pEvent)
{
	if(m_ButtonHeld == true)
	{
		DrawButton(ImageType::Clicked);
	}
	else if(m_MouseOver == true)
	{
		DrawButton(ImageType::Highlighted);
	}
	else
	{
		DrawButton(ImageType::Normal);
	}
}

void Button::ButtonPressEvent(xcb_button_press_event_t* pEvent)
{
	if(pEvent->detail == XCB_BUTTON_INDEX_1)
	{
		m_ButtonHeld = true;
		DrawButton(ImageType::Clicked);
	}
}

void Button::ButtonReleaseEvent(xcb_button_release_event_t* pEvent)
{
	if(pEvent->detail == XCB_BUTTON_INDEX_1)
	{
		m_ButtonHeld = false;
		DrawButton(ImageType::Highlighted);

        if(m_MouseOver == true)
        {
            OnClick();
        }
	}
}

void Button::MouseEnterEvent(xcb_enter_notify_event_t* pEvent)
{
	m_MouseOver = true;
	if(m_ButtonHeld == true)
	{
		DrawButton(ImageType::Clicked);
	}
	else
	{
		DrawButton(ImageType::Highlighted);
	}
}

void Button::MouseLeaveEvent(xcb_leave_notify_event_t* pEvent)
{
	m_MouseOver = false;
	DrawButton(ImageType::Normal);
}

//---------------------------------------------------------------------------------
// Internal draw
//---------------------------------------------------------------------------------

void Button::DrawButton(ImageType image)
{
    int index = static_cast<int>(image);
    xcb_copy_area(
        m_pConnection, m_ButtonPixmaps[index], m_ButtonWindow, m_GraphicsContext,
        0,
        0,
        0,
        0,
        m_Width,
        m_Height);
    xcb_flush(m_pConnection);
}
