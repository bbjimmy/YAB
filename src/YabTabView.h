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
//					Jan Bungeroth (jan@be-logos.org)
//					Kacper Kasper (kacperkasper@gmail.com)
//	Description:	YabTabView provides the framework for containing and
//					managing groups of BView objects.
//------------------------------------------------------------------------------

#ifndef YABTAB_VIEW_H
#define YABTAB_VIEW_H

// Standard Includes -----------------------------------------------------------

#include <TabView.h>

// YabTabView class ------------------------------------------------------------------
class YabTabView : public BTabView
{
public:
						YabTabView(BRect frame, const char *name,
							button_width width = B_WIDTH_AS_USUAL, 
							uint32 resizingMode = B_FOLLOW_ALL,
							uint32 flags = B_FULL_UPDATE_ON_RESIZE |
								B_WILL_DRAW | B_NAVIGABLE_JUMP |
								B_FRAME_EVENTS | B_NAVIGABLE);
						~YabTabView();

virtual const char* 		GetTabName(int32 index) const;

virtual	void			AddTab(BView *target, const char* tabname);

virtual	void				Select(int32 index);
virtual	void				MakeFocus(bool focused = true);
virtual	void				SetFocusTab(int32 tab, bool focused);

		int32			FocusChanged;
		int32			OldTabView;

private:
		BList			*fTabNames;
};
//------------------------------------------------------------------------------

#endif 
