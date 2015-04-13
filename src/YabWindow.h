#ifndef YABWINDOW_H
#define YABWINDOW_H

#include <FilePanel.h>
#include <List.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Window.h>

class YabWindow : public BWindow
{
	public:
		YabWindow(BRect frame, const char* title, const char* id, window_look winlook, window_feel winfeel, uint32 flags);
		~YabWindow();
		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();
		const BString getMessages();
		virtual	void WindowActivated(bool state);
		virtual	void FrameMoved(BPoint new_position);
		virtual	void FrameResized(float new_width, float new_height);
		int layout;
		int showing;
		int WActivated;
		int WFrameResized;
		int Wpx;
		int Wpy;
		int Wph;
		int Wpw;
		int WFrameMoved;
		BString dropMsg;
		BString idString;
	private:
		bool IsPaper(uint8* a);
		BString messageString;
};

#endif
