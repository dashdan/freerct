/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h %Window handling data structures. */

#ifndef WINDOW_H
#define WINDOW_H

#include "geometry.h"
#include "orientation.h"
#include "widget.h"

class Viewport;

/**
 * Available types of windows.
 * @ingroup window_group
 */
enum WindowTypes {
	WC_MAINDISPLAY,  ///< Main display of the world.
	WC_TOOLBAR,      ///< Main toolbar.
	WC_QUIT,         ///< Quit program window.
	WC_PATH_BUILDER, ///< Path build gui.
};

/** Codes of the #NotifyChange function, which gets forwarded through the #Window::OnChange method. */
enum ChangeCodes {
	CHG_UPDATE_BUTTONS,   ///< Recompute the state of the buttons.
	CHG_VIEWPORT_ROTATED, ///< Viewport rotated.
};

/** Various state flags of the %Window. */
enum WindowFlags {
	WF_HIGHLIGHT = 1 << 0, ///< %Window edge is highlighted.
};

/**
 * Known mouse buttons.
 * @ingroup window_group
 */
enum MouseButtons {
	MB_LEFT   = 1, ///< Left button down.
	MB_MIDDLE = 2, ///< Middle button down.
	MB_RIGHT  = 4, ///< Right button down.

	MB_CURRENT    = 0x07, ///< Bitmask for current mouse state.
	MB_PREVIOUS   = 0x70, ///< Bitmask for previous mouse state.
	MB_PREV_SHIFT = 4,    ///< Amount of shifting to get previous mouse state.
};
DECLARE_ENUM_AS_BIT_SET(MouseButtons)

/**
 * Mouse events of the window manager. Value is returned from the Window::OnMouseButtonEvent.
 * @ingroup window_group
 */
enum WmMouseEvent {
	WMME_NONE,         ///< Do nothing special.
	WMME_CLOSE_WINDOW, ///< Close the window.
	WMME_MOVE_WINDOW,  ///< Initiate a window move.
};

/**
 * Available mouse modes of the window manager.
 * @ingroup window_group
 */
enum WmMouseModes {
	WMMM_PASS_THROUGH, ///< No special mode, pass events on to the windows.
	WMMM_MOVE_WINDOW,  ///< Move the current window.
};

/**
 * %Window base class.
 * @ingroup window_group
 */
class Window {
public:
	Window(WindowTypes wtype);
	virtual ~Window();

	Rectangle32 rect;  ///< Screen area covered by the window.
	WindowTypes wtype; ///< %Window type.

	/**
	 * Timeout counter.
	 * Decremented on each iteration. When it reaches 0, #TimeoutCallback is called.
	 */
	uint8 timeout;
	uint8 flags;    ///< %Window flags. @see WindowFlags
	Window *higher; ///< %Window above this window (managed by #WindowManager).
	Window *lower;  ///< %Window below this window (managed by #WindowManager).

	virtual void SetSize(uint width, uint height);
	void SetPosition(int x, int y);
	virtual Point32 OnInitialPosition();

	void MarkDirty();

	virtual void OnDraw();
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();

	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);
	virtual void OnChange(int code, uint32 parameter);
};

/**
 * Base class for windows with a widget tree.
 * @ingroup window_group
 */
class GuiWindow : public Window {
public:
	GuiWindow(WindowTypes wtype);
	virtual ~GuiWindow();
	virtual void OnDraw();

	virtual void UpdateWidgetSize(int wid_num, BaseWidget *wid);
	virtual void DrawWidget(int wid_num, const BaseWidget *wid) const;
	virtual void SetSize(uint width, uint height);

	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseLeaveEvent();
	virtual void TimeoutCallback();
	virtual void SetHighlight(bool value);

	/**
	 * Get the horizontal position of the top-left corner of a widget (of this window) at the screen.
	 * @param wid %Widget to use.
	 * @return Horizontal position of the top-left corner of the widget at the screen.
	 */
	int GetWidgetScreenX(const BaseWidget *wid) const
	{
		return this->rect.base.x + wid->pos.base.x;
	}

	/**
	 * Get the vertical position of the top-left corner of a widget (of this window) at the screen.
	 * @param wid %Widget to use.
	 * @return Vertical position of the top-left corner of the widget at the screen.
	 */
	int GetWidgetScreenY(const BaseWidget *wid) const
	{
		return this->rect.base.y + wid->pos.base.y;
	}

protected:
	Point16 mouse_pos;    ///< Mouse position relative to the window (negative coordinates means 'out of window').

	void SetupWidgetTree(const WidgetPart *parts, int length);

	template <typename WID>
	inline WID *GetWidget(WidgetNumber wnum);

	template <typename WID>
	inline const WID *GetWidget(WidgetNumber wnum) const;

	void SetWidgetChecked(WidgetNumber widget, bool value);
	bool IsWidgetChecked(WidgetNumber widget) const;

	void SetWidgetPressed(WidgetNumber widget, bool value);
	bool IsWidgetPressed(WidgetNumber widget) const;

	void SetWidgetShaded(WidgetNumber widget, bool value);
	bool IsWidgetShaded(WidgetNumber widget) const;

	void SetRadioButtonsSelected(const WidgetNumber *wids, WidgetNumber selected);
	WidgetNumber GetSelectedRadioButton(const WidgetNumber *wids);

	virtual void OnClick(WidgetNumber widget);

private:
	BaseWidget *tree;     ///< Tree of widgets.
	BaseWidget **widgets; ///< Array of widgets with a non-negative index (use #GetWidget to get the widgets from this array).
	uint16 num_widgets;   ///< Number of widgets in #widgets.
};

/**
 * Get the widget.
 * @tparam WID %Widget class.
 * @param wnum %Widget number to get.
 * @return Address of the widget.
 */
template <typename WID>
inline WID *GuiWindow::GetWidget(WidgetNumber wnum)
{
	assert(wnum < this->num_widgets);
	return dynamic_cast<WID *>(this->widgets[wnum]);
}

/**
 * Get the widget.
 * @tparam WID %Widget class.
 * @param wnum %Widget number to get.
 * @return Address of the widget.
 */
template <typename WID>
inline const WID *GuiWindow::GetWidget(WidgetNumber wnum) const
{
	assert(wnum < this->num_widgets);
	return dynamic_cast<WID *>(this->widgets[wnum]);
}

/**
 * Specialized template for #BaseWidget
 * @param wnum %Widget number to get.
 * @return Address of the base widget.
 */
template <>
inline BaseWidget *GuiWindow::GetWidget(WidgetNumber wnum)
{
	assert(wnum < this->num_widgets);
	return this->widgets[wnum];
}

/**
 * Specialized template for #BaseWidget
 * @param wnum %Widget number to get.
 * @return Address of the base widget.
 */
template <>
inline const BaseWidget *GuiWindow::GetWidget(WidgetNumber wnum) const
{
	assert(wnum < this->num_widgets);
	return this->widgets[wnum];
}


/**
 * %Window manager class, manages the window stack.
 * @ingroup window_group
 */
class WindowManager {
public:
	WindowManager();
	~WindowManager();

	bool HasWindow(Window *w);
	void AddTostack(Window *w);
	void DeleteWindow(Window *w);

	void MouseMoveEvent(const Point16 &pos);
	void MouseButtonEvent(MouseButtons button, bool pressed);
	void MouseWheelEvent(int direction);
	void Tick();

	Point16 GetMousePosition() const;

	Window *top;        ///< Top-most window in the window stack.
	Window *bottom;     ///< Lowest window in the window stack.

private:
	Window *FindWindowByPosition(const Point16 &pos) const;
	bool UpdateCurrentWindow();

	void StartWindowMove();

	Point16 mouse_pos;      ///< Last reported mouse position.
	Window *current_window; ///< 'Current' window under the mouse.
	uint8 mouse_state;      ///< Last reported mouse button state (lower 4 bits).
	uint8 mouse_mode;       ///< Mouse mode of the window manager. @see WmMouseModes

	Point16 move_offset;    ///< Offset from the top-left of the #current_window being moved in #WMMM_MOVE_WINDOW mode to the mouse position.
};

extern WindowManager _manager;

bool IsLeftClick(uint8 state);

void UpdateWindows();
Window *GetWindowByType(WindowTypes wtype);
bool HighlightWindowByType(WindowTypes wtype);
void NotifyChange(WindowTypes wtype, int code, uint32 parameter);

Viewport *ShowMainDisplay();
void ShowToolbar();
void ShowPathBuildGui();

#endif
