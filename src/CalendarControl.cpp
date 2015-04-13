// Calendar Control version 2.5
// by Al.V. Sarikov.
// Kherson, Ukraine, 2006.
// E-mail: avix@ukrpost.net.
// Home page: http://avix.pp.ru.

// Updated for Haiku and removed all stuff that is not needed and refactored by jan__64 2009

// Control which allows to work with dates:
// enter date to text field and choose it from calendar.

// Distributed under BSD license (see LICENSE file).

#define __LANG_ENGLISH // to compile english version

#include "CalendarControl.h"

#define myButtonMessage 'DCBP'

#include "DateTextView.cpp"
#include "MonthWindow.cpp"

#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <Point.h>
#include <ControlLook.h>


CalendarControl::CalendarControl(BPoint p, const char* name, int day, int month, int year, uint32 flags, uint32 look)
                :BControl(BRect(100,100,200,200),name, NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
 
	uint32 divider=look & CC_ALL_DIVIDERS;
 
	myDateTextView = new DateTextView(day,month,year,flags,divider);
	myButton = new CalendarButton(BRect(70,0,85,15), "CalendarButton", "", new BMessage(myButtonMessage), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	myOrigin = p;
 
	AddChild(myDateTextView);
	myDateTextView->MoveTo(3,3);
	ResizeTo(myDateTextView->Bounds().Width()+6,myDateTextView->Bounds().Height()+7);
 
	AddChild(myButton);
 
	myButton->ResizeTo(Bounds().Height()*0.7,Bounds().Height()-1);
	myButton->MoveTo(Bounds().right+1, Bounds().top);
	ResizeBy(myButton->Bounds().Width()+1, 0);
}


CalendarControl::~CalendarControl()
{
	RemoveChild(myDateTextView);
	delete myDateTextView;
	RemoveChild(myButton);
	delete myButton;
}


void CalendarControl::AttachedToWindow(void)
{
	BControl::AttachedToWindow();
 
	myButton->SetTarget(this);
 
	if(Parent()!=NULL) 
		view_color=Parent()->ViewColor();
	else
		view_color.red=view_color.green=view_color.blue=view_color.alpha=255;

	SetViewColor(view_color); // function of CalendarControl class

	// MakeButton(); // for BeOS interface is called only from here,
	MoveTo(myOrigin);
}


void CalendarControl::Draw(BRect r)
{
	BRect bounds(Bounds());
	bounds.bottom--;
	bounds.right = myButton->Frame().left - 1;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	bool active = myDateTextView->IsFocus() && Window()->IsActive();
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= BControlLook::B_DISABLED;
	if (active)
		flags |= BControlLook::B_FOCUSED;
	be_control_look->DrawTextControlBorder((BView*)this, bounds, r, base, flags, 15);
}


void CalendarControl::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
	if(bytes[0]==B_TAB) Draw(Bounds());
}


void CalendarControl::MakeFocus(bool focused)
{
	myDateTextView->MakeFocus(focused);
}


void CalendarControl::MessageReceived(BMessage *msg)
{
	switch(msg->what)
 	{
  		case myButtonMessage:
		{
 			if(IsEnabled())
			{
				MakeFocus(true);
				int day, month, year;
				int first_year, last_year;
				GetDate(&day, &month, &year);
				GetYearRange(&first_year, &last_year);
				new MonthWindow(ConvertToScreen(BPoint(Bounds().left+1,Bounds().bottom+1)),
					new BMessenger(this), day, month, year, first_year, last_year);
   			}
   			break;
  		}
		case 'MVME': // message has come from window with calendar
		{
			int32 day, month, year;
			msg->FindInt32("day",&day);
			msg->FindInt32("month",&month);
			msg->FindInt32("year",&year);
			SetDate((int)day, (int)month, (int)year);
			break;
		}
		default:
			BControl::MessageReceived(msg);
	}
}


void CalendarControl::SetEnabled(bool enabled)
{
	if(enabled==IsEnabled()) return;
	BControl::SetEnabled(enabled);
	myButton->SetEnabled(enabled);
	myDateTextView->SetEnabled(enabled);
	Invalidate();
}


void CalendarControl::SetViewColor(rgb_color color)
{
	view_color=color;
	BControl::SetViewColor(view_color);
	Draw(Bounds());
	Invalidate();
}


void CalendarControl::SetViewColor(uchar red, uchar green,
                                   uchar blue, uchar alpha)
{
	rgb_color color={red, green, blue, alpha};
	SetViewColor(color);
}


void CalendarControl::WindowActivated(bool active)
{
	myWindowActive = active; // true if window where control is placed is active
	Draw(Bounds());
}


const char* CalendarControl::Text() const
{
	return myDateTextView->Text();
}


void CalendarControl::GetDate(int *day, int *month, int *year)
{
	myDateTextView->GetDate(day,month,year);
}


void CalendarControl::SetDate(int day, int month, int year)
{
	myDateTextView->SetDate(day,month,year);
}


void CalendarControl::SetDate(const char *tdate)
{
	myDateTextView->SetDate(tdate);
}


void CalendarControl::GetYearRange(int *first_year, int *last_year)
{
	myDateTextView->GetYearRange(first_year, last_year);
}


uint32 CalendarControl::GetLook()
{
	return (myDateTextView->GetDivider());
}


void CalendarControl::SetLook(uint32 look)
{
	myDateTextView->SetDivider(look & CC_ALL_DIVIDERS);
}


uint32 CalendarControl::GetFlags()
{
	return myDateTextView->GetDateFlags();
}


void CalendarControl::SetFlags(uint32 flags)
{
	myDateTextView->SetDateFlags(flags);
}


BTextView *CalendarControl::TextView(void) const
{
	return (BTextView *)myDateTextView;
}


void CalendarButton::Draw(BRect update)
{
	BButton::Draw(update);
	BRect rect = Bounds();
	rect.InsetBy(2.0,4.0);
	uint32 flags = 0;
        rgb_color base = ui_color(B_PANEL_TEXT_COLOR);
        float tint = B_NO_TINT;
        if(!IsEnabled())
	{
                tint = B_LIGHTEN_MAX_TINT;
		flags |= BControlLook::B_DISABLED;
	}
        be_control_look->DrawArrowShape(this, rect, update, base, 3, flags, tint);
}
