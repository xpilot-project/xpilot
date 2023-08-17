/*
 * ImgWindow.h
 *
 * Integration for dear imgui into X-Plane.
 *
 * Copyright (C) 2018,2020 Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef IMGWINDOW_H
#define IMGWINDOW_H

#include "SystemGL.h"

#include <climits>
#include <string>
#include <memory>

#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
#include <imgui.h>
#include <queue>

#include "ImgFontAtlas.h"

/** ImgWindow is a Window for creating dear imgui widgets within.
 *
 * There's a few traps to be aware of when using dear imgui with X-Plane:
 *
 * 1) The Dear ImGUI coordinate scheme is inverted in the Y axis vs the X-Plane
 *    (and OpenGL default) scheme. You must be careful if you're trying to
 *    directly manipulate positioning of widgets rather than letting imgui
 *    self-layout.  There are (private) functions in ImgWindow to do the
 *    coordinate mapping.
 *
 * 2) The Dear ImGUI rendering space is only as big as the window - this means
 *    popup elements cannot be larger than the parent window.  This was
 *    unavoidable on XP11 because of how popup windows work and the possibility
 *    for negative coordinates (which imgui doesn't like).
 *
 * 3) There is no way to detect if the window is hidden without a per-frame
 *    processing loop or similar.
 *
 * @note It should be possible to map globally on XP9 & XP10 letting you run
 *     popups as large as you need, or to use the ImGUI native titlebars instead
 *     of the XP10 ones - source for this may be provided later, but could also
 *     be trivially adapted from this one by adjusting the way the space is
 *     translated and mapped in the DrawWindowCB and constructor.
 */
class
    ImgWindow
{
public:
    /** sFontAtlas is the global shared font-atlas.
     *
     * If you want to share fonts between windows, this needs to be set before
     * any dialogs are actually instantiated.  It will be automatically handed
     * over to the contexts as they're created.
     */
    static std::shared_ptr<ImgFontAtlas> sFontAtlas;

    virtual ~ImgWindow();

    /** Gets the current window geometry */
    void GetWindowGeometry(int& left, int& top, int& right, int& bottom) const
    {
        XPLMGetWindowGeometry(mWindowID, &left, &top, &right, &bottom);
    }

    /** Sets the current window geometry */
    void SetWindowGeometry(int left, int top, int right, int bottom)
    {
        XPLMSetWindowGeometry(mWindowID, left, top, right, bottom);
    }

    /** Gets the current window geometry of a popped out window */
    void GetWindowGeometryOS(int& left, int& top, int& right, int& bottom) const
    {
        XPLMGetWindowGeometryOS(mWindowID, &left, &top, &right, &bottom);
    }

    /** Sets the current window geometry of a popped out window */
    void SetWindowGeometryOS(int left, int top, int right, int bottom)
    {
        XPLMSetWindowGeometryOS(mWindowID, left, top, right, bottom);
    }

    /** Gets the current window size of window in VR */
    void GetWindowGeometryVR(int& width, int& height) const
    {
        XPLMGetWindowGeometryVR(mWindowID, &width, &height);
    }

    /** Sets the current window size of window in VR */
    void SetWindowGeometryVR(int width, int height)
    {
        XPLMSetWindowGeometryVR(mWindowID, width, height);
    }

    /** Gets the current valid geometry (free, OS, or VR
        If VR, then left=bottom=0 and right=width and top=height*/
    void GetCurrentWindowGeometry(int& left, int& top, int& right, int& bottom) const;

    /** Set resize limits. If set this way then the window object knows. */
    void SetWindowResizingLimits(int minW, int minH, int maxW, int maxH);

    /** SetVisible() makes the window visible after making the onShow() call.
     * It is also at this time that the window will be relocated onto the VR
     * display if the VR headset is in use.
     *
     * @param inIsVisible true to be displayed, false if the window is to be
     * hidden.
     */
    virtual void SetVisible(bool inIsVisible);

    /** GetVisible() returns the current window visibility.
     * @return true if the window is visible, false otherwise.
    */
    bool GetVisible() const;

    /** Is Window popped out */
    bool IsPoppedOut() const
    {
        return XPLMWindowIsPoppedOut(mWindowID) != 0;
    }

    /** Is Window in VR? */
    bool IsInVR() const
    {
        return XPLMWindowIsInVR(mWindowID) != 0;
    }

    /** Is Window inside the sim? */
    bool IsInsideSim() const
    {
        return !IsPoppedOut() && !IsInVR();
    }

    /** Set the positioning mode
     * @see https://developer.x-plane.com/sdk/XPLMDisplay/#XPLMWindowPositioningMode */
    void SetWindowPositioningMode(XPLMWindowPositioningMode inPosMode,
        int                       inMonitorIdx = -1)
    {
        XPLMSetWindowPositioningMode(mWindowID, inPosMode, inMonitorIdx);
    }

    /** Bring window to front of Z-order */
    void BringWindowToFront()
    {
        XPLMBringWindowToFront(mWindowID);
    }

    /** Is Window in front of Z-order? */
    bool IsWindowInFront() const
    {
        return XPLMIsWindowInFront(mWindowID) != 0;
    }

    /** @brief Define Window drag area, ie. an area in which dragging with the mouse
     * moves the entire window.
     * @details Useful for windows without decoration.
     * For convenience (often you want a strip at the top of the window to be the drag area,
     * much like a little title bar), coordinates originate in the top-left
     * corner of the window and go right/down, ie. vertical axis is contrary
     * to what X-Plane usually uses, but in line with the ImGui system.
     * Right/Bottom can be set much large than window size just to extend the
     * drag area to the window's edges. So 0,0,INT_MAX,INT_MAX will surely
     * make the entire window the drag area.
     * @param left Left begin of drag area, relative to window's origin
     * @param top Ditto, top begin
     * @param right Ditto, right end
     * @param bottom Ditto, bottom end
     */
    void SetWindowDragArea(int left = 0, int top = 0, int right = INT_MAX, int bottom = INT_MAX);

    /** Clear the drag area, ie. stop the drag-the-window functionality */
    void ClearWindowDragArea();

    /** Is a drag area defined? Return its sizes if wanted */
    bool HasWindowDragArea(int* pL = nullptr, int* pT = nullptr,
        int* pR = nullptr, int* pB = nullptr) const;

    /** Is given position inside the defined drag area?
     * @param x Horizontal position in ImGui coordinates (0,0 in top/left corner)
     * @param y Vertical position in ImGui coordinates
     */
    bool IsInsideWindowDragArea(int x, int y) const;

protected:
    /** mFirstRender can be checked during buildInterface() to see if we're
     * being rendered for the first time or not.  This is particularly
     * important for windows that use Columns as SetColumnWidth() should only
     * be called once.
     *
     * There may be other times where it's advantageous to make specific ImGui
     * calls once and once only.
     */
    bool mFirstRender;

    /** Construct a window with the specified bounds
     *
     * @param left Left edge of the window's contents in global boxels.
     * @param top Top edge of the window's contents in global boxels.
     * @param right Right edge of the window's contents in global boxels.
     * @param bottom Bottom edge of the window's contents in global boxels.
     * @param decoration The decoration style to use (see notes)
     * @param layer the preferred layer to present this window in (see notes)
     *
     * @note The decoration should generally be one presented/rendered by XP -
     *     the ImGui window decorations are very intentionally supressed by
     *     ImgWindow to allow them to fit in with the rest of the simulator.
     *
     * @note The only layers that really make sense are Floating and Modal.  Do
     *     not set VR layer here however unless the window is ONLY to be
     *     rendered in VR.
     */
    ImgWindow(
        int left,
        int top,
        int right,
        int bottom,
        XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
        XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows);

    /** An ImgWindow object must not be copied!
     */
    ImgWindow(const ImgWindow&) = delete;
    ImgWindow& operator = (const ImgWindow&) = delete;

    /** SetWindowTitle sets the title of the window both in the ImGui layer and
     * at the XPLM layer.
     *
     * @param title the title to set.
     */
    void SetWindowTitle(const std::string& title);

    /** moveForVR() is an internal helper for moving the window to either it's
     * preferred layer or the VR layer depending on if the headset is in use.
     */
    void moveForVR();

    /** A hook called right before ImGui::Begin in case you want to set up something
     * before interface building begins
     * @return addition flags to be passed to the imgui::begin() call,
     *         like for example ImGuiWindowFlags_MenuBar */
    virtual ImGuiWindowFlags_ beforeBegin()
    {
        return ImGuiWindowFlags_None;
    }

    /** buildInterface() is the method where you can define your ImGui interface
     * and handle events.  It is called every frame the window is drawn.
     *
     * @note You must NOT delete the window object inside buildInterface() -
     *     use SafeDelete() for that.
     */
    virtual void buildInterface() = 0;

    /** A hook called after all rendering is done, right before the
     * X-Plane window draw call back returns
     * in case you want to do something that otherwise would conflict with rendering. */
    virtual void afterRendering()
    {
    }

    /** onShow() is called before making the Window visible.  It provides an
     * opportunity to prevent the window being shown.
     *
     * @note the implementation in the base-class is a null handler.  You can
     *     safely override this without chaining.
     *
     * @return true if the Window should be shown, false if the attempt to show
     *     should be suppressed.
     */
    virtual bool onShow();

    /** SafeDelete() can be used within buildInterface() to get the object to
     *     self-delete once it's finished rendering this frame.
     */
    void SafeDelete();

    /** Returns X-Plane's internal Window id */
    XPLMWindowID GetWindowId() const
    {
        return mWindowID;
    }

private:
    std::shared_ptr<ImgFontAtlas> mFontAtlas;

    static void DrawWindowCB(XPLMWindowID inWindowID, void* inRefcon);

    static int HandleMouseClickCB(
        XPLMWindowID inWindowID,
        int x, int y,
        XPLMMouseStatus inMouse,
        void* inRefcon);

    static void HandleKeyFuncCB(
        XPLMWindowID inWindowID,
        char inKey,
        XPLMKeyFlags inFlags,
        char inVirtualKey,
        void* inRefcon,
        int losingFocus);

    static XPLMCursorStatus HandleCursorFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        void* inRefcon);

    static int HandleMouseWheelFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        int wheel,
        int clicks,
        void* inRefcon);

    static int HandleRightClickFuncCB(
        XPLMWindowID inWindowID,
        int x, int y,
        XPLMMouseStatus inMouse,
        void* inRefcon);

    static float SelfDestructCallback(float inElapsedSinceLastCall,
        float inElapsedTimeSinceLastFlightLoop,
        int inCounter,
        void* inRefcon);
    static std::queue<ImgWindow*>  sPendingDestruction;
    static XPLMFlightLoopID         sSelfDestructHandler;

    int HandleMouseClickGeneric(
        int x, int y,
        XPLMMouseStatus inMouse,
        int button = 0);

    void RenderImGui(ImDrawData* draw_data);

    void updateImgui();

    void updateMatrices();

    void boxelsToNative(int x, int y, int& outX, int& outY);

    void translateImguiToBoxel(float inX, float inY, int& outX, int& outY);

    void translateToImguiSpace(int inX, int inY, float& outX, float& outY);

    float mModelView[16], mProjection[16];
    int mViewport[4];

    std::string mWindowTitle;

    XPLMWindowID mWindowID;
    ImGuiContext* mImGuiContext;
    GLuint mFontTexture;

    int mTop;
    int mBottom;
    int mLeft;
    int mRight;

    XPLMWindowLayer mPreferredLayer;

    /** Set if `xplm_WindowDecorationSelfDecoratedResizable`,
     *  ie. we need to handle resizing ourselves: X-Plane provides
     *  the "hand" mouse icon but as we catch mouse events X-Plane
     *  cannot handle resizing. (And passing on mouse events is
     *  discouraged.
     *  @see https://developer.x-plane.com/sdk/XPLMHandleMouseClick_f/ */
    const bool bHandleWndResize;

    /** Resize limits. There's no way to query XP, so we need to keep track ourself */
    int minWidth = 100;
    int minHeight = 100;
    int maxWidth = INT_MAX;
    int maxHeight = INT_MAX;

    /** Window drag area in ImGui coordinates (0,0 is top/left corner) */
    int dragLeft = -1;
    int dragTop = -1;
    int dragRight = -1;       // right > left
    int dragBottom = -1;       // bottom > top!

    /** Last (processed) mouse drag pos while moving/resizing */
    int lastMouseDragX = -1;
    int lastMouseDragY = -1;

    /** What are we dragging right now? */
    struct DragTy
    {
        bool wnd : 1;
        bool left : 1;
        bool top : 1;
        bool right : 1;
        bool bottom : 1;

        DragTy()
        {
            clear();
        }
        void clear()
        {
            wnd = left = top = right = bottom = false;
        }
        operator bool() const
        {
            return wnd || left || top || right || bottom;
        }
    } dragWhat;

    typedef struct
    {
        char inKey;
        XPLMKeyFlags inFlags;
        char inVirtualKey;
    } KeyPress;
    std::queue<KeyPress> mKeyQueue;
};

#endif // #ifndef IMGWINDOW_H