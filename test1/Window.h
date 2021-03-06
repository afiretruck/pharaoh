/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <X11/Xlib.h>
#include <set>

namespace Pharaoh
{
    class PharaohWindow
    {
    public:
        //! \brief ctor - Create a PharaohWindow object. Represents a top-level window.
        //! \param pXDisplay The X-display to use.
        //! \param rootWindow The root window of the given X-display.
        //! \param clientWindow The actual X window to handle.
        PharaohWindow(Display* pXDisplay, Window rootWindow, Window clientWindow);

        //! \brief ctor - Create a PharaohWindow object. Represents a top-level window.
        //! \param pXDisplay The X-display to use.
        //! \param rootWindow The root window of the given X-display.
        //! \param clientWindow The actual X window to handle.
        //! \param x Initial x-position.
        //! \param y Initial y-position.
        //! \param width Initial width.
        //! \param height Initial height.
        PharaohWindow(
            Display* pXDisplay, 
            Window rootWindow, 
            Window clientWindow, 
            int x, 
            int y, 
            unsigned int width, 
            unsigned int height);

        //! \brief Configure the window.
        //! \param windowChanges The window configuration to pass to X with XConfigureWindow.
        //! \param valueMask Passed to XConfigureWindow.
        void Configure(XWindowChanges& windowChanges, unsigned int valueMask);

        //! \brief Show the window. This triggers creation of any decorations.
        //! \param decorationWindows Global set of window handles to ignore various events for.
        void Map(std::set<Window>& decorationWindows);

        //! \brief Hide the window. This will destroy any decorations.
        //! \param decorationWindows Global set of window handles to ignore various events for.
        void Unmap(std::set<Window>& decorationWindows);

        //! \brief Return true if the window is mapped (on-screen), false if not.
        bool IsMapped() const;

        //! \brief Move the window to a new location.
        //! \param destinationX The new X-coordinate for the window frame.
        //! \param destinationY The new Y-coordinate for the window frame.
        void SetLocation(const int destinationX, const int destinationY);

        //! \brief Get the window location.
        //! \param x Output variable for the x-coordinate.
        //! \param y Output variable for the y-coordinate.
        void GetLocation(int& x, int &y) const;

        //! \brief  Resize the window. Note that this is the size of the window frame.
        //!         The client area will be resized appropriately. A frameless client
        //!         will adopt the whole size given, as it has no window decoration.
        //! \param width The new width.
        //! \param height The new height.
        void SetSize(const unsigned int width, const unsigned int height);

        //! \brief Get the window size.
        //! \param width The output variable for the window width.
        //! \param height The output variable for the window height.
        void GetSize(unsigned int& width, unsigned int& height) const;

        //! \brief If this window is mapped, bring it to the top and give it focus
        void RaiseAndSetFocus();

        //! \brief If the window is mapped, bring it to the top
        void Raise();

        enum LocationInFrame
        {
            LocationInFrame_None,
            LocationInFrame_DragBar,
            LocationInFrame_ResizeFrameLeft,
            LocationInFrame_ResizeFrameRight,
            LocationInFrame_ResizeFrameTop,
            LocationInFrame_ResizeFrameBottom,
            LocationInFrame_ResizeFrameTopLeft,
            LocationInFrame_ResizeFrameTopRight,
            LocationInFrame_ResizeFrameBottomLeft,
            LocationInFrame_ResizeFrameBottomRight,

        };

        //! \brief Get the frame window for this window. Only valid if the window is mapped.
        Window GetFrameWindow() const;

        //! \brief  Takes the frame-relative coordinates given in x & y and 
        //!         returns which part of the frame those coordinates fall into.
        //! \param x The X-coordinate relative to the frame.
        //! \param y The Y-coordinate relative to the frame.
        LocationInFrame GetPositionInFrame(const int x, const int y) const;

    private:
        // core data
        Display* m_pXDisplay;
        Window m_RootWindow;
        Window m_ClientWindow;

        // helpful data
        bool m_IsMapped = false;
        int m_X = 0;
        int m_Y = 0;
        unsigned int m_Width = 0;
        unsigned int m_Height = 0;

        // decoration data
        Window m_FrameWindow;

    };
}

#endif