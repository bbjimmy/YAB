// Calendar Control version 2.5
// by Al.V. Sarikov.
// Kherson, Ukraine, 2006.
// E-mail: avix@ukrpost.net.
// Home page: http://avix.pp.ru.

// Control which allows to work with dates:
// enter date to text field and choose it from calendar.

// Distributed under BSD license (see LICENSE file).

#include <Point.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>

#include "MonthView.cpp"

class MonthWindow: public BWindow
{
	public:
		MonthWindow(BPoint LT, BMessenger *msng, int day, int month, int year,
			int first_year, int last_year);
		virtual void MessageReceived(BMessage *msg);
		virtual bool QuitRequested(void);
		virtual void WindowActivated(bool active);
	private:
		MonthView *myMonthView;
		BMessenger *messenger;
  
		int first_year;
		int last_year;
};


MonthWindow::MonthWindow(BPoint LT, BMessenger *msng, int day, int month, int year, int first_year, int last_year)
            :BWindow(BRect(LT,BPoint(LT.x+200,LT.y+200)),"MonthWindowAViX",
                     B_BORDERED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_MOVABLE|B_AVOID_FOCUS)
{
	this->first_year=first_year;
	this->last_year=last_year;
 
	myMonthView = new MonthView(day, month, year, first_year, last_year);

	AddChild(myMonthView);
	myMonthView->MakeFocus(true);
 
	messenger = msng;

	BRect screenFrame = BScreen(this).Frame();
	if(LT.x < 0)
		LT.x = 0;
	if(LT.x > screenFrame.right - Bounds().Width())
		LT.x = screenFrame.right - Bounds().Width();
	if(LT.y > screenFrame.bottom - Bounds().Height())
		LT.y = screenFrame.bottom - Bounds().Height();
	MoveTo(LT);
 
	Show();
}


void MonthWindow::MessageReceived(BMessage *msg)
{
	if(msg->what=='MVME')
	{
		// Is date correct?
		int32 day, month, year;
		if(msg->FindInt32("day",&day)!=B_OK) return;
		if(msg->FindInt32("month",&month)!=B_OK) return;
		if(msg->FindInt32("year",&year)!=B_OK) return;
	 
		if(year<first_year || year>last_year) return;
		if(month<1 || month>12) return;
		int32 tmp;
	 
		tmp=31;
		if(month==4 || month==6 || month==9 || month==11) 
			tmp=30;
		else if(month==2)
		{
			if(year%4==0)
			{
				if(year%100==0 && year%400!=0) 
					tmp=28;
				else 
					tmp=29;
			}
			else 
				tmp=28;
		}
		if(day<1 || day>tmp) return;
		messenger->SendMessage(msg);
		Quit();
	}
	else 
		BWindow::MessageReceived(msg);
}


bool MonthWindow::QuitRequested(void)
{
	return true;
}


void MonthWindow::WindowActivated(bool active)
{
	// exit if unfocused
	if(!active) Quit();
}
