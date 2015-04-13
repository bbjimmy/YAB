// Calendar Control version 2.5
// by Al.V. Sarikov.
// Kherson, Ukraine, 2006.
// E-mail: avix@ukrpost.net.
// Home page: http://avix.pp.ru.

// Control which allows to work with dates:
// enter date to text field and choose it from calendar.

// Distributed under BSD license (see LICENSE file).

#include <ControlLook.h>
#include <Message.h>
#include <Messenger.h>
#include <Point.h>
#include <Rect.h>
#include <StringView.h>

class MouseSenseStringView:public BStringView
{
	public:
		MouseSenseStringView(BMessage *msg,
                       BMessenger *msng,
                       BRect frame,
                       const char *name,
                       const char *text,
                       uint32 resizingMode=B_FOLLOW_LEFT|B_FOLLOW_TOP,
                       uint32 flags=B_WILL_DRAW);
		virtual void MouseDown(BPoint p);
		virtual void MouseUp(BPoint p);
		void Draw(BRect(update));
	
	private:
		BMessage *msg;
		BMessenger *msng;
		bool isMouseDown;
};

MouseSenseStringView::MouseSenseStringView(BMessage *msg,
                                           BMessenger *msng,
                                           BRect frame,
                                           const char *name,
                                           const char *text,
                                           uint32 resizingMode,
                                           uint32 flags)
                     :BStringView(frame,name,text,resizingMode,flags)
{
	this->msg=msg;
	this->msng=msng;
	isMouseDown = false;
}


void MouseSenseStringView::MouseDown(BPoint p)
{
	isMouseDown = true;
	// if(msg!=NULL) if(msng!=NULL)
	// 	msng->SendMessage(msg);
}

void MouseSenseStringView::MouseUp(BPoint p)
{
	BPoint mouse;
	uint32 buttons;
	GetMouse(&mouse, &buttons);
	if(Bounds().Contains(mouse))
		if(msg!=NULL) if(msng!=NULL)
			msng->SendMessage(msg);
	isMouseDown = false;
}

void MouseSenseStringView::Draw(BRect update)
{
	BString t(Text());
	BRect r1(Bounds());
	r1.right = r1.right/2;
	BRect r2(Bounds());
	r2.left= r2.right/4;
	r2.right= r2.right*3/4;
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;

	if(t == "<<")
	{
		be_control_look->DrawArrowShape(this, r1, update, base, 0);
		be_control_look->DrawArrowShape(this, r2, update, base, 0);
	}
	else if(t == ">>")
	{
		be_control_look->DrawArrowShape(this, r1, update, base, 1);
		be_control_look->DrawArrowShape(this, r2, update, base, 1);
	}
	else
		BStringView::Draw(update);
}
