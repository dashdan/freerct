/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file widget.cpp Widget code. */

#include "stdafx.h"
#include "math_func.h"
#include "sprite_store.h"
#include "widget.h"
#include "window.h"
#include "video.h"

/**
 * @defgroup widget_group Widgets and supporting functions of the program
 */

/**
 * @defgroup widget_parts_group %Widget parts for specifying windows
 * @ingroup widget_group
 */

/**
 * Base class widget constructor.
 * @param wtype %Widget type.
 */
BaseWidget::BaseWidget(WidgetType wtype)
{
	this->wtype = wtype;
	this->number = INVALID_WIDGET_INDEX;

	this->min_x = 0;
	this->min_y = 0;
	this->pos.base.x = 0;
	this->pos.base.y = 0;
	this->pos.width  = 0;
	this->pos.height = 0;

	this->fill_x = 0;
	this->fill_y = 0;
	this->resize_x = 0;
	this->resize_y = 0;
	for (int i = 0; i < PAD_COUNT; i++) this->paddings[i] = 0;

	switch (wtype) {
		case WT_TITLEBAR:
			this->fill_x = 1;
			this->resize_x = 1;
			break;

		case WT_LEFT_TEXT:
		case WT_CENTERED_TEXT:
		case WT_RIGHT_TEXT:
			this->fill_x = 1;
			break;

		case WT_TEXT_BUTTON:
		case WT_TEXT_PUSHBUTTON:
			this->fill_x = 1;
			this->fill_y = 1;
			break;

		case WT_VERT_SCROLLBAR:
			this->fill_y = 1;
			this->resize_y = 1;
			break;

		case WT_HOR_SCROLLBAR:
			this->fill_x = 1;
			this->resize_x = 1;
			break;

		default: break; // Default: Leave all zero.
	}
}

BaseWidget::~BaseWidget()
{
}

/**
 * Initialize the minimal size of the widget based on the width and height of the content, and the necessary border space.
 * @param content_width Minimal width of the content.
 * @param content_height Minimal height of the content.
 * @param border_hor Horizontal border space.
 * @param border_vert Vertical border space.
 */
void BaseWidget::InitMinimalSize(uint16 content_width, uint16 content_height, uint16 border_hor, uint16 border_vert)
{
	this->min_x = max(this->min_x, (uint16)(content_width  + border_hor  + this->paddings[PAD_LEFT] + this->paddings[PAD_RIGHT]));
	this->min_y = max(this->min_y, (uint16)(content_height + border_vert + this->paddings[PAD_TOP] + this->paddings[PAD_BOTTOM]));
}

/**
 * Initialize the minimal size of the widget based on the width and height of the content, and the border sprites.
 * @param bsd Used border sprites.
 * @param content_width Minimal width of the content.
 * @param content_height Minimal height of the content.
 */
void BaseWidget::InitMinimalSize(const BorderSpriteData *bsd, uint16 content_width, uint16 content_height)
{
	content_width = max(content_width, bsd->min_width);
	if (bsd->hor_stepsize > 0) content_width = bsd->min_width + (content_width - bsd->min_width + bsd->hor_stepsize - 1) / bsd->hor_stepsize;

	content_height = max(content_height, bsd->min_height);
	if (bsd->vert_stepsize > 0) content_height = bsd->min_height + (content_height - bsd->min_height + bsd->vert_stepsize - 1) / bsd->vert_stepsize;

	this->InitMinimalSize(content_width, content_height,  bsd->border_left + bsd->border_right, bsd->border_top + bsd->border_bottom);
	this->fill_x = LeastCommonMultiple(this->fill_x, bsd->hor_stepsize);
	this->fill_y = LeastCommonMultiple(this->fill_y, bsd->vert_stepsize);
	this->resize_x = LeastCommonMultiple(this->resize_x, bsd->hor_stepsize);
	this->resize_y = LeastCommonMultiple(this->resize_y, bsd->vert_stepsize);
}

/**
 * Add the widget to the widget array \a wid_array.
 * @param wid_array The widget array.
 */
void BaseWidget::SetWidget(BaseWidget **wid_array)
{
	if (this->number < 0) return;
	assert(wid_array[this->number] == NULL);
	wid_array[this->number] = this;
}

/**
 * Set up minimal size (#min_x and #min_y), fill step (#fill_x and #fill_y) and resize step (#resize_x and #resize_y) of the widget.
 * In addition, if the widget has a non-negative widget number, add the widget to the \a wid_array after verifying the position is still empty.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
/* virtual */ void BaseWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	this->SetWidget(wid_array);

	switch (this->wtype) {
		case WT_EMPTY:
			/* Do nothing (all variables are already set while converting from widget parts). */
			break;

		case WT_RESIZEBOX:
			this->min_x = max((uint16)10, this->min_x);
			this->min_y = max((uint16)10, this->min_y);
			this->fill_x = 0;
			this->fill_y = 1;
			this->resize_x = 0;
			this->resize_y = 0;
			break;

		default:
			NOT_REACHED();
	}

	if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
}

/**
 * Set the minimal size of the widget, and assign \a rect to the current position and size.
 * @param rect Smallest size, and suggested position of the (entire) widget.
 */
/* virtual */ void BaseWidget::SetSmallestSizePosition(const Rectangle16 &rect)
{
	this->pos = rect;
	this->min_x = rect.width;
	this->min_y = rect.height;
}

/**
 * Draw the widget.
 * @param w %Window being drawn.
 */
/* virtual */ void BaseWidget::Draw(const GuiWindow *w)
{
	/* Nothing to do for WT_EMPTY */
	if (this->number != INVALID_WIDGET_INDEX) w->DrawWidget(this->number, this);
}

/**
 * Get the widget at the given relative window position.
 * @param pt Relative point.
 * @return The widget underneath the point, or \c NULL.
 */
/* virtual */ BaseWidget *BaseWidget::GetWidgetByPosition(const Point16 &pt)
{
	if (this->pos.IsPointInside(pt)) return this;
	return NULL;
}

/**
 * Raise all push buttons in the tree.
 * @param base Base position of the window.
 */
/* virtual */ void BaseWidget::AutoRaiseButtons(const Point32 &base)
{
}

/**
 * Denote the widget as being needed to redraw.
 * @param base %Window base coordinate.
 */
void BaseWidget::MarkDirty(const Point32 &base)
{
	Rectangle32 rect = Rectangle32(base.x + this->pos.base.x, base.y + this->pos.base.y, this->pos.width, this->pos.height);
	_video->MarkDisplayDirty(rect);
}

/**
 * Base class leaf widget constructor.
 * @param wtype %Widget type.
 */
LeafWidget::LeafWidget(WidgetType wtype) : BaseWidget(wtype)
{
	this->flags = 0;
	this->colour = 0;
	this->tooltip = STR_NULL;
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 * @todo Add support for #WT_RESIZEBOX.
 */
/* virtual */ void LeafWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	this->SetWidget(wid_array);

	switch (this->wtype) {
		case WT_CLOSEBOX: {
			const ImageData *im = _gui_sprites.close_sprite;
			this->InitMinimalSize(&_gui_sprites.panel, im->width + 2, im->height + 2);

			this->fill_x = 0;
			this->fill_y = 1;
			this->resize_x = 0;
			this->resize_y = 0;
			break;
		}

		case WT_RADIOBUTTON: {
			const CheckableWidgetSpriteData cwsd = _gui_sprites.radio_button;
			this->InitMinimalSize(cwsd.width, cwsd.height, 0, 0);

			this->fill_x = 0;
			this->fill_y = 0;
			this->resize_x = 0;
			this->resize_y = 0;
			break;
		}

		default:
			NOT_REACHED();
	}
}

/* virtual */ void LeafWidget::Draw(const GuiWindow *w)
{
	assert(this->wtype != WT_RESIZEBOX);

	int left   = w->GetWidgetScreenX(this) + this->paddings[PAD_LEFT];
	int top    = w->GetWidgetScreenY(this) + this->paddings[PAD_TOP];
	int right  = w->GetWidgetScreenX(this) + this->pos.width - 1 - this->paddings[PAD_RIGHT];
	int bottom = w->GetWidgetScreenY(this) + this->pos.height - 1 - this->paddings[PAD_BOTTOM];

	static Recolouring rc;
	rc.SetRecolouring(COL_RANGE_BEIGE, (ColourRange)this->colour);

	if (this->wtype == WT_RADIOBUTTON) {
		int spr_num = ((this->flags & LWF_CHECKED) != 0) ? WCS_CHECKED : WCS_EMPTY;
		if ((this->flags & LWF_SHADED) != 0) {
			spr_num += WCS_SHADED_EMPTY;
		} else if ((this->flags & LWF_PRESSED) != 0) {
			spr_num += WCS_EMPTY_PRESSED;
		}
		_video->BlitImage(left, top, _gui_sprites.radio_button.sprites[spr_num], rc, 0);
		return;
	}
	assert(this->wtype == WT_CLOSEBOX);

	const BorderSpriteData &bsd = _gui_sprites.panel;
	left += bsd.border_left;
	top += bsd.border_top;
	right -= bsd.border_right;
	bottom -= bsd.border_bottom;
	assert(right - left + 1 >= 0);
	assert(bottom - top + 1 >= 0);

	Rectangle32 rect(left, top, right - left + 1, bottom - top + 1);
	DrawBorderSprites(bsd, false, rect, COL_BACKGROUND);

	int xoffset = left + (right - 1 - left - _gui_sprites.close_sprite->width) / 2;
	int yoffset = top + (bottom - 1 - top - _gui_sprites.close_sprite->height) / 2;

	const ImageData *imgdata = _gui_sprites.close_sprite;
	if (imgdata != NULL) _video->BlitImage(xoffset + 1, yoffset + 1, imgdata, rc, 0);
}

/* virtual */ void LeafWidget::AutoRaiseButtons(const Point32 &base)
{
	if ((this->wtype == WT_TEXT_PUSHBUTTON || this->wtype == WT_IMAGE_PUSHBUTTON) && this->IsPressed()) {
		this->SetPressed(false);
		this->MarkDirty(base);
	}
}

/**
 * Data widget constructor.
 * @param wtype %Widget type.
 */
DataWidget::DataWidget(WidgetType wtype) : LeafWidget(wtype)
{
	this->value = 0;
	this->value_width = 0;
	this->value_height = 0;
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
/* virtual */ void DataWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	this->SetWidget(wid_array);

	const BorderSpriteData *bsd = NULL;
	uint8 pressable = 0; // Add extra space for a pressable widget.
	switch (this->wtype) {
		case WT_TITLEBAR:
			bsd = &_gui_sprites.titlebar;
			break;

		case WT_LEFT_TEXT:
		case WT_CENTERED_TEXT:
		case WT_RIGHT_TEXT:
			break;

		case WT_TEXT_BUTTON:
		case WT_TEXT_PUSHBUTTON:
			bsd = &_gui_sprites.button;
			pressable = 1;
			break;

		case WT_IMAGE_BUTTON:
		case WT_IMAGE_PUSHBUTTON: {
			const Rectangle16 &rect = _sprite_manager.GetTableSpriteSize(this->value);
			this->value_width = rect.width;
			this->value_height = rect.height;
			this->InitMinimalSize(&_gui_sprites.button, this->value_width + 1, this->value_height + 1);
			if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
			return;
		}

		default:
			NOT_REACHED();
	}

	if (this->number >= 0) w->SetWidgetStringParameters(this->number);
	GetTextSize(w->TranslateStringNumber(this->value), &this->value_width, &this->value_height);
	if (bsd != NULL) {
		this->InitMinimalSize(bsd, this->value_width + pressable, this->value_height + pressable);
	} else {
		this->InitMinimalSize(this->value_width + pressable, this->value_height + pressable, 0, 0);
	}

	if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
}

/**
 * Draw a #DataWidget.
 * @param w Window that the widget belongs to.
 * @todo Fix the hardcoded colour of the text.
 */
/* virtual */ void DataWidget::Draw(const GuiWindow *w)
{
	const BorderSpriteData *bsd = NULL;
	uint8 pressed = 0;
	switch (this->wtype) {
		case WT_TITLEBAR:
			bsd = &_gui_sprites.titlebar;
			break;

		case WT_LEFT_TEXT:
		case WT_CENTERED_TEXT:
		case WT_RIGHT_TEXT:
			break;

		case WT_TEXT_BUTTON:
		case WT_IMAGE_BUTTON:
		case WT_TEXT_PUSHBUTTON:
		case WT_IMAGE_PUSHBUTTON:
			bsd = &_gui_sprites.button;
			pressed = this->IsPressed() ? 1 : 0;
			break;

		default:
			NOT_REACHED();
	}
	int left   = w->GetWidgetScreenX(this) + this->paddings[PAD_LEFT];
	int top    = w->GetWidgetScreenY(this) + this->paddings[PAD_TOP];
	int right  = w->GetWidgetScreenX(this) + this->pos.width  - 1 - this->paddings[PAD_RIGHT];
	int bottom = w->GetWidgetScreenY(this) + this->pos.height - 1 - this->paddings[PAD_BOTTOM];
	if (bsd != NULL) {
		left += bsd->border_left;
		top += bsd->border_top;
		right -= bsd->border_right;
		bottom -= bsd->border_bottom;
		assert(right - left + 1 >= 0);
		assert(bottom - top + 1 >= 0);

		Rectangle32 rect(left, top, right - left + 1, bottom - top + 1);
		DrawBorderSprites(*bsd, (pressed != 0), rect, this->colour);
	}
	int xoffset;
	if (this->wtype == WT_LEFT_TEXT) {
		xoffset = left;
	} else if (this->wtype == WT_RIGHT_TEXT) {
		xoffset = left + (right + 1 - left - this->value_width);
	} else {
		xoffset = left + (right + 1 - left - this->value_width) / 2;
	}
	int yoffset = top + (bottom + 1 - top - this->value_height) / 2;
	if (this->wtype == WT_IMAGE_BUTTON || this->wtype == WT_IMAGE_PUSHBUTTON) {
		const Rectangle16 rect = _sprite_manager.GetTableSpriteSize(this->value);
		xoffset -= rect.base.x;
		yoffset -= rect.base.y;
		const ImageData *imgdata = _sprite_manager.GetTableSprite(this->value);
		if (imgdata != NULL) {
			static Recolouring rc; // Never modified
			_video->BlitImage(xoffset + pressed, yoffset + pressed, imgdata, rc, 0);
		}
	} else {
		if (this->number >= 0) w->SetWidgetStringParameters(this->number);
		DrawString(w->TranslateStringNumber(this->value), TEXT_WHITE, xoffset + pressed, yoffset + pressed);
	}
}

/**
 * Scrollbar widget constructor.
 * @param wtype %Widget type.
 */
ScrollbarWidget::ScrollbarWidget(WidgetType wtype) : LeafWidget(wtype)
{
	this->canvas_widget = canvas_widget;
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
/* virtual */ void ScrollbarWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	this->SetWidget(wid_array);

	if (this->wtype == WT_HOR_SCROLLBAR) {
		this->min_x = _gui_sprites.hor_scroll.min_length_all;
		this->min_y = _gui_sprites.hor_scroll.height;
		this->fill_x = _gui_sprites.hor_scroll.stepsize_bar;
		this->fill_y = 0;
		this->resize_x = _gui_sprites.hor_scroll.stepsize_bar;
		this->resize_y = 0;
	} else {
		this->min_x = _gui_sprites.vert_scroll.height;
		this->min_y = _gui_sprites.vert_scroll.min_length_all;
		this->fill_x = 0;
		this->fill_y = _gui_sprites.vert_scroll.stepsize_bar;
		this->resize_x = 0;
		this->resize_y = _gui_sprites.vert_scroll.stepsize_bar;
	}
}

/* virtual */ void ScrollbarWidget::Draw(const GuiWindow *w)
{
	static Recolouring rc; // Only COL_RANGE_BEIGE is modified each time.
	rc.SetRecolouring(COL_RANGE_BEIGE, (ColourRange)colour);

	const ScrollbarSpriteData &scroll_sprites = (this->wtype == WT_HOR_SCROLLBAR) ? _gui_sprites.hor_scroll : _gui_sprites.vert_scroll;
	if (!scroll_sprites.IsLoaded()) return;

	const ImageData * const *imd = this->IsShaded() ? scroll_sprites.shaded : scroll_sprites.normal;

	Point32 pos;
	pos.x = w->GetWidgetScreenX(this);
	pos.y = w->GetWidgetScreenY(this);

	/* Draw left/up button. */
	_video->BlitImage(pos, imd[WLS_LEFT_BUTTON], rc, 0);
	if (this->wtype == WT_HOR_SCROLLBAR) pos.x += imd[WLS_LEFT_BUTTON]->width;
	if (this->wtype != WT_HOR_SCROLLBAR) pos.y += imd[WLS_LEFT_BUTTON]->height;

	/* Draw top/left underground. */
	_video->BlitImage(pos, imd[WLS_LEFT_BED], rc, 0);
	if (this->wtype == WT_HOR_SCROLLBAR) pos.x += imd[WLS_LEFT_BED]->width;
	if (this->wtype != WT_HOR_SCROLLBAR) pos.y += imd[WLS_LEFT_BED]->height;

	/* Draw middle underground. */
	if (this->wtype == WT_HOR_SCROLLBAR) {
		int others = imd[WLS_LEFT_BUTTON]->width + imd[WLS_LEFT_BED]->width + imd[WLS_RIGHT_BED]->width + imd[WLS_RIGHT_BUTTON]->width;
		uint count = (others < this->pos.width)
				? (this->pos.width - others) / scroll_sprites.stepsize_bar : 0;
		_video->BlitHorizontal(pos.x, count, pos.y, imd[WLS_MIDDLE_BED], rc);
		pos.x += count * scroll_sprites.stepsize_bar;
	} else {
		int others = imd[WLS_LEFT_BUTTON]->height + imd[WLS_LEFT_BED]->height + imd[WLS_RIGHT_BED]->height + imd[WLS_RIGHT_BUTTON]->height;
		uint count = (others < this->pos.height)
				? (this->pos.height - others) / scroll_sprites.stepsize_bar : 0;
		_video->BlitVertical(pos.y, count, pos.x, imd[WLS_MIDDLE_BED], rc);
		pos.y += count * scroll_sprites.stepsize_bar;
	}

	/* Draw bottom/right underground. */
	_video->BlitImage(pos, imd[WLS_RIGHT_BED], rc, 0);
	if (this->wtype == WT_HOR_SCROLLBAR) pos.x += imd[WLS_RIGHT_BED]->width;
	if (this->wtype != WT_HOR_SCROLLBAR) pos.y += imd[WLS_RIGHT_BED]->height;

	/* Draw right/bottom button. */
	_video->BlitImage(pos, imd[WLS_RIGHT_BUTTON], rc, 0);
}

/**
 * Baseclass background widget constructor.
 * @param wtype %Widget type.
 */
BackgroundWidget::BackgroundWidget(WidgetType wtype) : LeafWidget(wtype)
{
	this->child = NULL;
}

BackgroundWidget::~BackgroundWidget()
{
	if (this->child != NULL) delete this->child;
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
/* virtual */ void BackgroundWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	this->SetWidget(wid_array);

	if (this->child != NULL) {
		this->child->SetupMinimalSize(w, wid_array);
		this->min_x = this->child->min_x;
		this->min_y = this->child->min_y;
		this->fill_x = this->child->fill_x;
		this->fill_y = this->child->fill_y;
		this->resize_x = this->child->resize_x;
		this->resize_y = this->child->resize_y;
	}
	this->InitMinimalSize(&_gui_sprites.panel, this->min_x, this->min_y);
}

/* virtual */ void BackgroundWidget::SetSmallestSizePosition(const Rectangle16 &rect)
{
	this->pos = rect;
	this->min_x = rect.width;
	this->min_y = rect.height;

	if (this->child != NULL) {
		uint16 left = rect.base.x;
		uint16 right = left + rect.width; // One pixel further than right, actually.
		left += this->paddings[PAD_LEFT] + _gui_sprites.panel.border_left;
		right -= this->paddings[PAD_RIGHT] + _gui_sprites.panel.border_right;
		if (right < left) right = left;

		uint16 top = rect.base.y;
		uint16 bottom = top + rect.height; // One pixel below the bottom, actually.
		top += this->paddings[PAD_TOP] + _gui_sprites.panel.border_top;
		bottom -= this->paddings[PAD_BOTTOM] + _gui_sprites.panel.border_bottom;
		if (bottom < top) bottom = top;

		Rectangle16 rect_child(left, top, right - left, bottom - top);
		this->child->SetSmallestSizePosition(rect_child);
	}
}

/* virtual */ void BackgroundWidget::Draw(const GuiWindow *w)
{
	int left   = w->GetWidgetScreenX(this) + this->paddings[PAD_LEFT];
	int top    = w->GetWidgetScreenY(this) + this->paddings[PAD_TOP];
	int right  = w->GetWidgetScreenX(this) + this->pos.width  - 1 - this->paddings[PAD_RIGHT];
	int bottom = w->GetWidgetScreenY(this) + this->pos.height - 1 - this->paddings[PAD_BOTTOM];
	left += _gui_sprites.panel.border_left;
	top += _gui_sprites.panel.border_top;
	right -= _gui_sprites.panel.border_right;
	bottom -= _gui_sprites.panel.border_bottom;
	assert(right - left + 1 >= 0);
	assert(bottom - top + 1 >= 0);

	Rectangle32 rect(left, top, right - left + 1, bottom - top + 1);
	DrawBorderSprites(_gui_sprites.panel, false, rect, this->colour);

	if (this->number != INVALID_WIDGET_INDEX) w->DrawWidget(this->number, this);
	if (this->child != NULL) this->child->Draw(w);
}

/* virtual */ BaseWidget *BackgroundWidget::GetWidgetByPosition(const Point16 &pt)
{
	if (this->pos.IsPointInside(pt)) {
		if (this->child != NULL) {
			BaseWidget *res = this->child->GetWidgetByPosition(pt);
			if (res != NULL) return res;
		}
		return this;
	}
	return NULL;
}

/* virtual */ void BackgroundWidget::AutoRaiseButtons(const Point32 &base)
{
	if (this->child != NULL) this->child->AutoRaiseButtons(base);
}

/** Initialize the row/column data. */
void RowColData::InitRowColData()
{
	this->min_size = 0;
	this->fill = 1;
	this->resize = 1;

}

/**
 * Merge a new minimal size, fill step, and resize step into the data.
 * @param min_size Minimal size to merge.
 * @param fill Fill step to merge.
 * @param resize Resize step to merge.
 */
void RowColData::Merge(uint16 min_size, uint16 fill, uint16 resize)
{
	this->min_size = max(this->min_size, min_size);
	this->fill = LeastCommonMultiple(this->fill, fill);
	this->resize = LeastCommonMultiple(this->resize, resize);
}


/**
 * Constructor for intermediate widgets.
 * @param num_rows Number of rows. Use \c 0 for 'manual' claiming.
 * @param num_cols Number of columns. Use \c 0 for 'manual' claiming.
 * @see FillWidget
 */
IntermediateWidget::IntermediateWidget(uint8 num_rows, uint8 num_cols) : BaseWidget(WT_GRID)
{
	this->childs = NULL;
	this->rows = NULL;
	this->columns = NULL;
	this->num_rows = num_rows;
	this->num_cols = num_cols;
	this->flags = 0;

	if (this->num_cols > 0 && this->num_rows > 0) this->ClaimMemory();
}

/**
 * Claim memory for childs 'manually'.
 * @pre Memory must not have been claimed, and #num_cols and #num_rows must be bigger than \c 0.
 */
void IntermediateWidget::ClaimMemory()
{
	assert(this->num_cols > 0 && this->num_rows > 0);
	assert(this->childs == NULL);

	this->childs = (BaseWidget **)malloc(num_rows * num_cols * sizeof(BaseWidget **));
	assert(this->childs != NULL);
	for (uint16 idx = 0; idx < (uint16)this->num_rows * this->num_cols; idx++) {
		this->childs[idx] = NULL;
	}

	this->rows = (RowColData *)malloc(this->num_rows * sizeof(RowColData));
	assert(this->rows != NULL);

	this->columns = (RowColData *)malloc(this->num_cols * sizeof(RowColData));
	assert(this->columns != NULL);
}

IntermediateWidget::~IntermediateWidget()
{
	if (this->childs != NULL) {
		for (uint16 idx = 0; idx < (uint16)this->num_rows * this->num_cols; idx++) {
			delete this->childs[idx];
		}
		free(this->childs);
	}
	free(this->rows);
	free(this->columns);
}

/**
 * Add a child widget.
 * @param x Horizontal index of the child in the grid.
 * @param y Vertical index of the child in the grid.
 * @param w Child widget to add.
 */
void IntermediateWidget::AddChild(uint8 x, uint8 y, BaseWidget *w)
{
	assert(x < this->num_cols && y < this->num_rows);
	assert(this->childs[y * (uint16)this->num_cols + x] == NULL);
	this->childs[y * (uint16)this->num_cols + x] = w;
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
/* virtual */ void IntermediateWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	this->SetWidget(wid_array);

	/* Step 1: Initialize rows and columns. */
	for (uint8 y = 0; y < this->num_rows; y++) {
		this->rows[y].InitRowColData();
	}
	for (uint8 x = 0; x < this->num_cols; x++) {
		this->columns[x].InitRowColData();
	}

	/* Step 2: Process child widgets. */
	for (uint8 y = 0; y < this->num_rows; y++) {
		for (uint8 x = 0; x < this->num_cols; x++) {
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x];
			bw->SetupMinimalSize(w, wid_array);
			this->rows[y].Merge(bw->min_y, bw->fill_y, bw->resize_y);
			this->columns[x].Merge(bw->min_x, bw->fill_x, bw->resize_x);
		}
	}

	/* Step 3a: Handle vertical equal size. */
	uint16 max_minsize = 0;
	if ((this->flags & EQS_VERTICAL) != 0) { // Equal sizes vertically requested, do a pre-size computation.
		for (uint8 y = 0; y < this->num_rows; y++) {
			max_minsize = max(max_minsize, this->rows[y].min_size);
		}
		for (uint8 y = 0; y < this->num_rows; y++) {
			if (this->rows[y].fill > 0) {
				uint16 diff = max_minsize - this->rows[y].min_size;
				this->rows[y].min_size += diff - diff % this->rows[y].fill;
			}
		}
	}

	uint16 child_tmp_minsize[10]; // Temporary array for minimal size of children. Size is arbitrary.
	assert(this->num_rows < lengthof(child_tmp_minsize));
	assert(this->num_cols < lengthof(child_tmp_minsize));

	/* Step 3b: Make sure we derive a minimal vertical size that is actually feasible for the child widgets. */
	for (uint8 y = 0; y < this->num_rows; y++) {
		/* Initialize the child_tmp_minsize array with current minsize of each child widget in the row. */
		for (uint8 x = 0; x < this->num_cols; x++) {
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x];
			child_tmp_minsize[x] = bw->min_y;
		}
		/* Try to find a consistent minimal vertical size for all children. Due to a fill size > 1, this may be
		 * bigger than the current minimal vertical size.
		 */
		uint16 cur_minsize = this->rows[y].min_size; // the target we are aiming for currently.
		bool consistent = false;
		/* No overflow, and not insanely bigger (upper limit is arbitrary). */
		while (cur_minsize >= this->rows[y].min_size && this->rows[y].min_size + 50 > cur_minsize) {
			consistent = true;
			for (uint8 x = 0; x < this->num_cols; x++) {
				if (child_tmp_minsize[x] == cur_minsize) continue;

				BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x];
				if (bw->fill_y <= 1) continue; // 0 does not play, 1 will always work

				/* Increment minimal size if y fill steps. */
				while (child_tmp_minsize[x] < cur_minsize) child_tmp_minsize[x] += bw->fill_y;
				consistent &= (child_tmp_minsize[x] == cur_minsize);
				assert(cur_minsize <= child_tmp_minsize[x]);
				cur_minsize = child_tmp_minsize[x]; // May cause increasing of cur_minsize.
			}
			if (consistent) break;
		}
		if (consistent) this->rows[y].min_size = cur_minsize;
	}

	/* Step 3c: Compute vertical fields in rows[y]. */
	this->min_y = this->paddings[PAD_BOTTOM];
	this->fill_y = 0;
	this->resize_y = 0;
	for (uint8 y = 0; y < this->num_rows; y++) {
		this->min_y += ((y == 0) ? this->paddings[PAD_TOP] : this->paddings[PAD_VERTICAL]) + this->rows[y].min_size;
		if (this->rows[y].fill > 0 && (this->fill_y == 0 || this->fill_y > this->rows[y].fill)) this->fill_y = this->rows[y].fill;
		if (this->rows[y].resize > 0 && (this->resize_y == 0 || this->resize_y > this->rows[y].resize)) {
			this->resize_y = this->rows[y].resize;
		}
	}

	/* Step 4a: Handle horizontal equal size. */
	max_minsize = 0;
	if ((this->flags & EQS_HORIZONTAL) != 0) { // Equal sizes vertically requested, do a pre-size computation.
		for (uint8 x = 0; x < this->num_cols; x++) {
			max_minsize = max(max_minsize, this->columns[x].min_size);
		}
		for (uint8 x = 0; x < this->num_cols; x++) {
			if (this->columns[x].fill > 0) {
				uint16 diff = max_minsize - this->columns[x].min_size;
				this->columns[x].min_size += diff - diff % this->columns[x].fill;
			}
		}
	}

	/* Step 4b: Make sure we derive a minimal horizontal size that is actually feasible for the child widgets. */
	for (uint8 x = 0; x < this->num_cols; x++) {
		/* Initialize the child_tmp_minsize array with current minsize of each child widget in the column. */
		for (uint8 y = 0; y < this->num_rows; y++) {
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x];
			child_tmp_minsize[y] = bw->min_x;
		}
		/*  Try to find a consistent minimal horizontal size for all children. Due to a fill size > 1, this may be
		 * bigger than the current minimal horizontal size.
		 */
		uint16 cur_minsize = this->columns[x].min_size; // the target we are aiming for currently.
		bool consistent = false;
		/* No overflow, and not insanely bigger (upper limit is arbitrary). */
		while (cur_minsize >= this->columns[x].min_size && this->columns[x].min_size + 50 > cur_minsize) {
			consistent = true;
			for (uint8 y = 0; y < this->num_rows; y++) {
				if (child_tmp_minsize[y] == cur_minsize) continue;

				BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x];
				if (bw->fill_x <= 1) continue; // 0 does not play, 1 will always work

				/* Increment minimal size if y fill steps. */
				while (child_tmp_minsize[y] < cur_minsize) child_tmp_minsize[y] += bw->fill_x;
				consistent &= (child_tmp_minsize[y] == cur_minsize);
				assert(cur_minsize <= child_tmp_minsize[y]);
				cur_minsize = child_tmp_minsize[y]; // May cause increasing of cur_minsize.
			}
			if (consistent) break;
		}
		if (consistent) this->columns[x].min_size = cur_minsize;
	}

	/* Step 4c: Compute horizontal fields in columns[x]. */
	this->min_x = this->paddings[PAD_RIGHT];
	this->fill_x = 0;
	this->resize_x = 0;
	for (uint8 x = 0; x < this->num_cols; x++) {
		this->min_x += ((x == 0) ? this->paddings[PAD_LEFT] : this->paddings[PAD_HORIZONTAL]) + this->columns[x].min_size;
		if (this->columns[x].fill > 0 && (this->fill_x == 0 || this->fill_x > this->columns[x].fill)) this->fill_x = this->columns[x].fill;
		if (this->columns[x].resize > 0 && (this->resize_x == 0 || this->resize_x > this->columns[x].resize)) {
			this->resize_x = this->columns[x].resize;
		}
	}
}

/**
 * Set the smallest size of the intermediate widget (and all widgets under it).
 * @param rect Smallest size to set.
 * @todo Handle RTL languages too.
 */
/* virtual */ void IntermediateWidget::SetSmallestSizePosition(const Rectangle16 &rect)
{
	this->pos = rect;

	/* Distribute additional vertical size over fillable children. */
	uint16 diff = this->paddings[PAD_BOTTOM];
	uint8 count = 0;
	uint16 max_step = 0;
	for (uint8 y = 0; y < this->num_rows; y++) {
		diff += ((y == 0) ? this->paddings[PAD_TOP] : this->paddings[PAD_VERTICAL]) + this->rows[y].min_size;
		if (this->rows[y].fill > 0) {
			if (count == 0 || this->rows[y].fill > max_step) max_step = this->rows[y].fill;
			count++;
		}
	}
	diff = (diff < rect.height) ? rect.height - diff : 0;

	while (diff > 0 && count > 0) {
		uint16 new_max = 0;
		for (uint8 y = 0; y < this->num_rows; y++) {
			if (this->rows[y].fill == 0 || this->rows[y].fill > max_step) continue;
			if (this->rows[y].fill == max_step) {
				uint16 increment = diff / count;
				increment -= increment % max_step;
				this->rows[y].min_size += increment;
				diff -= increment;
				count--;
				continue;
			}
			new_max = max(new_max, this->rows[y].fill);
		}
		max_step = new_max;
	}

	/* Distribute additional horizontal size over fillable children. */
	diff = this->paddings[PAD_RIGHT];
	count = 0;
	max_step = 0;
	for (uint8 x = 0; x < this->num_cols; x++) {
		diff += ((x == 0) ? this->paddings[PAD_LEFT] : this->paddings[PAD_HORIZONTAL]) + this->columns[x].min_size;
		if (this->columns[x].fill > 0) {
			if (count == 0 || this->columns[x].fill > max_step) max_step = this->columns[x].fill;
			count++;
		}
	}
	diff = (diff < rect.width) ? rect.width - diff : 0;

	while (diff > 0 && count > 0) {
		uint16 new_max = 0;
		for (uint8 x = 0; x < this->num_cols; x++) {
			if (this->columns[x].fill == 0 || this->columns[x].fill > max_step) continue;
			if (this->columns[x].fill == max_step) {
				uint16 increment = diff / count;
				increment -= increment % max_step;
				this->columns[x].min_size += increment;
				diff -= increment;
				count--;
				continue;
			}
			new_max = max(new_max, this->columns[x].fill);
		}
		max_step = new_max;
	}

	/* Tell the children about the allocated sizes. */
	uint16 top = rect.base.y;
	for (uint8 y = 0; y < this->num_rows; y++) {
		top += (y == 0) ? this->paddings[PAD_TOP] : this->paddings[PAD_VERTICAL];
		uint16 left = rect.base.x;
		for (uint8 x = 0; x < this->num_cols; x++) {
			left += (x == 0) ? this->paddings[PAD_LEFT] : this->paddings[PAD_HORIZONTAL];
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x];
			Rectangle16 rect2(left, top, this->columns[x].min_size, this->rows[y].min_size);
			bw->SetSmallestSizePosition(rect2);

			left += this->columns[x].min_size;
		}
		top += this->rows[y].min_size;
	}
}

/* virtual */ void IntermediateWidget::Draw(const GuiWindow *w)
{
	for (uint16 idx = 0; idx < (uint16)this->num_rows * this->num_cols; idx++) {
		this->childs[idx]->Draw(w);
	}
}

/* virtual */ BaseWidget *IntermediateWidget::GetWidgetByPosition(const Point16 &pt)
{
	BaseWidget *res = NULL;
	if (this->pos.IsPointInside(pt)) {
		for (uint16 idx = 0; idx < (uint16)this->num_rows * this->num_cols; idx++) {
			res = this->childs[idx]->GetWidgetByPosition(pt);
			if (res != NULL) break;
		}
	}
	return res;
}

/* virtual */ void IntermediateWidget::AutoRaiseButtons(const Point32 &base)
{
	for (uint16 idx = 0; idx < (uint16)this->num_rows * this->num_cols; idx++) {
		this->childs[idx]->AutoRaiseButtons(base);
	}
}


/**
 * Create a new #WT_GRID widget.
 * @param num_rows Number of rows.
 * @param num_cols Number of columns.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart Intermediate(uint8 num_rows, uint8 num_cols)
{
	WidgetPart part;

	part.type = WPT_NEW_INTERMEDIATE;
	part.data.new_intermediate.num_rows = num_rows;
	part.data.new_intermediate.num_cols = num_cols;
	return part;
}

/**
 * Create a new widget.
 * @param wtype Widget type.
 * @param number Widget number.
 * @param colour Widget colour.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart Widget(WidgetType wtype, WidgetNumber number, uint8 colour)
{
	WidgetPart part;

	part.type = WPT_NEW_WIDGET;
	part.data.new_widget.wtype = wtype;
	part.data.new_widget.number = number;
	part.data.new_widget.colour = colour;
	return part;
}

/**
 * Set minimal size.
 * @param x Horizontal minimal size.
 * @param y Vertical minimal size.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetMinimalSize(int16 x, int16 y)
{
	WidgetPart part;

	assert(x > 0 && y > 0);
	part.type = WPT_MIN_SIZE;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set fill step.
 * @param x Horizontal fill step.
 * @param y Vertical fill step.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetFill(uint8 x, uint8 y)
{
	WidgetPart part;

	part.type = WPT_FILL;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set resize step.
 * @param x Horizontal resize step.
 * @param y Vertical resize step.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetResize(uint8 x, uint8 y)
{
	WidgetPart part;

	part.type = WPT_RESIZE;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set padding around a widget (start upwards, and then around with the clock).
 * @param top Padding at the top.
 * @param right Padding at the right.
 * @param bottom Padding at the bottom.
 * @param left Padding at the left.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetPadding(uint8 top, uint8 right, uint8 bottom, uint8 left)
{
	WidgetPart part;

	part.type = WPT_PADDING;
	for (int i = 0; i < PAD_COUNT; i++) part.data.padding[i] = 0;
	part.data.padding[PAD_TOP] = top;
	part.data.padding[PAD_LEFT] = left;
	part.data.padding[PAD_RIGHT] = right;
	part.data.padding[PAD_BOTTOM] = bottom;
	return part;
}

/**
 * Set padding of a horizontal bar.
 * @param pre Padding at the left.
 * @param inter Vertical padding between elements.
 * @param post Padding at the right.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetHorPIP(uint8 pre, uint8 inter, uint8 post)
{
	WidgetPart part;

	part.type = WPT_HOR_PIP;
	for (int i = 0; i < PAD_COUNT; i++) part.data.padding[i] = 0;
	part.data.padding[PAD_LEFT] = pre;
	part.data.padding[PAD_RIGHT] = post;
	part.data.padding[PAD_VERTICAL] = inter;
	return part;
}

/**
 * Set padding of a vertical bar.
 * @param pre Padding at the top.
 * @param inter Horizontal padding between elements.
 * @param post Padding at the bottom.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetVertPIP(uint8 pre, uint8 inter, uint8 post)
{
	WidgetPart part;

	part.type = WPT_VERT_PIP;
	for (int i = 0; i < PAD_COUNT; i++) part.data.padding[i] = 0;
	part.data.padding[PAD_TOP] = pre;
	part.data.padding[PAD_BOTTOM] = post;
	part.data.padding[PAD_HORIZONTAL] = inter;
	return part;
}

/**
 * Set data and tool tip of the widget.
 * @param value Additional data of the widget (string or sprite id).
 * @param tip Tool tip text.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetData(uint16 value, uint16 tip)
{
	WidgetPart part;

	part.type = WPT_DATA;
	part.data.dat.value = value;
	part.data.dat.tip = tip;
	return part;
}

/**
 * Define equal size of child widgets for intermediate widgets (in both directions).
 * @param hor_equal Try to keep all childs equally wide.
 * @param vert_equal Try to keep all child equally high.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetEqualSize(bool hor_equal, bool vert_equal)
{
	WidgetPart part;

	part.type = WPT_EQUAL_SIZE;
	part.data.flags = (hor_equal ? EQS_HORIZONTAL : 0) | (vert_equal ? EQS_VERTICAL : 0);
	return part;
}

/**
 * Denote the end of a container.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart EndContainer()
{
	WidgetPart part;

	part.type = WPT_END_CON;
	return part;
}

/**
 * Construct a widget from widget parts.
 * @param parts Base of parts array.
 * @param remaining Number of parts still available.
 * @param dest Pointer for storing the constructed widget.
 * @return Read number of parts.
 * @ingroup widget_parts_group
 */
static int MakeWidget(const WidgetPart *parts, int remaining, BaseWidget **dest)
{
	int num_used = 0;

	*dest = NULL;
	while (num_used < remaining) {
		switch (parts->type) {
			case WPT_NEW_WIDGET: {
				if (*dest != NULL) return num_used;
				switch (parts->data.new_widget.wtype) {
					case WT_EMPTY:
						*dest = new BaseWidget(parts->data.new_widget.wtype);
						break;

					case WT_PANEL:
						*dest = new BackgroundWidget(WT_PANEL);
						break;

					case WT_TEXT_BUTTON:
					case WT_IMAGE_BUTTON:
					case WT_TEXT_PUSHBUTTON:
					case WT_IMAGE_PUSHBUTTON:
					case WT_TITLEBAR:
					case WT_LEFT_TEXT:
					case WT_CENTERED_TEXT:
					case WT_RIGHT_TEXT:
						*dest = new DataWidget(parts->data.new_widget.wtype);
						break;

					case WT_RADIOBUTTON:
					case WT_CLOSEBOX:
					case WT_RESIZEBOX:
						*dest = new LeafWidget(parts->data.new_widget.wtype);
						break;

					case WT_HOR_SCROLLBAR:
					case WT_VERT_SCROLLBAR:
						*dest = new ScrollbarWidget(parts->data.new_widget.wtype);
						break;

					default: NOT_REACHED();
				}
				if (parts->data.new_widget.number >= 0) (*dest)->number = parts->data.new_widget.number;
				LeafWidget *lw = dynamic_cast<LeafWidget *>(*dest);
				if (lw != NULL) lw->colour = parts->data.new_widget.colour;
				break;
			}

			case WPT_NEW_INTERMEDIATE:
				if (*dest != NULL) return num_used;
				(*dest) = new IntermediateWidget(parts->data.new_intermediate.num_rows, parts->data.new_intermediate.num_cols);
				break;

			case WPT_MIN_SIZE: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					bw->min_x = parts->data.size.x;
					bw->min_y = parts->data.size.y;
				}
				break;
			}

			case WPT_FILL: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					bw->fill_x = parts->data.size.x;
					bw->fill_y = parts->data.size.y;
				}
				break;
			}

			case WPT_RESIZE: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					bw->resize_x = parts->data.size.x;
					bw->resize_y = parts->data.size.y;
				}
				break;
			}

			case WPT_PADDING:
			case WPT_HOR_PIP:
			case WPT_VERT_PIP: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					for (int i = 0; i < PAD_COUNT; i++) bw->paddings[i] += parts->data.padding[i];
				}
				break;
			}

			case WPT_DATA: {
				if (*dest == NULL) break;
				LeafWidget *lw = dynamic_cast<LeafWidget *>(*dest);
				if (lw != NULL) lw->tooltip = parts->data.dat.tip;
				DataWidget *bw = dynamic_cast<DataWidget *>(*dest);
				if (bw != NULL) bw->value = parts->data.dat.value;
				break;
			}

			case WPT_EQUAL_SIZE: {
				if (*dest == NULL) break;
				IntermediateWidget *iw = dynamic_cast<IntermediateWidget *>(*dest);
				if (iw != NULL) iw->flags = parts->data.flags;
				break;
			}

			case WPT_END_CON:
				return num_used;

			default: NOT_REACHED();
		}
		num_used++;
		parts++;
	}
	return num_used;
}

static int MakeWidgetSubTree(const WidgetPart *parts, int remaining, BaseWidget **dest, int16 *biggest);

/**
 * Fill a row of an intermediate widget with its children.
 * @param parts Parts to assemble into child widgets of the row.
 * @param remaining_parts Number of parts still available.
 * @param widgets Temporary storage for the child widgets.
 * @param remaining_widgets Amount of space left for the child widgets.
 * @param cols [inout] Number of elements of a row. \c 0 means it is not known.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts.
 * @ingroup widget_parts_group
 */
static int FillWidgetRow(const WidgetPart *parts, int remaining_parts, BaseWidget **widgets, int remaining_widgets, uint8 *cols, int16 *biggest)
{
	int total_used = 0;

	uint8 c = 0;
	for (;;) {
		if (remaining_parts == 0 || remaining_widgets == 0) break;
		if (parts->type == WPT_END_CON) break;

		int used = MakeWidgetSubTree(parts, remaining_parts, widgets, biggest);
		parts += used;
		remaining_parts -= used;
		total_used += used;
		if (*widgets == NULL) break;

		widgets++;
		remaining_widgets--;

		if (c == 255) break;
		c++;
		if (*cols == c) break;
	}

	if (*cols == 0) {
		assert(c > 0);
		*cols = c;
		if (remaining_parts > 0 && parts->type == WPT_END_CON) return total_used + 1;
	}
	return total_used;
}

/**
 * Fill an intermediate widget with its children.
 * @param parts Parts to assemble into children of the widget.
 * @param remaining_parts Number of parts still available.
 * @param wid Intermediate widget to fill.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts for the children.
 * @ingroup widget_parts_group
 */
static int FillWidget(const WidgetPart *parts, int remaining_parts, IntermediateWidget *wid, int16 *biggest)
{
	int total_used = 0;

	static const int MAX_CHILDS = 500;
	BaseWidget *grid[500];
	for (int i = 0; i < 500; i++) grid[i] = NULL;

	bool need_claim_memory = wid->num_rows == 0 || wid->num_cols == 0;

	int remaining_widgets = MAX_CHILDS;
	BaseWidget **widgets = &grid[0];
	uint8 r = 0;
	for (;;) {
		int used = FillWidgetRow(parts, remaining_parts, widgets, remaining_widgets, &wid->num_cols, biggest);
		parts += used;
		remaining_parts -= used;
		total_used += used;

		remaining_widgets -= wid->num_cols;
		widgets += wid->num_cols;

		if (r == 255) break;
		r++;
		if (r == wid->num_rows) break;

		if (wid->num_rows == 0 && (remaining_parts == 0 || parts->type == WPT_END_CON)) break;
	}

	if (wid->num_rows == 0) {
		assert(r > 0);
		wid->num_rows = r;
		if (remaining_parts > 0 && parts->type == WPT_END_CON) total_used++;
	}

	if (need_claim_memory) wid->ClaimMemory();
	int count = (int)wid->num_rows * wid->num_cols;
	assert(remaining_widgets == MAX_CHILDS - count);

	widgets = &grid[0];
	for (uint8 r = 0; r < wid->num_rows; r++) {
		for (uint8 c = 0; c < wid->num_cols; c++) {
			wid->AddChild(c, r, *widgets);
			widgets++;
		}
	}

	return total_used;
}

/**
 * Build a (sub) tree of widgets.
 * @param parts Parts to assemble into a tree.
 * @param remaining Number of parts still available.
 * @param dest Pointer for storing the constructed widget tree.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts for the tree.
 * @ingroup widget_parts_group
 */
static int MakeWidgetSubTree(const WidgetPart *parts, int remaining, BaseWidget **dest, int16 *biggest)
{
	int total_used = 0;

	*dest = NULL;
	int used = MakeWidget(parts, remaining, dest);
	parts += used;
	remaining -= used;
	total_used += used;

	if (*dest == NULL) return total_used;

	*biggest = max(*biggest, (*dest)->number); // Update biggest widget number.

	if ((*dest)->wtype == WT_PANEL) {
		/* Panel widget. */
		BackgroundWidget *bg = static_cast<BackgroundWidget *>(*dest);
		if (remaining > 0 && parts->type == WPT_END_CON) {
			used = 1;
		} else {
			used = MakeWidgetSubTree(parts, remaining, &bg->child, biggest);
		}
		parts += used;
		remaining -= used;
		total_used += used;
	} else if ((*dest)->wtype == WT_GRID) {
		/* Container widget; also load child widgets. */
		IntermediateWidget *iw = static_cast<IntermediateWidget *>(*dest);
		used = FillWidget(parts, remaining, iw, biggest);
		parts += used;
		remaining -= used;
		total_used += used;
	}
	return total_used;
}

/**
 * Construct a widget tree from its parts.
 * @param parts Parts to assemble into a tree.
 * @param length Number of parts available.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Constructed widget tree.
 * @ingroup widget_parts_group
 */
BaseWidget *MakeWidgetTree(const WidgetPart *parts, int length, int16 *biggest)
{
	BaseWidget *root = NULL;
	*biggest = INVALID_WIDGET_INDEX;
	MakeWidgetSubTree(parts, length, &root, biggest);
	return root;
}
