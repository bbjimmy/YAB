#ifndef YABBITMAPVIEW_H
#define YABBITMAPVIEW_H

#include <View.h>

class YabBitmapView : public BView
{
	public:
		YabBitmapView(BRect frame, const char *name, uint32 resizingMode, uint32 flags); 
		~YabBitmapView();
		virtual void Draw(BRect updateRect);
		BBitmap* GetBitmap();
		BView* GetBitmapView();
		BBitmap *bmp;

                virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
                virtual void MouseUp(BPoint point);
                virtual void MouseDown(BPoint point);
                int mouseStateInfo;
                int mouseMovedInfo;
                int mouseX;
                int mouseY;
                uint mouseLButton;
                uint mouseMButton;
                uint mouseRButton;
	private:
		int prevMouseStateInfo;

};

#endif
