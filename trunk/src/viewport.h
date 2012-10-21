/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport.h %Main display data. */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "window.h"

class Viewport;

/**
 * Known mouse modes.
 * @ingroup viewport_group
 */
enum ViewportMouseMode {
	MM_INACTIVE,       ///< Inactive mode.
	MM_TILE_TERRAFORM, ///< Terraforming tiles.
	MM_PATH_BUILDING,  ///< Construct paths.
	MM_SHOP_PLACEMENT, ///< Placement of a shop.

	MM_COUNT,          ///< Number of mouse modes.
};

/**
 * Available cursor types.
 * @ingroup viewport_group
 */
enum CursorType {
	CUR_TYPE_NORTH,    ///< Show a N corner highlight.
	CUR_TYPE_EAST,     ///< Show a E corner highlight.
	CUR_TYPE_SOUTH,    ///< Show a S corner highlight.
	CUR_TYPE_WEST,     ///< Show a W corner highlight.
	CUR_TYPE_TILE,     ///< Show a tile highlight.
	CUR_TYPE_ARROW_NE, ///< Show a build arrow in the NE direction.
	CUR_TYPE_ARROW_SE, ///< Show a build arrow in the SE direction.
	CUR_TYPE_ARROW_SW, ///< Show a build arrow in the SW direction.
	CUR_TYPE_ARROW_NW, ///< Show a build arrow in the NW direction.

	CUR_TYPE_INVALID,  ///< Invalid/unused cursor.
};

/**
 * Data about a cursor.
 * @ingroup viewport_group
 */
class Cursor {
public:
	Cursor(Viewport *vp)
	{
		this->vp = vp;
		this->xpos = 0;
		this->ypos = 0;
		this->zpos = 0;
		this->type = CUR_TYPE_INVALID;
	}

	Viewport *vp;    ///< Parent viewport object.

	uint16 xpos;     ///< %Voxel x position of the cursor.
	uint16 ypos;     ///< %Voxel y position of the cursor.
	uint8  zpos;     ///< %Voxel z position of the cursor.
	CursorType type; ///< Type of cursor.

	inline void MarkDirty();

	/**
	 * Set a cursor.
	 * @param xpos X position of the voxel containing the cursor.
	 * @param ypos Y position of the voxel containing the cursor.
	 * @param zpos Z position of the voxel containing the cursor.
	 * @param type Type of cursor to set.
	 * @param always Always set the cursor (else, only set it if it changed).
	 * @return Cursor has been set/changed.
	 */
	bool SetCursor(uint16 xpos, uint16 ypos, uint8 zpos, CursorType type, bool always = false)
	{
		if (!always && this->xpos == xpos && this->ypos == ypos && this->zpos == zpos && this->type == type) return false;
		this->MarkDirty();
		this->xpos = xpos;
		this->ypos = ypos;
		this->zpos = zpos;
		this->type = type;
		this->MarkDirty();
		return true;
	}

	/**
	 * Get a cursor.
	 * @param xpos Expected x coordinate of the cursor.
	 * @param ypos Expected y coordinate of the cursor.
	 * @param zpos Expected z coordinate of the cursor.
	 * @return The cursor sprite if the cursor exists and the coordinates are correct, else \c NULL.
	 */
	inline CursorType GetCursor(uint16 xpos, uint16 ypos, uint8 zpos)
	{
		if (this->xpos != xpos || this->ypos != ypos || this->zpos != zpos) return CUR_TYPE_INVALID;
		return this->type;
	}

	/** Mark the cursor as being invalid, and update the viewport if necessary. */
	void SetInvalid()
	{
		this->MarkDirty();
		this->type = CUR_TYPE_INVALID;
	}
};

/**
 * Class for displaying parts of the world.
 * @ingroup viewport_group
 */
class Viewport : public Window {
public:
	Viewport(int x, int y, uint w, uint h);
	~Viewport();

	void MarkVoxelDirty(int16 xpos, int16 ypos, int16 zpos, int16 height = 0);
	virtual void OnDraw();

	void Rotate(int direction);
	void MoveViewport(int dx, int dy);

	void SetMouseModeState(ViewportMouseMode mode, uint8 state = 0);
	ViewportMouseMode GetMouseMode();

	bool ComputeCursorPosition(bool select_corner, uint16 *xvoxel, uint16 *yvoxel, uint8 *zvoxel, CursorType *cur_type);
	CursorType GetCursorAtPos(uint16 xpos, uint16 ypos, uint8 zpos);
	uint8 GetMaxCursorHeight(uint16 xpos, uint16 ypos, uint8 zpos);

	void EnableWorldAdditions();
	void DisableWorldAdditions();

	int32 xview; ///< X position of the center point of the viewport.
	int32 yview; ///< Y position of the center point of the viewport.
	int32 zview; ///< Z position of the center point of the viewport.

	uint16 tile_width;           ///< Width of a tile.
	uint16 tile_height;          ///< Height of a tile.
	ViewOrientation orientation; ///< Direction of view.
	Cursor tile_cursor;          ///< Cursor for selecting a tile (or tile corner).
	Cursor arrow_cursor;         ///< Cursor for showing the path/track build direction.

private:
	ViewportMouseMode mouse_mode; ///< Mode of the mouse, decides how to react to mouse clicks, drags, etc.
	Point16 mouse_pos;            ///< Last known position of the mouse.
	uint8 mouse_state;            ///< Last known state of the mouse buttons.

	bool additions_enabled;       ///< Flashing of world additions is enabled.
	bool additions_displayed;     ///< Additions in #_additions are displayed to the user.

	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual WmMouseEvent OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);

	virtual void TimeoutCallback();

	Point32 ComputeHorizontalTranslation(int dx, int dy);
	int32 ComputeX(int32 xpos, int32 ypos);
	int32 ComputeY(int32 xpos, int32 ypos, int32 zpos);
};

/** Update the cursor at the screen. */
inline void Cursor::MarkDirty()
{
	if (this->type != CUR_TYPE_INVALID) this->vp->MarkVoxelDirty(this->xpos, this->ypos, this->zpos);
}

/** A single mouse mode. */
class MouseMode {
public:
	MouseMode(WindowTypes wtype, ViewportMouseMode mode);
	virtual ~MouseMode();

	/**
	 * Query the mode whether it can be enabled.
	 * @return Whether the mode may be enabled.
	 */
	virtual bool ActivateMode() = 0;

	/** Notification that the mouse mode has been disabled. */
	virtual void LeaveMode() = 0;

	virtual void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos);
	virtual void OnMouseButtonEvent(Viewport *vp, uint8 state);
	virtual void OnMouseWheelEvent(Viewport *vp, int direction);

	const WindowTypes wtype;      ///< Type of window associated with this mouse mode.
	const ViewportMouseMode mode; ///< Mouse mode implemented by the object.
};

/** Default mouse mode, selected when no other mouse mode is available. */
class DefaultMouseMode : public MouseMode {
public:
	DefaultMouseMode();

	virtual bool ActivateMode();
	virtual void LeaveMode();
};

/** All mouse modes. */
class MouseModes {
public:
	MouseModes();

	void RegisterMode(MouseMode *mm);

	Viewport *main_display;     ///< Main screen, managed by #Viewport.
	MouseMode *current;         ///< Current mouse mode.
	MouseMode *modes[MM_COUNT]; ///< Registered mouse modes.

private:
	DefaultMouseMode default_mode;
};

extern MouseModes _mouse_modes;

Viewport *GetViewport();
void SetViewportMousemode();

#endif
