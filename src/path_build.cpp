/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_build.cpp %Path building manager code. */

#include "stdafx.h"
#include "path_build.h"
#include "map.h"
#include "window.h"
#include "viewport.h"

PathBuildManager _path_builder; ///< Path build manager.

/** Imploded path tile sprite number to use for an 'up' slope from a given edge. */
static const PathSprites _path_up_from_edge[EDGE_COUNT] = {
	PATH_RAMP_NE, ///< EDGE_NE
	PATH_RAMP_SE, ///< EDGE_SE
	PATH_RAMP_SW, ///< EDGE_SW
	PATH_RAMP_NW, ///< EDGE_NW
};

/** Imploded path tile sprite number to use for an 'down' slope from a given edge. */
static const PathSprites _path_down_from_edge[EDGE_COUNT] = {
	PATH_RAMP_SW, ///< EDGE_NE
	PATH_RAMP_NW, ///< EDGE_SE
	PATH_RAMP_NE, ///< EDGE_SW
	PATH_RAMP_SE, ///< EDGE_NW
};

/**
 * Get the right path sprite for putting in the world.
 * @param tsl Slope of the path.
 * @param edge Edge to connect from.
 * @todo Path sprites should connect to neighbouring paths.
 */
static uint8 GetPathSprite(TrackSlope tsl, TileEdge edge)
{
	assert(edge < EDGE_COUNT);

	switch (tsl) {
		case TSL_FLAT: return PATH_EMPTY;
		case TSL_DOWN: return _path_down_from_edge[edge];
		case TSL_UP:   return _path_up_from_edge[edge];
		default: NOT_REACHED();
	}
}

/**
 * In the given voxel, can a path be build in the voxel from the bottom at the given edge?
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @param edge Entry edge.
 * @return Bit-set of track slopes, indicating the directions of building paths.
 */
static uint8 CanBuildPathFromEdge(int16 xpos, int16 ypos, int8 zpos, TileEdge edge)
{
	if (zpos < 0 || zpos >= MAX_VOXEL_STACK_SIZE - 1) return 0;

	const VoxelStack *vs = _world.GetStack(xpos, ypos);

	const Voxel *above = (zpos < MAX_VOXEL_STACK_SIZE) ? vs->Get(zpos + 1) : NULL;
	if (above != NULL && above->GetType() != VT_EMPTY) return 0; // Not empty just above us -> path will not work here.

	const Voxel *below = (zpos > 0) ? vs->Get(zpos - 1) : NULL;
	if (below != NULL && below->GetType() == VT_SURFACE) {
		const SurfaceVoxelData *svd = below->GetSurface();
		if (svd->path.type != PT_INVALID) return 0; // A path just below us won't work either.
	}

	const Voxel *level = vs->Get(zpos);
	if (level != NULL) {
		switch (level->GetType()) {
			case VT_COASTER:
				return 0; // A roller-coaster  is in the way.

			case VT_SURFACE: {
				const SurfaceVoxelData *svd = level->GetSurface();
				if (svd->foundation.type != FDT_INVALID) return 0;
				if (svd->path.type != PT_INVALID) {
					if (svd->path.slope < PATH_FLAT_COUNT) return 1 << TSL_FLAT; // Already a flat path there.
					if (_path_down_from_edge[edge] == svd->path.slope) return 1 << TSL_UP; // Already a sloped path up.
					return 0; // A path but cannot connect to it.
				}
				if (svd->ground.type != GTP_INVALID) {
					TileSlope ts = ExpandTileSlope(svd->ground.slope);
					if ((ts & TCB_STEEP) != 0) return 0; // Steep slope in the way.
					if ((ts & _corners_at_edge[edge]) != 0) return 0; // Some hilly stuff at the entry.
				}
				break;
			}

			case VT_EMPTY:
				break;

			case VT_REFERENCE:
				// XXX return 0 is a too brute-force.
				return 0;

			default:
				NOT_REACHED();
		}
	}

	/* No trivial cases or show stoppers, do a more precise check for each slope.
	 * Above: empty
	 * Below: Does not contain a surface + path.
	 * Level: Is not a coaster or a reference, does not contain a path or foundations, has no steep ground nor raised corners at the entrance edge.
	 */
	uint8 result = 0;

	/* Try building a upward slope.
	 * Works if not at the top, and the voxel at z+2 is also empty.
	 */
	if (zpos < MAX_VOXEL_STACK_SIZE - 2) {
		const Voxel *v = vs->Get(zpos + 2);
		if (v == NULL || v->GetType() == VT_EMPTY) result |= 1 << TSL_UP;
	}

	/* Try building a level slope. */
	{
		if (level == NULL) {
			result |= 1 << TSL_FLAT;
		} else {
			VoxelType vt = level->GetType();
			if (vt == VT_EMPTY) {
				result |= 1 << TSL_FLAT;
			} else {
				assert(vt == VT_SURFACE);
				const SurfaceVoxelData *svd = level->GetSurface();
				assert(svd->path.type == PT_INVALID && svd->foundation.type == FDT_INVALID);
				if (svd->ground.type != GTP_INVALID && svd->ground.slope == 0) result |= 1 << TSL_FLAT;
			}
		}
	}

	/* Try building a downward slope. */
	if ((level == NULL || level->GetType() == VT_EMPTY) && zpos > 0) {
		VoxelType vt = (below != NULL) ? below->GetType() : VT_EMPTY;
		if (vt == VT_EMPTY) {
			result |= 1 << TSL_DOWN;
		} else if (vt == VT_SURFACE) {
			const SurfaceVoxelData *svd = below->GetSurface();
			if (svd->foundation.type == FDT_INVALID && svd->path.type == PT_INVALID) {
				/* No foundations and paths. */
				if (svd->ground.type == GTP_INVALID) {
					result |= 1 << TSL_DOWN;
				} else {
					TileSlope ts = ExpandTileSlope(svd->ground.slope);
					if (((TCB_STEEP | _corners_at_edge[(edge + 2) % 4]) & ts) == 0) result |= 1 << TSL_DOWN;
				}
			}
		}
	}

	return result;
}

/**
 * Compute the attach points of a path in a voxel.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @return Attach points for paths starting from the given voxel coordinates.
 *         Upper 4 bits are the edges at the top of the voxel, lower 4 bits are the attach points for the bottom of the voxel.
 */
static uint8 GetPathAttachPoints(int16 xpos, int16 ypos, int8 zpos)
{
	if (xpos >= _world.GetXSize()) return 0;
	if (ypos >= _world.GetYSize()) return 0;
	if (zpos >= MAX_VOXEL_STACK_SIZE - 1) return 0; // the voxel containing the flat path, and one above it.

	const Voxel *v = _world.GetVoxel(xpos, ypos, zpos);
	if (v->GetType() != VT_SURFACE) return 0; // XXX Maybe also handle referenced surface voxels?
	const SurfaceVoxelData *svd = v->GetSurface();

	uint8 edges = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		uint16 x = xpos + _tile_dxy[edge].x;
		uint16 y = ypos + _tile_dxy[edge].y;
		if (x >= _world.GetXSize() || y >= _world.GetYSize()) continue;

		if (svd->path.type != PT_INVALID) {
			if (svd->path.slope < PATH_FLAT_COUNT) {
				if (CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				if (_path_up_from_edge[edge] == svd->path.slope
						&& CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
				if (_path_down_from_edge[edge] == svd->path.slope
						&& CanBuildPathFromEdge(x, y, zpos + 1, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		if (svd->ground.type != GTP_INVALID) {
			TileSlope ts = ExpandTileSlope(svd->ground.slope);
			if ((ts & TCB_STEEP) != 0) continue;
			if ((ts & _corners_at_edge[edge]) == 0) {
				if (CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				if (CanBuildPathFromEdge(x, y, zpos + 1, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		/* No path and no ground -> Invalid, do next edge. */
	}
	return edges;
}

/** Default constructor. */
PathBuildManager::PathBuildManager()
{
	this->state = PBS_IDLE;
	this->selected_arrow = INVALID_EDGE;
	this->selected_slope = TSL_INVALID;
	DisableWorldAdditions();
}

/** Restart the path build interaction sequence. */
void PathBuildManager::Reset()
{
	this->selected_arrow = INVALID_EDGE;
	this->selected_slope = TSL_INVALID;
	if (this->state != PBS_IDLE) this->state = PBS_WAIT_VOXEL;
	this->UpdateState();
}

/**
 * Set the state of the path build GUI.
 * @param opened If \c true the path gui just opened, else it closed.
 */
void PathBuildManager::SetPathGuiState(bool opened)
{
	this->state = opened ? PBS_WAIT_VOXEL : PBS_IDLE;
	this->UpdateState();
	Viewport *vp = GetViewport();
	if (opened || (vp != NULL && vp->GetMouseMode() == MM_PATH_BUILDING)) SetViewportMousemode();
}

/**
 * User clicked somewhere. Check whether it is a good tile or path, and if so, move the building process forward.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 */
void PathBuildManager::TileClicked(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->state == PBS_IDLE) return;
	uint8 dirs = GetPathAttachPoints(xpos, ypos, zpos);
	if (dirs == 0) return;

	this->xpos = xpos;
	this->ypos = ypos;
	this->zpos = zpos;
	this->allowed_arrows = dirs;
	this->state = PBS_WAIT_ARROW;
	this->UpdateState();
}

/**
 * User selected a build direction. Check it, and if so, move the building process forward.
 * @param direction Direction of build.
 */
void PathBuildManager::SelectArrow(TileEdge direction)
{
	if (this->state < PBS_WAIT_ARROW || direction >= INVALID_EDGE) return;
	if ((this->allowed_arrows & (0x11 << direction)) == 0) return;
	this->selected_arrow = direction;
	this->state = PBS_WAIT_SLOPE;
	this->UpdateState();
}

/**
 * See whether moving in the indicated direction of the tile position is possible/makes sense.
 * @param direction Direction of movement.
 * @param delta_z Proposed change of Z height.
 * @param need_path %Voxel should contain a path.
 * @return Tile cursor position was moved.
 */
bool PathBuildManager::TryMove(TileEdge direction, int delta_z, bool need_path)
{
	Point16 dxy = _tile_dxy[direction];
	if ((dxy.x < 0 && this->xpos == 0) || (dxy.x > 0 && this->xpos == _world.GetXSize() - 1)) return false;
	if ((dxy.y < 0 && this->ypos == 0) || (dxy.y > 0 && this->ypos == _world.GetYSize() - 1)) return false;
	if ((delta_z < 0 && this->zpos == 0) || (delta_z > 0 && this->zpos == MAX_VOXEL_STACK_SIZE - 1)) return false;
	const Voxel *v = _world.GetVoxel(this->xpos + dxy.x, this->ypos + dxy.y, this->zpos + delta_z);
	if (v != NULL && (v->GetType() == VT_COASTER || v->GetType() == VT_REFERENCE)) return false;
	if (need_path) {
		if (v == NULL || v->GetType() != VT_SURFACE) return false;
		const SurfaceVoxelData *svd = v->GetSurface();
		if (svd->path.type == PT_INVALID) return false;
	}

	this->xpos += dxy.x;
	this->ypos += dxy.y;
	this->zpos += delta_z;
	this->state = PBS_WAIT_ARROW;
	this->UpdateState();
	return true;
}

/**
 * Try to move the tile cursor to a new tile.
 * @param edge Direction of movement.
 * @param move_up Current tile seems to move up.
 */
void PathBuildManager::MoveCursor(TileEdge edge, bool move_up)
{
	if (this->state <= PBS_WAIT_ARROW || edge == INVALID_EDGE) return;

	/* First try to find a voxel with a path at it. */
	if (this->TryMove(edge, 0, true)) return;
	if ( move_up && this->TryMove(edge,  1, true)) return;
	if (!move_up && this->TryMove(edge, -1, true)) return;

	/* Otherwise just settle for a surface. */
	if (this->TryMove(edge, 0, false)) return;
	if ( move_up && this->TryMove(edge,  1, false)) return;
	if (!move_up && this->TryMove(edge, -1, false)) return;
}

/**
 * User clicked 'forward' or 'back'. Figure out which direction to move in.
 * @param move_forward Move forward (if \c false, move back).
 */
void PathBuildManager::SelectMovement(bool move_forward)
{
	if (this->state <= PBS_WAIT_ARROW) return;

	TileEdge edge = (move_forward) ? this->selected_arrow : (TileEdge)((this->selected_arrow + 2) % 4);

	bool move_up;
	const Voxel *v = _world.GetVoxel(this->xpos, this->ypos, this->zpos);
	if (v == NULL || v->GetType() != VT_SURFACE) return;
	const SurfaceVoxelData *svd = v->GetSurface();
	if (svd->path.type != PT_INVALID) {
		move_up = (svd->path.slope == _path_down_from_edge[edge]);
	} else if (svd->ground.type != GTP_INVALID) {
		TileSlope ts = ExpandTileSlope(svd->ground.slope);
		if ((ts & TCB_STEEP) != 0) return;
		move_up = ((ts & _corners_at_edge[edge]) != 0);
	} else {
		return; // Surface type but no valid ground/path surface.
	}

	this->MoveCursor(edge, move_up);
}

/**
 * Compute the voxel to display the arrow cursor.
 * @param xpos [out] Computed X position of the voxel that should contain the arrow cursor.
 * @param ypos [out] Computed Y position of the voxel that should contain the arrow cursor.
 * @param zpos [out] Computed Z position of the voxel that should contain the arrow cursor.
 */
void PathBuildManager::ComputeArrowCursorPosition(uint16 *xpos, uint16 *ypos, uint8 *zpos)
{
	assert(this->state > PBS_WAIT_ARROW);
	assert(this->selected_arrow != INVALID_EDGE);

	Point16 dxy = _tile_dxy[this->selected_arrow];
	*xpos = this->xpos + dxy.x;
	*ypos = this->ypos + dxy.y;

	uint8 bit = 1 << this->selected_arrow;
	*zpos = this->zpos;
	if ((bit & this->allowed_arrows) == 0) { // Build direction is not at the bottom of the voxel.
		assert(((bit << 4) &  this->allowed_arrows) != 0); // Should be available at the top of the voxel.
		(*zpos)++;
	}

	/* Do some paranoia checking. */
	assert(*xpos < _world.GetXSize());
	assert(*ypos < _world.GetYSize());
	assert(*zpos < MAX_VOXEL_STACK_SIZE);
}

/**
 * Compute the new contents of the voxel where the path should be added from the #_world.
 * @param svd  [out] New contents of the (surface) voxel if it can be placed.
 * @param xpos [out] Computed X position of the voxel that should contain the arrow cursor.
 * @param ypos [out] Computed Y position of the voxel that should contain the arrow cursor.
 * @param zpos [out] Computed Z position of the voxel that should contain the arrow cursor.
 * @return An addition was computed successfully.
 */
bool PathBuildManager::ComputeWorldAdditions(SurfaceVoxelData *svd, uint16 *xpos, uint16 *ypos, uint8 *zpos)
{
	assert(this->state == PBS_WAIT_BUY); // It needs selected_arrow and selected_slope.

	if (((1 << this->selected_slope) & this->allowed_slopes) == 0) return false;

	this->ComputeArrowCursorPosition(xpos, ypos, zpos);
	if (this->selected_slope == TSL_DOWN) (*zpos)--;
	const Voxel *v = _world.GetVoxel(*xpos, *ypos, *zpos);
	if (v == NULL || v->GetType() == VT_EMPTY) {
		svd->path.type = PT_CONCRETE;
		svd->path.slope = GetPathSprite(this->selected_slope, (TileEdge)((this->selected_arrow + 2) % 4));
		svd->ground.type = GTP_INVALID;
		svd->foundation.type = FDT_INVALID;
		return true;
	} else if (v->GetType() == VT_SURFACE) {
		*svd = *(v->GetSurface()); // Copy the data.
		svd->path.type = PT_CONCRETE;
		svd->path.slope = GetPathSprite(this->selected_slope, (TileEdge)((this->selected_arrow + 2) % 4));
		return true;
	}
	return false;
}

/**
 * Add edges of the neighbouring path tiles in #_additions.
 * @param xpos X coordinate of the central voxel with a path tile.
 * @param ypos Y coordinate of the central voxel with a path tile.
 * @param zpos Z coordinate of the central voxel with a path tile.
 * @param slope Path slope of the central voxel.
 * @param use_additions Use #_additions rather than #_world.
 * @param add_edges If set, add edges (else, remove them).
 * @return Updated slope at the central voxel.
 */
static uint8 AddRemovePathEdges(uint16 xpos, uint16 ypos, uint8 zpos, uint8 slope, bool use_additions, bool add_edges)
{
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		int delta_z = 0;
		if (slope >= PATH_FLAT_COUNT) {
			if (_path_down_from_edge[edge] == slope) {
				delta_z = 1;
			} else if (_path_up_from_edge[edge] != slope) {
				continue;
			}
		}
		Point16 dxy = _tile_dxy[edge];
		if ((dxy.x < 0 && xpos == 0) || (dxy.x > 0 && xpos == _world.GetXSize() - 1)) continue;
		if ((dxy.y < 0 && ypos == 0) || (dxy.y > 0 && ypos == _world.GetYSize() - 1)) continue;

		TileEdge edge2 = (TileEdge)((edge + 2) % 4);
		bool modified = false;
		if (delta_z <= 0 || zpos < MAX_VOXEL_STACK_SIZE - 1) {
			Voxel *v;
			if (use_additions) {
				v = _additions.GetCreateVoxel(xpos + dxy.x, ypos + dxy.y, zpos + delta_z, false);
			} else {
				v = _world.GetCreateVoxel(xpos + dxy.x, ypos + dxy.y, zpos + delta_z, false);
			}
			if (v != NULL && v->GetType() == VT_SURFACE) {
				SurfaceVoxelData *svd = v->GetSurface();
				if (svd->path.type != PT_INVALID) {
					svd->path.slope = SetPathEdge(svd->path.slope, edge2, add_edges);
					modified = true;
				}
			}
		}
		delta_z--;
		if (delta_z >= 0 || zpos > 0) {
			Voxel *v;
			if (use_additions) {
				v = _additions.GetCreateVoxel(xpos + dxy.x, ypos + dxy.y, zpos + delta_z, false);
			} else {
				v = _world.GetCreateVoxel(xpos + dxy.x, ypos + dxy.y, zpos + delta_z, false);
			}
			if (v != NULL && v->GetType() == VT_SURFACE) {
				SurfaceVoxelData *svd = v->GetSurface();
				if (svd->path.type != PT_INVALID) {
					svd->path.slope = SetPathEdge(svd->path.slope, edge2, add_edges);
					modified = true;
				}
			}
		}
		if (modified && slope < PATH_FLAT_COUNT) slope = SetPathEdge(slope, edge, add_edges);
	}
	return slope;
}

/** Update the state of the path build process. */
void PathBuildManager::UpdateState()
{
	Viewport *vp = GetViewport();

	if (this->state == PBS_IDLE) {
		this->selected_arrow = INVALID_EDGE;
		this->selected_slope = TSL_INVALID;
	}

	/* The tile cursor is controlled by the viewport if waiting for a voxel or earlier. */
	if (vp != NULL && this->state > PBS_WAIT_VOXEL) {
		vp->tile_cursor.SetCursor(this->xpos, this->ypos, this->zpos, CUR_TYPE_TILE);
	}

	/* See whether the PBS_WAIT_ARROW state can be left automatically. */
	if (this->state == PBS_WAIT_ARROW) {
		this->allowed_arrows = GetPathAttachPoints(this->xpos, this->ypos, this->zpos);

		/* If a valid selection has been made, or if only one choice exists, take it. */
		if (this->selected_arrow != INVALID_EDGE && ((0x11 << this->selected_arrow) & this->allowed_arrows) != 0) {
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_NE) || this->allowed_arrows == (0x10 << EDGE_NE)) {
			this->selected_arrow = EDGE_NE;
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_NW) || this->allowed_arrows == (0x10 << EDGE_NW)) {
			this->selected_arrow = EDGE_NW;
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_SE) || this->allowed_arrows == (0x10 << EDGE_SE)) {
			this->selected_arrow = EDGE_SE;
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_SW) || this->allowed_arrows == (0x10 << EDGE_SW)) {
			this->selected_arrow = EDGE_SW;
			this->state = PBS_WAIT_SLOPE;
		}
	}

	/* Set the arrow cursor. Note that display is controlled later. */
	if (vp != NULL) {
		if (this->state > PBS_WAIT_ARROW) {
			uint16 x_arrow, y_arrow;
			uint8 z_arrow;
			this->ComputeArrowCursorPosition(&x_arrow, &y_arrow, &z_arrow);
			vp->arrow_cursor.SetCursor(x_arrow, y_arrow, z_arrow, (CursorType)(CUR_TYPE_ARROW_NE + this->selected_arrow));
		} else {
			vp->arrow_cursor.SetInvalid();
		}
	}

	/* See whether the PBS_WAIT_SLOPE state can be left automatically. */
	if (this->state == PBS_WAIT_SLOPE) {
		/* Compute allowed slopes. */
		uint16 x_arrow, y_arrow;
		uint8 z_arrow;
		this->ComputeArrowCursorPosition(&x_arrow, &y_arrow, &z_arrow);
		this->allowed_slopes = CanBuildPathFromEdge(x_arrow, y_arrow, z_arrow, (TileEdge)((this->selected_arrow + 2) % 4));

		/* If a valid selection has been made, or if only one choice exists, take it. */
		if (this->selected_slope != TSL_INVALID && ((1 << this->selected_slope) & this->allowed_slopes) != 0) {
			this->state = PBS_WAIT_BUY;
		} else if (this->allowed_slopes == (1 << TSL_DOWN)) {
			this->selected_slope = TSL_DOWN;
			this->state = PBS_WAIT_BUY;
		} else if (this->allowed_slopes == (1 << TSL_FLAT)) {
			this->selected_slope = TSL_FLAT;
			this->state = PBS_WAIT_BUY;
		} else if (this->allowed_slopes == (1 << TSL_UP)) {
			this->selected_slope = TSL_UP;
			this->state = PBS_WAIT_BUY;
		}
	}

	/* Handle _additions display. */
	if (vp != NULL) {
		if (this->state == PBS_WAIT_SLOPE) {
			_additions.Clear();
			EnableWorldAdditions();
		} else if (this->state == PBS_WAIT_BUY) {
			_additions.Clear();

			uint16 xpos, ypos;
			uint8 zpos;
			SurfaceVoxelData svd;
			if (this->ComputeWorldAdditions(&svd, &xpos, &ypos, &zpos)) {
				Voxel *v = _additions.GetCreateVoxel(xpos, ypos, zpos, true);
				svd.path.slope = AddRemovePathEdges(xpos, ypos, zpos, svd.path.slope, true, true); // Change the neighbouring edges too.
				v->SetSurface(svd);
			}
			EnableWorldAdditions();
		} else {
			DisableWorldAdditions();
		}
	}

	NotifyChange(WC_PATH_BUILDER, CHG_UPDATE_BUTTONS, 0);
}

/**
 * Can the user press the 'remove' button at the path gui?
 * @return Button is enabled.
 */
bool PathBuildManager::GetRemoveIsEnabled() const
{
	if (this->state == PBS_IDLE || this->state == PBS_WAIT_VOXEL) return false;
	/* If current tile has a path, it can be removed. */
	const Voxel *v = _world.GetVoxel(this->xpos, this->ypos, this->zpos);
	if (v != NULL && v->GetType() == VT_SURFACE) {
		const SurfaceVoxelData *svd = v->GetSurface();
		if (svd->path.type != PT_INVALID) return true;
	}
	return this->state == PBS_WAIT_BUY;
}

/**
 * Select a slope from the allowed slopes.
 * @param slope Slope to select.
 */
void PathBuildManager::SelectSlope(TrackSlope slope)
{
	if (this->state < PBS_WAIT_SLOPE || slope >= TSL_COUNT_GENTLE) return;
	if ((this->allowed_slopes & (1 << slope)) != 0) {
		this->selected_slope = slope;
		this->state = PBS_WAIT_SLOPE;
		this->UpdateState();
	}
}

/** Enter long path building mode. */
void PathBuildManager::SelectLong()
{
	return; // XXX Not implemented currently
}

/**
 * User selected 'buy' or 'remove'. Perform the action, and update the path build state.
 * @param buying If \c true, user selected 'buy'.
 */
void PathBuildManager::SelectBuyRemove(bool buying)
{
	if (buying) {
		// Buying a path tile.
		if (this->state != PBS_WAIT_BUY) return;
		_additions.Commit();
		this->SelectMovement(true);
	} else {
		// Removing a path tile.
		if (this->state <= PBS_WAIT_VOXEL) return;
		Voxel *v = _world.GetCreateVoxel(this->xpos, this->ypos, this->zpos, false);
		if (v == NULL || v->GetType() != VT_SURFACE) return;
		SurfaceVoxelData *svd = v->GetSurface();
		if (svd->path.type == PT_INVALID) return;

		svd->path.type = PT_INVALID;
		AddRemovePathEdges(this->xpos, this->ypos, this->zpos, svd->path.slope, false, false); // Change the neighbouring paths too.

		Viewport *vp = GetViewport();
		if (vp) vp->MarkVoxelDirty(this->xpos, this->ypos, this->zpos);

		/* Short-cut version of this->SelectMovement(false), as that function fails after removing the path. */
		TileEdge edge = (TileEdge)((this->selected_arrow + 2) % 4);
		bool move_up = (svd->path.slope == _path_down_from_edge[edge]);
		this->MoveCursor(edge, move_up);
		this->UpdateState();
	}
}

