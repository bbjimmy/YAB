/*
	Spinner.cpp: A number spinner control.
	Written by DarkWyrm <bpmagic@columbus.rr.com>, Copyright 2004
	Released under the MIT license.
	
	Original BScrollBarButton class courtesy Haiku project
*/
#include "Spinner.h"
#include <String.h>
#include <ScrollBar.h>
#include <Window.h>
#include <stdio.h>
#include <Font.h>
#include <Box.h>
#include <MessageFilter.h>
#include <ControlLook.h>
#include "global.h"

enum
{
	M_UP='mmup',
	M_DOWN,
	M_TEXT_CHANGED='mtch'
};

typedef enum
{
	ARROW_LEFT=0,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	ARROW_NONE
} arrow_direction;

class SpinnerMsgFilter : public BMessageFilter
{
public:
	SpinnerMsgFilter(void);
	~SpinnerMsgFilter(void);
	virtual filter_result Filter(BMessage *msg, BHandler **target);
};

class SpinnerArrowButton : public BView
{
public:
	SpinnerArrowButton(BPoint location, const char *name, 
		arrow_direction dir);
	~SpinnerArrowButton(void);
	void AttachedToWindow(void);
	void DetachedToWindow(void);
	void MouseDown(BPoint pt);
	void MouseUp(BPoint pt);
	void MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
	void Draw(BRect update);
	void SetEnabled(bool value);
	bool IsEnabled(void) const { return enabled; }
	void SetViewColor(rgb_color color);
	
private:
	arrow_direction fDirection;
	BPoint tri1,tri2,tri3;
	Spinner *parent;
	bool mousedown;
	bool enabled;
	rgb_color myColor;
};

class SpinnerPrivateData
{
public:
	SpinnerPrivateData(void)
	{
		thumbframe.Set(0,0,0,0);
		enabled=true;
		tracking=false;
		mousept.Set(0,0);
		thumbinc=1.0;
		repeaterid=-1;
		exit_repeater=false;
		arrowdown=ARROW_NONE;
		
		#ifdef TEST_MODE
			sbinfo.proportional=true;
			sbinfo.double_arrows=false;
			sbinfo.knob=0;
			sbinfo.min_knob_size=14;
		#else
			get_scroll_bar_info(&sbinfo);
		#endif
	}
	
	~SpinnerPrivateData(void)
	{
		if(repeaterid!=-1)
		{
			exit_repeater=false;
			kill_thread(repeaterid);
		}
	}
	thread_id repeaterid;
	scroll_bar_info sbinfo;
	BRect thumbframe;
	bool enabled;
	bool tracking;
	BPoint mousept;
	float thumbinc;
	bool exit_repeater;
	arrow_direction arrowdown;
};

Spinner::Spinner(BRect frame, const char *name, const char *label, int32 min, int32 max, int32 step, BMessage *msg,
		uint32 resize,uint32 flags)
 : BControl(frame, name,NULL,msg,resize,flags)
{

	BRect r(Bounds());
	if(r.Height()<14*2)
		r.bottom=r.top+14*2;
	ResizeTo(r.Width(),r.Height());
	
	r.right-=14;
	
	font_height fh;
	BFont font;
	font.GetHeight(&fh);
	float textheight=fh.ascent+fh.descent+fh.leading;

	BString t("");
	t << max;
	BFont f(be_plain_font);
	float width1 = f.StringWidth(t.String())+2;
	float width2 = f.StringWidth(label);
	ResizeTo(width1+width2+18+14, 14*2-2);
	r = Bounds();
	r.right-=14+3;
	
	r.bottom=r.top+textheight+8;
	r.OffsetTo(0, ( (Bounds().Height()-r.Height())/2) );
	
	fTextControl=new BTextControl(r,"textcontrol",label,"0",new BMessage(M_TEXT_CHANGED),
			B_FOLLOW_ALL,B_WILL_DRAW|B_NAVIGABLE);
	AddChild(fTextControl);
	BTextView *tview=fTextControl->TextView();
	tview->SetAlignment(B_ALIGN_LEFT);
	tview->SetWordWrap(false);
	
	fTextControl->SetDivider(width2+5);

	BString string("QWERTYUIOP[]\\ASDFGHJKL;'ZXCVBNM,/qwertyuiop{}| "
		"asdfghjkl:\"zxcvbnm<>?!@#$%^&*()-_=+`~\r");
	
	for(int32 i=0; i<string.CountChars(); i++)
	{
		char c=string.ByteAt(i);
		tview->DisallowChar(c);
	}
	r=Bounds();
	r.left=r.right-15;
	r.bottom/=2;
	
	fUpButton=new SpinnerArrowButton(BPoint(r.left, r.top),"up",ARROW_UP);
	AddChild(fUpButton);

	r=Bounds();
	r.left=r.right-15;
	r.top=r.bottom/2+1;
	
	fDownButton=new SpinnerArrowButton(BPoint(r.left, r.top-1),"down",ARROW_DOWN);
	AddChild(fDownButton);
	
	fStep=step;
	fMin=min;
	fMax=max;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	privatedata=new SpinnerPrivateData;
	fFilter=new SpinnerMsgFilter;
	
	SetValue(min);
}

Spinner::~Spinner(void)
{
	delete privatedata;
	delete fFilter;
}

void Spinner::AttachedToWindow(void)
{
	Window()->AddCommonFilter(fFilter);
	fTextControl->SetTarget(this);
}

void Spinner::DetachedFromWindow(void)
{
	Window()->RemoveCommonFilter(fFilter);
}

void Spinner::SetValue(int32 value)
{
	if(value>GetMax() || value<GetMin())
		return;
	
	BControl::SetValue(value);
	
	char string[50];
	sprintf(string,"%ld",value);
	fTextControl->SetText(string);
}

void Spinner::SetViewColor(rgb_color color)
{
	BControl::SetViewColor(color);
	fTextControl->SetViewColor(color);
	fTextControl->SetLowColor(color);
	fUpButton->SetViewColor(color);
	fDownButton->SetViewColor(color);
}

void Spinner::SetLabel(const char *text)
{
	fTextControl->SetLabel(text);
}

void Spinner::ValueChanged(int32 value)
{
}

void Spinner::MessageReceived(BMessage *msg)
{
	if(msg->what==M_TEXT_CHANGED)
	{
		BString string(fTextControl->Text());
		int32 newvalue=0;
		
		sscanf(string.String(),"%ld",&newvalue);
		if(newvalue>=GetMin() && newvalue<=GetMax())
		{
			// new value is in range, so set it and go
			SetValue(newvalue);
			Invoke();
			Draw(Bounds());
			ValueChanged(Value());
		}
		else
		{
			// new value is out of bounds. Clip to range if current value is not
			// at the end of its range
			if(newvalue<GetMin() && Value()!=GetMin())
			{
				SetValue(GetMin());
				Invoke();
				Draw(Bounds());
				ValueChanged(Value());
			}
			else
			if(newvalue>GetMax() && Value()!=GetMax())
			{
				SetValue(GetMax());
				Invoke();
				Draw(Bounds());
				ValueChanged(Value());
			}
			else
			{
				char string[100];
				sprintf(string,"%ld",Value());
				fTextControl->SetText(string);
			}
		}
	}
	else
		BControl::MessageReceived(msg);
}

void Spinner::SetSteps(int32 stepsize)
{
	fStep=stepsize;
}

void Spinner::SetRange(int32 min, int32 max)
{
	SetMin(min);
	SetMax(max);
}

void Spinner::GetRange(int32 *min, int32 *max)
{
	*min=fMin;
	*max=fMax;
}

void Spinner::SetMax(int32 max)
{
	fMax=max;
	if(Value()>fMax)
		SetValue(fMax);
}

void Spinner::SetMin(int32 min)
{
	fMin=min;
	if(Value()<fMin)
		SetValue(fMin);
	
}

void Spinner::SetEnabled(bool value)
{
	if(IsEnabled()==value)
		return;
	
	BControl::SetEnabled(value);
	fTextControl->SetEnabled(value);
	fTextControl->TextView()->MakeSelectable(value);
	fUpButton->SetEnabled(value);
	fDownButton->SetEnabled(value);
}

void Spinner::MakeFocus(bool value)
{
	fTextControl->MakeFocus(value);
}


SpinnerArrowButton::SpinnerArrowButton(BPoint location, 
	const char *name, arrow_direction dir)
 :BView(BRect(0,0,16,12).OffsetToCopy(location),
 		name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	fDirection=dir;
	enabled=true;
	myColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	mousedown=false;
}

SpinnerArrowButton::~SpinnerArrowButton(void)
{
}

void SpinnerArrowButton::MouseDown(BPoint pt)
{
	if(enabled==false)
		return;
	
	if (!IsEnabled())
		return;

	mousedown=true;
	int redcost = 1000;
	Draw(Bounds());

	BRect bounds = Bounds();
	uint32 buttons;
	BPoint point;

	int32 step=parent->GetSteps();
	int32 newvalue=parent->Value();
	int32 waitvalue=150000;
	
	do
	{
		if(fDirection==ARROW_UP)
		{
			parent->privatedata->arrowdown=ARROW_UP;
			newvalue+=step;
		}
		else
		{
			parent->privatedata->arrowdown=ARROW_DOWN;
			newvalue-=step;
		}

		if( newvalue>=parent->GetMin() && newvalue<=parent->GetMax())
		{
			// new value is in range, so set it and go
			parent->SetValue(newvalue);
			parent->Invoke();
//			parent->Draw(parent->Bounds());
			parent->ValueChanged(parent->Value());
		}
		else
		{
			// new value is out of bounds. Clip to range if current value is not
			// at the end of its range
			if(newvalue<parent->GetMin() && parent->Value()!=parent->GetMin())
			{
				parent->SetValue(parent->GetMin());
				parent->Invoke();
//				parent->Draw(parent->Bounds());
				parent->ValueChanged(parent->Value());
			}
			else
			if(newvalue>parent->GetMax() && parent->Value()!=parent->GetMax())
			{
				parent->SetValue(parent->GetMax());
				parent->Invoke();
//				parent->Draw(parent->Bounds());
				parent->ValueChanged(parent->Value());
			}
			else
			{
				// cases which go here are if new value is <minimum and value already at
				// minimum or if > maximum and value already at maximum
				return;
			}
		}
		
		Window()->UpdateIfNeeded();
		
		snooze(waitvalue);

		GetMouse(&point, &buttons, false);

 		bool inside = bounds.Contains(point);
			
	//	if ((parent->Value() == B_CONTROL_ON) != inside)
	//		parent->SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
		
		if(waitvalue<=50000)
			waitvalue=50000;
		else
		{
			waitvalue -= redcost;
			redcost = redcost*10;
		}
			
	} while (buttons != 0);

}

void SpinnerArrowButton::MouseUp(BPoint pt)
{
	if(enabled)
	{
		mousedown=false;
		
		if(parent)
		{
			parent->privatedata->arrowdown=ARROW_NONE;
			parent->privatedata->exit_repeater=true;
		}
		Draw(Bounds());
	}
}

void SpinnerArrowButton::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if(enabled==false)
		return;
	
	if(transit==B_ENTERED_VIEW)
	{
		BPoint point;
		uint32 buttons;
		GetMouse(&point,&buttons);
		if( (buttons & B_PRIMARY_MOUSE_BUTTON)==0 &&
				(buttons & B_SECONDARY_MOUSE_BUTTON)==0 &&
				(buttons & B_PRIMARY_MOUSE_BUTTON)==0 )
			mousedown=false;
		else
			mousedown=true;
		Draw(Bounds());
	}
	
	if(transit==B_EXITED_VIEW || transit==B_OUTSIDE_VIEW)
		MouseUp(Bounds().LeftTop());
}

void SpinnerArrowButton::Draw(BRect update)
{
	BRect rect(Bounds());
	rgb_color background = B_TRANSPARENT_COLOR;
	if (Parent())
		background = Parent()->ViewColor();
	if (background == B_TRANSPARENT_COLOR)
		background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = 0; 
	if(!parent->IsEnabled())
		flags |= BControlLook::B_DISABLED;
	if(mousedown)
		flags = 1 << 2;
	BRect r(Bounds());
	if(fDirection == ARROW_UP)
		r.bottom = r.bottom*2;
	else
		r.top = - r.bottom;
	be_control_look->DrawButtonFrame(this, r, update, base, background, flags);
	be_control_look->DrawButtonBackground(this, r, update, base, flags);

	rect.InsetBy(2.0,2.0);
	base = ui_color(B_PANEL_TEXT_COLOR);
	float tint = B_NO_TINT;
	if(!parent->IsEnabled())
		tint = B_LIGHTEN_MAX_TINT;
	be_control_look->DrawArrowShape(this, rect, update, base, fDirection, flags, tint);
}

void SpinnerArrowButton::AttachedToWindow(void)
{
	parent=(Spinner*)Parent();
}

void SpinnerArrowButton::DetachedToWindow(void)
{
	parent=NULL;
}

void SpinnerArrowButton::SetEnabled(bool value)
{
	enabled=value;
	Invalidate();
}

void SpinnerArrowButton::SetViewColor(rgb_color color)
{
	myColor = color;
	Invalidate();
}

SpinnerMsgFilter::SpinnerMsgFilter(void)
 : BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE,B_KEY_DOWN)
{
}

SpinnerMsgFilter::~SpinnerMsgFilter(void)
{
}

filter_result SpinnerMsgFilter::Filter(BMessage *msg, BHandler **target)
{
	int32 c;
	msg->FindInt32("raw_char",&c);
	switch(c)
	{
		case B_ENTER:
		{
			BTextView *text=dynamic_cast<BTextView*>(*target);
			if(text && text->IsFocus())
			{
				BView *view=text->Parent();
				while(view)
				{					
					Spinner *spin=dynamic_cast<Spinner*>(view);
					if(spin)
					{
						BString string(text->Text());
						int32 newvalue=0;
						
						sscanf(string.String(),"%ld",&newvalue);
						if(newvalue!=spin->Value())
						{
							spin->SetValue(newvalue);
							spin->Invoke();
						}
						return B_SKIP_MESSAGE;
					}
					view=view->Parent();
				}
			}
			return B_DISPATCH_MESSAGE;
		}
		case B_TAB:
		{
			return B_DISPATCH_MESSAGE;	
		}
		/*
		case B_TAB:
		{
			// Cause Tab characters to perform keybaord navigation
			BHandler *h=*target;
			BView *v=NULL;

			h=h->NextHandler();
			while(h)
			{
				v=dynamic_cast<BView*>(h);
				if(v)
				{
					*target=v->Window();
					return B_DISPATCH_MESSAGE;
				}
				h=h->NextHandler();
			}
			return B_SKIP_MESSAGE;
			// return B_DISPATCH_MESSAGE;
		}*/
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		{
			BTextView *text=dynamic_cast<BTextView*>(*target);
			if(text && text->IsFocus())
			{
				// We have a text view. See if it currently has the focus and belongs
				// to a Spinner control. If so, change the value of the spinner
				
				// TextViews are complicated beasts which are actually multiple views.
				// Travel up the hierarchy to see if any of the target's ancestors are
				// a Spinner.
				
				BView *view=text->Parent();
				while(view)
				{					
					Spinner *spin=dynamic_cast<Spinner*>(view);
					if(spin)
					{
						int32 step=spin->GetSteps();
						if(c==B_DOWN_ARROW)
							step=0-step;
						
						spin->SetValue(spin->Value()+step);
						spin->Invoke();
						return B_SKIP_MESSAGE;
					}
					view=view->Parent();
				}
			}
		
			return B_DISPATCH_MESSAGE;
		}
		default:
			return B_DISPATCH_MESSAGE;
	}
	
	// shut the stupid compiler up
	return B_SKIP_MESSAGE;
}
