/*
 * Adapted from LiveTraffic's LTImgWindow
 * (c) 2018-2020 Birger Hoppe
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:\n
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include "XPImgWindow.h"

#define FATAL_COULD_NOT_LOAD_FONT "Could not load GUI font %s"
#define WARN_NOT_LOADED_ICON_FONT "Could not load icon font, icons will not be displayed properly"
constexpr ImU32 LTIM_BTN_COL = IM_COL32(0xB0, 0xB0, 0xB0, 0xFF);

namespace ImGui
{
    /// width of a single-icon button
    static float gWidthIconBtn = NAN;

    // Get width of an icon button (calculate on first use)
    IMGUI_API float GetWidthIconBtn(bool _bWithSpacing)
    {
        if (std::isnan(gWidthIconBtn))
            gWidthIconBtn = CalcTextSize(ICON_FA_WINDOW_MAXIMIZE).x;
        if (_bWithSpacing)
            return gWidthIconBtn + ImGui::GetStyle().ItemSpacing.x;
        else
            return gWidthIconBtn;
    }

    // Helper for creating unique IDs
    IMGUI_API void PushID_formatted(const char* format, ...)
    {
        // format the variable string
        va_list args;
        char sz[500];
        va_start(args, format);
        vsnprintf(sz, sizeof(sz), format, args);
        va_end(args);
        // Call the actual push function
        PushID(sz);
    }

    // Outputs aligned text
    IMGUI_API void TextAligned(AlignTy _align, const std::string& s)
    {
        // Change cursor position so that text becomes aligned
        if (_align != IM_ALIGN_LEFT)
        {
            ImVec2 avail = GetContentRegionAvail();
            ImVec2 pos = GetCursorPos();
            const float txtWidth = CalcTextSize(s.c_str()).x;
            if (_align == IM_ALIGN_CENTER)
                pos.x += avail.x / 2.0f - txtWidth / 2.0f;
            else
                pos.x += avail.x - txtWidth - ImGui::GetStyle().ItemSpacing.x;
            SetCursorPos(pos);
        }
        // Output the text
        TextUnformatted(s.c_str());
    }

    // Small Button with on-hover popup helper text
    IMGUI_API bool SmallButtonTooltip(const char* label,
        const char* tip,
        ImU32 colFg,
        ImU32 colBg)
    {
        // Setup colors
        if (colFg != IM_COL32(1, 1, 1, 0))
            PushStyleColor(ImGuiCol_Text, colFg);
        if (colBg != IM_COL32(1, 1, 1, 0))
            PushStyleColor(ImGuiCol_Button, colBg);

        // do the button (and we _really_ using also x frame padding = 0)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        bool b = SmallButton(label);
        ImGui::PopStyleVar(2);

        // restore previous colors
        if (colBg != IM_COL32(1, 1, 1, 0))
            PopStyleColor();
        if (colFg != IM_COL32(1, 1, 1, 0))
            PopStyleColor();

        // do the tooltip
        if (tip && IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(300);
            ImGui::TextUnformatted(tip);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        // return if button pressed
        return b;
    }

    IMGUI_API bool ButtonTooltip(const char* label,
        const char* tip,
        ImU32 colFg,
        ImU32 colBg,
        const ImVec2& size)
    {
        // Setup colors
        if (colFg != IM_COL32(1, 1, 1, 0))
            PushStyleColor(ImGuiCol_Text, colFg);
        if (colBg != IM_COL32(1, 1, 1, 0))
            PushStyleColor(ImGuiCol_Button, colBg);

        // do the button
        bool b = Button(label, size);

        // restore previous colors
        if (colBg != IM_COL32(1, 1, 1, 0))
            PopStyleColor();
        if (colFg != IM_COL32(1, 1, 1, 0))
            PopStyleColor();

        // do the tooltip
        if (tip && IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(300);
            ImGui::TextUnformatted(tip);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        // return if button pressed
        return b;
    }

    IMGUI_API bool ButtonIcon(const char* label, const char* tooltip, bool rightAligned)
    {
        // Setup colors for window sizing buttons
        PushStyleColor(ImGuiCol_Text, LTIM_BTN_COL);                                         // very light gray
        PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);                               // transparent
        PushStyleColor(ImGuiCol_ButtonHovered, GetColorU32(ImGuiCol_ScrollbarGrab));  // gray

        if (rightAligned)
            SetCursorPosX(GetWindowContentRegionMax().x - GetWidthIconBtn() - ImGui::GetStyle().ItemSpacing.x);

        bool b = ButtonTooltip(label, tooltip);

        // Restore colors
        PopStyleColor(3);

        return b;
    }
}

XPImgWindow::XPImgWindow(WndMode _mode, WndStyle _style, WndRect _initPos) :
    ImgWindow(_initPos.left(), _initPos.top(),
        _initPos.right(), _initPos.bottom(),
        toDeco(_style), toLayer(_style)),
    wndStyle(_style),
    rectFloat(_initPos),
    mIsVREnabled("sim/graphics/VR/enabled")
{
    // Disable reading/writing of "imgui.ini"
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Create a flight loop id, but don't schedule it yet
    XPLMCreateFlightLoop_t flDef = {
        sizeof(flDef),                              // structSize
        xplm_FlightLoop_Phase_BeforeFlightModel,    // phase
        cbChangeWndMode,                               // callbackFunc
        (void*)this,                                // refcon
    };
    flChangeWndMode = XPLMCreateFlightLoop(&flDef);

    // Safeguard position a bit against "out of view" positions
    if (_initPos.left() > 0 || _initPos.bottom())
    {
        WndRect screen;
        XPLMGetScreenBoundsGlobal(&screen.left(), &screen.top(),
            &screen.right(), &screen.bottom());
        if (!screen.contains(_initPos.tl) ||
            !screen.contains(_initPos.tl.shiftedBy(50, -20)))
        {
            // top left corner is not (sufficiently) inside screen bounds
            // -> make left/botom = 0 so that wnd will be centered
            //    while retaining width/height
            _initPos.shift(-_initPos.left(), -_initPos.bottom());
        }
    }

    // If _initPos defines left/bottom already then we don't do centered
    if (_initPos.left() > 0 || _initPos.bottom() > 0)
    {
        // Don't center the window as we have a proper position
        if (_mode == WND_MODE_FLOAT_CENTERED)
            _mode = WND_MODE_FLOAT;
        else if (_mode == WND_MODE_FLOAT_CNT_VR)
            _mode = WND_MODE_FLOAT_OR_VR;
    }

    // Set the positioning mode
    SetMode(_mode);
}

XPImgWindow::~XPImgWindow()
{
    if (flChangeWndMode)
        XPLMDestroyFlightLoop(flChangeWndMode);
    flChangeWndMode = nullptr;
}

/// Set the window mode, move the window if needed
void XPImgWindow::SetMode(WndMode _mode)
{
    // auto-set VR mode if requested
    if (_mode == WND_MODE_FLOAT_OR_VR)
        _mode = IsVREnabled() ? WND_MODE_VR : WND_MODE_FLOAT;
    else if (_mode == WND_MODE_FLOAT_CNT_VR)
        _mode = IsVREnabled() ? WND_MODE_VR : WND_MODE_FLOAT_CENTERED;

    // Floating: Save current geometry to have a chance to get back there
    if (GetMode() == WND_MODE_FLOAT && _mode != WND_MODE_FLOAT)
        rectFloat = GetCurrentWindowGeometry();

    // Do set the XP window positioning mode
    SetWindowPositioningMode(toPosMode(_mode, IsVREnabled()));

    // reset a wish to re-position
    nextWinMode = WND_MODE_NONE;

    // If we pop in, then we need to explicitely set a position for the window to appear
    if (_mode == WND_MODE_FLOAT && !rectFloat.empty()) {
        SetWindowGeometry(rectFloat.left(), rectFloat.top(),
            rectFloat.right(), rectFloat.bottom());
        rectFloat.clear();
    }
    // if we set any of the "centered" modes
    // we shall set it back to floating a few flight loops later
    else if (_mode == WND_MODE_FLOAT_CENTERED)
    {
        nextWinMode = WND_MODE_FLOAT;           // to floating
        rectFloat.clear();                      // but don't move the window!
        XPLMScheduleFlightLoop(flChangeWndMode, -5.0, 1);  // in 5 flight loops time
    }
}

/// Get current window mode
WndMode XPImgWindow::GetMode() const
{
    if (IsInVR())
        return WND_MODE_VR;
    if (IsPoppedOut())
        return WND_MODE_POPOUT;
    return WND_MODE_FLOAT;
}

// Get current window geometry as an WndRect structure
WndRect XPImgWindow::GetCurrentWindowGeometry() const
{
    WndRect r;
    ImgWindow::GetCurrentWindowGeometry(r.left(), r.top(), r.right(), r.bottom());
    return r;
}

// Loose keyboard foucs, ie. return focus to X-Plane proper, if I have it now
bool XPImgWindow::ReturnKeyboardFocus()
{
    if (XPLMHasKeyboardFocus(GetWindowId())) {
        XPLMTakeKeyboardFocus(0);
        return true;
    }
    return false;
}

// flight loop callback for stuff we cannot do during drawing callback
// Outside all rendering we can change things like window mode
float XPImgWindow::cbChangeWndMode(float, float, int, void* inRefcon)
{
    // refcon is pointer to ImguiWidget
    XPImgWindow& wnd = *reinterpret_cast<XPImgWindow*>(inRefcon);

    // Has user requested to close the window?
    if (wnd.nextWinMode == WND_MODE_CLOSE)
        delete& wnd;

    // Has user requested a change in window mode?
    else if (wnd.nextWinMode > WND_MODE_NONE)
        wnd.SetMode(wnd.nextWinMode);

    return 0.0f;
}

bool XPImgWindowInit()
{
    if (!ImgWindow::sFontAtlas)
        ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();

    if (!ImgWindow::sFontAtlas->AddFontFromFileTTF((GetXPlanePath() + WND_STANDARD_FONT).c_str(), WND_FONT_SIZE))
    {
        LOG_MSG(logFATAL, FATAL_COULD_NOT_LOAD_FONT, WND_STANDARD_FONT);
        return false;
    }

    ImFontConfig config;
    config.MergeMode = true;

    static ImVector<ImWchar> icon_ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddText(ICON_FA_QUESTION_CIRCLE);
    builder.AddText(ICON_FA_HEADSET);
    builder.AddText(ICON_FA_INFO);
    builder.BuildRanges(&icon_ranges);

    if (!ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(
        fa_solid_900_compressed_data,
        fa_solid_900_compressed_size,
        WND_FONT_SIZE,
        &config,
        icon_ranges.Data))
    {
        LOG_MSG(logWARN, "%s", WARN_NOT_LOADED_ICON_FONT);
    }

    return true;
}

void XPImgWindowCleanup()
{
    ImgWindow::sFontAtlas.reset();
}
