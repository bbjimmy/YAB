#include <Bitmap.h>
#include <Path.h>
#include <Picture.h>
#include <Region.h>
#include <View.h>
#include "YabWindow.h"
#include "YabView.h"

YabView::YabView(BRect frame, const char *name, uint32 resizingMode, uint32 flags) 
	: BView(frame, name, resizingMode, flags)
{
/*
	SetViewColor(216,216,216,255);
	SetLowColor(216,216,216,255);
	SetHighColor(0,0,0,255);
	*/

	rgb_color b1 = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color b2 = {0,0,0,255};

	SetViewColor(b1);
	SetLowColor(b1);
	SetHighColor(b2);

	drawList = new BList(1);

 	YabDrawing *t = new YabDrawing();
        t->command = 6;
        t->r = 0; t->g = 0;
        t->b = 0; t->alpha = 255;
        drawList->AddItem(t);  

 	YabDrawing *t2 = new YabDrawing();
        t2->command = 7;
        t2->r = b1.red; t2->g = b1.green;
        t2->b = b1.blue; t2->alpha = 255;
        drawList->AddItem(t2);  

	mouseMovedInfo = 1;
	mouseStateInfo = -1;
	prevMouseStateInfo = 0;
	
	mouseX = 0;
	mouseY = 0;
	mouseLButton = 0;
	mouseMButton = 0;
	mouseRButton = 0;
	dropZone = false;
	pressedKeys.SetTo("");
	SetDrawingMode(B_OP_COPY);

	// BTab uses view Name() as displayed label storage, so SetLabel changes it
	// this interferes with yab method of referencing views by name
	// to be removed
	nameWAForTabView = name;
}

YabView::~YabView()
{
	while(drawList->CountItems()>0)
	{
		YabDrawing *t = (YabDrawing*)drawList->LastItem();
		drawList->RemoveItem(t);
		if(t->command == 0) delete [] t->chardata;
		delete t;
	}
	delete drawList;
}

void YabView::MessageReceived(BMessage *msg)
{
	entry_ref ref;
	switch (msg->what)
	{
		case B_SIMPLE_DATA:
			{	
				if(dropZone)
				{
					BString tmp("");
					int32 count;
					uint32 type;
					const char* name;
					int i =0;

					while(msg->FindRef("refs", i, &ref) == B_OK)
					{
						BEntry dropEntry(&ref);
						BPath tmpDirectory;
						dropEntry.GetPath(&tmpDirectory);
						tmp << Name();
						tmp << ":_Dropped";
						tmp << ":" << tmpDirectory.Path();
						tmp << "|";
						i++;
					}
					YabWindow *yabWin = (YabWindow*)Window();
					yabWin->dropMsg.SetTo(tmp);
				}
				else
					BView::MessageReceived(msg);
			}
			break;
		default:
			BView::MessageReceived(msg);
			break;
	}    	
}

void YabView::Draw(BRect updateRect)
{
	SetFont(be_plain_font);
	updateRect = Bounds();
	SetDrawingMode(B_OP_OVER);
	for(int i=0; i<drawList->CountItems(); i++)
	{
		YabDrawing *e = (YabDrawing*)drawList->ItemAt(i);
		switch(e->command)
		{
			case 0: DrawString(e->chardata, BPoint(e->x1, e->y1));
				break;
			case 1: StrokeLine(BPoint(e->x1,e->y1),BPoint(e->x2,e->y2), e->p);
				break;
			case 2:	StrokeEllipse(BPoint(e->x1,e->y1), e->x2, e->y2, e->p);
				break;
			case 3:	FillEllipse(BPoint(e->x1,e->y1), e->x2, e->y2, e->p);
				break;
			case 4:	StrokeRect(BRect(e->x1,e->y1,e->x2,e->y2), e->p);
				break;
			case 5: FillRect(BRect(e->x1,e->y1,e->x2,e->y2), e->p);
				break;
			case 6: {
					if(e->alpha == 255) 	
						SetDrawingMode(B_OP_OVER);
					else 
						SetDrawingMode(B_OP_ALPHA);
					SetHighColor(e->r,e->g,e->b,e->alpha);
				}
				break;
			case 7: {
					if(e->alpha == 255) 
						SetDrawingMode(B_OP_OVER);
					else 
						SetDrawingMode(B_OP_ALPHA);
					SetLowColor(e->r,e->g,e->b,e->alpha);
				}
				break;
			case 8: {
					BPoint p[4];
				        p[0].Set(e->x1,e->y1);
					p[1].Set(e->x2,e->y2);
					p[2].Set(e->x3,e->y3);
					p[3].Set(e->x4,e->y4);
					SetPenSize(1.01);
					StrokeBezier(p, e->p);
					SetPenSize(1.0);
				}
				break;
			case 9: {
					BPoint p[4];
				        p[0].Set(e->x1,e->y1);
					p[1].Set(e->x2,e->y2);
					p[2].Set(e->x3,e->y3);
					p[3].Set(e->x4,e->y4);
					SetPenSize(2.0);
					FillBezier(p, e->p);
					SetPenSize(1.0);
				}
				break;
			case 10: {
					drawing_mode mode = DrawingMode();
					if(IsPrinting())
						SetDrawingMode(B_OP_OVER);
					else
						SetDrawingMode(B_OP_ALPHA);
					DrawBitmap(e->bitmap, BPoint(e->x1, e->y1));
					SetDrawingMode(mode);
				}
				break;
			case 11: {
					drawing_mode mode = DrawingMode();
					if(IsPrinting())
						SetDrawingMode(B_OP_OVER);
					else
						SetDrawingMode(B_OP_ALPHA);
					DrawBitmap(e->bitmap, BRect(e->x1, e->y1, e->x2, e->y2));
					SetDrawingMode(mode);
				}
				break;
			case 12: {
					// SetFont(&e->font, B_FONT_FAMILY_AND_STYLE);
					// SetFont(&e->font, B_FONT_SIZE);
					SetFont(&e->font, B_FONT_ALL);
				}
				break;
		}
	}
}

void YabView::MouseDown(BPoint point)
{
	BPoint ptCursor;
	uint32 uButtons = 0;
	GetMouse(&ptCursor, &uButtons, false); 

	mouseX = (int)ptCursor.x;
	mouseY = (int)ptCursor.y;
	if(uButtons & B_PRIMARY_MOUSE_BUTTON) mouseLButton = 1; else mouseLButton = 0;
	if(uButtons & B_SECONDARY_MOUSE_BUTTON) mouseRButton = 1; else mouseRButton = 0;
	if(uButtons & B_TERTIARY_MOUSE_BUTTON) mouseMButton = 1; else mouseMButton = 0;
	mouseStateInfo = 4;
	BView::MouseDown(point);
}

void YabView::MouseUp(BPoint point)
{
	BPoint ptCursor;
	uint32 uButtons = 0;
	GetMouse(&ptCursor, &uButtons, false); 

	mouseX = (int)ptCursor.x;
	mouseY = (int)ptCursor.y;
	if(uButtons & B_PRIMARY_MOUSE_BUTTON) mouseLButton = 1; else mouseLButton = 0;
	if(uButtons & B_SECONDARY_MOUSE_BUTTON) mouseRButton = 1; else mouseRButton = 0;
	if(uButtons & B_TERTIARY_MOUSE_BUTTON) mouseMButton = 1; else mouseMButton = 0;
	mouseStateInfo = 5;
	BView::MouseUp(point);
}

void YabView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BPoint ptCursor;
	uint32 uButtons = 0;
	GetMouse(&ptCursor, &uButtons, true); 

	mouseX = (int)ptCursor.x;
	mouseY = (int)ptCursor.y;
	if(uButtons & B_PRIMARY_MOUSE_BUTTON) mouseLButton = 1; else mouseLButton = 0;
	if(uButtons & B_SECONDARY_MOUSE_BUTTON) mouseRButton = 1; else mouseRButton = 0;
	if(uButtons & B_TERTIARY_MOUSE_BUTTON) mouseMButton = 1; else mouseMButton = 0;

	switch(transit)
	{
		case B_INSIDE_VIEW:
			if(prevMouseStateInfo==1)
				mouseStateInfo = 0;
			else
				{
					mouseStateInfo = 1;
					prevMouseStateInfo = 1;
				}
			mouseMovedInfo = 0;
			break;		
		case B_ENTERED_VIEW:
			mouseStateInfo = 1;
			mouseMovedInfo = 0;
			break;
		case B_OUTSIDE_VIEW:
			mouseStateInfo = 2;
			mouseMovedInfo = 1;
			break;
		case B_EXITED_VIEW:
			mouseStateInfo = 3;
			mouseMovedInfo = 1;
			prevMouseStateInfo = 0;
			break;
	}
	
	BView::MouseMoved(point, transit, message);
}

void YabView::KeyDown(const char *bytes, int32 numBytes) 
{
	BMessage *msg = Window()->CurrentMessage(); 
	if(msg)
	{
		pressedKeys.SetTo("");
		int32 key, modifiers;
		msg->FindInt32("key", &key);
		msg->FindInt32("modifiers", &modifiers);
		if(modifiers&B_CONTROL_KEY) pressedKeys << "ctrl-";
		if(modifiers&B_COMMAND_KEY) pressedKeys << "command-";
		if(modifiers&B_OPTION_KEY) pressedKeys << "option-";
		if(modifiers&B_SHIFT_KEY) pressedKeys << "shift-";
		switch(bytes[0])
		{
			case B_BACKSPACE: pressedKeys << "backspace"; break;
			case B_ENTER: pressedKeys << "enter"; break;
			case B_TAB: pressedKeys << "tab"; break;
			case B_ESCAPE: pressedKeys << "esc"; break;
			case B_LEFT_ARROW: pressedKeys << "left"; break;
			case B_RIGHT_ARROW: pressedKeys << "right"; break;
			case B_UP_ARROW: pressedKeys << "up"; break;
			case B_DOWN_ARROW: pressedKeys << "down"; break;
			case B_INSERT: pressedKeys << "insert"; break;
			case B_DELETE: pressedKeys << "del"; break;
			case B_HOME: pressedKeys << "home"; break;
			case B_END: pressedKeys << "end"; break;
			case B_PAGE_UP: pressedKeys << "pageup"; break;
			case B_PAGE_DOWN: pressedKeys << "pagedown"; break;
			case B_FUNCTION_KEY:
				{
					switch(key)
					{
						case B_F1_KEY: pressedKeys << "f1"; break;
						case B_F2_KEY: pressedKeys << "f2"; break;
						case B_F3_KEY: pressedKeys << "f3"; break;
						case B_F4_KEY: pressedKeys << "f4"; break;
						case B_F5_KEY: pressedKeys << "f5"; break;
						case B_F6_KEY: pressedKeys << "f6"; break;
						case B_F7_KEY: pressedKeys << "f7"; break;
						case B_F8_KEY: pressedKeys << "f8"; break;
						case B_F9_KEY: pressedKeys << "f9"; break;
						case B_F10_KEY: pressedKeys << "f10"; break;
						case B_F11_KEY: pressedKeys << "f11"; break;
						case B_F12_KEY: pressedKeys << "f12"; break;
						case B_PRINT_KEY: pressedKeys << "print"; break;
						case B_SCROLL_KEY: pressedKeys << "scroll"; break;
						case B_PAUSE_KEY: pressedKeys << "pause"; break;
						default:
							pressedKeys.SetTo(bytes);
							break;
					}
				}
				break;
			default:
				pressedKeys.SetTo(bytes);
				break;
		}
	}
	else
		pressedKeys.SetTo(bytes);

	if(bytes[0]!=B_TAB) BView::KeyDown(bytes,numBytes);
}

void YabView::KeyUp(const char *bytes, int32 numBytes) 
{
	pressedKeys.SetTo("");
	BView::KeyUp(bytes,numBytes);
}

