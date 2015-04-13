#include <Bitmap.h>
#include <Path.h>
#include <Picture.h>
#include <Region.h>
#include <View.h>
#include "YabWindow.h"
#include "YabBitmapView.h"

YabBitmapView::YabBitmapView(BRect frame, const char *name, uint32 resizingMode, uint32 flags) 
	: BView(frame, name, resizingMode, flags)
{
	bmp = new BBitmap(BRect(0,0, frame.Width(), frame.Height()), B_RGBA32, true);
	BView *myView = new BView(BRect(0,0, frame.Width(), frame.Height()), "canvas", B_FOLLOW_NONE, 0);
	bmp->AddChild(myView);
	SetDrawingMode(B_OP_COPY);
	SetViewColor(0,0,0,255);
	mouseMovedInfo = 1;
	mouseStateInfo = -1;
	prevMouseStateInfo = 0;
	mouseX = 0;
	mouseY = 0;
	mouseLButton = 0;
	mouseMButton = 0;
	mouseRButton = 0;	
}

YabBitmapView::~YabBitmapView()
{
	delete bmp;
}

BBitmap* YabBitmapView::GetBitmap()
{
	return bmp;
}

BView* YabBitmapView::GetBitmapView()
{
	return bmp->FindView("canvas");
}

void YabBitmapView::Draw(BRect updateRect)
{
	DrawBitmap(bmp, updateRect, updateRect);
}

void YabBitmapView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
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

void YabBitmapView::MouseDown(BPoint point)
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

void YabBitmapView::MouseUp(BPoint point)
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
                          
