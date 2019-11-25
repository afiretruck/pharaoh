/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <string>
#include <functional>
#include <array>

namespace Emperor
{
    class Button
    {
    public:
        Button(
            int width,
            int height,
            int x,
            int y,
            xcb_window_t parentWindow,
            xcb_connection_t* pConnection,
            xcb_screen_t* pScreenData,
            const std::string& normalImage,
            const std::string& highlightedImage,
            const std::string& clickedImage,
            const std::function<void()>& onClick);

        virtual ~Button();

        xcb_window_t GetWindow() const;

        void ExposeEvent(xcb_expose_event_t* pEvent);
        void ButtonPressEvent(xcb_button_press_event_t* pEvent);
        void ButtonReleaseEvent(xcb_button_release_event_t* pEvent);
        void MouseEnterEvent(xcb_enter_notify_event_t* pEvent);
        void MouseLeaveEvent(xcb_leave_notify_event_t* pEvent);

    private:
        xcb_connection_t* m_pConnection;
        xcb_window_t m_ButtonWindow;
        std::array<xcb_image_t*, 3> m_ButtonImages;
        std::array<xcb_pixmap_t, 3> m_ButtonPixmaps;
        xcb_gcontext_t m_GraphicsContext;
        bool m_ButtonHeld;
        bool m_MouseOver;
        int m_Width;
        int m_Height;

        enum class ImageType
        {
            Normal = 0,
            Highlighted = 1,
            Clicked = 2
        };

        void DrawButton(ImageType image);


    };
}
