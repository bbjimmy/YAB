//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		YabTabView.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
// 				Jan Bungeroth (jan@be-logos.org)
//	Description:	YabTabView provides the framework for containing and
//                  managing groups of BView objects.
//------------------------------------------------------------------------------

#ifndef YABTAB_VIEW_H
#define YABTAB_VIEW_H

// Standard Includes -----------------------------------------------------------

#include <View.h>
#include "YabControlLook.h"

// Local Defines ---------------------------------------------------------------
enum tab_position {
	B_TAB_FIRST = 999,
	B_TAB_FRONT,
	B_TAB_ANY
};

enum tab_orientation {
	B_TAB_TOP = 0,
	B_TAB_BOTTOM
};

// YabTabView class ------------------------------------------------------------------
class YabTabView : public BView
{
public:
						YabTabView(BRect frame, const char *name,
							button_width width = B_WIDTH_AS_USUAL, 
							uint32 resizingMode = B_FOLLOW_ALL,
							uint32 flags = B_FULL_UPDATE_ON_RESIZE |
								B_WILL_DRAW | B_NAVIGABLE_JUMP |
								B_FRAME_EVENTS | B_NAVIGABLE);
						~YabTabView();

virtual void			KeyDown(const char *bytes, int32 numBytes);
virtual void			MouseDown(BPoint point);

virtual	void			Select(int32 tab);
	int32			Selection() const;

virtual	void			MakeFocus(bool focused = true);
virtual	void			SetFocusTab(int32 tab, bool focused);
	int32			FocusTab() const;

virtual	void			Draw(BRect updateRect);
virtual	BRect			DrawTabs();
virtual	void			DrawBox(BRect selTabRect);
virtual	BRect			TabFrame(int32 tab_index) const;
virtual void			DrawFocusMark(BRect frame, int32 current);
virtual void 			DrawLabel(int32 current, BRect frame);
virtual void 			DrawTab(BRect frame, int32 current, tab_position position, bool full);
virtual const char* 		GetTabName(int32 index) const;

virtual	void			AddTab(BView *target, const char* tabname);
virtual	BView			*RemoveTab(int32 tabIndex);

virtual	BView			*TabAt ( int32 tab_index );
virtual	void			SetTabWidth(button_width width);
button_width			TabWidth() const;

	void			SetOrientation(tab_orientation side);
	tab_orientation		Orientation();
		
virtual	void			SetTabHeight(float height);
		float			TabHeight(); 

		BView			*ContainerView();

		int32			CountTabs() const;
		int32			FocusChanged;
		int32			OldTabView;

private:
		BList			*fTabList;
		BList			*fTabNames;
		BView			*fContainerView;
		button_width	fTabWidthSetting;
		float 			fTabWidth;
		float			fTabHeight;
		int32			fSelection;
		int32			fInitialSelection;
		int32			fFocus;	
		float                   fTabOffset;
		tab_orientation		fTabOrientation;
		YabControlLook		fYabControlLook;
};
//------------------------------------------------------------------------------

#endif 
