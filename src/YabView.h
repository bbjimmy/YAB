#ifndef YABVIEW_H
#define YABVIEW_H

#include <String.h>
#include <View.h>

struct YabDrawing
{
	int command;
	double x1,y1,x2,y2,x3,y3,x4,y4;
	int r,g,b,alpha;
	const char* chardata;
	pattern p;
	BBitmap *bitmap;
	BFont font;
};

class YabView : public BView
{
	public:
		YabView(BRect frame, const char *name, uint32 resizingMode, uint32 flags); 
		~YabView();
		virtual void MessageReceived(BMessage *msg);
		virtual void Draw(BRect updateRect);
		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		virtual void MouseUp(BPoint point);
		virtual void MouseDown(BPoint point);
		virtual void KeyUp(const char *bytes, int32 numBytes);
		virtual void KeyDown(const char *bytes, int32 numBytes);
		const char* NameForTabView() { return nameWAForTabView; }
		BList *drawList;
		int mouseMovedInfo;
		int mouseStateInfo;
		int mouseX;
		int mouseY;
		uint mouseLButton;
		uint mouseMButton;
		uint mouseRButton;
		bool dropZone;
		BString pressedKeys;
	private:
		int prevMouseStateInfo;
		// TODO: revisit at a later time, more info in constructor
		BString nameWAForTabView;
};

#endif
