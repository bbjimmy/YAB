/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


// #include <MessageRunner.h>
#include <StringView.h>
#include <Window.h>

#include "ToolTip.h"

class MouseToolTip : public BToolTip {
public:
	MouseToolTip()
	{
		fView = new MouseView();
		SetSticky(true);
	}

	virtual ~MouseToolTip()
	{
		delete fView;
	}

	virtual BView* View() const
	{
		return fView;
	}

private:
	BStringView*	fView;
};


class ImmediateView : public BStringView {
public:
	ImmediateView(const char* name, const char* label)
		:
		BStringView(name, label)
	{
		SetToolTip("Easy but immediate!");
		ToolTip()->SetSticky(true);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (transit == B_ENTERED_VIEW)
			ShowToolTip(ToolTip());
	}
};


class Window : public BWindow {
public:
							Window();

	virtual	bool			QuitRequested();
};


class Application : public BApplication {
public:
							Application();

	virtual	void			ReadyToRun();
};


//	#pragma mark -


Window::Window()
	:
	BWindow(BRect(100, 100, 520, 430), "ToolTip-Test",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView* simple = new BStringView("1", "Simple Tool Tip");
	simple->SetToolTip("This is a really\nsimple tool tip!");

	BView* custom = new BStringView("2", "Custom Tool Tip");
	custom->SetToolTip(new CustomToolTip());

	BView* changing = new BStringView("3", "Changing Tool Tip");
	changing->SetToolTip(new ChangingToolTip());

	BView* mouse = new BStringView("3", "Mouse Tool Tip (sticky)");
	mouse->SetToolTip(new MouseToolTip());

	BView* immediate = new ImmediateView("3", "Immediate Tool Tip (sticky)");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(simple)
		.Add(custom)
		.Add(changing)
		.Add(mouse)
		.Add(immediate);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


Application::Application()
	: BApplication("application/x-vnd.haiku-tooltiptest")
{
}


void
Application::ReadyToRun()
{
	BWindow *window = new Window();
	window->Show();
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}

