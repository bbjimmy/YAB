//------------------------------------------------------------------------------
//      Copyright (c) 2001-2005, Haiku
//
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the "Software"),
//      to deal in the Software without restriction, including without limitation
//      the rights to use, copy, modify, merge, publish, distribute, sublicense,
//      and/or sell copies of the Software, and to permit persons to whom the
//      Software is furnished to do so, subject to the following conditions:
//
//      The above copyright notice and this permission notice shall be included in
//      all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//      FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//      DEALINGS IN THE SOFTWARE.
//
//      File Name:              YabTabView.cpp
//      Author:                 Marc Flerackers (mflerackers@androme.be)
//				Modified by Jan Bungeroth (jan@be-logos.org)
//      Description:  YabTabView provides the framework for containing and
//                  managing groups of BView objects. Modified for *sane*
//		    view handling (they stay connected to the window).
//------------------------------------------------------------------------------   

#include <List.h>
#include <Rect.h>  
#include <String.h>
#include <View.h>  
#include <stdio.h>
#include <ControlLook.h>
#include "YabControlLook.h"
#include "YabTabView.h"

#define X_OFFSET 0.0f

YabTabView::YabTabView(BRect frame, const char* name, button_width width, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
    fTabList = new BList;
	fTabNames = new BList;

	fTabWidthSetting = width;
	fSelection = 0;
	fFocus = -1;
	fTabOffset = 0.0f;

	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(color);
	SetLowColor(color);

	font_height fh;
	GetFontHeight(&fh);
	fTabHeight = fh.ascent + fh.descent + fh.leading + 8.0f;

/*
	if (layouted) {
		BGroupLayout* layout = new(std::nothrow) BGroupLayout(B_HORIZONTAL);
		if (layout) {
			layout->SetInsets(3.0, 3.0 + TabHeight() - 1, 3.0, 3.0);
			SetLayout(layout);
		}

		fContainerView = new BView("view container", B_WILL_DRAW);
		fContainerView->SetLayout(new(std::nothrow) BCardLayout());
	} else {
		*/
		BRect bounds = Bounds();

		bounds.top += TabHeight();
		bounds.InsetBy(3.0f, 3.0f);

		fContainerView = new BView(bounds, "view container", B_FOLLOW_ALL,
			B_WILL_DRAW);
//	}

	fContainerView->SetViewColor(color);
	fContainerView->SetLowColor(color);

	AddChild(fContainerView);
    FocusChanged = 1;
    OldTabView = 1;

	fTabOrientation = B_TAB_TOP;
}

YabTabView::~YabTabView()
{
	for(int i=0; i<CountTabs(); i++)
	{
		delete TabAt(i);
		delete (BString*)fTabNames->RemoveItem(i);
	}

	delete fTabList;
	delete fTabNames;
}

void YabTabView::AddTab(BView *tabView, const char* label)
{
	if(tabView)
	{
               	tabView->Hide();
		fContainerView->AddChild(tabView);
		Invalidate();
              	if(CountTabs() == 0) tabView->Show();


		fTabList->AddItem(tabView);
		fTabNames->AddItem(new BString(label));
	}
}

BView* YabTabView::RemoveTab(int32 index)
{
	if(index < 0 || index >= CountTabs())
		return NULL;
	
	BView *tab = (BView*)fTabList->RemoveItem(index);
	delete (BString*)fTabNames->RemoveItem(index);

        if (index <= fSelection && fSelection != 0)
                fSelection--;

        Select(fSelection);

        if (fFocus == CountTabs() - 1)
                SetFocusTab(fFocus, false);
        else
                SetFocusTab(fFocus, true);

        return tab; 	
}

int32 YabTabView::CountTabs() const
{
	return fTabList->CountItems();
}

int32 YabTabView::Selection() const
{
        return fSelection;
}  

void YabTabView::Select(int32 index)
{
        if (index < 0 || index >= CountTabs())
                index = Selection();

        BView *tab = TabAt(Selection());
        if (tab)
	{
		Invalidate();
		tab->Hide();
	}

        tab = TabAt(index);
        if (tab) 
	{
			if (index != 0 && !Bounds().Contains(TabFrame(index))){
		if (!Bounds().Contains(TabFrame(index).LeftTop()))
			fTabOffset += TabFrame(index).left - Bounds().left - 20.0f;
		else
			fTabOffset += TabFrame(index).right - Bounds().right + 20.0f;
			}


		Invalidate();
                tab->Show();
                fSelection = index;
                FocusChanged = index+1;
        }

}

void YabTabView::MakeFocus(bool focused)
{
        BView::MakeFocus(focused);

        SetFocusTab(Selection(), focused);
}    

int32 YabTabView::FocusTab() const
{
        return fFocus;
} 

void YabTabView::SetFocusTab(int32 tab, bool focused)
{

    if (tab >= CountTabs())
		tab = 0;

	if (tab < 0)
		tab = CountTabs() - 1;

	if (focused) {
		if (tab == fFocus)
			return;

		if (fFocus != -1){
			if (TabAt (fFocus) != NULL)
				TabAt(fFocus)->MakeFocus(false);
			Invalidate(TabFrame(fFocus));
		}
		if (TabAt(tab) != NULL){
			TabAt(tab)->MakeFocus(true);
			Invalidate(TabFrame(tab));
			fFocus = tab;
		}
	} else if (fFocus != -1) {
		TabAt(fFocus)->MakeFocus(false);
		Invalidate(TabFrame(fFocus));
		fFocus = -1;
	}
}

BView* YabTabView::TabAt(int32 index)
{
	if(index < 0 || index >= CountTabs())
		return NULL;

	return (BView*)fTabList->ItemAt(index);
}     

const char* YabTabView::GetTabName(int32 index) const
{
	if(index < 0 || index >= CountTabs())
		return NULL; 

	return ((BString*)fTabNames->ItemAt(index))->String();
}

void YabTabView::KeyDown(const char *bytes, int32 numBytes)
{
        if (IsHidden())
                return;

        switch (bytes[0]) {
                case B_DOWN_ARROW:
                case B_LEFT_ARROW: {
                        int32 focus = fFocus - 1;
                        if (focus < 0)
                                focus = CountTabs() - 1;
                        SetFocusTab(focus, true);
                        FocusChanged = 1;
                        break;
                }

                case B_UP_ARROW:
                case B_RIGHT_ARROW: {
                        int32 focus = fFocus + 1;
                        if (focus >= CountTabs())
                                focus = 0;
                        SetFocusTab(focus, true);
                        FocusChanged = 1;
                        break;
                }

                case B_RETURN:
                case B_SPACE:
                        Select(FocusTab());
                        break;

                default:
                        BView::KeyDown(bytes, numBytes);
        }
}  

void YabTabView::MouseDown(BPoint point)
{
        if (fTabOrientation == B_TAB_TOP && point.y > fTabHeight)
                return;
        if (fTabOrientation == B_TAB_BOTTOM && point.y < Bounds().bottom-fTabHeight)
                return;

        for (int32 i = 0; i < CountTabs(); i++) {
                if (TabFrame(i).Contains(point) && i != Selection()) {
                        Select(i);
			fFocus = -1;
                        return;
                }
        }

        BView::MouseDown(point);
}      

BRect YabTabView::TabFrame(int32 tab_index) const
{
	float width = 100.0;
	float height = fTabHeight;;
	BRect tabFrame(Bounds());
	switch (fTabWidthSetting) {
		case B_WIDTH_FROM_LABEL:
		{
			float x = 0.0;
			for (int32 i = 0; i < tab_index; i++){
				x += StringWidth(GetTabName(i)) + 20.0;
			}
		
			if(fTabOrientation == B_TAB_TOP)
				tabFrame.Set(x, 0.0, x + StringWidth(GetTabName(tab_index)) + 20.0, height);
			else
				tabFrame.Set(x, tabFrame.bottom - height, x + StringWidth(GetTabName(tab_index)) + 20.0, tabFrame.bottom);
			return tabFrame;
		}

		case B_WIDTH_FROM_WIDEST:
			width = 0.0;
			for (int32 i = 0; i < CountTabs(); i++) {
				float tabWidth = StringWidth(GetTabName(i)) + 20.0;
				if (tabWidth > width)
					width = tabWidth;
			}
			// fall through

		case B_WIDTH_AS_USUAL:
		default:
			if(fTabOrientation == B_TAB_TOP)
				tabFrame.Set(tab_index * width, 0.0, tab_index * width + width, height);
			else
				tabFrame.Set(tab_index * width, tabFrame.bottom - height, tab_index * width + width, tabFrame.bottom);
			return tabFrame;
	}
}  

void YabTabView::SetTabWidth(button_width width)
{
        fTabWidthSetting = width;

        Invalidate();
}  

button_width YabTabView::TabWidth() const
{
        return fTabWidthSetting;
}  

void YabTabView::SetTabHeight(float height)
{
        if (fTabHeight == height)
                return;

        fContainerView->MoveBy(0.0f, height - fTabHeight);
        fContainerView->ResizeBy(0.0f, height - fTabHeight);

        fTabHeight = height;

        Invalidate();
}


float YabTabView::TabHeight()
{
        return fTabHeight;
}


BView *YabTabView::ContainerView()
{
        return fContainerView;
}  

void YabTabView::Draw(BRect updateRect)
{
        // DrawBox(DrawTabs());
//			SetHighColor(223,223,223);
//			StrokeLine(BPoint(Bounds().left, Bounds().bottom-TabHeight()-2), BPoint(Bounds().right, Bounds().bottom-TabHeight()-2));
	DrawBox(TabFrame(fSelection));
	DrawTabs();

        if (IsFocus() && fFocus != -1)
	{
		DrawFocusMark(TabFrame(fFocus), fFocus);
		// Invalidate(TabFrame(fFocus));
	}
}


BRect YabTabView::DrawTabs()
{
        	float left = 0;

	for (int32 i = 0; i < CountTabs(); i++) {
		BRect tabFrame = TabFrame(i);
		DrawTab(tabFrame, i, i == fSelection ? B_TAB_FRONT : (i == 0) ? B_TAB_FIRST : B_TAB_ANY, i + 1 != fSelection);

		left = tabFrame.right;
	}

	BRect frame(Bounds());
	if (left < frame.right) {
		frame.left = left;
		if(fTabOrientation == B_TAB_TOP)
			frame.bottom = fTabHeight;
		else
			frame.top = frame.bottom - fTabHeight;
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER | BControlLook::B_RIGHT_BORDER;
		if (left == 0)
			borders |= BControlLook::B_LEFT_BORDER;
		if(fTabOrientation == B_TAB_TOP)
			be_control_look->DrawInactiveTab(this, frame, frame, base, 0, borders);
		else
			fYabControlLook.DrawInactiveTabBottom(this, frame, frame, base, 0, borders);
	}

	if (fSelection < CountTabs())
		return TabFrame(fSelection);

	return BRect();
}      

void YabTabView::DrawLabel(int32 current, BRect frame)
{
    BString label = GetTabName(current);
	if (label == NULL)
		return;

	float frameWidth = frame.Width();
	float width = StringWidth(label.String());
	font_height fh;

	if (width > frameWidth) {
		BFont font;
		GetFont(&font);
		font.TruncateString(&label, B_TRUNCATE_END, frameWidth);
		width = frameWidth;
		font.GetHeight(&fh);
	} else {
		GetFontHeight(&fh);
	}

	SetDrawingMode(B_OP_OVER);
	SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
	DrawString(label.String(),
		BPoint((frame.left + frame.right - width) / 2.0,
 			(frame.top + frame.bottom - fh.ascent - fh.descent) / 2.0
 			+ fh.ascent));
}


void YabTabView::DrawTab(BRect frame, int32 current, tab_position position, bool full)
{

    BView *owner = this;
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);

//		uint32 borders = BControlLook::B_RIGHT_BORDER
//			| BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;
//		if (frame.left == owner->Bounds().left)
//			borders |= BControlLook::B_LEFT_BORDER;
//		be_control_look->DrawButtonFrame(owner, frame, frame,
//			no_tint, 0, borders);
//		if (position == B_TAB_FRONT)
//			no_tint = tint_color(no_tint, B_DARKEN_2_TINT);
//		be_control_look->DrawButtonBackground(owner, frame, frame, no_tint);

	uint32 borders = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER;
	if (frame.left == owner->Bounds().left)
		borders |= BControlLook::B_LEFT_BORDER;
	if (frame.right == owner->Bounds().right)
		borders |= BControlLook::B_RIGHT_BORDER;

	if (position == B_TAB_FRONT) {
		if(fTabOrientation == B_TAB_TOP)
		{
			frame.bottom += 1;
			be_control_look->DrawActiveTab(owner, frame, frame, no_tint, 0, borders);
		}
		else
		{
			frame.top -= 1;
			fYabControlLook.DrawActiveTabBottom(owner, frame, frame, no_tint, 0, borders);
		}
	} else {
		if(fTabOrientation == B_TAB_TOP)
			be_control_look->DrawInactiveTab(owner, frame, frame, no_tint, 0, borders);
		else
			fYabControlLook.DrawInactiveTabBottom(owner, frame, frame, no_tint, 0, borders);
	}

	DrawLabel(current, frame);
	return;

} 

void YabTabView::DrawFocusMark(BRect frame, int32 current)
{
	float width = StringWidth(GetTabName(current));

	SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));

	float offset = (fSelection == current) ? 3 : 2;
	StrokeLine(BPoint((frame.left + frame.right - width) / 2.0,
			frame.bottom - offset),
		BPoint((frame.left + frame.right + width) / 2.0,
			frame.bottom - offset));
}  

void YabTabView::DrawBox(BRect selTabRect)
{
	BRect rect(Bounds());
	if(fTabOrientation == B_TAB_TOP)
		rect.top = selTabRect.bottom;
	else
		rect.bottom -= selTabRect.Height();

//		BRegion clipping(Bounds());
//		selTabRect.left += 2;
//		selTabRect.right -= 2;
//		clipping.Exclude(selTabRect);
//		ConstrainClippingRegion(&clipping);

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawGroupFrame(this, rect, rect, base);

//		ConstrainClippingRegion(NULL);

}

void YabTabView::SetOrientation(tab_orientation side)
{
	if(fTabOrientation != side)
		if(side == B_TAB_BOTTOM)
			fContainerView->MoveTo(3.0, 3.0);
		else
			fContainerView->MoveTo(3.0, TabHeight());
	fTabOrientation = side;
}

tab_orientation YabTabView::Orientation()
{
	return fTabOrientation;
}

