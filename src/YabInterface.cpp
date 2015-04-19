#include "global.h"
#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <BitmapStream.h>   
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <Clipboard.h>
#include <ColorControl.h>
#include <Deskbar.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Locale.h>
#include <List.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <NodeInfo.h>
#include <OS.h>
#include <OutlineListView.h>
#include <Path.h>
#include <Picture.h>
#include <PictureButton.h>
#include <PlaySound.h>
#include <PopUpMenu.h>
#include <PrintJob.h>
#include <PropertyInfo.h>
#include <RadioButton.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StatusBar.h>
#include <String.h>
#include <interface/StringView.h>
#include <kernel/fs_attr.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslatorRoster.h>
#include <View.h>
#include <Window.h>

#include "config.h"

#include "CalendarControl.h"
#include "YabFilePanel.h"
#include "SplitPane.h"
#include "URLView.h"
#include "Spinner.h"
#include "YabTabView.h"

#ifdef LIBBSVG
	#include <SVGView.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "YabInterface.h"
#include "YabWindow.h"
#include "YabView.h"
#include "YabBitmapView.h"
#include "YabList.h"

#include "YabText.h"
#include "YabMenu.h"
#include "YabStackView.h"
#include "column/YabColumnType.h"
#include "column/ColorTools.h"
#include "column/ColumnListView.h"


const uint32 YABBUTTON			= 'YBbu';
const uint32 YABMENU			= 'YBme';
const uint32 YABSUBMENU			= 'YBsu';
const uint32 YABTEXTCONTROL		= 'YBtc';
const uint32 YABCHECKBOX		= 'YBcb';
const uint32 YABRADIOBUTTON		= 'YBrb';
const uint32 YABLISTBOXSELECT		= 'YBls';
const uint32 YABLISTBOXINVOKE		= 'YBli';
const uint32 YABDROPBOX			= 'YBdb';
const uint32 YABSLIDER			= 'YBsl';
const uint32 YABCOLORCONTROL		= 'YBco';
const uint32 YABTREEBOXSELECT		= 'YBts';
const uint32 YABTREEBOXINVOKE		= 'YBti';
const uint32 YABFILEBOXSELECT		= 'YBfs';
const uint32 YABFILEBOXINVOKE		= 'YBfi';
const uint32 YABSHORTCUT		= 'YBsh';

const uint32 TYPE_YABVIEW		= 1;
char * refsRec=(char*)"";

BCatalog *yabCatalog;

static bool localize = false;
static bool quitting = false;
static property_info prop_list[] = { 
	{ "YabSendString", {B_SET_PROPERTY, 0}, {B_NAME_SPECIFIER, 0}, "Send a string to MESSAGE$"},
	0 // terminate list 
}; 

const char* _L(const char* text)
{
	if(localize && yabCatalog)
		return yabCatalog->GetString(text, NULL); //B_TRANSLATE_CONTEXT);
	return text;
}

/**
 * Start the interpreter thread
 */
int32 interpreter(void *data)
{
	int argc,t;
	char **argv;
	YabInterface *yab;
	BList *myData = (BList*)data;
	argc = (int)myData->ItemAt(0);
	argv = (char**)myData->ItemAt(1);
	yab = (YabInterface*)myData->ItemAt(2);

	t = mmain(argc,argv, yab);
	return t;
}

/**
 * Constructor sets application directory, spawn the interpreter thread
 */
YabInterface::YabInterface(int argc, char **argv, const char* signature)
	:BApplication(signature)
{
	BPath AppDirectory;

	// app directory
	app_info appinfo;

	if( GetAppInfo( &appinfo) == B_OK)
	{
		BEntry ApplicationEntry( &appinfo.ref);
		BEntry ApplicationDirectoryEntry;

		if( ApplicationEntry.GetParent( &ApplicationDirectoryEntry) == B_OK)
		{
			if( AppDirectory.SetTo( &ApplicationDirectoryEntry) == B_OK)
			{
				strcpy(ApplicationDirectory, AppDirectory.Path());
				// ApplicationDirectory.SetTo(AppDirectory.Path());
			}
		}
	}

	localMessage = "";

	BList *myData = new BList(3);
	myData->AddItem((void*)argc);
	myData->AddItem((void*)argv);
	myData->AddItem((void*)this);
	myThread = spawn_thread(interpreter,"YabInterpreter",B_NORMAL_PRIORITY,(void*)myData);
	if(myThread < B_OK)
	{
		printf("Can not start thread. Out of memory or maximum thread amount reached.\n");
		printf("Exiting now \n\n");
		exit(1);
	}
	
	if(resume_thread(myThread) < B_OK)
	{
		printf("Error while starting interpreter!\n");
		printf("Exiting now \n\n");
		exit(1);
	}

	viewList = new YabList();
	yabbitmaps = new BList();
	yabcanvas = new BList();
	drawStroking = false;
	yabPattern = B_SOLID_HIGH;
	yabAlpha = 255;
	errorCode = 0;
	Roster = NULL;
	currentLineNumber = -1;
	exiting = false;
	
	for(int i=0; i<63; i++)
		mousemessagebuffer[i] = ' ';
	mousemessagebuffer[63] = '\0';

	myProps = new BPropertyInfo(prop_list);
	currentLib = "";
	lastMouseMsg = "";
}

YabInterface::~YabInterface()
{
	delete mainFileName;
	// delete song;
	// delete fopen;
	// delete fsave;
	delete viewList;
	// delete Roster;
	delete myProps;
	if(yabCatalog)
		delete yabCatalog;
	while(yabbitmaps->CountItems()>0)
	{
		int i = 0;
		BBitmap *b = (BBitmap*)yabbitmaps->RemoveItem(i);
		delete b;
	}
}

/**
 * Returns the application directory
 */
const char* YabInterface::GetApplicationDirectory()
{
	return (const char*) ApplicationDirectory;
}

status_t YabInterface::GetSupportedSuites(BMessage *msg) 
{ 
	msg->AddString("suites", "suite/vnd.yab-YabInterface"); 
	BPropertyInfo prop_info(prop_list); 
	msg->AddFlat("messages", &prop_info); 
	return BApplication::GetSupportedSuites(msg); 
} 

BHandler* YabInterface::ResolveSpecifier(BMessage *msg, int32 index, BMessage *spec, int32 form, const char *prop) 
{ 
	if (myProps->FindMatch(msg, index, spec, form, prop) >= 0) 
		return (BHandler*)this; 
	return BApplication::ResolveSpecifier(msg, index, spec, form, prop); 
}

void YabInterface::MessageReceived(BMessage *message)
{
	// message->PrintToStream();
	switch(message->what)
	{
		case B_SET_PROPERTY:
			{
				BMessage msg;
				int32 i,w;
				const char *prop;

				if(message->GetCurrentSpecifier(&i, &msg, &w, &prop) != B_BAD_SCRIPT_SYNTAX)
				{
					BString s;
					msg.FindString("name", &s);
					localMessage += "_Scripting:";
					localMessage += s;
					localMessage +="|";
				}
			}
			break;
		case B_REFS_RECEIVED:
			{
				entry_ref ref;
				if(message->FindRef("refs", 0, &ref)==B_OK)
				{
					BEntry e(&ref);
					BPath path;
					e.GetPath(&path);
					localMessage += "_RefsReceived:";
					localMessage += path.Path();
					localMessage +="|";
				}
			}
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

/**
 * The QUIT_REQUESTED message arrived. If the interpreter thread is still active,
 * kill it, otherwise exit directly.
 */
bool YabInterface::QuitRequested()
{
	exiting = true;
	return true;
}

void YabInterface::RefsReceived(BMessage *message){
	entry_ref ref;
	BString tempstr;
	if(message->FindRef("refs", 0, &ref)==B_OK)
	{
		BEntry e(&ref);
		BPath path;
		e.GetPath(&path);
		tempstr += path.Path();
		refsRec = strdup(tempstr.String());
	}
}

bool YabInterface::ExitRequested()
{
	status_t exit_val;
	thread_info t;
		 // printf("QUITDEBUG: Exit\n");
	Lock();
		 // printf("QUITDEBUG: Locked Ok\n");
	// if(!quitting)
	// {
		// 	printf("QUITDEBUG: quitting\n");
		// kill_thread(myThread);
		// 	 printf("QUITDEBUG: Kill Thread Ok\n");
	// }
		// printf("QUITDEBUG: 1\n");

	for(int i=0; i<CountWindows(); i++)
	{
		YabWindow *w = cast_as(WindowAt(i), YabWindow);
		if(w)
			if(w->Lock())
				w->Quit();
	}

	snooze(15000);
		 // printf("QUITDEBUG: 3\n");


	// BMessenger(be_app).SendMessage(new BMessage(B_QUIT_REQUESTED));
	Quit();
		  // printf("QUITDEBUG: Quit\n");
		 // printf("QUITDEBUG: wait\n");
	// wait_for_thread(myThread, &exit_val);
	// get_thread_info(myThread, &t);
	// kill_team(t.team);
		// printf("QUITDEBUG: Stopped waiting\n");
	Unlock();
		// printf("QUITDEBUG: Unlock\n");

	exit_thread(B_OK);

	return true;
}

/**
 * Open a window, add the main view.
 */
void YabInterface::OpenWindow(const BRect frame, const char* id, const char* title)
{
	YabWindow* w = new YabWindow(frame,title,id, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS);
	YabView* myView = new YabView(w->Bounds(), id, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_NAVIGABLE_JUMP);
	w->Lock();
		w->AddChild(myView);

		viewList->AddView(id, myView, TYPE_YABVIEW);
		// viewList->PrintOut();

		// w->Minimize();
		w->SetSizeLimits(10,3000,10,3000);
		w->Show();
	w->Unlock();
	w->layout = -1;
}

int YabInterface::CloseWindow(const char* view)
{
	int tmp = 0;
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BView *child, *oldchild;
			if(child = myView->ChildAt(0))
			{
				while(child)
				{
					if(is_kind_of(child, YabTabView))
					{
						for(int i = 0; i<((YabTabView*)child)->CountTabs(); i++)
						{
							YabView *t = (YabView*)((YabTabView*)child)->TabAt(i);
							RemoveView(t);
							viewList->DelView(t->Name());
						}
					}
					if(is_kind_of(child, YabBitmapView))
						yabcanvas->RemoveItem(child);
						
					// viewList->PrintOut();
					BView *subchild;
					if(subchild = child->ChildAt(0))
						while(subchild)
						{
							if(is_kind_of(subchild, YabTabView))
							{
								for(int i = 0; i<((YabTabView*)subchild)->CountTabs(); i++)
								{
									YabView *t = (YabView*)((YabTabView*)subchild)->TabAt(i);
									RemoveView(t);
									viewList->DelView(t->Name());
								}
							}
							if(viewList->GetView(subchild->Name()))
							{
								RemoveView(subchild);
								viewList->DelView(subchild->Name());
							}
							subchild = subchild->NextSibling();
						}
					if(viewList->GetView(child->Name()))
					{
						RemoveView(child);
						viewList->DelView(child->Name());
					}

					oldchild = child;
					child = child->NextSibling();

					if(is_kind_of(oldchild, YabView))
						DrawClear(oldchild->Name(), true);
					if(is_kind_of(oldchild, BMenuBar))
					{
						oldchild->Hide();
					}
					oldchild->RemoveSelf();
					delete oldchild;
				}
			}
			if(is_kind_of(myView, YabView))
				DrawClear(myView->Name(), true);
			if(is_kind_of(myView, BMenuBar))
				myView->Hide();
			myView->RemoveSelf();
			delete myView;
			viewList->DelView(view);
					// viewList->PrintOut();
			w->Quit();
			tmp = 1;
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");

	return tmp;
}

void YabInterface::View(BRect frame, const char* id, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabView *newView = new YabView(frame, id, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_NAVIGABLE_JUMP);
			if(w->layout == -1)
				newView->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				newView->SetResizingMode(w->layout);
			myView->AddChild(newView);

			viewList->AddView(id, newView, TYPE_YABVIEW);
			// viewList->PrintOut();

			newView->Invalidate();

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::BoxView(BRect frame, const char* id, const char* text, int lineType, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BBox *newBox = new BBox(frame, id, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE_JUMP);
			font_height fh;
			(be_bold_font)->GetHeight(&fh);
			float y1 = fh.ascent + fh.descent + fh.leading + 1.0;
			YabView *newView = new YabView(BRect(3,y1,frame.Width()-3,frame.Height()-3), id, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE_JUMP);
			if(w->layout == -1)
			{
				newBox->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
				newView->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			}
			else
			{
				newBox->SetResizingMode(w->layout);
				newView->SetResizingMode(w->layout);
			}
			newBox->SetLabel(text);
			switch(lineType)
			{
				case 0: newBox->SetBorder(B_NO_BORDER);
					break;
				case 1: newBox->SetBorder(B_PLAIN_BORDER);
					break;
				default: newBox->SetBorder(B_FANCY_BORDER);
					break;
			}

			newBox->AddChild(newView);
			myView->AddChild(newBox);
			viewList->AddView(id, newView, TYPE_YABVIEW);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::BoxViewSet(const char* id, const char* option, const char* value)
{
	
	YabView *myView = NULL;
	BString tmpOption(option);
	BString tmpValue(value);
	BBox *myBox = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{ 
			
				w->Lock();
				
				myBox = cast_as(myView->FindView(id), BBox);
				if(myBox)
				{
					if(tmpOption.IFindFirst("label")!=B_ERROR)
					{
						myBox->SetLabel(tmpValue);
						w->Unlock();
						return;
					}
					if(tmpOption.IFindFirst("line")!=B_ERROR)
					{
						if(tmpValue.IFindFirst("0")!=B_ERROR)
						{
							myBox->SetBorder(B_NO_BORDER);
					 		w->Unlock();
							return;
						}
						if(tmpValue.IFindFirst("1")!=B_ERROR)
						{
							myBox->SetBorder(B_PLAIN_BORDER);
							w->Unlock();
							return;
						}		
						if(tmpValue.IFindFirst("2")!=B_ERROR)
						{
							myBox->SetBorder(B_FANCY_BORDER);
							w->Unlock();
							return;
						}
				
					}
					w->Unlock();
				}
				else
				w->Unlock();
				Error(id, "BOXVIEW");
				
			}		
				
				
		}
	
	
	}
}
void YabInterface::Tab(BRect frame, const char* id, const char* mode, const char* view)
{
	tab_orientation side;		
	BString option(mode);
	if(option.IFindFirst("top")!=B_ERROR)
		side = B_TAB_TOP;	
	else if(option.IFindFirst("bottom")!=B_ERROR)
		side = B_TAB_BOTTOM;	
	else ErrorGen("Invalid Option");

	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();

			YabTabView *myTabView = new YabTabView(frame, id);

			if(w->layout == -1)
				myTabView->SetResizingMode(B_FOLLOW_ALL);
			else
				myTabView->SetResizingMode(w->layout);

			myTabView->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE|B_NAVIGABLE_JUMP);
			
			myTabView->SetOrientation(side);
			myTabView->SetTabWidth(B_WIDTH_FROM_LABEL);

			myView->AddChild(myTabView); 

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::TabAdd(const char* id, const char* tabname)
{
	YabView *myView = NULL;
	YabTabView *myTabView = NULL;

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTabView = cast_as(myView->FindView(id), YabTabView);
				if(myTabView)
				{
					BString t(id);
					t << myTabView->CountTabs()+1;

					BRect contentFrame = myTabView->Bounds();

					YabView *newView = new YabView(contentFrame, t.String(), B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_NAVIGABLE_JUMP);
					viewList->AddView(t.String(), newView, TYPE_YABVIEW);

					myTabView->AddTab(newView, tabname);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TABVIEW");
}

void YabInterface::TabDel(const char* id, int num)
{
/*
	YabView *myView = NULL;
	#ifdef BUILD_HAIKUTAB
		YabTabView *myTabView = NULL;
	#else
		BTabView *myTabView = NULL;
	#endif
	
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				#ifdef BUILD_HAIKUTAB
					myTabView = cast_as(myView->FindView(id), YabTabView);
				#else
					myTabView = cast_as(myView->FindView(id), BTabView);
				#endif
				if(myTabView)
				{
					if(num-1<0 || num-1>myTabView->CountTabs()) ErrorGen("Invalid tab number");
					BView *child, *oldchild;
					if(child = myView->ChildAt(0))
					{
						while(child)
						{
							BView *subchild;
							if(subchild = child->ChildAt(0))
								while(subchild)
								{
									#ifdef BUILD_HAIKUTAB
										if(is_kind_of(subchild, YabTabView))
										{
											for(int i = 0; i<((YabTabView*)subchild)->CountTabs(); i++)
											{
												YabView *t = (YabView*)((YabTabView*)subchild)->TabAt(i);
												RemoveView(t);
												viewList->DelView(t->Name());
											}
										}
									#endif
									if(viewList->GetView(subchild->Name()))
									{
										RemoveView(subchild);
										viewList->DelView(subchild->Name());
									}
									subchild = subchild->NextSibling();
								}
							if(viewList->GetView(child->Name()))
							{
								RemoveView(child);
								viewList->DelView(child->Name());
							}

	
							oldchild = child;
							child = child->NextSibling();

							if(is_kind_of(oldchild, YabView))
								DrawClear(oldchild->Name(), true);
							if(is_kind_of(oldchild, BMenuBar))
							{
								oldchild->Hide();
							}
							oldchild->RemoveSelf();
							delete oldchild;
						}
					}
					if(is_kind_of(myView, YabView))
						DrawClear(myView->Name(), true);
					if(is_kind_of(myView, BMenuBar))
						myView->Hide();
					BBox *box = cast_as(myView->Parent(), BBox); 
					myView->RemoveSelf();
					delete myView;
					viewList->DelView(window);
					if(box)
					{
						box->RemoveSelf();
						delete box;
					}
					#ifdef BUILD_HAIKUTAB
						WindowClear(myTabView->ItemAt(num-1)->Name());
						RemoveView(myTabView->ItemAt(num-1));
					#else
						WindowClear((myTabView->TabAt(num-1)).GetTargetView().Name());
					#endif
					myTabView->RemoveTab(num-1);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TABVIEW");
*/
}






void YabInterface::TabSet(const char* id, int num)
{
	YabView *myView = NULL;
	YabTabView *myTabView = NULL;
	
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTabView = cast_as(myView->FindView(id), YabTabView);
				if(myTabView)
				{
					if(num>0 && num<=myTabView->CountTabs())
						myTabView->Select(num-1);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TABVIEW");
}

int YabInterface::TabViewGet(const char* id)
{
	int ret = -1;
	YabView *myView = NULL;
	YabTabView *myTabView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTabView = cast_as(myView->FindView(id), YabTabView);
				if(myTabView)
				{
					ret = myTabView->Selection();
					w->Unlock();
					return ret+1;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TABVIEW");
}

void YabInterface::Launch(const char* strg)
{
	BString tst(strg);

	entry_ref *ref = new entry_ref();
	BEntry entry(strg);
	entry.GetRef(ref);
	if(entry.IsDirectory())
	{
		BMessage msg, reply;
		msg.what = B_REFS_RECEIVED;
		msg.AddRef("refs", ref);
		BMessenger("application/x-vnd.Be-TRAK").SendMessage(&msg, &reply);
		return;
	}
		
	status_t t = be_roster->Launch(ref);
	if(t != B_OK)
	{
		if(tst.FindFirst("http://") != B_ERROR || tst.FindFirst("file://") != B_ERROR || tst.FindFirst("www.") != B_ERROR)
		{
			char *link = tst.LockBuffer( tst.Length()+1 );
               		status_t result = be_roster->Launch( "text/html", 1, &link );
                	tst.UnlockBuffer(); 
		}
	}
	delete ref;
}

void YabInterface::CreateButton(BRect frame, const char* id, const char* title, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BButton* myButton = new BButton(frame,id,title,new BMessage(YABBUTTON));
			if(w->layout == -1)
				myButton->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				myButton->SetResizingMode(w->layout);
			myButton->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			// trick Haiku, resize button again
			myButton->ResizeTo(frame.Width(), frame.Height());
			myView->AddChild(myButton);
			// viewList->AddView(id, myButton, TYPE_BBUTTON);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

int YabInterface::CreateImage(BPoint coordinates, const char* FileName, const char* window)
{
	BBitmap* myBitmap = NULL;
	BFile imageFile;
	BPath imagePath;
	int ret = 0;

	if(*FileName=='/')
		imageFile.SetTo(FileName, B_READ_ONLY);
	else
		if(!strcmp(ApplicationDirectory,""))
		{
	 		if(imagePath.SetTo((const char*)ApplicationDirectory, FileName)==B_OK)
				imageFile.SetTo(imagePath.Path(), B_READ_ONLY);
		}
	if(imageFile.InitCheck()!=B_OK)
		imageFile.SetTo(FileName, B_READ_ONLY);
	
	if(imageFile.InitCheck()!=B_OK)
		return 1;

	Roster = BTranslatorRoster::Default();

	if(!Roster)
		return 2;

	BBitmapStream Stream;

	if(Roster->Translate(&imageFile, NULL, NULL, &Stream, B_TRANSLATOR_BITMAP)<B_OK)
		return 3;

	if(Stream.DetachBitmap(&myBitmap)!=B_OK)
		return 4;

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			t->command = 10;
			t->x1 = coordinates.x; t->y1 = coordinates.y;
			t->bitmap = myBitmap;
			myView->drawList->AddItem(t);
			myView->Invalidate(BRect(coordinates.x, coordinates.y, coordinates.x+myBitmap->Bounds().Width(), coordinates.y+myBitmap->Bounds().Height()));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
        {
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
				drawing_mode mode = bview->DrawingMode();
				bview->SetDrawingMode(B_OP_ALPHA);
                                bview->DrawBitmap(myBitmap, coordinates);
				bview->SetDrawingMode(mode);
                                bview->Sync();
                                b->Unlock();
				delete Roster;
				delete myBitmap;
                                return 0;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
					drawing_mode mode = bView->DrawingMode();
					bView->SetDrawingMode(B_OP_ALPHA);
                                	bView->DrawBitmap(myBitmap, coordinates);
					bView->SetDrawingMode(mode);
                                        bView->Sync();
                                        b->Unlock();

					myView->Draw(BRect(coordinates.x, coordinates.y, coordinates.x+myBitmap->Bounds().Width(), coordinates.y+myBitmap->Bounds().Height()));
                                        w->Unlock();
					delete Roster;
					delete myBitmap;
                                        return 0;
                                }
                                else
                                        ErrorGen("Unable to lock window");
			}
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
	delete Roster;
	return 0;
}

int YabInterface::CreateImage(BRect frame, const char* FileName, const char* window)
{
	int scaling = 0;
	if(frame.right == -1) scaling = 1;
	if(frame.bottom == -1) scaling = 2;
	if(frame.right == -1 && frame.bottom == -1) scaling = 3;

	BBitmap* myBitmap = NULL;
	BFile ImageFile;
	BPath ImagePath;
	int ret = 0;

	if(*FileName=='/')
		ImageFile.SetTo(FileName, B_READ_ONLY);
	else
		if(!strcmp(ApplicationDirectory,""))
		{
	 		if( ImagePath.SetTo((const char*)ApplicationDirectory, FileName) == B_OK)
				ImageFile.SetTo(ImagePath.Path(), B_READ_ONLY);
		}

	if(ImageFile.InitCheck()!=B_OK)
		ImageFile.SetTo(FileName, B_READ_ONLY);
	
	if(ImageFile.InitCheck()!=B_OK)
		return 1;

	Roster = BTranslatorRoster::Default();

	if(!Roster)
		return 2;

	BBitmapStream Stream;

	if(Roster->Translate(&ImageFile, NULL, NULL, &Stream, B_TRANSLATOR_BITMAP) < B_OK)
		return 3;

	if(Stream.DetachBitmap(&myBitmap) != B_OK)
		return 4;

	BRect newframe;
	switch(scaling)
	{
		case 1:
		{
			BRect t(myBitmap->Bounds());
			double width;
			newframe = frame;
			width = (t.right-t.left)*((frame.bottom-frame.top)/(t.bottom-t.top));
			newframe.right = newframe.left+width;
		}
		break;
		case 2:
		{
			BRect t(myBitmap->Bounds());
			double height;
			newframe = frame;
			height = (t.bottom-t.top)*((frame.right-frame.left)/(t.right-t.left));
			newframe.bottom = newframe.top+height;
		}
		break;
		case 3:	newframe = myBitmap->Bounds();
		break;
		default: newframe = frame;
	}

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			t->command = 11;
			t->x1 = newframe.left; t->y1 = newframe.top;
			t->x2 = newframe.right; t->y2 = newframe.bottom;
			t->bitmap = myBitmap;
			myView->drawList->AddItem(t);
			myView->Invalidate(newframe);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
        {
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                                drawing_mode mode = bview->DrawingMode();
                                bview->SetDrawingMode(B_OP_ALPHA);
                                bview->DrawBitmap(myBitmap, newframe);
                                bview->SetDrawingMode(mode);
                                bview->Sync();
                                b->Unlock();
                                delete Roster;
								delete myBitmap;
                                return 0;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                                        drawing_mode mode = bView->DrawingMode();
                                        bView->SetDrawingMode(B_OP_ALPHA);
										bView->DrawBitmap(myBitmap, newframe);
                                        bView->SetDrawingMode(mode);
                                        bView->Sync();
                                        b->Unlock();

                                        myView->Draw(newframe);
                                        w->Unlock();
                                        delete Roster;
										delete myBitmap;
                                        return 0;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        }
                }
                Error(window, "VIEW, BITMAP or CANVAS");
        } 
	delete Roster;
	return 0;
}

int YabInterface::CreateSVG(BRect frame, const char* FileName, const char* window)
{
#ifdef LIBBSVG
	BPath path;
	BString file;
	file.SetTo(FileName);
	if(FileName[0]!='/')
	{	
		if(!strcmp(ApplicationDirectory,""))
		{
			if(path.SetTo((const char*)ApplicationDirectory, FileName) == B_OK)
				file.SetTo(path.Path());
		}
	}
	int ret = 0;

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView); // untested!
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BSVGView *mySVG = new BSVGView(frame,"svgview",0);
			mySVG->SetViewColor(myView->ViewColor());
			mySVG->SetScaleToFit(true);
			if(w->layout == -1)
				mySVG->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				mySVG->SetResizingMode(w->layout);
			mySVG->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);

			if(mySVG->LoadFromFile(file.String())!=B_OK)
				ret = 1;
			else
				myView->AddChild(mySVG);
			myView->Invalidate();
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
	return ret;
#else
	return 2;
#endif
}

void YabInterface::StatusBar(BRect frame, const char* id, const char* label1, const char* label2, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BStatusBar *bar = new BStatusBar(frame, id, label1, label2);
			bar->SetBarHeight((float)frame.Height()-(be_plain_font)->Size()-5);
                        if(w->layout == -1)
                                bar->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
                        else
                                bar->SetResizingMode(w->layout);
			myView->AddChild(bar);
			bar->Draw(frame);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::StatusBarSet(const char* id, const char* label1, const char* label2, double state)
{
	YabView *myView = NULL;
	BStatusBar *myBar = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myBar = cast_as(myView->FindView(id), BStatusBar);
				if(myBar)
				{
					myBar->Reset();
					myBar->Update((float)state, label1, label2);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
			else
				ErrorGen("Unable to lock window");
		}
	}
	Error(id, "STATUSBAR");
}

void YabInterface::StatusBarSet(BRect frame, const char* id, const char* view)
{
 // empty!
}

void YabInterface::StatusBarSet(const char* id, int r, int g, int b)
{
	YabView *myView = NULL;
	BStatusBar *myBar = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myBar = cast_as(myView->FindView(id), BStatusBar);
				if(myBar)
				{
					rgb_color rgb = {r,g,b,255};
					myBar->SetBarColor(rgb);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
			else
				ErrorGen("Unable to lock window");
		}
	}
	Error(id, "STATUSBAR");
}

void YabInterface::CreateMenu(const char* menuhead, const char* menuitem, const char *shortcut, const char* window)
{
	char myShortcut;
	int32 modifiers = 0;
	BString t(shortcut);
	if(t.Length()>1)
	{
		myShortcut = shortcut[t.Length()-1];
		if(t.IFindFirst("s")!=B_ERROR && t.IFindFirst("s")<t.Length()-1)
			modifiers = modifiers|B_SHIFT_KEY;
		if(t.IFindFirst("c")!=B_ERROR && t.IFindFirst("c")<t.Length()-1)
			modifiers = modifiers|B_CONTROL_KEY;
		if(t.IFindFirst("o")!=B_ERROR && t.IFindFirst("o")<t.Length()-1)
			modifiers = modifiers|B_OPTION_KEY;
	}
	else
		myShortcut = shortcut[0];
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BMenuBar *menubar;
			YabMenu *menu = NULL;
			BMenuItem *item = NULL;
			w->Lock();
			menubar = cast_as(myView->FindView("menubar"), BMenuBar);
			if(menubar == NULL)
			{
				menubar = new BMenuBar(myView->Bounds(),"menubar");
				myView->AddChild(menubar);
			}
			for(int i=0; i<menubar->CountItems(); i++)
				if(!strcmp( menubar->ItemAt(i)->Label(), menuhead))
					menu = cast_as(menubar->SubmenuAt(i), YabMenu);
			if(menu == NULL)
			{
				menu = new YabMenu(menuhead);
				menubar->AddItem((BMenu*)menu);
			}
			if(!strcmp(menuitem,"--"))
				menu->AddItem(new BSeparatorItem());
			else
				menu->AddItem(new BMenuItem(menuitem, new BMessage(YABMENU), myShortcut, modifiers));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::CreateTextControl(BRect frame, const char* id, const char* label, const char* text, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{ 
			w->Lock();
			BTextControl *textControl = new BTextControl(frame, id ,label, text, new BMessage(YABTEXTCONTROL));
			textControl->SetDivider(textControl->StringWidth(label)+5.0);
			
			if(w)
			{
				if(w->layout == -1)
					textControl->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
				else
					textControl->SetResizingMode(w->layout);
			}
			textControl->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(textControl);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::CreateCheckBox(double x, double y, const char* id, const char* label, int isActivated, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w) 
		{
			w->Lock();
			BRect frame(x,y,x+1,y+1);
			BCheckBox *checkBox = new BCheckBox(frame, id, label, new BMessage(YABCHECKBOX));
			checkBox->ResizeToPreferred();
			if(isActivated>0) checkBox->SetValue(B_CONTROL_ON);
			if(w->layout == -1)
				checkBox->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				checkBox->SetResizingMode(w->layout);
			checkBox->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(checkBox);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::CreateRadioButton(double x, double y, const char* groupID, const char* label, int isActivated, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BRect frame(x,y,x+1,y+1);
			BRadioButton *radio = new BRadioButton(frame, groupID, label, new BMessage(YABRADIOBUTTON));
			radio->ResizeToPreferred();
			if(isActivated>0) radio->SetValue(B_CONTROL_ON);
			if(w->layout == -1)
				radio->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				radio->SetResizingMode(w->layout);
			radio->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(radio);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::CreateListBox(BRect frame, const char* title, int scrollbar, const char* window)
{
	YabView *myView = cast_as(((BView*)viewList->GetView(window)), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			uint32 resizeMode;

			w->Lock();

			if(scrollbar == 3 || scrollbar == 1) frame.right -= B_V_SCROLL_BAR_WIDTH;
			if(scrollbar>2) frame.bottom -= B_H_SCROLL_BAR_HEIGHT;

			BListView  *list = new BListView(frame,title);
			if(w->layout == -1)
				resizeMode = B_FOLLOW_ALL;
			else
				resizeMode = w->layout;
			list->SetResizingMode(resizeMode);
			list->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			list->SetSelectionMessage(new BMessage(YABLISTBOXSELECT));
			list->SetInvocationMessage(new BMessage(YABLISTBOXINVOKE));
			switch(scrollbar)
			{
				case 3:  // both
					myView->AddChild(new BScrollView("scroll_list", list, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, true, true));
					break;
				case 2:  // horizontal
					myView->AddChild(new BScrollView("scroll_list", list, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, true, false));
					break;
				case 0:  // none
					myView->AddChild(list);
					break;
				default: // vertical is default
					myView->AddChild(new BScrollView("scroll_list", list, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, false, true));
					break;
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::CreateDropBox(BRect frame, const char* title, const char* label, const char* window)
{ 
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BPopUpMenu *dropmenu = new BPopUpMenu("");
			BMenuField *drop = new BMenuField(frame,title,label, dropmenu, true);
			drop->SetDivider(drop->StringWidth(label)+5.0);
			if(w->layout == -1)
				drop->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				drop->SetResizingMode(w->layout);
			drop->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(drop);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::CreateItem(const char* id, const char* item)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(id), BMenuField);
				if(myMenuField)
				{
					BPopUpMenu *myPopup = cast_as(myMenuField->Menu(), BPopUpMenu);
					if(myPopup) 
					{
						if(!strcmp(item,"--"))
							myPopup->AddItem(new BSeparatorItem());
						else
						{
							BMenuItem *tmp = new BMenuItem(item, new BMessage(YABDROPBOX));
							myPopup->AddItem(tmp);
							if(myPopup->CountItems()==1) // first Item
								tmp->SetMarked(true);	
						}
						w->Unlock();
						return;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(id, "DROPBOX");
}

void YabInterface::RemoveItem(const char* title, const char* item)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BListView *myList = cast_as(myView->FindView(title), BListView);
				if(myList) 
				{
					for(int i=0; i<myList->CountItems(); i++)
					{
						BStringItem *stritem = cast_as(myList->ItemAt(i), BStringItem);
						if(stritem && !strcmp(stritem->Text(), item))
						{
							myList->RemoveItem(i);
							w->Unlock();
							return;
						}
					}
					w->Unlock();
					ErrorGen("Item not found");
				}
				w->Unlock();
			}
		}
	}
	Error(title, "DROPBOX");
}

void YabInterface::ClearItems(const char* title)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BListView *myList = cast_as(myView->FindView(title), BListView);
				if(myList) 
				{
					myList->RemoveItems(0,myList->CountItems());
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "DROPBOX");
}

void YabInterface::DrawText(BPoint coordinates, const char* text, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			t->command = 0;
			t->x1 = coordinates.x; t->y1 = coordinates.y;
			t->chardata = strdup(text);
			myView->drawList->AddItem(t);

			font_height height;
			myView->GetFontHeight(&height);
			BRect bbox;
			bbox.left = coordinates.x;
			bbox.top = coordinates.y - height.ascent;
			bbox.right = coordinates.x + myView->StringWidth(text);
			bbox.bottom = coordinates.y + height.descent;

			BFont tFont;
			myView->GetFont(&tFont);
			if(tFont.Rotation() == 0.0 && tFont.Shear() == 90.0)
				myView->Invalidate(bbox);
			else
				myView->Invalidate();
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                               	bview->DrawString(text, coordinates);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                               		bView->DrawString(text, coordinates);
                                        bView->Sync();
                                        b->Unlock();

					font_height height;
					bView->GetFontHeight(&height);
					BRect bbox;
					bbox.left = coordinates.x;
					bbox.top = coordinates.y - height.ascent;
					bbox.right = coordinates.x + bView->StringWidth(text);
					bbox.bottom = coordinates.y + height.descent;

                                        myView->Draw(bbox);
                                        w->Unlock();
                                        return;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        } 
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawRect(BRect frame, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			if(drawStroking)
				t->command = 4;
			else
				t->command = 5;
			t->x1 = frame.left; t->y1 = frame.top;
			t->x2 = frame.right; t->y2 = frame.bottom;
			t->p = yabPattern; 
			myView->drawList->AddItem(t);
			myView->Invalidate(frame);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
				if(drawStroking)
                               		bview->StrokeRect(frame, yabPattern);
				else
                               		bview->FillRect(frame, yabPattern);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
					if(drawStroking)
                               			bView->StrokeRect(frame, yabPattern);
					else
                               			bView->FillRect(frame, yabPattern);
                                        bView->Sync();
                                        b->Unlock();

                                        myView->Draw(frame);
                                        w->Unlock();
                                        return;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        } 
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawClear(const char* window, bool isExit)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			rgb_color lowcolor = myView->LowColor();
			rgb_color highcolor = myView->HighColor();
			BFont lastfont;
			myView->GetFont(&lastfont);
			while(myView->drawList->CountItems()>0)
        		{
		                YabDrawing *t = (YabDrawing*)myView->drawList->LastItem();
               			myView->drawList->RemoveItem(t);
		                if(t->command == 0) delete [] t->chardata;
		                if(t->command == 10 || t->command == 11) delete t->bitmap;
               			delete t;
        		}
			YabDrawing *t1 = new YabDrawing();
			t1->command = 7;
			t1->r = lowcolor.red; t1->g = lowcolor.green;
			t1->b = lowcolor.blue; t1->alpha = yabAlpha;
			myView->drawList->AddItem(t1);
			YabDrawing *t2 = new YabDrawing();
			t2->command = 6;
			t2->r = highcolor.red; t2->g = highcolor.green;
			t2->b = highcolor.blue; t2->alpha = yabAlpha;
			myView->drawList->AddItem(t2);
			YabDrawing *t3 = new YabDrawing();
			t3->command = 12;
			t3->font = lastfont;
			myView->drawList->AddItem(t3);
			myView->Invalidate();
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		if(!isExit) Error(window, "VIEW");
}

void YabInterface::DrawDot(double x, double y, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			t->command = 1;
			t->x1 = x; t->y1 = y;
			t->x2 = x; t->y2 = y;
			t->p = yabPattern;
			myView->drawList->AddItem(t);
			myView->Invalidate(BRect(x,y,x,y));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                                bview->StrokeLine(BPoint(x,y), BPoint(x,y), yabPattern);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                                        bView->StrokeLine(BPoint(x,y), BPoint(x,y), yabPattern);
                                        bView->Sync();
                                        b->Unlock();

                                        myView->Draw(BRect(x,y,x,y));
                                        w->Unlock();
                                        return;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        }  
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawLine(double x1, double y1, double x2, double y2, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			t->command = 1;
			t->x1 = x1; t->y1 = y1;
			t->x2 = x2; t->y2 = y2;
			t->p = yabPattern;
			myView->drawList->AddItem(t);
			double minx1 = x1<x2?x1:x2; double minx2 = x1<x2?x2:x1;
			double miny1 = y1<y2?y1:y2; double miny2 = y1<y2?y2:y1;
			myView->Invalidate(BRect(minx1,miny1,minx2,miny2));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
		for(int i=0; i<yabbitmaps->CountItems(); i++)
		{
			BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
			BView *bview = b->FindView(window);
			if(bview)
			{
				b->Lock();
				bview->StrokeLine(BPoint(x1,y1), BPoint(x2,y2), yabPattern);
				bview->Sync();
				b->Unlock();
				return;
			}
		}
		for(int i=0; i<yabcanvas->CountItems(); i++)
		{
			YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
			if(!strcmp(myView->Name(), window))
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					w->Lock();
					BBitmap *b = myView->GetBitmap();
					BView *bView = myView->GetBitmapView();
					b->Lock();
					bView->StrokeLine(BPoint(x1,y1), BPoint(x2,y2), yabPattern);
					bView->Sync();
					b->Unlock();

					double minx1 = x1<x2?x1:x2; double minx2 = x1<x2?x2:x1;
					double miny1 = y1<y2?y1:y2; double miny2 = y1<y2?y2:y1;
					// myView->Invalidate(BRect(minx1,miny1,minx2,miny2));
					myView->Draw(BRect(minx1,miny1,minx2,miny2));
					w->Unlock();
					return;
				}
				else
					ErrorGen("Unable to lock window");
			}
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawCircle(double x, double y, double r, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			if(drawStroking)
				t->command = 2;
			else
				t->command = 3;
			t->x1 = x; t->y1 = y;
			t->x2 = r; t->y2 = r;
			t->p = yabPattern;
			myView->drawList->AddItem(t);
			myView->Invalidate(BRect(x-r,y-r,x+r,y+r));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                                if(drawStroking)
                                        bview->StrokeEllipse(BPoint(x,y), r, r, yabPattern);
                                else
                                        bview->FillEllipse(BPoint(x,y), r, r, yabPattern);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                                        if(drawStroking)
                                        	bView->StrokeEllipse(BPoint(x,y), r, r, yabPattern);
                                        else
                                        	bView->FillEllipse(BPoint(x,y), r, r, yabPattern);
                                        bView->Sync();
                                        b->Unlock();

					myView->Draw(BRect(x-r,y-r,x+r,y+r));
                                        w->Unlock();
                                        return;
                                }  
                                else
                                        ErrorGen("Unable to lock window");
                        } 
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawEllipse(double x, double y, double r1, double r2, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			if(drawStroking)
				t->command = 2;
			else
				t->command = 3;
			t->x1 = x; t->y1 = y;
			t->x2 = r1; t->y2 = r2;
			t->p = yabPattern;
			myView->drawList->AddItem(t);
			myView->Invalidate(BRect(x-r1,y-r2,x+r1,y+r2));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                                if(drawStroking)
                                        bview->StrokeEllipse(BPoint(x,y), r1, r2, yabPattern);
                                else
                                        bview->FillEllipse(BPoint(x,y), r1, r2, yabPattern);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                                        if(drawStroking)
                                                bView->StrokeEllipse(BPoint(x,y), r1, r2, yabPattern);
                                        else
                                                bView->FillEllipse(BPoint(x,y), r1, r2, yabPattern);
                                        bView->Sync();
                                        b->Unlock();

					myView->Draw(BRect(x-r1,y-r2,x+r1,y+r2));
                                        w->Unlock();
                                        return;
                                }  
                                else
                                        ErrorGen("Unable to lock window");
                        } 
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawCurve(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, const char* window)
{
	double invx1 = x1<x2?x1:x2; invx1 = invx1<x3?invx1:x3; invx1 = invx1<x4?invx1:x4;
	double invx2 = x1>x2?x1:x2; invx2 = invx2>x3?invx2:x3; invx2 = invx2>x4?invx2:x4;
	double invy1 = y1<y2?y1:y2; invy1 = invy1<y3?invy1:y3; invy1 = invy1<y4?invy1:y4;
	double invy2 = y1>y2?y1:y2; invy2 = invy2>y3?invy2:y3; invy2 = invy2>y4?invy2:y4;

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabDrawing *t = new YabDrawing();
			if(drawStroking)
				t->command = 8;
			else
				t->command = 9;
			t->x1 = x1; t->y1 = y1;
			t->x2 = x2; t->y2 = y2;
			t->x3 = x3; t->y3 = y3;
			t->x4 = x4; t->y4 = y4;
			t->p = yabPattern;
			myView->drawList->AddItem(t);
			myView->Invalidate(BRect(invx1,invy1,invx2,invy2));
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                                BPoint p[4];
                                p[0].Set(x1,y1);
                                p[1].Set(x2,y2);
                                p[2].Set(x3,y3);
                                p[3].Set(x4,y4);

				if(drawStroking)
				{
                                	bview->SetPenSize(1.01);
                                	bview->StrokeBezier(p, yabPattern);
                                	bview->SetPenSize(1.0);
				}
				else
                                	bview->FillBezier(p, yabPattern);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                                	BPoint p[4];
                                	p[0].Set(x1,y1);
                                	p[1].Set(x2,y2);
                                	p[2].Set(x3,y3);
                                	p[3].Set(x4,y4);

					if(drawStroking)
					{
                                		bView->SetPenSize(1.01);
                                		bView->StrokeBezier(p, yabPattern);
                                		bView->SetPenSize(1.0);
					}
					else
                                		bView->FillBezier(p, yabPattern);
                                        bView->Sync();
                                        b->Unlock();

					myView->Draw(BRect(invx1,invy1,invx2,invy2));
                                        w->Unlock();
                                        return;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        } 
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::CreateText(double x, double y, const char* id, const char* text, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			double h,b;
			b = be_plain_font->StringWidth(text)+1;
			h = be_plain_font->Size();
			BStringView *s = new BStringView(BRect(x,y-3,x+b,y+h-3), id, text);
			s->ResizeToPreferred();
			if(w->layout == -1)
				s->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				s->SetResizingMode(w->layout);
			s->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);
			myView->AddChild(s);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::Text2(BRect frame, const char* id, const char* text, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BStringView *s = new BStringView(frame, id, text);
			if(w->layout == -1)
				s->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				s->SetResizingMode(w->layout);
			s->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);
			myView->AddChild(s);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::TextAlign(const char* txt, const char* option)
{
	BString tmp(option);
	alignment align;
	if(tmp.IFindFirst("align-left")!=B_ERROR)
		align = B_ALIGN_LEFT;
	else if(tmp.IFindFirst("align-center")!=B_ERROR)
		align = B_ALIGN_CENTER;
	else if(tmp.IFindFirst("align-right")!=B_ERROR)
		align = B_ALIGN_RIGHT;
	else 
		ErrorGen("Unknown option");

	YabView *myView = NULL;
	BStringView *myStringView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myStringView = cast_as(myView->FindView(txt), BStringView);
				if(myStringView)
				{
					myStringView->SetAlignment(align);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(txt, "TEXT");
}

void YabInterface::Slider(BRect frame, const char* id, const char* title, int min, int max, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BSlider *mySlider = new BSlider(frame, id, title, new BMessage(YABSLIDER), min, max);
			if(w->layout == -1)
				mySlider->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				mySlider->SetResizingMode(w->layout);
			mySlider->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(mySlider);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::Slider(BRect frame, const char* id, const char* title, int min, int max, const char* option, const char* view)
{
	BString tmp(option);
	bool thumb = true, orient = true;
	if(tmp.IFindFirst("vertical")!=B_ERROR)
		orient = false;
	if(tmp.IFindFirst("triangle")!=B_ERROR)
		thumb = false;
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BSlider *mySlider = new BSlider(frame, id, title, new BMessage(YABSLIDER), min, max);
			if(w->layout == -1)
				mySlider->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				mySlider->SetResizingMode(w->layout);
			mySlider->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			if(!orient)
			{
				mySlider->SetOrientation(B_VERTICAL);
				mySlider->SetBarThickness(10);
			}
			if(!thumb) mySlider->SetStyle(B_TRIANGLE_THUMB);
			myView->AddChild(mySlider);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::SetSlider(const char* id, const char* label1, const char* label2)
{
	YabView *myView = NULL;
	BSlider *mySlider = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySlider = cast_as(myView->FindView(id), BSlider);
				if(mySlider)
				{
					mySlider->SetLimitLabels(label1, label2);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "SLIDER");
}

void YabInterface::SetSlider(const char* id, const char* bottomtop, int count)
{
	hash_mark_location location = B_HASH_MARKS_BOTH;
	BString tmp(bottomtop);
	if(tmp.IFindFirst("none")!=B_ERROR)
		location = B_HASH_MARKS_NONE;
	if(tmp.IFindFirst("left")!=B_ERROR)
		location = B_HASH_MARKS_LEFT;
	if(tmp.IFindFirst("right")!=B_ERROR)
		location = B_HASH_MARKS_RIGHT;
	if(tmp.IFindFirst("top")!=B_ERROR)
		location = B_HASH_MARKS_TOP;
	if(tmp.IFindFirst("bottom")!=B_ERROR)
		location = B_HASH_MARKS_BOTTOM;
	YabView *myView = NULL;
	BSlider *mySlider = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySlider = cast_as(myView->FindView(id), BSlider);
				if(mySlider)
				{
					mySlider->SetHashMarks(location);
					mySlider->SetHashMarkCount(count);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "SLIDER");
}

void YabInterface::SetSlider(const char* id, const char* part, int r, int g, int b)
{
	bool barcolor = true;
	BString tmp(part);
	if(tmp.IFindFirst("fillcolor"))
		barcolor = false;

	YabView *myView = NULL;
	BSlider *mySlider = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySlider = cast_as(myView->FindView(id), BSlider);
				if(mySlider)
				{
					rgb_color rgb = {r,g,b,255};
					if(barcolor)
						mySlider->SetBarColor(rgb);
					else
						mySlider->UseFillColor(true,&rgb);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "SLIDER");
}

void YabInterface::SetSlider(const char* id, int value)
{
	YabView *myView = NULL;
	BSlider *mySlider = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySlider = cast_as(myView->FindView(id), BSlider);
				if(mySlider)
				{
					mySlider->SetValue(value);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "SLIDER");
}

void YabInterface::SetOption(const char* id, const char* option, const char* value)
{
	bool label = false;
	BString tmpOption(option);
	if(tmpOption.IFindFirst("label")!=B_ERROR)
		label = true;

	if(!label)
		ErrorGen("Unknown option");
		
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BControl *target = cast_as(myView->FindView(id), BControl);
				if(target)
				{
					target->SetLabel(value);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "CONTROL");
}

void YabInterface::SetOption(const char* id, const char* option, int r, int g, int b)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BView *target = myView->FindView(id);
				if(target)
				{
					rgb_color rgb = {r,g,b,0};
					target->SetViewColor(rgb);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "VIEW");
}

void YabInterface::SetOption(const char* id, const char* option)
{
	BString tmpOption(option);
	if(tmpOption.IFindFirst("auto-resize")==B_ERROR)
		ErrorGen("Unknown option");

	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BView *target = myView->FindView(id);
				if(target)
				{
					target->ResizeToPreferred();
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	ErrorGen("View not found");
}

void YabInterface::SetOption(const char* id, const char* option, int value)
{
	BString tmpOption(option);
	bool isFocus = false;
	bool isEnabled = false;
	bool isVisible = false;

	if(tmpOption.IFindFirst("enabled")!=B_ERROR)
		isEnabled = true;
	if(tmpOption.IFindFirst("focus")!=B_ERROR)
		isFocus = true;
	if(tmpOption.IFindFirst("visible")!=B_ERROR)
		isVisible = true;

	if(!isFocus && !isEnabled && !isVisible)
		ErrorGen("Unknown option");

	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BView *target = myView->FindView(id);
				if(target)
				{
					if(isEnabled)
					{
						BControl *myControl = cast_as(target, BControl);
						if(myControl)
							myControl->SetEnabled(value);
						else
						{
							BMenuField *myMenu = cast_as(target, BMenuField);
							if(myMenu)
								myMenu->SetEnabled(value);
							else
								Error(id, "CONTROL or DROPBOX");
						}
					}
					if(isFocus)
					{
						target->MakeFocus(value);
					}
					if(isVisible)
					{
						BControl *myControl = cast_as(target, BControl);
						if(myControl)
						{
							if(value)
							{
								if (myControl->IsHidden())
								{
									myControl->Show();
								}
							}
							else
							{
								if (!myControl->IsHidden())
								{
									myControl->Hide();
								}
							}
							
						}
						
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	ErrorGen("View not found");
}

void YabInterface::DropZone(const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
		myView->dropZone = true;
	else
		Error(view, "VIEW");
}

void YabInterface::ColorControl(double x, double y, const char* id, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BColorControl *myCControl = new BColorControl(BPoint(x,y), B_CELLS_32x8, 2, id, new BMessage(YABCOLORCONTROL),false);
			if(w->layout == -1)
				myCControl->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				myCControl->SetResizingMode(w->layout);
			myCControl->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(myCControl);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::ColorControl(const char* id, int r, int g, int b)
{
	YabView *myView = NULL;
	BColorControl *myCControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myCControl = cast_as(myView->FindView(id), BColorControl);
				if(myCControl)
				{
					rgb_color t = {r,g,b,255};
					myCControl->SetValue(t);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "COLORCONTROL");
}

void YabInterface::TextControl(const char* id, const char* text)
{
	YabView *myView = NULL;
	BTextControl *myTControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTControl = cast_as(myView->FindView(id), BTextControl);
				if(myTControl)
				{
					myTControl->SetText(text);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TEXTCONTROL");
}

void YabInterface::TextControl(const char* id, int mode)
{
	YabView *myView = NULL;
	BTextControl *myTControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTControl = cast_as(myView->FindView(id), BTextControl);
				if(myTControl)
				{
					BTextView *myTView = myTControl->TextView();
					
					switch(mode)
					{
						case 1:
						myTView->HideTyping(true);
						break;
																
						default:
						myTView->HideTyping(false);
						
						break;
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TEXTCONTROL");
}

void YabInterface::TextControl(const char* id, const char* option, const char* value)
{
	YabView *myView = NULL;
	BString tmpOption(option);
	BString tmpValue(value);
	BTextControl *myTControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				int32 x=0;
				myTControl = cast_as(myView->FindView(id), BTextControl);
				if(myTControl)
				{
					
					BTextView *myTView = myTControl->TextView();
					if(tmpOption.IFindFirst("focus")!=B_ERROR)
					{
						if(tmpValue.IFindFirst("true")!=B_ERROR)
						{
							bool focused = true;
							int32 ofset=0;
							myTControl -> MakeFocus(focused);
							myTView-> Select(ofset,ofset); 
						}
						if(tmpValue.IFindFirst("false")!=B_ERROR)
						{
							bool focused = false;
							myTControl -> MakeFocus(focused);
						}
					}
					if(tmpOption.IFindFirst("Curser")!=B_ERROR)
					{
						const char* str_int = tmpValue.String();
						bool focused = true;
						int32 ofset=0;
						ofset= atoi(str_int);
						myTControl -> MakeFocus(focused);
						myTView-> Select(ofset,ofset); 
						}
					if(tmpOption.IFindFirst("type")!=B_ERROR)
					{
						if(tmpValue.IFindFirst("number")!=B_ERROR)
						{
							for (x=0;x<48; x++)
						{
							myTView->DisallowChar(x);
						}
							for (x=58;x<128; x++)
						{
							myTView->DisallowChar(x);
						}
							x=46;
							myTView-> AllowChar(x);
						}
						if(tmpValue.IFindFirst("alphanumeric")!=B_ERROR)
						{	
							for (x=0;x<128; x++)
						{
							myTView->AllowChar(x);
						}
							
						}
						
					}
					if(tmpOption.IFindFirst("align")!=B_ERROR)
					{
						if(tmpValue.IFindFirst("right")!=B_ERROR)
						{
							myTControl->SetAlignment(B_ALIGN_LEFT,B_ALIGN_RIGHT);
						}
						if(tmpValue.IFindFirst("center")!=B_ERROR)
						{
							myTControl->SetAlignment(B_ALIGN_LEFT,B_ALIGN_CENTER);
						}
						if(tmpValue.IFindFirst("left")!=B_ERROR)
						{
							myTControl->SetAlignment(B_ALIGN_LEFT,B_ALIGN_LEFT);
						}
					}
					if(tmpOption.IFindFirst("length")!=B_ERROR)
					{
						const char* str_int = tmpValue.String();
						int i = atoi(str_int);
						if (i>0)
						{
							myTView->SetMaxBytes(i);
							myTView->SetFontAndColor(be_fixed_font);
						}
						if (i=0)
						{
							ErrorGen("Bad length");
						}
					}
					if(tmpOption.IFindFirst("exclude")!=B_ERROR)
					{
						int i;
						for (i=0; i<= tmpValue.CountChars();i++)
						{
							x=tmpValue.ByteAt(i);
							myTView->DisallowChar(x);
						}						
					}	
					if(tmpOption.IFindFirst("include")!=B_ERROR)
					{
						int i;
						for (i=0; i<= tmpValue.CountChars();i++)
						{
							x=tmpValue.ByteAt(i);
							myTView->AllowChar(x);
						}						
					}	
				}
				w->Unlock();
				return;
			}
			w->Unlock();
		}
	}
Error(id, "TEXTCONTROL");
}

void YabInterface::TextControl(const char* id)
{
	YabView *myView = NULL;
	BTextControl *myTControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTControl = cast_as(myView->FindView(id), BTextControl);
				if(myTControl)
				{
					myTControl->SetText("");
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TEXTCONTROL");
}


void YabInterface::CreateAlert(const char* text, const char* button1, const char* option)
{
	alert_type tmp;
	BString typ(option);
	tmp = B_EMPTY_ALERT;
	if(typ.IFindFirst("info")!=B_ERROR) tmp = B_INFO_ALERT;
	else if(typ.IFindFirst("idea")!=B_ERROR) tmp = B_IDEA_ALERT;
	else if(typ.IFindFirst("warning")!=B_ERROR) tmp = B_WARNING_ALERT;
	else if(typ.IFindFirst("stop")!=B_ERROR) tmp = B_STOP_ALERT;
	
	(new BAlert("Alert!",text,button1,NULL,NULL,B_WIDTH_AS_USUAL,tmp))->Go();
}

const char* YabInterface::LoadFilePanel(const char *mode, const char* title, const char* directory)
{
	int myMode = -1;
	BString opt(mode);
	if(opt.IFindFirst("Load-File")!=B_ERROR)
		myMode = 0;
	if(opt.IFindFirst("Save-File")!=B_ERROR)
		myMode = 1;
	if(opt.IFindFirst("Load-Directory")!=B_ERROR)
		myMode = 2;
	if(opt.IFindFirst("Load-File-and-Directory")!=B_ERROR)
		myMode = 3;
	if(myMode == -1) ErrorGen("Invalid Option");

	YabFilePanel tmp;
	BPath path;
	BString myTitle(title);
	BEntry *entry = tmp.MyFilePanel(myTitle.String(),directory, "", myMode);
	entry->GetPath(&path);
	if(myMode != 1 && !entry->Exists()) 
		loadPanel[0] = '\0';
	else
	{
		if(path.InitCheck() == B_OK) 
			strcpy(loadPanel,path.Path());
		else
			loadPanel[0] = '\0';
	}
			
	delete entry;
	
	return (const char*)loadPanel;
}

const char* YabInterface::SaveFilePanel(const char *mode, const char* title, const char* directory, const char* filename)
{
	int myMode = -1;
	BString opt(mode);
	if(opt.IFindFirst("Load-File")!=B_ERROR)
		myMode = 0;
	if(opt.IFindFirst("Save-File")!=B_ERROR)
		myMode = 1;
	if(opt.IFindFirst("Load-Directory")!=B_ERROR)
		myMode = 2;
	if(opt.IFindFirst("Load-File-and-Directory")!=B_ERROR)
		myMode = 3;
	if(myMode == -1) ErrorGen("Invalid Option");

	YabFilePanel tmp;
	BPath path;
	BString myTitle(title);
	BEntry *entry = tmp.MyFilePanel(myTitle.String(),directory, filename, myMode);
	entry->GetPath(&path);
	if(myMode != 1 && !entry->Exists()) 
		loadPanel[0] = '\0';
	else
	{
		if(path.InitCheck() == B_OK) 
			strcpy(loadPanel,path.Path());
		else
			loadPanel[0] = '\0';
	}
	delete entry;
	
	return (const char*)loadPanel;
}

void YabInterface::SetLayout(const char* layout, const char* window) 
{
	BString tmp(layout);
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			if(tmp.IFindFirst("standard")!=B_ERROR)
				w->layout = -1;
			else if(tmp.IFindFirst("all")!=B_ERROR)
				w->layout = B_FOLLOW_ALL;
			else if(tmp.IFindFirst("none")!=B_ERROR)
				w->layout = B_FOLLOW_NONE;
			else
			{
				uint32 horizontal, vertical;
				if(tmp.IFindFirst("h-center")!=B_ERROR)
					horizontal = B_FOLLOW_H_CENTER;
				else if((tmp.IFindFirst("left")!=B_ERROR)&&(tmp.IFindFirst("right")!=B_ERROR))
					horizontal = B_FOLLOW_LEFT_RIGHT;
				else if(tmp.IFindFirst("right")!=B_ERROR)
					horizontal = B_FOLLOW_RIGHT;
				else
					horizontal = B_FOLLOW_LEFT;

				if(tmp.IFindFirst("v-center")!=B_ERROR)
					vertical = B_FOLLOW_V_CENTER;
				else if((tmp.IFindFirst("top")!=B_ERROR)&&(tmp.IFindFirst("bottom")!=B_ERROR))
					vertical = B_FOLLOW_TOP_BOTTOM;
				else if(tmp.IFindFirst("bottom")!=B_ERROR)
					vertical = B_FOLLOW_BOTTOM;
				else
					vertical = B_FOLLOW_TOP;
				w->layout = horizontal|vertical;
			}
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::WindowSet(const char* option, const char* value, const char* window)
{
	BString tmp(option);
	BString val(value);
	uint32 flags = 0;

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			if(tmp.IFindFirst("Flags")!=B_ERROR)
			{
				if(val.IFindFirst("Reset")!=B_ERROR)
				{
					w->SetFlags(0);
					w->SetFlags(B_ASYNCHRONOUS_CONTROLS);
				}
				else
				{
					flags = w->Flags();
					if(val.IFindFirst("Not-Movable")!=B_ERROR)
						flags = flags|B_NOT_MOVABLE;
					if(val.IFindFirst("Not-Closable")!=B_ERROR)
						flags = flags|B_NOT_CLOSABLE;
					if(val.IFindFirst("Not-Zoomable")!=B_ERROR)
						flags = flags|B_NOT_ZOOMABLE;
					if(val.IFindFirst("Not-Minimizable")!=B_ERROR)
						flags = flags|B_NOT_MINIMIZABLE;
					if(val.IFindFirst("Not-Resizable")!=B_ERROR)
						flags = flags|B_NOT_RESIZABLE;
					if(val.IFindFirst("Not-H-Resizable")!=B_ERROR)
						flags = flags|B_NOT_H_RESIZABLE;
					if(val.IFindFirst("Not-V-Resizable")!=B_ERROR)
						flags = flags|B_NOT_V_RESIZABLE;
					if(val.IFindFirst("Accept-First-Click")!=B_ERROR)
						flags = flags|B_WILL_ACCEPT_FIRST_CLICK;
					if(val.IFindFirst("No-Workspace-Activation")!=B_ERROR)
						flags = flags|B_NO_WORKSPACE_ACTIVATION;
					w->SetFlags(flags|B_ASYNCHRONOUS_CONTROLS);
				}
			}
			else if(tmp.IFindFirst("Look")!=B_ERROR)
			{
				if(val.IFindFirst("Document")!=B_ERROR)
					w->SetLook(B_DOCUMENT_WINDOW_LOOK);
				else if(val.IFindFirst("Titled")!=B_ERROR)
					w->SetLook(B_TITLED_WINDOW_LOOK);
				else if(val.IFindFirst("Floating")!=B_ERROR)
					w->SetLook(B_FLOATING_WINDOW_LOOK);
				else if(val.IFindFirst("Modal")!=B_ERROR)
					w->SetLook(B_MODAL_WINDOW_LOOK);
				else if(val.IFindFirst("Bordered")!=B_ERROR)
					w->SetLook(B_BORDERED_WINDOW_LOOK);
				else if(val.IFindFirst("No-Border")!=B_ERROR)
					w->SetLook(B_NO_BORDER_WINDOW_LOOK);
				else
					ErrorGen("Unknown option");
			}
			else if(tmp.IFindFirst("Feel")!=B_ERROR)
			{
				if(val.IFindFirst("Normal")!=B_ERROR)
					w->SetFeel(B_NORMAL_WINDOW_FEEL);
				else if(val.IFindFirst("Modal-App")!=B_ERROR)
					w->SetFeel(B_MODAL_APP_WINDOW_FEEL);
				else if(val.IFindFirst("Modal-All")!=B_ERROR)
					w->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
				else if(val.IFindFirst("Floating-App")!=B_ERROR)
					w->SetFeel(B_FLOATING_APP_WINDOW_FEEL);
				else if(val.IFindFirst("Floating-All")!=B_ERROR)
					w->SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
				else
					ErrorGen("Unknown option");
			}
			else if(tmp.IFindFirst("Workspace")!=B_ERROR)
			{
				if(val.IFindFirst("Current")!=B_ERROR)
					w->SetWorkspaces(B_CURRENT_WORKSPACE);
				else if(val.IFindFirst("All")!=B_ERROR)
					w->SetWorkspaces(B_ALL_WORKSPACES);
				else if(atoi(val.String()) >= 1)
				{
					int bit = 1 << atoi(val.String()) - 1;
					w->SetWorkspaces(bit);
				}
				else
					ErrorGen("Unknown option");
			}
			else if(tmp.IFindFirst("Title")!=B_ERROR)
			{
				w->SetTitle(value);
			}
			else
				ErrorGen("Unknown option");
			// w->UpdateIfNeeded();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::WindowSet(const char* option, const char* window)
{
	BString tmp(option);
	uint32 flags = 0;

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			if(tmp.IFindFirst("maximize")!=B_ERROR)
				w->Zoom();
			else if(tmp.IFindFirst("minimize")!=B_ERROR)
				w->Minimize(!w->IsMinimized());
			else if(tmp.IFindFirst("deactivate")!=B_ERROR)
				w->Activate(false);
			else if(tmp.IFindFirst("activate")!=B_ERROR)
				w->Activate(true);
			else if(tmp.IFindFirst("disable-updates")!=B_ERROR)
				w->DisableUpdates();
			else if(tmp.IFindFirst("enable-updates")!=B_ERROR)
				w->EnableUpdates();
			else
				ErrorGen("Unknown option");
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::WindowSet(const char* option, int r, int g, int b, const char* window)
{
	BString tmp(option);
	if(r>255) r=255; if(r<0) r=0;
	if(g>255) g=255; if(g<0) g=0;
	if(b>255) b=255; if(b<0) b=0;

	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			if(tmp.IFindFirst("BGColor")!=B_ERROR)
			{
				myView->SetViewColor(r,g,b,yabAlpha);
				myView->Invalidate();
			}
			else if(tmp.IFindFirst("HighColor")!=B_ERROR)
			{
				if(yabAlpha == 255) 
					myView->SetDrawingMode(B_OP_COPY);
				else
					myView->SetDrawingMode(B_OP_ALPHA);
				myView->SetHighColor(r,g,b,yabAlpha);
				YabDrawing *t = new YabDrawing();
				t->command = 6;
				t->r = r; t->g = g;
				t->b = b; t->alpha = yabAlpha;
				myView->drawList->AddItem(t);
			}
			else if(tmp.IFindFirst("LowColor")!=B_ERROR)
			{
				if(yabAlpha == 255) 
					myView->SetDrawingMode(B_OP_COPY);
				else
					myView->SetDrawingMode(B_OP_ALPHA);
				myView->SetLowColor(r,g,b,yabAlpha);
				YabDrawing *t = new YabDrawing();
				t->command = 7;
				t->r = r; t->g = g;
				t->b = b; t->alpha = yabAlpha;
				myView->drawList->AddItem(t);
			}
			else
				ErrorGen("Unknown option");
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
        {
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *bmp = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bView = bmp->FindView(window);
                        if(bView)
                        {
				if(tmp.IFindFirst("HighColor")!=B_ERROR)
				{
                                	bmp->Lock();
					if(yabAlpha == 255) 
						bView->SetDrawingMode(B_OP_COPY);
					else
						bView->SetDrawingMode(B_OP_ALPHA);
					bView->SetHighColor(r,g,b,yabAlpha);
                                	bView->Sync();
                                	bmp->Unlock();
                                	return;
				}
				else if(tmp.IFindFirst("LowColor")!=B_ERROR)
				{
                                	bmp->Lock();
					if(yabAlpha == 255) 
						bView->SetDrawingMode(B_OP_COPY);
					else
						bView->SetDrawingMode(B_OP_ALPHA);
					bView->SetLowColor(r,g,b,yabAlpha);
                                	bView->Sync();
                                	bmp->Unlock();
                                	return;
				}
				else
					ErrorGen("Unknown option");
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *bmp = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
					if(tmp.IFindFirst("HighColor")!=B_ERROR)
					{
                                		bmp->Lock();
						if(yabAlpha == 255) 
							bView->SetDrawingMode(B_OP_COPY);
						else
							bView->SetDrawingMode(B_OP_ALPHA);
						bView->SetHighColor(r,g,b,yabAlpha);
                                		bView->Sync();
                                		bmp->Unlock();
                                        	w->Unlock();
                                		return;
					}
					else if(tmp.IFindFirst("LowColor")!=B_ERROR)
					{
       	                         		bmp->Lock();
						if(yabAlpha == 255) 
							bView->SetDrawingMode(B_OP_COPY);
						else
							bView->SetDrawingMode(B_OP_ALPHA);
						bView->SetLowColor(r,g,b,yabAlpha);
       		                         	bView->Sync();
       		                         	bmp->Unlock();
                                        	w->Unlock();
       		                         	return;
					}
					else
						ErrorGen("Unknown option");
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        }
                } 
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::WindowSet(const char* option, double x, double y, const char* window)
{
	BString tmp(option);
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			if(tmp.IFindFirst("ResizeTo")!=B_ERROR)
				w->ResizeTo(x,y);
			else if(tmp.IFindFirst("MoveTo")!=B_ERROR)
				w->MoveTo(x,y);
			else if(tmp.IFindFirst("MinimumTo")!=B_ERROR)
			{
				float x1, x2, y1, y2;
				w->GetSizeLimits(&x1,&x2,&y1,&y2);
				w->SetSizeLimits((float)x,x2,(float)y,y2);
			}
			else if(tmp.IFindFirst("MaximumTo")!=B_ERROR)
			{
				float x1, x2, y1, y2;
				w->GetSizeLimits(&x1,&x2,&y1,&y2);
				w->SetSizeLimits(x1,(float)x,y1,(float)y);
			}
			else
				ErrorGen("Unknown option");
			w->Unlock();
			// w->UpdateIfNeeded();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::WindowClear(const char* window)
{
	bool delMenuBar;
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w && myView->Parent())
		{
			w->Lock();
			BView *child, *oldchild;
			if(child = myView->ChildAt(0))
			{
				while(child)
				{
					if(is_kind_of(child, YabTabView))
					{
						for(int i = 0; i<((YabTabView*)child)->CountTabs(); i++)
						{
							YabView *t = (YabView*)((YabTabView*)child)->TabAt(i);
							RemoveView(t);
							viewList->DelView(t->Name());
						}
					}
					if(is_kind_of(child, YabBitmapView))
						yabcanvas->RemoveItem(child);
					BView *subchild;
					if(subchild = child->ChildAt(0))
						while(subchild)
						{
							if(is_kind_of(subchild, YabTabView))
							{
								for(int i = 0; i<((YabTabView*)subchild)->CountTabs(); i++)
								{
									YabView *t = (YabView*)((YabTabView*)subchild)->TabAt(i);
									RemoveView(t);
									viewList->DelView(t->Name());
								}
							}
							if(viewList->GetView(subchild->Name()))
							{
								RemoveView(subchild);
								viewList->DelView(subchild->Name());
							}
							subchild = subchild->NextSibling();
						}
					if(viewList->GetView(child->Name()))
					{
						RemoveView(child);
						viewList->DelView(child->Name());
					}


					oldchild = child;
					child = child->NextSibling();

					if(is_kind_of(oldchild, YabView))
						DrawClear(oldchild->Name(), true);
					if(is_kind_of(oldchild, BMenuBar))
					{
						oldchild->Hide();
					}
					oldchild->RemoveSelf();
					delete oldchild;
				}
			}
			if(is_kind_of(myView, YabView))
				DrawClear(myView->Name(), true);
			if(is_kind_of(myView, BMenuBar))
				myView->Hide();
			BBox *box = cast_as(myView->Parent(), BBox); 
			myView->RemoveSelf();
			delete myView;
			viewList->DelView(window);
			if(box)
			{
				box->RemoveSelf();
				delete box;
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
} 

void YabInterface::RemoveView(BView *myView)
{
	BView *child, *oldchild;
	if(child = myView->ChildAt(0))
	while(child)
	{

		if(is_kind_of(child, YabTabView))
		{
			for(int i = 0; i<((YabTabView*)child)->CountTabs(); i++)
			{
				YabView *t = (YabView*)((YabTabView*)child)->TabAt(i);
				RemoveView(t);
				viewList->DelView(t->Name());
			}
		}
		if(is_kind_of(child, YabBitmapView))
			yabcanvas->RemoveItem(child);
		BView *subchild;
		if(subchild = child->ChildAt(0))
			while(subchild)
			{
				if(is_kind_of(subchild, YabTabView))
				{
					for(int i = 0; i<((YabTabView*)subchild)->CountTabs(); i++)
					{
						YabView *t = (YabView*)((YabTabView*)subchild)->TabAt(i);
						RemoveView(t);
						viewList->DelView(t->Name());
					}
				}
				if(viewList->GetView(subchild->Name()))
				{
					RemoveView(subchild);
					viewList->DelView(subchild->Name());
				}
				subchild = subchild->NextSibling();
			}
		if(viewList->GetView(child->Name()))
		{
			RemoveView(child);
			viewList->DelView(child->Name());
		}
		oldchild = child;
		child = child->NextSibling();

		if(is_kind_of(oldchild, YabView))
			DrawClear(oldchild->Name(), true);
		if(is_kind_of(oldchild, BMenuBar))
			oldchild->Hide();
		if(is_kind_of(oldchild, BMenuBar))
		{
			BMenuBar *b = cast_as(oldchild, BMenuBar);
			for(int i=0; i<b->CountItems(); i++)
			{
				YabMenu *m = (YabMenu*)b->SubmenuAt(i);
				if(m)
				{
					// check for subsubmenus
					for(int j=0; j<m->CountItems(); j++)
					{
						YabMenu *n = (YabMenu*)m->SubmenuAt(j);
						if(n) n->MyHide();
					}
					m->MyHide();
					// printf("hiden\n");
				}
			}
			b->Hide();
		}
		oldchild->RemoveSelf();
		delete oldchild;
	}
}

void YabInterface::TextEdit(BRect frame, const char* title, int scrollbar, const char* window)
{
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			uint32 resizeMode;
			BRect textframe;

			w->Lock();

			if(scrollbar == 3 || scrollbar == 1) frame.right -= B_V_SCROLL_BAR_WIDTH;
			if(scrollbar>2) frame.bottom -= B_H_SCROLL_BAR_HEIGHT;

			textframe = frame;
			textframe.OffsetTo(B_ORIGIN);

			if(w->layout == -1)
				resizeMode = B_FOLLOW_ALL;
			else
				resizeMode = w->layout;

			// BTextView *txtView = new BTextView(frame, title, textframe, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_PULSE_NEEDED|B_NAVIGABLE); 
			YabText *txtView = new YabText(frame, title, textframe, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_PULSE_NEEDED|B_NAVIGABLE); 
			txtView->SetWordWrap(true); 
			// txtView->SetFontAndColor(be_fixed_font); 
			
			switch(scrollbar)
			{
				case 3:  // both
					myView->AddChild(new BScrollView("scroll_list", txtView, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, true, true));
					break;
				case 2:  // horizontal
					myView->AddChild(new BScrollView("scroll_list", txtView, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, true, false));
					break;
				case 0:  // none
					myView->AddChild(txtView);
					break;
				default: // vertical is default
					myView->AddChild(new BScrollView("scroll_list", txtView, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, false, true));
					break;
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(window, "VIEW");
}

void YabInterface::TextAdd(const char* title, const char* text)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					myText->Insert(text);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

void YabInterface::TextSet(const char* title, const char* option)
{
	BString tmp(option);
	YabView *myView = NULL;
	YabText *myText = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("Cut")!=B_ERROR)
						myText->Cut(be_clipboard);
					else if(tmp.IFindFirst("Copy")!=B_ERROR)
					{
						int32 a,b;
						myText->GetSelection(&a, &b);
						if(a != b)
							myText->Copy(be_clipboard);
					}
					else if(tmp.IFindFirst("Paste")!=B_ERROR)
						myText->Paste(be_clipboard);
					else if(tmp.IFindFirst("Clear")!=B_ERROR)
						myText->Clear();
					else if(tmp.IFindFirst("Select-All")!=B_ERROR)
						myText->SelectAll();
					else if(tmp.IFindFirst("Undo")!=B_ERROR)
						myText->Undo(be_clipboard);
					// else if(tmp.IFindFirst("Redo")!=B_ERROR)
					// 	; // myText->Redo(be_clipboard);
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

void YabInterface::TextSet(const char* title, const char* option, const char* value)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);
	 BString tmp2(value);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("align")!=B_ERROR)
					{ 
						if(tmp2.IFindFirst("left")!=B_ERROR)
							myText->SetAlignment(B_ALIGN_LEFT);
						else if(tmp2.IFindFirst("center")!=B_ERROR)
							myText->SetAlignment(B_ALIGN_CENTER);
						else if(tmp2.IFindFirst("right")!=B_ERROR)
							myText->SetAlignment(B_ALIGN_RIGHT);
					}		
					else if(tmp.IFindFirst("autocomplete")!=B_ERROR)
						myText->AddWord(new BString(value));
					else if(tmp.IFindFirst("font")!=B_ERROR)
					{
						BFont myFont;
						BString opt;

						// Font family
						int pos1 = 0;
						int pos2 = tmp2.FindFirst(',');
						if(pos2 != B_ERROR) 
						{
							tmp2.CopyInto(opt, pos1, pos2-pos1);
							while(opt[0] == ' ') opt.RemoveFirst(" ");
							while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
							font_family fam;
							sprintf((char*)fam, "%s" , opt.String());
							if(myFont.SetFamilyAndFace(fam, B_REGULAR_FACE) == B_OK)
							{
								myView->SetFont(&myFont, B_FONT_FAMILY_AND_STYLE);
								// Font style
								pos1 = pos2+1;
								pos2 = tmp2.FindFirst(',', pos2+1);
								if(pos2 != B_ERROR) 
								{
									tmp2.CopyInto(opt, pos1, pos2-pos1);
									while(opt[0] == ' ') opt.RemoveFirst(" ");
									while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
									font_style style;
									sprintf((char*)style, "%s" , opt.String());
									if(myFont.SetFamilyAndStyle(fam,style) == B_OK)
									{
										// Font size
										pos1 = pos2+1;
										pos2 = tmp2.FindFirst(',', pos2+1);
										if(pos2 == B_ERROR) pos2 = tmp2.Length();
										tmp2.CopyInto(opt, pos1, pos2-pos1);
										while(opt[0] == ' ') opt.RemoveFirst(" ");
										while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
										double size = atof(opt.String());
										myFont.SetSize(size);
									}
								}
							}
						}
						else if(tmp2.IFindFirst("system-plain")!=B_ERROR)
							myFont = be_plain_font;
						else if(tmp2.IFindFirst("system-fixed")!=B_ERROR)
							myFont = be_fixed_font;
						else if(tmp2.IFindFirst("system-bold")!=B_ERROR)
							myFont = be_bold_font;
														
						else
							ErrorGen("Unknown option");
						int32 start,finish;
						myText->GetSelection(&start, &finish);
						myText->SelectAll();
						myText->SetFontAndColor(&myFont);
						myText->Select(start,finish);
					}
					else if (tmp.IFindFirst("focus")!=B_ERROR)
							{
							if (tmp2.IFindFirst("true")!=B_ERROR)
							{
								bool focused = true;
								myText->MakeFocus(focused);
							}
							else
							{
							bool focused = false;
							myText->MakeFocus(focused);
							}
							}
					else
					
						ErrorGen("Unknown option");
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

void YabInterface::TextSet(const char* title, const char* option, int value)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("autocomplete-start")!=B_ERROR)
						myText->SetAutoCompleteStart(value-1);
					else if(tmp.IFindFirst("has-autocompletion")!=B_ERROR)
						myText->HasAutoCompletion((bool)value);
					else if(tmp.IFindFirst("autoindent")!=B_ERROR)
						myText->SetAutoindent((bool)value);
					else if(tmp.IFindFirst("wordwrap")!=B_ERROR)
						myText->SetWordWrap((bool)value);
					else if(tmp.IFindFirst("editable")!=B_ERROR)
						myText->MakeEditable((bool)value);
					else if(tmp.IFindFirst("color-case-sensitive")!=B_ERROR)
						myText->SetCaseSensitive((bool)value);
					else if(tmp.IFindFirst("tabwidth")!=B_ERROR)
						myText->SetTabWidth(value);
					else if(tmp.IFindFirst("textwidth")!=B_ERROR)
					{
						// BRect txtframe = myText->TextRect();
						// txtframe.right = txtframe.left + value;
						// myText->SetTextRect(txtframe);
						myText->SetTextRect(BRect(0,0, value,1));
						// BRect txtbounds = myText->Bounds();
						// myText->FrameResized(txtbounds.Width(), txtbounds.Height());
        BRect bounds(myText->Bounds());
        BScrollBar* horizontalScrollBar = myText->ScrollBar(B_HORIZONTAL);

        // do we have a horizontal scroll bar?
        if (horizontalScrollBar != NULL) {
                long viewWidth = bounds.IntegerWidth();
                long dataWidth = (long)value;

                long maxRange = dataWidth - viewWidth;
                maxRange = max_c(maxRange, 0);

		horizontalScrollBar->SetRange(0, 1000); //(float)maxRange);
                // horizontalScrollBar->SetProportion((float)viewWidth / (float)dataWidth);
                // horizontalScrollBar->SetSteps(10.0, dataWidth / 10);
		// std::cout << "dataWidth: " << dataWidth << " maxrange: " << maxRange << std::endl;
        }

					}
					else if(tmp.IFindFirst("gotoline")!=B_ERROR)
					{
						if(value<1) value = 1;
						myText->GoToLine(value-1);
						myText->ScrollToSelection();
					}
					else if(tmp.IFindFirst("select")!=B_ERROR)
					{
						int start, num;
						if(value <= 0)
							myText->Select(0,0);
						else 
						{
							if(value-1 == 0)
								start = 0;
							else
								start = myText->OffsetAt(value-1);
							if(myText->CountLines()>value)
								num = myText->OffsetAt(value)-start-1;
							else
								num = myText->OffsetAt(value)-start;
								// num = myText->TextLength()-start;
							myText->Select(start,start+num);
							myText->ScrollToSelection();
						}
					}
					else if(tmp.IFindFirst("changed")!=B_ERROR)
						myText->SetChanged((bool)value);
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

void YabInterface::TextColor(const char* title, const char* option, const char* command)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("Color1")!=B_ERROR)
						myText->AddCommand(command,0);
					else if(tmp.IFindFirst("Color2")!=B_ERROR)
						myText->AddCommand(command,1);
					else if(tmp.IFindFirst("Color3")!=B_ERROR)
						myText->AddCommand(command,2);
					else if(tmp.IFindFirst("Color4")!=B_ERROR)
						myText->AddCommand(command,3);
					else if(tmp.IFindFirst("char-color")!=B_ERROR)
						myText->AddCommand(command,4);
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

void YabInterface::TextColor(const char* title, const char* option, int r, int g, int b)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("color1")!=B_ERROR)
						myText->SetColors(0,r,g,b);
					else if(tmp.IFindFirst("color2")!=B_ERROR)
						myText->SetColors(1,r,g,b);
					else if(tmp.IFindFirst("color3")!=B_ERROR)
						myText->SetColors(2,r,g,b);
					else if(tmp.IFindFirst("color4")!=B_ERROR)
						myText->SetColors(3,r,g,b);
					else if(tmp.IFindFirst("char-color")!=B_ERROR)
						myText->SetColors(4,r,g,b);
					else if(tmp.IFindFirst("bgcolor")!=B_ERROR)
						myText->SetColors(5,r,g,b);
					else if(tmp.IFindFirst("textcolor")!=B_ERROR)
						myText->SetColors(6,r,g,b);
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

int YabInterface::TextGet(const char* title, const char* option, const char* option2)
{
	int ret = -1;
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("case-sensitive-find")!=B_ERROR)
					{
						int32 startOffset, endOffset;
						myText->GetSelection(&startOffset, &endOffset);
						bool isFinished = false;
						int foundOffset, l = myText->TextLength() - endOffset;
						char* s;
						s = new char[l+1];
						myText->GetText(endOffset, l, s);
						BString line(s);
						foundOffset = line.FindFirst(option2);
						if(foundOffset == B_ERROR)
						{
							delete s;
							s = new char[endOffset];
							myText->GetText(0, endOffset-1, s);
							line = s;
							foundOffset = line.FindFirst(option2);
						}
						else
							foundOffset += endOffset;
						if(foundOffset != B_ERROR)
						{
							delete s;
							myText->Select(foundOffset, foundOffset+strlen(option2));
							myText->ScrollToSelection();
							ret = myText->LineAt(foundOffset)+1;
							// myText->GoToLine(myText->LineAt(foundOffset));
						}
					}
					else if(tmp.IFindFirst("find")!=B_ERROR)
					{
						int32 startOffset, endOffset;
						myText->GetSelection(&startOffset, &endOffset);
						// = myText->OffsetAt(myText->CurrentLine());
						bool isFinished = false;
						int foundOffset, l = myText->TextLength() - endOffset;
						char* s;
						s = new char[l+1];
						myText->GetText(endOffset, l, s);
						BString line(s);
						foundOffset = line.IFindFirst(option2);
						if(foundOffset == B_ERROR)
						{
							delete s;
							s = new char[endOffset];
							myText->GetText(0, endOffset-1, s);
							line = s;
							foundOffset = line.IFindFirst(option2);
						}
						else
							foundOffset += endOffset;
						if(foundOffset != B_ERROR)
						{
							delete s;
							myText->Select(foundOffset, foundOffset+strlen(option2));
							myText->ScrollToSelection();
							ret = myText->LineAt(foundOffset)+1;
							// myText->GoToLine(myText->LineAt(foundOffset));
						}
					}
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

double YabInterface::TextGet(const char* title, const char* option, int line)
{
	double ret = -1.0;
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("line-width")!=B_ERROR)
						ret = myText->LineWidth(line);
					else if(tmp.IFindFirst("line-height")!=B_ERROR)
						ret = myText->LineHeight(line);
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

int YabInterface::TextGet(const char* title, const char* option)
{
	int ret = -1;
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("currentline")!=B_ERROR)
						ret = myText->CurrentLine()+1;
					else if(tmp.IFindFirst("vertical-scrollbar")!=B_ERROR)
						{
							float f = -1.0;
							BScrollView *s = cast_as(myText->Parent(), BScrollView);
							if(s)
							{
								BScrollBar *b = s->ScrollBar(B_VERTICAL);
								if(b) f = b->Value();
								else
									ErrorGen("TEXTEDIT has no vertical scrollbar");
							}
							else
								ErrorGen("TEXTEDIT has no vertical scrollbar");
							ret = (int)f;
						}
					else if(tmp.IFindFirst("horizontal-scrollbar")!=B_ERROR)
						{
							float f = -1.0;
							BScrollView *s = cast_as(myText->Parent(), BScrollView);
							if(s)
							{
								BScrollBar *b = s->ScrollBar(B_HORIZONTAL);
								if(b) f = b->Value();
								else
									ErrorGen("TEXTEDIT has no horizontal scrollbar");
							}
							else
								ErrorGen("TEXTEDIT has no horizontal scrollbar");
							ret = (int)f;
						}
					else if(tmp.IFindFirst("countlines")!=B_ERROR)
						ret = myText->CountLines();
					else if(tmp.IFindFirst("textlength")!=B_ERROR)
						ret = myText->TextLength();
					else if(tmp.IFindFirst("haschanged")!=B_ERROR)
						ret = myText->HasChanged()?1:0;
					else if(tmp.IFindFirst("cursor-position")!=B_ERROR)
					{
						int32 start, end, pos1,pos2;
						myText->GetSelection(&start, &end);
						ret = end;
					}
					else
						ErrorGen("Unknown option");
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

const char* YabInterface::TextGet(const char* title, int linenum)
{
	YabView *myView = NULL;
	YabText *myText = NULL;

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					char* ret;
					int start, num;
					if(linenum-1 == 0)
						start = 0;
					else
						start = myText->OffsetAt(linenum-1);
					if(myText->CountLines()>linenum)
						num = myText->OffsetAt(linenum)-start-1;
					else
						num = myText->TextLength()-start;
					ret = new char[num+1];
					myText->GetText(start, num, ret);
					w->Unlock();
					return (const char*)ret;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

const char* YabInterface::TextGet6(const char* title, const char* option)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	BString tmp(option);

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(),YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title),YabText);
				if(myText)
				{
					if(tmp.IFindFirst("selection")!=B_ERROR)
					{
						char* ret;
						int32 start, finish;
						myText->GetSelection(&start, &finish);
						if(finish == 0 || (finish-start)<=0)
						{
							ret = new char[1];
							ret[0] = '\0';
						}
						else
						{
							ret = new char[finish-start+1];
							myText->GetText(start, finish-start, ret);
						}

						w->Unlock();
						return (const char*)ret;
					}
					else
						ErrorGen("Unknown option");
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

double YabInterface::DrawGet(const char* option, const char* txt, const char* view)
{
	double ret = 0;
	BString tmp(option);
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			if(tmp.IFindFirst("Text-Width")!=B_ERROR)
				ret = myView->StringWidth(txt); 
			if(tmp.IFindFirst("Max-Text-Height")!=B_ERROR)
			{
				font_height height; 
				myView->GetFontHeight(&height);
				ret = height.ascent+height.descent;
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
        {
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), view))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
					if(tmp.IFindFirst("Text-Width")!=B_ERROR)
						ret = bView->StringWidth(txt); 
					if(tmp.IFindFirst("Max-Text-Height")!=B_ERROR)
					{
						font_height height; 
						bView->GetFontHeight(&height);
						ret = height.ascent+height.descent;
					}
                                        b->Unlock();

                                        w->Unlock();
                                        return ret;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        }  
		}
		Error(view, "VIEW or CANVAS");
	}
	return ret;
}

const char* YabInterface::DrawGet(const char* option)
{
	BString t(option);
	char* ret;
	if(t.IFindFirst("fontfamily")!=B_ERROR)
	{
		int32 numFamilies = count_font_families();
		ret = new char[numFamilies*(B_FONT_FAMILY_LENGTH + 1)];
		BString tmp("");
		for(int32 i=0; i<numFamilies; i++)
		{
			font_family f;
			uint32 flags;
			if (get_font_family(i, &f, &flags) == B_OK)
			{
				tmp+=f;
				tmp+="|";
			}
		}
		strcpy(ret, tmp.String());
	}
	else
	{
		uint32 flags;
		int32 numStyles = count_font_styles((char*)option);
		ret = new char[numStyles*(B_FONT_STYLE_LENGTH + 1)];
		BString tmp("");
		for(int32 i=0; i<numStyles ; i++)
		{
			font_style style;
			uint32 flags;
			if (get_font_style((char*)option, i, &style, &flags) == B_OK)
			{
				tmp+=style;
				tmp+="|";
			}
		}
		strcpy(ret, tmp.String());
	}
	
	return (const char*) ret;
}

int YabInterface::DrawGet(BPoint coord, const char* option, const char* view)
{
	BString t(option);
	
}

void YabInterface::TextClear(const char* title)
{
	YabView *myView = NULL;
	YabText *myText = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title), YabText);
				if(myText)
				{
					myText->SetText("", 0);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

const char* YabInterface::TextGet(const char* title)
{
	const char* tmp;
	YabView *myView = NULL;
	YabText *myText = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myText = cast_as(myView->FindView(title), YabText);
				if(myText)
				{
					tmp = myText->Text();
					w->Unlock();
					return tmp;
				}
				w->Unlock();
			}
		}
	}
	Error(title, "TEXTEDIT");
}

void YabInterface::TreeBox1(BRect frame, const char* id, int scrollbarType, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			uint32 resizeMode;

			w->Lock();

			if(scrollbarType == 3 || scrollbarType == 1) frame.right -= B_V_SCROLL_BAR_WIDTH;
			if(scrollbarType>2) frame.bottom -= B_H_SCROLL_BAR_HEIGHT;

			BOutlineListView  *list = new BOutlineListView(frame,id);
			if(w->layout == -1)
				resizeMode = B_FOLLOW_ALL;
			else
				resizeMode = w->layout;
			list->SetResizingMode(resizeMode);
			list->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			list->SetSelectionMessage(new BMessage(YABTREEBOXSELECT));
			list->SetInvocationMessage(new BMessage(YABTREEBOXINVOKE));
			switch(scrollbarType)
			{
				case 3:  // both
					myView->AddChild(new BScrollView("scroll_list", list, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, true, true));
					break;
				case 2:  // horizontal
					myView->AddChild(new BScrollView("scroll_list", list, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, true, false));
					break;
				case 0:  // none
					myView->AddChild(list);
					break;
				default: // vertical is default
					myView->AddChild(new BScrollView("scroll_list", list, resizeMode, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE, false, true));
					break;
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::TreeBox2(const char* id, const char* item)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					myTree->AddItem(new BStringItem(item));
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox3(const char* id, const char* head, const char* item, int isExpanded)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					for(int i=0; i<myTree->FullListCountItems(); i++)
					{
						BStringItem *stritem = (BStringItem*)myTree->FullListItemAt(i);
						if(!strcmp(stritem->Text(), head))
						{
							BStringItem *tmp = new BStringItem(item);
							myTree->AddUnder(tmp,stritem);
							if(isExpanded<1)
								myTree->Collapse(stritem);
							w->Unlock();
							return;
						}
					}
					w->Unlock();
					ErrorGen("Item not found");
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox4(const char* id)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					myTree->MakeEmpty();
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox5(const char* id, const char* item)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					for(int i=0; i<myTree->FullListCountItems(); i++)
					{
						BStringItem *stritem = (BStringItem*)myTree->FullListItemAt(i);
						if(!strcmp(stritem->Text(), item))
						{
							myTree->RemoveItem(i);
							w->Unlock();
							return;
						}
					}
					w->Unlock();
					ErrorGen("Item not found");
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox7(const char* id, int pos)
{
	pos--;
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					if(pos==0) 
						myTree->DeselectAll();
					else
					{
						BListItem *item = myTree->FullListItemAt(pos);
						if(item)
							myTree->Select(myTree->IndexOf(item));
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox8(const char* id, int pos)
{
	pos--;
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					myTree->RemoveItem(pos);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}
void YabInterface::TreeBox9(const char* id, const char* head, const char* item)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					for(int i=0; i<myTree->FullListCountItems(); i++)
					{
						BStringItem *stritem = (BStringItem*)myTree->FullListItemAt(i);
						if(!strcmp(stritem->Text(), head))
						{
							for(int j=0; i<myTree->CountItemsUnder(stritem, false); j++)
							{
								BStringItem *subitem = (BStringItem*)myTree->FullListItemAt(i+j+1);
								if(!strcmp(subitem->Text(), item))
								{
									myTree->RemoveItem((BListItem*)subitem);
									w->Unlock();
									return;
								}
							}
						}
					}
					w->Unlock();
					ErrorGen("Item not found");
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox10(const char* id, const char* head)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					for(int i=0; i<myTree->FullListCountItems(); i++)
					{
						BStringItem *stritem = (BStringItem*)myTree->FullListItemAt(i);
						if(!strcmp(stritem->Text(), head))
						{
							myTree->Expand((BListItem*)stritem);
							w->Unlock();
							return;
						}
					}
					w->Unlock();
					ErrorGen("Item not found");
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}
void YabInterface::TreeBox11(const char* id, const char* head)
{
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id), BOutlineListView);
				if(myTree)
				{
					for(int i=0; i<myTree->FullListCountItems(); i++)
					{
						BStringItem *stritem = (BStringItem*)myTree->FullListItemAt(i);
						if(!strcmp(stritem->Text(), head))
						{
							myTree->Collapse((BListItem*)stritem);
							w->Unlock();
							return;
						}
					}
					w->Unlock();
					ErrorGen("Item not found");
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

void YabInterface::TreeBox12(const char* id, const char* item, int pos)
{
	if(pos<1) pos = 1;

	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(id),BOutlineListView);
				if(myTree)
				{
					if(pos<=myTree->FullListCountItems())
					{
						uint32 outline = (myTree->FullListItemAt(pos-1))->OutlineLevel();
						myTree->AddItem(new BStringItem(item, outline),pos-1);
					}
					else
						myTree->AddItem(new BStringItem(item));
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

const char* YabInterface::TreeboxGet(const char* treebox, int pos)
{
	pos--;
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(treebox), BOutlineListView);
				if(myTree)
				{
					BStringItem *t = (BStringItem*)myTree->FullListItemAt(pos);
					if(t)
					{
						const char* txt = t->Text();
						w->Unlock();
						return txt;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(treebox, "TREEBOX");
}

int YabInterface::TreeboxCount(const char* treebox)
{
	int32 ret;
	YabView *myView = NULL;
	BOutlineListView *myTree = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTree = cast_as(myView->FindView(treebox), BOutlineListView);
				if(myTree)
				{
					ret = myTree->FullListCountItems();
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(treebox, "TREEBOX");
}

BBitmap* YabInterface::loadImage(const char* FileName)
{
	BBitmap* LogoBitmap = NULL;
	BFile ImageFile;
	BPath ImagePath;
	int ret = 0;

	if( *FileName == '/')
		ImageFile.SetTo( FileName, B_READ_ONLY);
	else
		// App directory.
		if(!strcmp(ApplicationDirectory,""))
		{
	 		if( ImagePath.SetTo((const char*)ApplicationDirectory, FileName) == B_OK)
				ImageFile.SetTo( ImagePath.Path(), B_READ_ONLY);
		}

	if( ImageFile.InitCheck() != B_OK)
		ImageFile.SetTo( FileName, B_READ_ONLY);
	
	if( ImageFile.InitCheck() != B_OK)
		return NULL;

	Roster = BTranslatorRoster::Default();

	if( !Roster)
		return NULL;

	BBitmapStream Stream;

	if( Roster->Translate( &ImageFile, NULL, NULL, &Stream, B_TRANSLATOR_BITMAP) < B_OK)
		return NULL;

	if( Stream.DetachBitmap( &LogoBitmap) != B_OK)
		return NULL;


	return LogoBitmap;
}

void YabInterface::ButtonImage(double x,double y, const char* id,const char* enabledon, const char* enabledoff, const char* disabled, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BPicture *pic1 = NULL, *pic2 = NULL, *pic3 = NULL;
			BBitmap *bitmap = NULL;
			BView *tmpView = new BView(BRect(0,0,1000,1000),"tmp",0,  B_WILL_DRAW);
			w->AddChild(tmpView);
			tmpView->SetDrawingMode(B_OP_ALPHA);

	        	bitmap = loadImage(enabledon);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap) 
				{
					// drawing_mode mode = myView->DrawingMode();
					// tmpView->SetDrawingMode(B_OP_ALPHA);
					tmpView->DrawBitmap(bitmap,bitmap->Bounds());
					// myView->SetDrawingMode(mode);
				}
	        	pic1 = tmpView->EndPicture();

			BRect r;
			r.SetLeftTop(BPoint(x,y));
			if(bitmap)
        			r.SetRightBottom(BPoint(x,y) + bitmap->Bounds().RightBottom());
			else
				r.SetRightBottom(BPoint(x,y));

	        	bitmap = loadImage(enabledoff);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap) 
				{
					// drawing_mode mode = myView->DrawingMode();
					// tmpView->SetDrawingMode(B_OP_ALPHA);
					tmpView->DrawBitmap(bitmap,bitmap->Bounds());
					// myView->SetDrawingMode(mode);
				}
	        	pic2 = tmpView->EndPicture();

	        	bitmap = loadImage(disabled);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap) tmpView->DrawBitmap(bitmap,bitmap->Bounds());
	        	pic3 = tmpView->EndPicture();


			BPictureButton *myButton = new BPictureButton(r, id, pic2, pic1, new BMessage(YABBUTTON));
			myButton->SetDisabledOff(pic3);

			if(w->layout == -1)
				myButton->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				myButton->SetResizingMode(w->layout);
			myButton->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(myButton);
			tmpView->RemoveSelf();
			delete tmpView;
		// 	delete Roster;
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::CheckboxImage(double x, double y,const char* id,const char* enabledon, const char* enabledoff, const char *disabledon, const char *disabledoff, int isActivated, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BPicture *pic1 = NULL, *pic2 = NULL, *pic3 = NULL, *pic4 = NULL;
			BBitmap *bitmap = NULL;
			BView *tmpView = new BView(BRect(0,0,1000,1000),"tmp",0,  B_WILL_DRAW);
			w->AddChild(tmpView);
			tmpView->SetDrawingMode(B_OP_ALPHA);

	        	bitmap = loadImage(enabledon);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap)
				{
					// drawing_mode mode = myView->DrawingMode();
					// myView->SetDrawingMode(B_OP_ALPHA);
					tmpView->DrawBitmap(bitmap,bitmap->Bounds());
					// myView->SetDrawingMode(mode);
				}
	        	pic1 = tmpView->EndPicture();

			BRect r;
			r.SetLeftTop(BPoint(x,y));
			if(bitmap)
        			r.SetRightBottom(BPoint(x,y) + bitmap->Bounds().RightBottom());
			else
				r.SetRightBottom(BPoint(x,y));

	        	bitmap = loadImage(enabledoff);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap)
				{
					// drawing_mode mode = myView->DrawingMode();
					// myView->SetDrawingMode(B_OP_ALPHA);
					tmpView->DrawBitmap(bitmap,bitmap->Bounds());
					// myView->SetDrawingMode(mode);
				}
	        	pic2 = tmpView->EndPicture();

	        	bitmap = loadImage(disabledon);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap)
				{
					// drawing_mode mode = myView->DrawingMode();
					// myView->SetDrawingMode(B_OP_ALPHA);
					tmpView->DrawBitmap(bitmap,bitmap->Bounds());
					// myView->SetDrawingMode(mode);
				}
	        	pic3 = tmpView->EndPicture();

	        	bitmap = loadImage(disabledoff);
	        	tmpView->BeginPicture(new BPicture);
				if(bitmap) 
				{
					// drawing_mode mode = myView->DrawingMode();
					// myView->SetDrawingMode(B_OP_ALPHA);
					tmpView->DrawBitmap(bitmap,bitmap->Bounds());
					// myView->SetDrawingMode(mode);
				}
	        	pic4 = tmpView->EndPicture();

			BPictureButton *myButton = new BPictureButton(r, id, pic2, pic1, new BMessage(YABCHECKBOX),B_TWO_STATE_BUTTON);
			myButton->SetDisabledOn(pic3);
			myButton->SetDisabledOff(pic4);

			if(w->layout == -1)
				myButton->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				myButton->SetResizingMode(w->layout);
			myButton->SetValue(isActivated);
			myButton->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			myView->AddChild(myButton);
			tmpView->RemoveSelf();
			delete tmpView;
			// delete Roster;
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::CheckboxSet(const char* id, int isActivated)
{
	YabView *myView = NULL;
	BCheckBox *myCheckBox = NULL;
	BPictureButton *myPicButton = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myCheckBox = cast_as(myView->FindView(id), BCheckBox);
				if(myCheckBox)
				{
					myCheckBox->SetValue(isActivated);
					w->Unlock();
					return;
				}
				else 
				{
					myPicButton = cast_as(myView->FindView(id), BPictureButton);
					if(myPicButton)
					{
						if(myPicButton->Behavior() == B_TWO_STATE_BUTTON)
						{
							myPicButton->SetValue(isActivated);
							w->Unlock();
							return;
						}
					}
				}
				w->Unlock();
			}
		}
	}
	Error(id, "CHECKBOX");
}

void YabInterface::RadioSet(const char* id, int isActivated)
{
	YabView *myView = NULL;
	BRadioButton *myRadioButton= NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myRadioButton = cast_as(myView->FindView(id), BRadioButton);
				if(myRadioButton)
				{
					myRadioButton->SetValue(isActivated);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "RADIOBUTTON");
}

const char* YabInterface::TextControlGet(const char* id)
{
	const char* tmp = NULL;
	YabView *myView = NULL;
	BTextControl *myTControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myTControl = cast_as(myView->FindView(id), BTextControl);
				if(myTControl)
				{
					tmp = myTControl->Text();
					w->Unlock();
					return tmp;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TEXTCONTROL");
}

void YabInterface::ToolTips(const char* view, const char* text)
{
	YabView *myView = NULL;
	BView *theView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				theView = w->FindView(view);
				if(theView)
				{
					if(theView->Name())
					{
						if(!strcmp(theView->Name(), view))
						{
							if(text[0] == '\0')
								// tooltip->SetHelp(theView, NULL);
								;
							else
								theView->SetToolTip(text);
							w->Unlock();
							return;
						}
					}
				}
				w->Unlock();
			}
		}
	}
	Error(view, "VIEW");
}

void YabInterface::ToolTipsColor(const char* color, int r, int g, int b)
{
/*
		BString tmp(color);
		rgb_color rgb = {r,g,b};
		if(tmp.IFindFirst("BGColor")!=B_ERROR)
			tooltip->SetColor(rgb);
		else if(tmp.IFindFirst("TextColor")!=B_ERROR)
			tooltip->SetTextColor(rgb);
*/
}

void YabInterface::TreeSort(const char* view)
{
	ErrorGen("Sorry, this command is not working yet");
	YabView *myView = NULL;
	BOutlineListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(view), BOutlineListView);
				if(myList)
				{
					myList->FullListSortItems((int(*)(const BListItem *, const BListItem *))YabInterface::compare);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(view, "TREEBOX");
}

void YabInterface::ListSort(const char* view)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(view), BListView);
				if(myList)
				{
					myList->SortItems((int(*)(const void*, const void*))YabInterface::compare);
					w->Unlock();
					return;

				}
				w->Unlock();
			}
		}
	}
	Error(view, "LISTBOX");
}

int YabInterface::compare(BListItem **firstArg, BListItem **secondArg)
{
	if(firstArg != NULL && secondArg != NULL)
	{
		BString item1(((BStringItem*)*firstArg)->Text());
		BString item2(((BStringItem*)*secondArg)->Text());
		if(((BListItem*)*firstArg)->OutlineLevel()!=((BListItem*)*secondArg)->OutlineLevel())
			return 0;
		return item1.ICompare(item2);
	}
	return 0;
}

void YabInterface::FileBox(BRect frame, const char* id, bool hasHScrollbar, const char* option, const char* view)
{
	BString tmp(option);
	
	border_style plain = B_PLAIN_BORDER;
	if(tmp.IFindFirst("no-border")!=B_ERROR)
		plain = B_NO_BORDER;
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BColumnListView* myColumnList;
		 	myColumnList = new BColumnListView(frame, id, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE,plain, hasHScrollbar);
			BMessage *msg1, *msg2;
			msg1 = new BMessage(YABFILEBOXINVOKE);
			msg1->AddPointer("source", myColumnList);
			msg2 = new BMessage(YABFILEBOXSELECT);
			msg2->AddPointer("source", myColumnList);
			myColumnList->SetInvocationMessage(msg1);
			myColumnList->SetSelectionMessage(msg2);
			myColumnList->SetSortingEnabled(false);
			myColumnList->SetSelectionMode(B_SINGLE_SELECTION_LIST);
			rgb_color rgb = {195,195,195,255};
			myColumnList->SetColor(B_COLOR_SELECTION, rgb);

			int flags = B_ALLOW_COLUMN_NONE;
			if(tmp.IFindFirst("movable")!=B_ERROR)
				flags += B_ALLOW_COLUMN_MOVE;
			if(tmp.IFindFirst("resizable")!=B_ERROR)
				flags += B_ALLOW_COLUMN_RESIZE;
			if(tmp.IFindFirst("popup")!=B_ERROR)
				flags += B_ALLOW_COLUMN_POPUP;
			if(tmp.IFindFirst("removable")!=B_ERROR)
				flags += B_ALLOW_COLUMN_REMOVE;
			myColumnList->SetColumnFlags((column_flags) flags);
			myColumnList->SetLatchWidth(0.0);
			myView->AddChild(myColumnList);

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::ColumnBoxAdd(const char* id, int column, int position, int height, const char* text)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(id), BColumnListView);
				if(myColumnList)
				{
					BRow *myRow = myColumnList->RowAt(position-1);
					if(!myRow) 
					{
						myRow = new BRow(height);
						myColumnList->AddRow(myRow, position);
						for(int j=0; j<myColumnList->CountColumns(); j++)
						{
							BYabField *myField = new BYabField("");
							myRow->SetField(myField, j);
						}
					}

					BYabField *myField = (BYabField*)myRow->GetField(column-1);
					myField->SetString(text, height);

					myColumnList->Refresh();
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "COLUMNBOX");
}

void YabInterface::FileBoxAdd(const char* columnbox, const char* name, int32 pos, double minWidth, double maxWidth, double width, const char* option)
{
	BString tmp(option);
	alignment align = B_ALIGN_LEFT;
	if(tmp.IFindFirst("align-left")!=B_ERROR)
		align = B_ALIGN_LEFT;
	if(tmp.IFindFirst("align-center")!=B_ERROR)
		align = B_ALIGN_CENTER;
	if(tmp.IFindFirst("align-right")!=B_ERROR)
		align = B_ALIGN_RIGHT;
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					BYabColumn *myColumn = new BYabColumn(name,width,maxWidth, minWidth,width, align);
					myColumnList->AddColumn(myColumn, pos-1);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

void YabInterface::FileBoxClear(const char* columnbox)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					myColumnList->Clear();
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

void YabInterface::ColumnBoxRemove(const char* columnbox, int position)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					BRow *myRow;
					myRow = myColumnList->RowAt(position-1);
					if(myRow)
					{
						myColumnList->RemoveRow(myRow);
						delete myRow;
					}
					else
					{
						w->Unlock();
						ErrorGen("Row not found");
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

void YabInterface::ColumnBoxColor(const char* columnbox, const char* option, int r, int g, int b)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	BString tmp(option);
	ColumnListViewColor col;
	if(tmp.IFindFirst("selection-text")!=B_ERROR)
		col = B_COLOR_SELECTION_TEXT;
	else if(tmp.IFindFirst("non-focus-selection")!=B_ERROR)
		col = B_COLOR_NON_FOCUS_SELECTION;
	else if(tmp.IFindFirst("selection")!=B_ERROR)
		col = B_COLOR_SELECTION;
	else if(tmp.IFindFirst("text")!=B_ERROR)
		col = B_COLOR_TEXT;
	else if(tmp.IFindFirst("row-divider")!=B_ERROR)
		col = B_COLOR_ROW_DIVIDER;
	else if(tmp.IFindFirst("background")!=B_ERROR)
		col = B_COLOR_BACKGROUND;
	else
		ErrorGen("Invalid option");
		
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					rgb_color rgb = {r,g,b,255};
					myColumnList->SetColor(col, rgb);
					myColumnList->Refresh();
					myColumnList->Invalidate();
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

void YabInterface::ColumnBoxSelect(const char* columnbox, int position)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					if(position == 0)
						myColumnList->DeselectAll();
					else
					{
						myColumnList->AddToSelection(myColumnList->RowAt(position-1));
						myColumnList->ScrollTo(myColumnList->RowAt(position-1));
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

const char* YabInterface::ColumnBoxGet(const char* columnbox, int column, int position)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					BRow* myRow = myColumnList->RowAt(position-1);
					if(myRow)
					{
						BYabField *myField = cast_as(myRow->GetField(column-1), BYabField);
						if(myField) 
						{
							const char* t = myField->String();
							w->Unlock();
							return t;
						}
						w->Unlock();
						ErrorGen("Column not found");
					}
					w->Unlock();
					ErrorGen("Row not found");
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

int YabInterface::ColumnBoxCount(const char* columnbox)
{
	YabView *myView = NULL;
	BColumnListView *myColumnList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myColumnList = cast_as(myView->FindView(columnbox), BColumnListView);
				if(myColumnList)
				{
					int32 ret = myColumnList->CountRows();
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(columnbox, "COLUMNBOX");
}

void YabInterface::ListboxAdd(const char* listbox, const char* item)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(listbox), BListView);
				// myList = (BListView*)myView->FindView(listbox);
				if(myList)
				{
					myList->AddItem(new BStringItem(item));
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(listbox, "LISTBOX");
}

void YabInterface::ListboxAdd(const char* listbox, int pos, const char* item)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(listbox), BListView);
				if(myList)
				{
					myList->AddItem(new BStringItem(item), pos-1);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(listbox, "LISTBOX");
}

void YabInterface::ListboxSelect(const char* listbox, int pos)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(listbox), BListView);
				if(myList)
				{
					if(pos == 0)
						myList->DeselectAll();
					else
					{
						myList->Select(pos-1);
						myList->ScrollToSelection();
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(listbox, "LISTBOX");
}

void YabInterface::ListboxRemove(const char* listbox, int pos)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(listbox), BListView);
				if(myList)
				{
					myList->RemoveItem(pos-1);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(listbox, "LISTBOX");
}

const char* YabInterface::ListboxGet(const char* listbox, int pos)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				columntext[0] = '\0';
				myList = cast_as(myView->FindView(listbox), BListView);
				if(myList)
				{
					BStringItem *t = cast_as(myList->ItemAt(pos-1), BStringItem);
					if(t)
					{
						const char* txt = t->Text();
						w->Unlock();
						return txt;
					}
					else
					{
						w->Unlock();
						ErrorGen("Item not found");
					}
				}
				w->Unlock();
			}
		}
	}
	Error(listbox, "LISTBOX");
}

int YabInterface::ListboxCount(const char* listbox)
{
	int ret = 0;
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(listbox), BListView);
				if(myList)
				{
					ret = myList->CountItems();
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(listbox, "LISTBOX");
}

void YabInterface::DrawSet1(const char* option, const char* window)
{
	BString tmp(option);

	BFont myFont;
	BString opt;

	// Font family
	int pos1 = 0;
	int pos2 = tmp.FindFirst(',');
	if(pos2 != B_ERROR) 
	{
		tmp.CopyInto(opt, pos1, pos2-pos1);
		while(opt[0] == ' ') opt.RemoveFirst(" ");
		while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
		font_family fam;
		sprintf((char*)fam, "%s" , opt.String());
		if(myFont.SetFamilyAndFace(fam, B_REGULAR_FACE) == B_OK)
		{
			// myView->SetFont(&myFont, B_FONT_FAMILY_AND_STYLE);

			// Font style
			pos1 = pos2+1;
			pos2 = tmp.FindFirst(',', pos2+1);
			if(pos2 != B_ERROR) 
			{
				tmp.CopyInto(opt, pos1, pos2-pos1);
				while(opt[0] == ' ') opt.RemoveFirst(" ");
				while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
				font_style style;
				sprintf((char*)style, "%s" , opt.String());
				if(myFont.SetFamilyAndStyle(fam,style) == B_OK)
				{
					// myView->SetFont(&myFont, B_FONT_FAMILY_AND_STYLE);

					// Font size
					pos1 = pos2+1;
					pos2 = tmp.FindFirst(',', pos2+1);
					if(pos2 == B_ERROR) pos2 = tmp.Length();
					tmp.CopyInto(opt, pos1, pos2-pos1);
					while(opt[0] == ' ') opt.RemoveFirst(" ");
					while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
					double size = atof(opt.String());
					myFont.SetSize(size);
					// myView->SetFont(&myFont, B_FONT_SIZE);

					if(pos2 != tmp.Length())
					{
						pos1 = pos2+1;
						pos2 = tmp.FindFirst(',', pos2+1);
						if(pos2 == B_ERROR) pos2 = tmp.Length();
						tmp.CopyInto(opt, pos1, pos2-pos1);
						while(opt[0] == ' ') opt.RemoveFirst(" ");
						while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
						if(opt.IFindFirst("bold") != B_ERROR)
							myFont.SetFace(B_BOLD_FACE);
						else if(opt.IFindFirst("italic") != B_ERROR)
							myFont.SetFace(B_ITALIC_FACE);
						else if(opt.IFindFirst("regular") != B_ERROR)
							myFont.SetFace(B_REGULAR_FACE);
						else if(opt.IFindFirst("outlined") != B_ERROR)
							myFont.SetFace(B_OUTLINED_FACE);
						else if(opt.IFindFirst("strikeout") != B_ERROR)
							myFont.SetFace(B_STRIKEOUT_FACE);
						else if(opt.IFindFirst("underscore") != B_ERROR)
							myFont.SetFace(B_UNDERSCORE_FACE);

						if(pos2 != tmp.Length())
						{
							pos1 = pos2 + 1;
							pos2 = tmp.FindFirst(',', pos2+1);
							if(pos2 == B_ERROR) pos2 = tmp.Length();
							tmp.CopyInto(opt, pos1, pos2-pos1);
							while(opt[0] == ' ') opt.RemoveFirst(" ");
							while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
							float rotation = atof(opt.String());
							myFont.SetRotation(rotation);

							if(pos2 != tmp.Length())
							{
								pos1 = pos2 + 1;
								pos2 = tmp.FindFirst(',', pos2+1);
								if(pos2 == B_ERROR) pos2 = tmp.Length();
								tmp.CopyInto(opt, pos1, pos2-pos1);
								while(opt[0] == ' ') opt.RemoveFirst(" ");
								while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
								float shear = atof(opt.String());
								myFont.SetShear(shear);
							}
						}

					}
					

					// Font flags
					/*bool looping = true;
					while(looping)
					{
						pos1 = pos2+1;
						pos2 = tmp.FindFirst(',', pos2+1);
						if(pos2 == B_ERROR) 
						{
							looping = false;
							pos2 = tmp.Length();
						}
						tmp.CopyInto(opt, pos1, pos2-pos1);
						while(opt[0] == ' ') opt.RemoveFirst(" ");
						while(opt[opt.Length()-1] == ' ') opt.RemoveLast(" ");
					}*/
				}
			}
		}
	}
	else if(tmp.IFindFirst("system-plain")!=B_ERROR)
	{
		myFont = be_plain_font;
		// myView->SetFont(&myFont);
	}
	else if(tmp.IFindFirst("system-fixed")!=B_ERROR)
	{
		myFont = be_fixed_font;
		// myView->SetFont(&myFont);
	}
	else if(tmp.IFindFirst("system-bold")!=B_ERROR)
	{
		myFont = be_bold_font;
		// myView->SetFont(&myFont);
	}
	
	YabView *myView = cast_as((BView*)viewList->GetView(window), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			myView->SetFont(&myFont);
			YabDrawing *t = new YabDrawing();
			t->command = 12;
			t->font = myFont;
			myView->drawList->AddItem(t);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bview = b->FindView(window);
                        if(bview)
                        {
                                b->Lock();
                                bview->SetFont(&myFont);
                                bview->Sync();
                                b->Unlock();
                                return;
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), window))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *b = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
                                        b->Lock();
                                	bView->SetFont(&myFont);
                                        bView->Sync();
                                        b->Unlock();

                                        // myView->Draw(myView->Bounds());
                                        w->Unlock();
                                        return;
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        }  
		}
		Error(window, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::DrawSet2(int fillorstroke, const char* mypattern)
{
	BString tmp(mypattern);
	if(fillorstroke) drawStroking = true; else drawStroking = false;
	if(tmp.IFindFirst("HighSolidFill")!=B_ERROR)
		yabPattern = B_SOLID_HIGH;
	else if(tmp.IFindFirst("LowSolidFill")!=B_ERROR)
		yabPattern = B_SOLID_LOW;
	else if(tmp.IFindFirst("CheckeredFill")!=B_ERROR)
		yabPattern = B_MIXED_COLORS;
	else
	{
		for(int i = 0; i<8; i++)
			if(i<tmp.Length()+3)
				{
					int t = 0;
					t = 100*(tmp[i*3]-48) + 10*(tmp[i*3+1]-48) + (tmp[i*3+2]-48);
					yabPattern.data[i] = t;
				}
			else
				yabPattern.data[i] = 0;
	}
}

int YabInterface::DeskbarParam(const char* option)
{
	int ret;
	BString opt(option);
	BDeskbar(deskbar);
	if( opt.IFindFirst("position") != B_ERROR )
	{
		deskbar_location pos = deskbar.Location();
		switch (pos)
		{
			case B_DESKBAR_LEFT_TOP:ret=1;
				break;
			case B_DESKBAR_TOP:ret=2;
				break;
			case B_DESKBAR_RIGHT_TOP:ret=3;
				break;
			case B_DESKBAR_RIGHT_BOTTOM:ret=4;
				break;
			case B_DESKBAR_BOTTOM:ret=5;
				break;
			case B_DESKBAR_LEFT_BOTTOM:ret=6;
				break;
		}
	}
	else if( opt.IFindFirst("expanded") != B_ERROR )
	{
		ret = (int)deskbar.IsExpanded();
	}
	else if( opt.IFindFirst("width") != B_ERROR )
	{
		ret = (int)deskbar.Frame().Width()+1;
	}
	else if( opt.IFindFirst("height") != B_ERROR )
	{
		ret = (int)deskbar.Frame().Height()+1;
	}
	else if( opt.IFindFirst("x") != B_ERROR )
	{
		ret = (int)deskbar.Frame().left;
	}
	else if( opt.IFindFirst("y") != B_ERROR )
	{
		ret = (int)deskbar.Frame().top;
	}
	return ret;
}

int YabInterface::DesktopParam(bool isWidth)
{
	BScreen myScreen(B_MAIN_SCREEN_ID);
	display_mode t;
	myScreen.GetMode(&t);
	if(isWidth) return t.virtual_width;
	return t.virtual_height;
}

int YabInterface::WindowGet(const char* view, const char* option)
{
	int opt = 0;
	int ret = -1;
	BString t(option);
    if(t.IFindFirst("position-x")!=B_ERROR) opt = 1;
	else if(t.IFindFirst("position-y")!=B_ERROR) opt = 2;
	else if(t.IFindFirst("minimum-width")!=B_ERROR) opt = 5;
	else if(t.IFindFirst("minimum-height")!=B_ERROR) opt = 6;
	else if(t.IFindFirst("maximum-width")!=B_ERROR) opt = 7;
	else if(t.IFindFirst("maximum-height")!=B_ERROR) opt = 8;
	else if(t.IFindFirst("width")!=B_ERROR) opt = 3;
	else if(t.IFindFirst("height")!=B_ERROR) opt = 4;	
	else if(t.IFindFirst("exists")!=B_ERROR) opt = 9;	


	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			if(opt>0 && opt<5)
			{
				BRect r = w->Frame();
				switch(opt)
				{
					case 1: ret = (int)r.LeftTop().x;
						break;
					case 2: ret = (int)r.LeftTop().y;
						break;
					case 3: ret = r.IntegerWidth();
						break;
					case 4: ret = r.IntegerHeight();
						break;
				}
			}
			if(opt>4)
			{
				float x1,y1,x2,y2;
				w->GetSizeLimits(&x1,&x2,&y1,&y2);
				switch(opt)
				{
					case 5: ret = (int)x1;
						break;
					case 6: ret = (int)y1;
						break;
					case 7: ret = (int)x2;
						break;
					case 8: ret = (int)y2;
						break;
				}
			}
			if (opt==9)
			{
				return true;
			}
		}
		else
		{
			if (opt==9)
			{
				return false;
			} 
			else
			ErrorGen("Unable to lock window");
		}
	}
	else
	{
		if (opt==9)
		{
			return false;
		} 
		else		
			Error(view, "VIEW");
	}
	return ret;
}

int YabInterface::ViewGet(const char* view, const char* option) 
{
	int opt = 0;
	int ret = -1;
	BString t(option);
	YabView *myView = NULL;
    if(t.IFindFirst("position-x")!=B_ERROR) opt = 1;
	else if(t.IFindFirst("position-y")!=B_ERROR) opt = 2;
	else if(t.IFindFirst("width")!=B_ERROR) opt = 3;
	else if(t.IFindFirst("height")!=B_ERROR) opt = 4;	
	else if(t.IFindFirst("exists")!=B_ERROR) opt = 5;
	else if(t.IFindFirst("focused")!=B_ERROR) opt = 6;
	else
		ErrorGen("Invalid option");

	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BView *theView = cast_as(myView->FindView(view), BView);
				if(theView)
				{
					BRect r = theView->Frame();
					switch(opt)
					{
						case 1: ret = (int)r.LeftTop().x;
							break;
						case 2: ret = (int)r.LeftTop().y;
							break;
						case 3: ret = r.IntegerWidth();
							break;
						case 4: ret = r.IntegerHeight();
							break;
						case 5: ret = true;
							break;
						case 6: ret = theView->IsFocus();
							break;
					}
					w->Unlock();
					return ret;					
				}
				w->Unlock();
			}
		}
	} 
	if( opt == 5 )
		ret=false;
	else
		Error(view, "VIEW");

	return ret;
}

void YabInterface::ClipboardCopy(const char* text)
{
	BMessage *clip = (BMessage *)NULL; 

	if (be_clipboard->Lock())
	{ 
		be_clipboard->Clear(); 
		if (clip = be_clipboard->Data())
		{ 
			clip->AddData("text/plain", B_MIME_TYPE, text, strlen(text)); 
			be_clipboard->Commit(); 
		} 
		be_clipboard->Unlock(); 
	}
}

int YabInterface::Printer(const char* docname, const char *config, const char* view)
{
	BPrintJob job(docname);
	BMessage *setup;
	BFile myFile(config, B_READ_ONLY);

	if(myFile.InitCheck()!=B_OK)
	{
		if(job.ConfigPage()==B_OK)
			setup = job.Settings();
		else
		{
			// (new BAlert(_L("Printer Error!"),_L("Could not setup the printer!"), "Ok"))->Go();
			return 1;
		}
	}
	else
	{
		setup = new BMessage();
		if(setup->Unflatten(&myFile)!=B_OK)
		{
			if(job.ConfigPage()==B_OK)
				setup = job.Settings();
			else
			{
				// (new BAlert(_L("Printer Error!"),_L("Could not setup the printer!"), "Ok"))->Go();
				return 1;
			}
		}
		else
			if(job.IsSettingsMessageValid(setup))
				job.SetSettings(setup);
			else
			{
				// (new BAlert(_L("Printer Error!"),_L("Could not setup the printer!"), "Ok"))->Go();
				return 2;
			}
	}

	int32 firstPage, lastPage, nbPages;
	BRect printableRect = job.PrintableRect();
	firstPage =0 ;//= job.FirstPage(); Since we aren't calling the set-up print pages, firstpage is always 0
	lastPage = job.LastPage();
	// printf("PRINTER DEBUG Printable BRect %f %f %f %f\n", printableRect.left,printableRect.top, printableRect.right, printableRect.bottom);
	YabView *myView = NULL;
	BView *newView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				newView = myView->FindView(view);
				w->Unlock();
				if(newView)
					break;
			}
		}
	}

	if(!newView)
	{
		// (new BAlert(_L("Printer Error!"),_L("Could not setup the printer!"), "Ok"))->Go();
		return 3;
	}


	BWindow *w = newView->Window();
	w->Lock();

	int32 viewHeight = newView->Bounds().IntegerHeight();
	float viewWidth = newView->Bounds().Width();
	if(is_kind_of(newView, YabText))
		viewHeight = (int32)((YabText*)newView)->TextHeight(0, ((YabText*)newView)->CountLines());
	if(is_kind_of(newView, BScrollView))
	{
		float a,b;

		if(((BScrollView*)newView)->ScrollBar(B_VERTICAL))
		{
			((BScrollView*)newView)->ScrollBar(B_VERTICAL)->GetRange(&a, &b);
			viewHeight = viewHeight + (int32)b;
			if(((BScrollView*)newView)->ScrollBar(B_HORIZONTAL))
				viewHeight -= (int32)B_H_SCROLL_BAR_HEIGHT;
		}
		if(((BScrollView*)newView)->ScrollBar(B_HORIZONTAL))
		{
			((BScrollView*)newView)->ScrollBar(B_HORIZONTAL)->GetRange(&a, &b);
			viewWidth = viewWidth + b;
			if(((BScrollView*)newView)->ScrollBar(B_VERTICAL))
				viewWidth -= B_V_SCROLL_BAR_WIDTH;
		}

		if(((BScrollView*)newView)->ScrollBar(B_VERTICAL))
			newView = ((BScrollView*)newView)->ScrollBar(B_VERTICAL)->Target();
		else
			newView = ((BScrollView*)newView)->ScrollBar(B_HORIZONTAL)->Target();
	}

	// printf("  %d %f \n", viewHeight, viewWidth);
	int32 printableHeight = printableRect.IntegerHeight();
	float printableWidth = printableRect.Width();
	w->Unlock();

	int32 maxPages = viewHeight / printableHeight + 1;
	if(lastPage>maxPages) 
		lastPage = maxPages;
		nbPages = lastPage - firstPage + 1;

	 //printf("PRINTER DEBUG First Page %d Last Page %d \n", firstPage, lastPage);
	// printf("PRINTER DEBUG View Height %d Printable Height %d \n", viewHeight, printableHeight);

	if(nbPages<=0)
	{
		// (new BAlert(_L("Printer Error!"),_L("Could not setup the printer!"), "Ok"))->Go();
		return 4;
	}

	job.BeginJob();
	bool can_continue = job.CanContinue();

	w->Lock();

	bool hasWordWrap;
	float textWidth, textHeight;

	if(is_kind_of(newView, YabText))
	{
		int lineheight; 
		hasWordWrap = ((YabText*)newView)->DoesWordWrap();
		if(!hasWordWrap) ((YabText*)newView)->SetWordWrap(true);
		lineheight = (int)((YabText*)newView)->LineHeight();
		textWidth = ((YabText*)newView)->TextRect().Width();
		textHeight = ((YabText*)newView)->TextRect().Height();

		((YabText*)newView)->SetTextRect(BRect(0,0,printableWidth, viewHeight));
		
		printableHeight -= printableHeight%lineheight;
	}

	int32 newHeight;
	if(printableHeight<viewHeight)
		newHeight = printableHeight;
	else
		newHeight = viewHeight;

	if(viewWidth<printableWidth)
		printableWidth = viewWidth;

	BRect currentRect;
	currentRect.SetLeftTop(BPoint(0,0));
	currentRect.SetRightBottom(BPoint(printableWidth, newHeight));

	for(int i=1; i<firstPage; i++)
	{
		currentRect.SetLeftTop(BPoint(0,newHeight+1));
		if(printableHeight<viewHeight-newHeight)
			newHeight = newHeight + printableHeight;
		else
			newHeight = viewHeight;
		currentRect.SetRightBottom(BPoint(printableWidth, newHeight));
		// printf("PRINTER DEBUG Skipping current BRect: %f %f %f %f\n", currentRect.left,currentRect.top, currentRect.right, currentRect.bottom);
	}

	// printf("PRINTER DEBUG Spooling current BRect %f %f %f %f\n", currentRect.left,currentRect.top, currentRect.right, currentRect.bottom);

	for(int i=firstPage; i<=lastPage; i++)
	{
		job.DrawView(newView, currentRect, printableRect.LeftTop());
		job.SpoolPage();
		can_continue = job.CanContinue();
		if(!can_continue)
			break;
		currentRect.SetLeftTop(BPoint(0,newHeight+1));
		if(printableHeight<viewHeight-newHeight)
			newHeight = newHeight + printableHeight;
		else
			newHeight = viewHeight;

		currentRect.SetRightBottom(BPoint(printableWidth, newHeight));
		if(currentRect.bottom<currentRect.top)
			break;
		// printf("PRINTER DEBUG Spooling current BRect: %f %f %f %f\n", currentRect.left,currentRect.top, currentRect.right, currentRect.bottom);
	}

	if(is_kind_of(newView, YabText))
	{
		((YabText*)newView)->SetWordWrap(hasWordWrap);
		((YabText*)newView)->SetTextRect(BRect(0,0,textWidth, textHeight));
	}

	w->Unlock();

	if(can_continue)
		job.CommitJob();
	else
		// (new BAlert(_L("Printer Error!"),_L("Could not setup the printer!"), "Ok"))->Go();
		return 5;
	return 0;
}

void YabInterface::PrinterConfig(const char* config)
{
	BPrintJob job("");
	if(job.ConfigPage()==B_OK)
	{
		BMessage *setup = job.Settings();
		BFile myFile(config, B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
		if(myFile.InitCheck()==B_OK)
			setup->Flatten(&myFile);
	}
}

const char* YabInterface::ClipboardPaste()
{
	const char *text; 
	int32 textlen; 
	BString returnstring; 
	BMessage *clip = (BMessage *)NULL; 

	if (be_clipboard->Lock()) 
	{ 
		BMessage *clip = be_clipboard->Data();
		clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &textlen); 
		be_clipboard->Unlock();
		if (text != NULL) {
			returnstring.SetTo(text, textlen);
		}
	} 

	return returnstring;
}

int YabInterface::NewAlert(const char* text, const char* button1, const char* button2, const char* button3, const char* option)
{
	alert_type tmp;
	BString typ(option);
	tmp = B_EMPTY_ALERT;
	if(typ.IFindFirst("info")!=B_ERROR) tmp = B_INFO_ALERT;
	else if(typ.IFindFirst("idea")!=B_ERROR) tmp = B_IDEA_ALERT;
	else if(typ.IFindFirst("warning")!=B_ERROR) tmp = B_WARNING_ALERT;
	else if(typ.IFindFirst("stop")!=B_ERROR) tmp = B_STOP_ALERT;

	if(!strcmp(button2,"")) button2 = NULL;
	if(!strcmp(button3,"")) button3 = NULL;
	
	return (new BAlert("Alert!",text,button1,button2,button3,B_WIDTH_AS_USUAL,tmp))->Go() + 1;
}

void YabInterface::Calendar(double x, double y, const char* id, const char* format, const char* date, const char* view)
{
	int day, month, year, look, myformat;
	BString tYear, tMonth, tDay;
	BString tDate(date);
	tDate.MoveInto(tYear, 6,4);
	tDate.MoveInto(tMonth, 3,2);
	tDate.MoveInto(tDay, 0,2);
	BString tFormat(format);
	look = CC_DOT_DIVIDER;
	myformat = CC_DD_MM_YYYY_FORMAT;
	if(tFormat.FindFirst("/")!=B_ERROR)
		look = CC_SLASH_DIVIDER;
	else if(tFormat.FindFirst("-")!=B_ERROR)
		look = CC_MINUS_DIVIDER;
	if(tFormat.IFindFirst("MD")!=B_ERROR)
		myformat = CC_MM_DD_YYYY_FORMAT;

	year = atoi(tYear.String());
	if(myformat == CC_MM_DD_YYYY_FORMAT)
	{
		month = atoi(tDay.String());
		day = atoi(tMonth.String());
	}
	else
	{
		month = atoi(tMonth.String());
		day = atoi(tDay.String());
	}
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			CalendarControl* myCalendar = new CalendarControl(BPoint(x,y),id,day, month, year, myformat, look);
			if(w->layout == -1)
				myCalendar->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				myCalendar->SetResizingMode(w->layout);
			myView->AddChild(myCalendar);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

const char* YabInterface::Calendar(const char* id)
{
	const char* txt;
	YabView *myView = NULL;
	CalendarControl *myCalendar = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myCalendar = cast_as(myView->FindView(id), CalendarControl);
				if(myCalendar)
				{
					txt = myCalendar->Text();
					w->Unlock();
					return txt;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "CALENDAR");
}

void YabInterface::Calendar(const char* id, const char* date)
{
	int day, month, year, look, myformat;
	BString tYear, tMonth, tDay;
	BString tDate(date);
	tDate.MoveInto(tYear, 6,4);
	tDate.MoveInto(tMonth, 3,2);
	tDate.MoveInto(tDay, 0,2);

	YabView *myView = NULL;
	CalendarControl *myCalendar = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myCalendar = cast_as(myView->FindView(id), CalendarControl);
				if(myCalendar)
				{
					myformat = myCalendar->GetFlags();
					year = atoi(tYear.String());
					if(myformat == CC_MM_DD_YYYY_FORMAT)
					{
						month = atoi(tDay.String());
						day = atoi(tMonth.String());
					}
					else
					{
						month = atoi(tMonth.String());
						day = atoi(tDay.String());
					}

					myCalendar->SetDate(day,month,year);

					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "CALENDAR");
}

void YabInterface::Scrollbar(const char* id, int format, const char* view)
{
	YabView *myView = NULL;
	BView *myBView = NULL;
	if(format != 0)
	{
		bool hasHor = false, hasVer = false;
		if(format>1) hasHor = true;
		if(format == 1 || format == 3) hasVer = true;

		for(int i=0; i<viewList->CountItems(); i++)
		{
			myView = cast_as((BView*)viewList->ItemAt(i), YabView);
			if(myView)
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					w->Lock();
					myBView = myView->FindView(view);
					if(myBView)
					{
						if(myView->RemoveChild(myBView))
						{
							BScrollView *myScrollView = new BScrollView(id, myBView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, hasHor, hasVer);
							if(w->layout == -1)
								myScrollView->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
							else
								myScrollView->SetResizingMode(w->layout);
							myScrollView->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
							myView->AddChild(myScrollView);
							myScrollView->SetViewColor(myBView->ViewColor());
							w->Unlock();
							return;
						}
					}
					w->Unlock();
				}
			}
		}
		Error(view, "VIEW");
	}
	ErrorGen("Unknown option");
}

void YabInterface::ScrollbarSet(const char* scrollview, const char* option, double position)
{
	BString tOption(option);
	orientation isHorizontal;
	if(tOption.IFindFirst("Vertical Position")!=B_ERROR) isHorizontal = B_VERTICAL;
	if(tOption.IFindFirst("Horizontal Position")!=B_ERROR) isHorizontal = B_HORIZONTAL;
	if(isHorizontal == B_VERTICAL || isHorizontal == B_HORIZONTAL)
	{
		YabView *myView = NULL;
		BScrollView *myScrollView = NULL;
		for(int i=0; i<viewList->CountItems(); i++)
		{
			myView = cast_as((BView*)viewList->ItemAt(i), YabView);
			if(myView)
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					w->Lock();
					myScrollView = cast_as((BView*)myView->FindView(scrollview),BScrollView);
					if(myScrollView)
					{
						BScrollBar *myScrollbar = myScrollView->ScrollBar(isHorizontal);
						if(myScrollbar) 
							myScrollbar->SetValue(position);
						else
							ErrorGen("SCROLLBAR is not valid!");
						w->Unlock();
						return;
					}
					w->Unlock();
				}
			}
		}
		Error(scrollview, "SCROLLBAR");
	}
	ErrorGen("Unknown option");
}

void YabInterface::ScrollbarSet(const char* scrollview, const char* option)
{
	BString tOption(option);
	border_style border;
	if(tOption.IFindFirst("no-border")!=B_ERROR) 
		border = B_NO_BORDER;
	else if(tOption.IFindFirst("plain-border")!=B_ERROR) 
		border = B_PLAIN_BORDER;
	else if(tOption.IFindFirst("fancy-border")!=B_ERROR) 
		border = B_FANCY_BORDER;
	else
		ErrorGen("Invalid option");

	YabView *myView = NULL;
	BScrollView *myScrollView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myScrollView = cast_as((BView*)myView->FindView(scrollview),BScrollView);
				if(myScrollView)
				{
					myScrollView->SetBorder(border);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(scrollview, "SCROLLBAR");
}

void YabInterface::ScrollbarSet(const char* scrollview, const char* option, double opt1, double opt2)
{
	BString tOption(option);
	orientation isHorizontal;
	int isRange = -1;
	if(tOption.IFindFirst("Vertical")!=B_ERROR) isHorizontal = B_VERTICAL;
	if(tOption.IFindFirst("Horizontal")!=B_ERROR) isHorizontal = B_HORIZONTAL;
	if(tOption.IFindFirst("Range")!=B_ERROR) isRange = 1;
	if(tOption.IFindFirst("Steps")!=B_ERROR) isRange = 0;
	if((isHorizontal == B_VERTICAL || isHorizontal == B_HORIZONTAL) && isRange != -1)
	{
		YabView *myView = NULL;
		BScrollView *myScrollView = NULL;
		for(int i=0; i<viewList->CountItems(); i++)
		{
			myView = cast_as((BView*)viewList->ItemAt(i), YabView);
			if(myView)
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					w->Lock();
					myScrollView = cast_as((BView*)myView->FindView(scrollview),BScrollView);
					if(myScrollView)
					{
					/*
						if(isRange)
						{
							BRect f(myView->Bounds());
							// printf("%f %f\n", opt1, opt2);
							if(isHorizontal == B_HORIZONTAL)
							{
								f.left = opt1;
								f.right = opt2;
							}
							else
							{
								f.top = opt1;
								f.bottom = opt2;
							}
							myScrollView->SetDataRect(f);
						}
						else
						{
							BScrollBar *myScrollbar = myScrollView->ScrollBar(isHorizontal);
							if(myScrollbar) myScrollbar->SetSteps(opt1,opt2);
						}*/
						
						BScrollBar *myScrollbar = myScrollView->ScrollBar(isHorizontal);
						if(isRange == 1)
						{
							if(myScrollbar) 
								myScrollbar->SetRange(opt1,opt2);
							else
								ErrorGen("SCROLLBAR is not valid!");
						}
						else
						{
							if(myScrollbar) 
								myScrollbar->SetSteps(opt1,opt2);
							else
								ErrorGen("SCROLLBAR is not valid!");
						}
						
						w->Unlock();
						return;
					}
					w->Unlock();
				}
			}
		}
		Error(scrollview, "SCROLLBAR");
	}
	ErrorGen("Unknown option");
}

double YabInterface::ScrollbarGet(const char* scrollview, const char* option)
{
	BString tOption(option);
	orientation isHorizontal;
	double res = 0;
	if(tOption.IFindFirst("Vertical")!=B_ERROR) isHorizontal = B_VERTICAL;
	if(tOption.IFindFirst("Horizontal")!=B_ERROR) isHorizontal = B_HORIZONTAL;
	if(isHorizontal == B_VERTICAL || isHorizontal == B_HORIZONTAL)
	{
		YabView *myView = NULL;
		BScrollView *myScrollView = NULL;
		for(int i=0; i<viewList->CountItems(); i++)
		{
			myView = cast_as((BView*)viewList->ItemAt(i), YabView);
			if(myView)
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					w->Lock();
					myScrollView = (BScrollView*)myView->FindView(scrollview);
					if(myScrollView)
					{
						BScrollBar *myScrollbar = myScrollView->ScrollBar(isHorizontal);
						if(myScrollbar) 
							res = myScrollbar->Value();
						else
							ErrorGen("SCROLLBAR is not valid!");
						w->Unlock();
						return res;
					}
					w->Unlock();
				}
			}
		}
		Error(scrollview, "SCROLLBAR");
	}
	ErrorGen("Unknown option");
}

void YabInterface::SplitView(BRect frame, const char* id, int isVertical, int style, const char* view)
{
	orientation posture = isVertical>0?B_VERTICAL:B_HORIZONTAL;

	double pos;
	
	if(posture==B_VERTICAL)
		pos = (frame.right-frame.left)/2;
	else
		pos = (frame.bottom-frame.top)/2;

	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();

			BRect frame1(frame);
			BRect frame2(frame);

			if(posture==B_VERTICAL)
			{
				frame1.Set(0,0,pos,frame.bottom-frame.top);
				frame2.Set(pos+10,0,frame.right-frame.left, frame.bottom-frame.top);
			}
			else
			{
				frame1.Set(0,0,frame.right-frame.left, pos-10);
				frame2.Set(0,pos,frame.right-frame.left,frame.bottom-frame.top);
			}

			BString t1(id); t1 += "1";
			BString t2(id); t2 += "2";

			YabView *newView1 = new YabView(frame1, t1.String(), B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_NAVIGABLE_JUMP);
			viewList->AddView(t1.String(), newView1, TYPE_YABVIEW);

			YabView *newView2 = new YabView(frame2, t2.String(), B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_NAVIGABLE_JUMP);
			viewList->AddView(t2.String(), newView2, TYPE_YABVIEW);

			SplitPane *mySplit = new SplitPane(frame, id, (BView*)newView1, (BView*)newView2, 0);
			if(style)
				mySplit->SetBarThickness(10);
			else
				mySplit->SetBarThickness(5);
			mySplit->SetAlignment(posture);

			if(w->layout == -1)
				mySplit->SetResizingMode(B_FOLLOW_ALL_SIDES);
			else
				mySplit->SetResizingMode(w->layout);

			myView->AddChild(mySplit);

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::SplitView(const char* splitView, const char* option, double position)
{
	YabView *myView = NULL;
	SplitPane *mySplit = NULL;
	BString t(option);
	if(t.IFindFirst("Divider")==B_ERROR)
		ErrorGen("Unknown option");
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySplit = cast_as(myView->FindView(splitView), SplitPane);
				if(mySplit)
				{
					mySplit->SetBarPosition((int)position);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(splitView, "SPLITVIEW");
}

void YabInterface::SplitView(const char* splitView, const char* option, double left, double right)
{
	YabView *myView = NULL;
	SplitPane *mySplit = NULL;
	BString t(option);
	if(t.IFindFirst("MinimumSizes")==B_ERROR)
		ErrorGen("Unknown option");
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySplit = cast_as(myView->FindView(splitView), SplitPane);
				if(mySplit)
				{
					mySplit->SetMinSizeOne((int)left);
					mySplit->SetMinSizeTwo((int)right);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(splitView, "SPLITVIEW");
}

double YabInterface::SplitViewGet(const char* splitView, const char* option)
{
	double ret = -1;
	YabView *myView = NULL;
	SplitPane *mySplit = NULL;
	BString t(option);
	if(t.IFindFirst("Divider")==B_ERROR)
		ErrorGen("Unknown option");
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySplit = cast_as(myView->FindView(splitView), SplitPane);
				if(mySplit)
				{
					ret = (double)mySplit->GetBarPosition();
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(splitView, "SPLITVIEW");
}

void YabInterface::StackViews(BRect frame, const char* id, int number, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView && number<1000)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();

			YabStackView *myStackView = new YabStackView(frame, id, number);
			if(w->layout == -1)
				myStackView->SetResizingMode(B_FOLLOW_ALL);
			else
				myStackView->SetResizingMode(w->layout);
			// myTabView->SetFlags(B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);

			YabView *newViews[number];
			for(int i=0; i<number; i++)
			{
				BString t(id);
				t << i+1;
				newViews[i] = new YabView(myStackView->Bounds(), t.String(), B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_NAVIGABLE_JUMP);
				viewList->AddView(t.String(), newViews[i], TYPE_YABVIEW);

			}
			myStackView->AddViews((BView**)newViews);

			myView->AddChild(myStackView); 

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		if(number<1000) Error(view, "VIEW");
		else ErrorGen("Too many views!");
}

void YabInterface::StackViews(const char* id, int num)
{
	YabView *myView = NULL;
	YabStackView *myStackView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myStackView = cast_as(myView->FindView(id), YabStackView);
				if(myStackView)
				{
					myStackView->SelectView(num-1);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "STACKVIEW");
}

int YabInterface::StackViewGet(const char* id)
{
	int ret;
	YabView *myView = NULL;
	YabStackView *myStackView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myStackView = cast_as(myView->FindView(id), YabStackView);
				if(myStackView)
				{
					ret = myStackView->CurrentView();
					w->Unlock();
					return ret+1;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "STACKVIEW");
}

void YabInterface::DrawSet3(const char* option, int transparency)
{
	BString t(option);
	if(t.IFindFirst("alpha") != B_ERROR)
	{
		yabAlpha = transparency;
		if(yabAlpha<0) yabAlpha = 0;
		if(yabAlpha>255) yabAlpha = 255;
	}
}

void YabInterface::TextURL(double x, double y, const char* id, const char* text, const char* url, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			double h,b;
			b = be_plain_font->StringWidth(text)+1;
			h = be_plain_font->Size() + 1;
			URLView *s = new URLView(BRect(x,y,x+b,y+h), id, text, url);
			s->SetHoverEnabled(true);
			if(w->layout == -1)
				s->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				s->SetResizingMode(w->layout);
			s->SetFlags(B_WILL_DRAW);
			myView->AddChild(s);
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::TextURL(const char* id, const char* option, int r, int g, int b)
{
	int opt = 0;
	BString t(option);
	if(t.IFindFirst("label") != B_ERROR) opt = 1;
	if(t.IFindFirst("click") != B_ERROR) opt = 2;
	if(t.IFindFirst("mouse-over") != B_ERROR) opt = 3;

	YabView *myView = NULL;
	URLView *myURLView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myURLView = cast_as(myView->FindView(id), URLView);
				if(myURLView)
				{
					rgb_color col = {r,g,b,255};
					switch(opt)
					{
						case 1: myURLView->SetColor(col);
							break;
						case 2: myURLView->SetClickColor(col);
							break;
						case 3: myURLView->SetHoverColor(col);
							break;
						default: break;
					}
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TEXTURL");
}

void YabInterface::Menu(const char* menuHead, int isRadio, const char* view)
{
	bool radio = isRadio?true:false;
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BMenuBar *menubar;
			BMenu *menu = NULL;
			BMenuItem *item = NULL;
			w->Lock();

			menubar = cast_as(myView->FindView("menubar"), BMenuBar);
			if(menubar != NULL)
			{
				for(int i=0; i<menubar->CountItems(); i++)
					if(!strcmp( menubar->ItemAt(i)->Label(), menuHead))
					{
						menu = menubar->SubmenuAt(i);
						menu->SetRadioMode(radio);
						break;
					}
			}

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::SubMenu(const char* menuHead, const char* menuItem, const char* subMenuItem, const char* shortcut, const char* view)
{
	char myShortcut;
	int32 modifiers = 0;
	BString t(shortcut);
	if(t.Length()>1)
	{
		myShortcut = shortcut[t.Length()-1];
		if(t.IFindFirst("s")!=B_ERROR && t.IFindFirst("s")<t.Length()-1)
			modifiers = modifiers|B_SHIFT_KEY;
		if(t.IFindFirst("c")!=B_ERROR && t.IFindFirst("c")<t.Length()-1)
			modifiers = modifiers|B_CONTROL_KEY;
		if(t.IFindFirst("o")!=B_ERROR && t.IFindFirst("o")<t.Length()-1)
			modifiers = modifiers|B_OPTION_KEY;
	}
	else
		myShortcut = shortcut[0];
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);

	if(!strcmp(menuItem, "--"))
		return;

	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BMenuBar *menubar;
			YabMenu *menu = NULL;
			YabMenu *submenu = NULL;
			BMenuItem *item = NULL;
			w->Lock();
			menubar = cast_as(myView->FindView("menubar"), BMenuBar);
			if(menubar == NULL)
			{
				menubar = new BMenuBar(myView->Bounds(),"menubar");
				myView->AddChild(menubar);
			}

			for(int i=0; i<menubar->CountItems(); i++)
				if(!strcmp( menubar->ItemAt(i)->Label(), menuHead))
					menu = (YabMenu*)menubar->SubmenuAt(i);

			if(menu == NULL)
			{
				menu = new YabMenu(menuHead);
				menubar->AddItem(menu);
			}
			
			int isInMenu = -1;
			for(int i=0; i<menu->CountItems(); i++)
				if(!strcmp(menu->ItemAt(i)->Label(), menuItem))
				{
					isInMenu = i;
					break;
				}

			if(isInMenu == -1)
			{
				submenu = new YabMenu(menuItem);
				menu->AddItem(submenu);
			}
			else
			{
				submenu = cast_as(menu->SubmenuAt(isInMenu),YabMenu);
				if(submenu == NULL)
				{
					BMenuItem *myItem = menu->RemoveItem(isInMenu);
					delete myItem;

					submenu = new YabMenu(menuItem);
					menu->AddItem(submenu, isInMenu);
				}
			}

			if(!strcmp(subMenuItem,"--"))
				submenu->AddItem(new BSeparatorItem());
			else
				submenu->AddItem(new BMenuItem(subMenuItem, new BMessage(YABSUBMENU), myShortcut, modifiers));

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::SubMenu(const char* menuHead, const char* menuItem, int isRadio, const char* view)
{
	bool radio = isRadio?true:false;
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BMenuBar *menubar;
			BMenu *menu = NULL;
			BMenu *submenu = NULL;
			BMenuItem *item = NULL;
			w->Lock();

			menubar = cast_as(myView->FindView("menubar"), BMenuBar);
			if(menubar != NULL)
			{
				for(int i=0; i<menubar->CountItems(); i++)
					if(!strcmp( menubar->ItemAt(i)->Label(), menuHead))
					{
						menu = menubar->SubmenuAt(i);
						for(int j=0; j<menu->CountItems(); j++)
							if(!strcmp( menu->ItemAt(j)->Label(), menuItem))
							{
								submenu = menu->SubmenuAt(j);
								if(submenu)
								{
									submenu->SetRadioMode(radio);
									break;
								}
							}

						if(submenu) break;
					}
			}

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

int YabInterface::ColorControlGet(const char* colorcontrol, const char* option)
{
	int ret = -1;
	int myOption = -1;
	BString myString(option);
	if(myString.IFindFirst("Red") != B_ERROR) myOption = 0;
	if(myString.IFindFirst("Green") != B_ERROR) myOption = 1;
	if(myString.IFindFirst("Blue") != B_ERROR) myOption = 2;

	if(myOption<0) ErrorGen("Unknown option");

	YabView *myView = NULL;
	BColorControl *myCControl = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView && myOption>-1)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myCControl = cast_as(myView->FindView(colorcontrol), BColorControl);
				if(myCControl)
				{
					rgb_color t = myCControl->ValueAsColor();
					switch(myOption)
					{
						case 0: ret = t.red;
							break;
						case 1: ret = t.green;
							break;
						case 2: ret = t.blue;
							break;
					}
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(colorcontrol, "COLORCONTROL");
}

int YabInterface::SliderGet(const char* slider)
{
	int ret = -1;
	YabView *myView = NULL;
	BSlider *mySlider = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySlider = cast_as(myView->FindView(slider), BSlider);
				if(mySlider)
				{
					ret = mySlider->Value();
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(slider, "SLIDER");
}

void YabInterface::SubMenu3(const char* menuHead, const char* menuItem, const char* subMenuItem, const char* option, const char* view)
{
	int myOption = -1;
	BString t(option);
	if(t.IFindFirst("disable")!=B_ERROR) myOption = 0;
	if(t.IFindFirst("enable")!=B_ERROR) myOption = 1;
	if(t.IFindFirst("mark")!=B_ERROR) myOption = 2;
	if(t.IFindFirst("plain")!=B_ERROR) myOption = 3;
	if(t.IFindFirst("remove")!=B_ERROR) myOption = 4;

	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView && myOption>-1)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BMenuBar *menubar;
			BMenu *menu = NULL;
			BMenu *submenu = NULL;
			BMenuItem *item = NULL;
			w->Lock();

			menubar = cast_as(myView->FindView("menubar"), BMenuBar);
			if(menubar)
			{
				for(int i=0; i<menubar->CountItems(); i++)
				{
					if(!strcmp( menubar->ItemAt(i)->Label(), menuHead))
					{
						menu = menubar->SubmenuAt(i);
						for(int j=0; j<menu->CountItems(); j++)
						{
							if(!strcmp( menu->ItemAt(j)->Label(), menuItem))
							{
								submenu = menu->SubmenuAt(j);
								if(submenu)
								{
									for(int k=0; k<submenu->CountItems(); k++)
									{
										if(!strcmp( submenu->ItemAt(k)->Label(), subMenuItem))
										{
											switch(myOption)
											{
												case 0:
													submenu->ItemAt(k)->SetEnabled(false);
													break;
												case 1:
													submenu->ItemAt(k)->SetEnabled(true);
													break;
												case 2:
													submenu->ItemAt(k)->SetMarked(true);
													break;
												case 3:
													submenu->ItemAt(k)->SetMarked(false);
													break;
												case 4: 
													submenu->RemoveItem( submenu->ItemAt(k) );
													if(submenu->CountItems() == 0) {
														if(menu->RemoveItem(submenu))
															menu->AddItem(new BMenuItem(menuItem, new BMessage(YABMENU)), j);
													}
													break;
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}

			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		if(myOption>-1) Error(view, "VIEW");
		else ErrorGen("Unknown option");
}

void YabInterface::Menu3(const char* menuHead, const char* menuItem, const char* option,const char* view)
{
	int myOption = -1;
	BString t(option);
	if(t.IFindFirst("disable")!=B_ERROR) myOption = 0;
	if(t.IFindFirst("enable")!=B_ERROR) myOption = 1;
	if(t.IFindFirst("mark")!=B_ERROR) myOption = 2;
	if(t.IFindFirst("plain")!=B_ERROR) myOption = 3;
	if(t.IFindFirst("remove")!=B_ERROR) myOption = 4;

	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView && myOption>-1)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BMenuBar *menubar;
			BMenu *menu = NULL;
//			BMenuItem *item = NULL;
			w->Lock();

			menubar = cast_as(myView->FindView("menubar"), BMenuBar);
			if(menubar != NULL)
			{
				for(int i=0; i<menubar->CountItems(); i++)
					if(!strcmp( menubar->ItemAt(i)->Label(), menuHead))
					{
						menu = menubar->SubmenuAt(i);
						for(int j=0; j<menu->CountItems(); j++)
							if(!strcmp( menu->ItemAt(j)->Label(), menuItem))
							{
								switch(myOption)
								{
									case 0:
										menu->ItemAt(j)->SetEnabled(false);
										break;
									case 1:
										menu->ItemAt(j)->SetEnabled(true);
										break;
									case 2:
										menu->ItemAt(j)->SetMarked(true);
										break;
									case 3:
										menu->ItemAt(j)->SetMarked(false);
										break;
									case 4:
										menu->RemoveItem( menu->ItemAt(j) );
								}
								break;
							}
					}
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		if(myOption>-1) Error(view, "VIEW");
		else ErrorGen("Unknown option");
}

void YabInterface::SpinControl(double x, double y, const char* id, const char* label, int min, int max, int step, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			Spinner *mySpin = new Spinner(BRect(x,y,x+10,y+10), id, label, min, max, step, NULL);
			if(w->layout == -1)
				mySpin->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				mySpin->SetResizingMode(w->layout);
			myView->AddChild(mySpin);
			mySpin->SetViewColor(myView->ViewColor());
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
}

void YabInterface::SpinControl(const char* spinControl, int value)
{
	YabView *myView = NULL;
	Spinner *mySpin = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySpin = cast_as(myView->FindView(spinControl),Spinner);
				if(mySpin)
				{
					mySpin->SetValue(value);
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(spinControl, "SPINCONTROL");
}

int YabInterface::SpinControlGet(const char *spinControl)
{
	int ret = 0;
	YabView *myView = NULL;
	Spinner *mySpin = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				mySpin = cast_as(myView->FindView(spinControl),Spinner);
				if(mySpin)
				{
					ret = mySpin->Value();
					w->Unlock();
					return ret;
				}
				w->Unlock();
			}
		}
	}
	Error(spinControl, "SPINCONTROL");
}

const char* YabInterface::PopUpMenu(double x, double y, const char* menuItems, const char* view)
{
	BString t(menuItems);
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			const char* res;
			w->Lock();
			BPopUpMenu *returnMe = new BPopUpMenu( "YabPopup", false, false );
			returnMe->SetAsyncAutoDestruct(true);

			int oldi = 0;
			for(int i=0; i<t.Length(); i++)
				if(t[i]=='|')
				{
					BString j;
					t.CopyInto(j,oldi, i-oldi);
					if(j == "--")
						returnMe->AddItem(new BSeparatorItem());
					else
						returnMe->AddItem(new BMenuItem(_L(j.String()), NULL)); 
					oldi=i+1;
				}
			BString j;
			t.CopyInto(j,oldi, t.Length()-oldi);
			returnMe->AddItem(new BMenuItem(j.String(), NULL)); 

			BMenuItem *result = returnMe->Go(myView->ConvertToScreen(BPoint(x,y)), false, true);
			if(result)
			{
				res = result->Label();
				w->Unlock();
				return res;
			}
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");

	return ""; 
}

void YabInterface::DropBoxSelect(const char* dropbox, int position)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(dropbox), BMenuField);
				if(myMenuField)
				{
					BMenu *myMenu = (BMenu*)myMenuField->Menu();
					if(myMenu)
					{
						if(position<1) position = 1;
						if(position>=myMenu->CountItems()) 
							position = myMenu->CountItems();
						if(myMenu->CountItems()!=0)
							(myMenu->ItemAt(position-1))->SetMarked(true);
						w->Unlock();
						return;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(dropbox, "DROPBOX");
}

void YabInterface::DropBoxClear(const char* dropbox)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(dropbox), BMenuField);
				if(myMenuField)
				{
					BMenu *myMenu = (BMenu*)myMenuField->Menu();
					if(myMenu)
					{
						while(myMenu->CountItems()>0) 
						{
							BMenuItem *myItem = myMenu->RemoveItem(myMenu->CountItems()-1);
							delete myItem;
						}

						// bad workaround! Add an empty MenuItem and delete it again
						// so that the menu changes
						BMenuItem *myItem = new BMenuItem("", NULL);
						myMenu->AddItem(myItem);
						myItem->SetMarked(true);
						myMenu->RemoveItem(myItem);
						delete myItem;

						w->Unlock();
						return;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(dropbox, "DROPBOX");
}

void YabInterface::DropBoxRemove(const char* dropbox, int position)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(dropbox), BMenuField);
				if(myMenuField)
				{
					BMenu *myMenu = (BMenu*)myMenuField->Menu();
					if(myMenu)
					{
						position --;
						if(position<0) position = 0;
						if(position>=myMenu->CountItems()) 
							position = myMenu->CountItems()-1;

						if(myMenu->CountItems()!=0)
						{
							BMenuItem *myItem = myMenu->ItemAt(position);
							if(myItem->IsMarked())
							{
								if(myMenu->CountItems()>1 && position>0)
									(myMenu->ItemAt(position-1))->SetMarked(true);
								else if(myMenu->CountItems()>1 && position == 0)
									(myMenu->ItemAt(position+1))->SetMarked(true);
								else
								{
									BMenuItem *myItem = new BMenuItem("", NULL);
									myMenu->AddItem(myItem);
									myItem->SetMarked(true);
									myMenu->RemoveItem(myItem);
									delete myItem;
								}

							}
							myMenu->RemoveItem(myItem);
							delete myItem;
						}
						w->Unlock();
						return;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(dropbox, "DROPBOX");
}

int YabInterface::DropBoxCount(const char* dropbox)
{
	int ret = -1;
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(dropbox), BMenuField);
				if(myMenuField)
				{
					BMenu *myMenu = (BMenu*)myMenuField->Menu();
					if(myMenu)
					{
						ret = myMenu->CountItems();
						w->Unlock();
						return ret;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(dropbox, "DROPBOX");
}

const char* YabInterface::DropBoxGet(const char* dropbox, int position)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(dropbox), BMenuField);
				if(myMenuField)
				{
					BMenu *myMenu = (BMenu*)myMenuField->Menu();
					if(myMenu)
					{
						const char *ret;
						position --;
						if(position<0) position = 0;
						if(position>=myMenu->CountItems()) 
							position = myMenu->CountItems()-1;
						if(myMenu->CountItems()!=0)
						{
							ret = (myMenu->ItemAt(position))->Label();
							w->Unlock();
							return ret;
						}
						else
						{
							w->Unlock();
							return "";
						}
					}
				}
				w->Unlock();
			}
		}
	}
	Error(dropbox, "DROPBOX");
}

double YabInterface::MenuHeight()
{
	double ret = -1;
	for(int i=0; i<CountWindows(); i++)
	{
		YabWindow *w = cast_as(WindowAt(i), YabWindow);
                if(w)
                {
                	BMenuBar *menubar;
			w->Lock();
			menubar = cast_as(w->FindView("menubar"), BMenuBar);
			if(menubar)
			{
				ret = menubar->Bounds().bottom - menubar->Bounds().top;
				w->Unlock();
				break;
			}
			w->Unlock();
		}
	}
	return ret;
}

double YabInterface::TabHeight()
{
	BFont f = be_plain_font;
	font_height fh;
	f.GetHeight(&fh);
	return fh.ascent + fh.descent + fh.leading + 10.0f; 
}

double YabInterface::ScrollbarWidth()
{
	return B_H_SCROLL_BAR_HEIGHT;
}

const int YabInterface::IsMouseIn(const char* view)
{
	int t = 2;
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			t = myView->mouseMovedInfo;
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
	return 1-t;
}

const char* YabInterface::GetMouseIn()
{
	snooze(20000);
 	BString ts;
	mousemessagebuffer[0] = '\0';

	int handled = -1;
	YabView *myView = NULL;

	for(int i=0; i<CountWindows(); i++)
	{
		YabWindow *w = cast_as(WindowAt(i), YabWindow);
		if(w && w->IsActive()) 
		{
			w->Lock();

			BView *view = w->LastMouseMovedView();
			if(!view) 
			{
				w->Unlock();
				break;
			}

			if(is_kind_of(view->Parent(), BTextControl))
				view = view->Parent();
			if(is_kind_of(view->Parent(), Spinner))
				view = view->Parent();
			if(is_kind_of(view->Parent(), CalendarControl))
				view = view->Parent();
			if(is_kind_of(view->Parent(), BColorControl))
				view = view->Parent();
			if(is_kind_of(view->Parent(), YabText))
				view = view->Parent();
			if(is_kind_of(view->Parent(), BColumnListView))
				view = view->Parent();
			if(is_kind_of(view, BBox))
				view = view->Parent();
			if(is_kind_of(view, BMenuBar))
				view = view->Parent();

			BString name = view->Name();
			BPoint coordinates;
			uint32 buttons;

			view->GetMouse(&coordinates, &buttons);

			ts << name << ":" << (int)coordinates.x << ":" << (int)coordinates.y << ":";
			if(buttons & B_PRIMARY_MOUSE_BUTTON)
				ts << 1 << ":";
			else
				ts << 0 << ":";
			if(buttons & B_TERTIARY_MOUSE_BUTTON)
				ts << 1 << ":";
			else
				ts << 0 << ":";
			if(buttons & B_SECONDARY_MOUSE_BUTTON)
				ts << 1;
			else
				ts << 0;

			w->Unlock();
		}
	}

	strcpy(mousemessagebuffer, ts.String());
	return (char*)mousemessagebuffer;
}

const char* YabInterface::KeyboardMessages(const char* view)
{
	snooze(20000);
	keyboardbuffer[0] = '\0';
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BString t("");
			w->Lock();
			// if(!myView->IsFocus()) myView->MakeFocus(true);
			if(myView->IsFocus()) 
				t << myView->pressedKeys;
			w->Unlock();
			strcpy(keyboardbuffer, t.String());
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
		Error(view, "VIEW");
	return (char*)keyboardbuffer;
}

const char* YabInterface::GetMouseMessages(const char* view)
{
	snooze(20000);
	mousemessagebuffer[0] = '\0';
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			BString t("");
			w->Lock();
			t << myView->mouseX; t += ":";
			t << myView->mouseY; t += ":";
			t << myView->mouseLButton; t += ":";
			t << myView->mouseMButton; t += ":";
			t << myView->mouseRButton;
			w->Unlock();
			strcpy(mousemessagebuffer, t.String());
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
	{
       		for(int i=0; i<yabcanvas->CountItems(); i++)
       		{
               		YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
               		if(!strcmp(myView->Name(), view))
               		{
                       		YabWindow *w = cast_as(myView->Window(), YabWindow);
                       		if(w)
                       		{
					BString t("");
                               		w->Lock();
					t << myView->mouseX; t += ":";
					t << myView->mouseY; t += ":";
					t << myView->mouseLButton; t += ":";
					t << myView->mouseMButton; t += ":";
					t << myView->mouseRButton;
                               		w->Unlock();
					strcpy(mousemessagebuffer, t.String());
					return (char*)mousemessagebuffer;
                               	}
                               	else
                               		ErrorGen("Unable to lock window");
                       	}  
		}
		Error(view, "VIEW");
	}
	return (char*)mousemessagebuffer;
}

int YabInterface::ThreadKill(const char* option, int id)
{
	int isTeam = -1;
	int ret = 0;
	BString t(option);
	if(t.IFindFirst("teamid")!=B_ERROR) isTeam = 1;
	if(t.IFindFirst("threadid")!=B_ERROR) isTeam = 0;
	if(isTeam==1)
	{
		if(kill_team(id)==B_OK) ret = 1;
	}
	else if(isTeam==0)
	{
		if(kill_thread(id)==B_OK) ret = 1;
	}
	else
		ErrorGen("Unknown Option");

	return ret;
}

int YabInterface::ThreadGet(const char* option, const char* appname)
{
	int isTeam = -1;
	int ret = -1;
	BString t(option);
	if(t.IFindFirst("teamid")!=B_ERROR) isTeam = 1;
	if(t.IFindFirst("threadid")!=B_ERROR) isTeam = 0;
	if(isTeam==0)
	{
		ret = find_thread(appname);
		if(ret == B_NAME_NOT_FOUND) ret = -1;
	}
	else if(isTeam==1)
	{
		int32 cookie=0;
		team_info info;
		BString t(appname);
		while (get_next_team_info(&cookie, &info) == B_OK)
		{
			if(t.FindFirst(info.args)==B_OK)
			{
				ret = info.team;
				break;
			}
		}
	}
	else
		ErrorGen("Unknown Option");

	return ret;
}

void YabInterface::Bitmap(double w, double h, const char* id)
{
	char *t;
	BBitmap *b = new BBitmap(BRect(0,0,w-1,h-1), B_RGBA32, true);
	BView *bview = new BView(BRect(0,0,w-1,h-1), id, B_FOLLOW_NONE, 0);
	b->AddChild(bview);
	t = (char*)b->Bits();
	for(int i=0; i<w*h*4; i = i + 4)
	{
		t[i] = t[i+1] = t[i+2] = 255;
		t[i+3] = 0;
	}
	yabbitmaps->AddItem(b);
}

int YabInterface::BitmapColor(double x, double y, const char* bitmap, const char* option)
{
	int rgb = 0;
	BString tmp(option);
	if(tmp.IFindFirst("red")!=B_ERROR) rgb = 1;
	else if(tmp.IFindFirst("green")!=B_ERROR) rgb = 2;
	else if(tmp.IFindFirst("blue")!=B_ERROR) rgb = 3;
	else 
		ErrorGen("Unknown Option");

	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(bitmap);
		if(bview)
		{
			int t = b->BytesPerRow()*(int)y + (int)x*4;
			unsigned char* bits = (unsigned char*)b->Bits();
			if(t <= b->BitsLength())
				if(rgb == 1)
					return (int)bits[t];
				else if(rgb == 2)
					return (int)bits[t+1];
				else if(rgb == 3)
					return (int)bits[t+2];
			return 0;
		}
	}
	Error(bitmap, "BITMAP");
}

void YabInterface::BitmapDraw(double x, double y, const char* bitmap, const char* mode, const char* view)
{
	drawing_mode myMode;
	BString tmp(mode);
	if(tmp.IFindFirst("copy")!=B_ERROR)
		myMode = B_OP_COPY;
	else if(tmp.IFindFirst("alpha")!=B_ERROR)
		myMode = B_OP_ALPHA;
	else
		ErrorGen("Unknown option");

	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(bitmap);
		if(bview)
		{
			YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
			if(myView)
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					BBitmap *newb = new BBitmap(b->Bounds(), B_RGBA32);
					char* newbits = (char*)newb->Bits();
					char* oldbits = (char*)b->Bits();
					for(int j=0; j<b->BitsLength(); j++)
						newbits[j] = oldbits[j];
					w->Lock();
					YabDrawing *t = new YabDrawing();
					t->command = 10;
					t->x1 = x; t->y1 = y;
					t->bitmap = newb;
					myView->drawList->AddItem(t);
					myView->Invalidate();
					w->Unlock();
					return;
				}
				else
					ErrorGen("Unable to lock window");
			}
			else
        		{
                		for(int i=0; i<yabbitmaps->CountItems(); i++)
                		{
                        		BBitmap *bb = (BBitmap*)yabbitmaps->ItemAt(i);
                        		BView *bbview = bb->FindView(view);
                        		if(bbview)
                        		{
                                		bb->Lock();
						drawing_mode t = bbview->DrawingMode();
						bbview->SetDrawingMode(myMode);
                                		bbview->DrawBitmap(b, BPoint(x,y));
						bbview->SetDrawingMode(t);
                                		bbview->Sync();
                                		bb->Unlock();
                                		return;
                        		}
                		}
                		for(int i=0; i<yabcanvas->CountItems(); i++)
                		{
                        		YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        		if(!strcmp(myView->Name(), view))
                        		{
                                		YabWindow *w = cast_as(myView->Window(), YabWindow);
                                		if(w)
                                		{
                                        		w->Lock();
                                        		BBitmap *bb = myView->GetBitmap();
                                        		BView *bView = myView->GetBitmapView();
                                        		bb->Lock();
							drawing_mode t = bView->DrawingMode();
							bView->SetDrawingMode(myMode);
                                        		bView->DrawBitmap(b, BPoint(x,y));
                                        		bView->Sync();
							bView->SetDrawingMode(t);
                                        		bb->Unlock();

                                        		myView->Draw(BRect(x,y,x+b->Bounds().Width(),y+b->Bounds().Height()));
                                        		w->Unlock();
                                        		return;
                                		}
                                		else
                                        		ErrorGen("Unable to lock window");
                        		}  
				}
				Error(view, "VIEW, BITMAP or CANVAS");
			}
		}
	}
	Error(bitmap, "BITMAP");
}

void YabInterface::BitmapDraw(BRect frame, const char* bitmap, const char* mode, const char* view)
{
	int scaling = 0;
	if(frame.right == -1) scaling = 1;
	if(frame.bottom == -1) scaling = 2;
	if(frame.right == -1 && frame.bottom == -1) scaling = 3;

	drawing_mode myMode;
	BString tmp(mode);
	if(tmp.IFindFirst("copy")!=B_ERROR)
		myMode = B_OP_COPY;
	else if(tmp.IFindFirst("alpha")!=B_ERROR)
		myMode = B_OP_ALPHA;
	else
		ErrorGen("Unknown option");

	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(bitmap);
		if(bview)
		{
			BRect newframe;
			switch(scaling)
			{
				case 1:
				{
					BRect t(b->Bounds());
					double width;
					newframe = frame;
					width = (t.right-t.left)*((frame.bottom-frame.top)/(t.bottom-t.top));
					newframe.right = newframe.left+width;
				}
				break;
				case 2:
				{
					BRect t(b->Bounds());
					double height;
					newframe = frame;
					height = (t.bottom-t.top)*((frame.right-frame.left)/(t.right-t.left));
					newframe.bottom = newframe.top+height;
				}
				break;
				case 3:	newframe = b->Bounds();
				break;
				default: newframe = frame;
			}

			YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
			if(myView)
			{
				YabWindow *w = cast_as(myView->Window(), YabWindow);
				if(w)
				{
					BBitmap *newb = new BBitmap(b->Bounds(), B_RGBA32);
					char* newbits = (char*)newb->Bits();
					char* oldbits = (char*)b->Bits();
					for(int j=0; j<b->BitsLength(); j++)
						newbits[j] = oldbits[j];
					w->Lock();
					YabDrawing *t = new YabDrawing();
					t->command = 11;
					t->x1 = newframe.left; t->y1 = newframe.top;
					t->x2 = newframe.right; t->y2 = newframe.bottom;
					t->bitmap = newb;
					myView->drawList->AddItem(t);
					myView->Invalidate();
					w->Unlock();
					return;
				}
				else
					ErrorGen("Unable to lock window");
			}
			else
			{
				for(int i=0; i<yabbitmaps->CountItems(); i++)
				{
					BBitmap *bb = (BBitmap*)yabbitmaps->ItemAt(i);
					BView *bbview = bb->FindView(view);
					if(bbview)
					{
						bb->Lock();
						drawing_mode t = bbview->DrawingMode();
						bbview->SetDrawingMode(myMode);
						bbview->DrawBitmap(b, newframe);
						bbview->SetDrawingMode(t);
						bbview->Sync();
						bb->Unlock();
						return;
					}
				}
				for(int i=0; i<yabcanvas->CountItems(); i++)
				{
					YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
					if(!strcmp(myView->Name(), view))
					{
						YabWindow *w = cast_as(myView->Window(), YabWindow);
						if(w)
						{
							w->Lock();
							BBitmap *bb = myView->GetBitmap();
							BView *bView = myView->GetBitmapView();
							bb->Lock();
							drawing_mode t = bView->DrawingMode();
							bView->SetDrawingMode(myMode);
							bView->DrawBitmap(b, frame);
							bView->Sync();
							bView->SetDrawingMode(t);
							bb->Unlock();

							myView->Draw(newframe);
							w->Unlock();
							return;
						}
						else
							ErrorGen("Unable to lock window");
					}  
				}
				Error(view, "VIEW, BITMAP or CANVAS");
			}
		}
	}
	Error(bitmap, "BITMAP");
}

void YabInterface::BitmapGet(BRect frame, const char* id, const char* bitmap)
{
	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(bitmap);
		if(bview)
		{
			char *oldbits, *newbits;
			BBitmap *newbmp = new BBitmap(BRect(0,0, frame.Width(), frame.Height()), B_RGBA32, true);
			BView *newbview = new BView(BRect(0,0, frame.Width(), frame.Height()), id, B_FOLLOW_NONE, 0);
			newbmp->AddChild(newbview);
			newbits = (char*)newbmp->Bits();
			for(int i=0; i<frame.Width()*frame.Height()*4; i = i + 4)
			{
				newbits[i] = newbits[i+1] = newbits[i+2] = 255;
				newbits[i+3] = 0;
			}
			oldbits = (char*)b->Bits();
			b->Lock();
			BRect tframe = bview->Bounds();
			b->Unlock();
			if(frame.top>tframe.bottom || frame.left>tframe.right || frame.bottom>tframe.bottom || frame.right>tframe.right || frame.top<0 || frame.left<0 || frame.right<0 || frame.bottom<0)
				ErrorGen("Out of bounds");
			for(int32 j = 0; j<frame.IntegerHeight(); j++)
				for(int32 k = 0; k<frame.IntegerWidth(); k++)
					for(int32 l = 0; l<4; l++)
						newbits[j*newbmp->BytesPerRow()+k*4+l] = oldbits[(int32)((j+frame.top)*b->BytesPerRow()+(k+frame.left)*4+l)];
			yabbitmaps->AddItem(newbmp);
			return;
		}
	}
	for(int i=0; i<yabcanvas->CountItems(); i++)
	{
		YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
		if(!strcmp(myView->Name(), bitmap))
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BBitmap *b = myView->GetBitmap();
				char *oldbits, *newbits;
				BBitmap *newbmp = new BBitmap(BRect(0,0, frame.Width(), frame.Height()), B_RGBA32, true);
				BView *newbview = new BView(BRect(0,0, frame.Width(), frame.Height()), id, B_FOLLOW_NONE, 0);
				newbmp->AddChild(newbview);
				newbits = (char*)newbmp->Bits();
				for(int i=0; i<frame.Width()*frame.Height()*4; i = i + 4)
				{
					newbits[i] = newbits[i+1] = newbits[i+2] = 255;
					newbits[i+3] = 0;
				}
				oldbits = (char*)b->Bits();
				BRect tframe = myView->Bounds();
				if(frame.top>tframe.bottom || frame.left>tframe.right || frame.bottom>tframe.bottom || frame.right>tframe.right || frame.top<0 || frame.left<0 || frame.right<0 || frame.bottom<0)
					ErrorGen("Out of bounds");
				for(int32 j = 0; j<frame.IntegerHeight(); j++)
					for(int32 k = 0; k<frame.IntegerWidth(); k++)
						for(int32 l = 0; l<4; l++)
							newbits[j*newbmp->BytesPerRow()+k*4+l] = oldbits[(int32)((j+frame.top)*b->BytesPerRow()+(k+frame.left)*4+l)];
				yabbitmaps->AddItem(newbmp);
				w->Unlock();
				return;
			}
		}
	}  
	Error(bitmap, "BITMAP or CANVAS");
}

void YabInterface::BitmapGet(double w, const char* id, const char* path)
{
	double h = w;
	BRect iFrame = BRect(0, 0, w-1, h-1);
	BBitmap *iBitmap = new BBitmap(iFrame, B_CMAP8, true);
	BBitmap *fBitmap = new BBitmap(iFrame, B_RGBA32, true);
	BView *bview = new BView(iFrame, id, B_FOLLOW_NONE, 0);
	fBitmap->AddChild(bview);

	char *b;
	b = (char*)fBitmap->Bits();
	for(int i=0; i<w*h*4; i = i + 4)
	{
		b[i] = b[i+1] = b[i+2] = 255;
		b[i+3] = 0;
	}

	BNode *fNode = new BNode(path);
	BNodeInfo fInfo(fNode);
	int i;
	i=int(w);
	icon_size ics;
	ics=(icon_size)i;
	
	fInfo.GetTrackerIcon( iBitmap, ics );
	
	

	fBitmap->Lock();
	bview->DrawBitmap( iBitmap, iFrame );
	fBitmap->Unlock();

	delete fNode;
	delete iBitmap;
	yabbitmaps->AddItem(fBitmap);
}

void YabInterface::BitmapGetIcon(const char* id, const char* option, const char* path)
{
	BString fString(option);
	if( fString.IFindFirst("Path") != B_ERROR )
	{
		int w;
		int h = w = 32;
		BRect iFrame = BRect(0, 0, w-1, h-1);
		BBitmap *iBitmap = new BBitmap(iFrame, B_CMAP8, true);
		BBitmap *fBitmap = new BBitmap(iFrame, B_RGBA32, true);
		BView *bview = new BView(iFrame, id, B_FOLLOW_NONE, 0);
		fBitmap->AddChild(bview);

		char *b;
		b = (char*)fBitmap->Bits();
		for(int i=0; i<w*h*4; i = i + 4)
		{
			b[i] = b[i+1] = b[i+2] = 255;
			b[i+3] = 0;
		}
		
		BNode *fNode = new BNode(path);
		BNodeInfo fInfo(fNode);
		fInfo.GetTrackerIcon( iBitmap, B_LARGE_ICON );

		fBitmap->Lock();
		bview->DrawBitmap( iBitmap, iFrame );
		fBitmap->Unlock();

		delete fNode;
		delete iBitmap;
		yabbitmaps->AddItem(fBitmap);

	}
	else if( fString.IFindFirst("Mime") != B_ERROR )
	{
		int w;
		int h = w = 16;
		icon_size iType = B_MINI_ICON;
		if( fString.IFindFirst("Mime32") != B_ERROR )
		{
			h = w = 32;
			iType = B_LARGE_ICON;
		}

		BRect iFrame = BRect(0, 0, w-1, h-1);
		BBitmap *iBitmap = new BBitmap(iFrame, B_CMAP8, true);
		BBitmap *fBitmap = new BBitmap(iFrame, B_RGBA32, true);
		BView *bview = new BView(iFrame, id, B_FOLLOW_NONE, 0);
		fBitmap->AddChild(bview);

		char *b;
		b = (char*)fBitmap->Bits();
		for(int i=0; i<w*h*4; i = i + 4)
		{
			b[i] = b[i+1] = b[i+2] = 255;
			b[i+3] = 0;
		}

		BMimeType mime(path);
		mime.GetIcon(iBitmap, iType);
		fBitmap->Lock();
		bview->DrawBitmap(iBitmap, iFrame);
		bview->Sync();
		fBitmap->Unlock();

		delete iBitmap;
		yabbitmaps->AddItem(fBitmap);
	}
	else
		ErrorGen("Unknown option");

}

void YabInterface::BitmapDrag(const char* bitmap)
{
}

void YabInterface::BitmapRemove(const char* bitmap)
{
	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(bitmap);
		if(bview)
		{
			yabbitmaps->RemoveItem(i);
			delete b;
			return;
		}
	}
	Error(bitmap, "BITMAP");
}

void YabInterface::Screenshot(BRect frame, const char* bitmap)
{
	char *t;
	int w = (int)frame.Width()-1;
	int h = (int)frame.Height()-1;

	BRect area = BRect(0,0,w,h);
	BBitmap *fBitmap = new BBitmap(area, B_RGBA32, true);
	BView *bview = new BView(area, bitmap, B_FOLLOW_NONE, 0);
	BScreen screen(B_MAIN_SCREEN_ID);
	fBitmap->AddChild(bview);

	t = (char*)fBitmap->Bits();
	for(int i=0; i<w*h*4; i = i + 4)
	{
		t[i] = t[i+1] = t[i+2] = 255;
		t[i+3] = 0;
	}

	screen.WaitForRetrace();
	screen.ReadBitmap(fBitmap, false, &frame);
	yabbitmaps->AddItem(fBitmap);
}

int YabInterface::BitmapLoad(const char* FileName, const char* id)
{
	BBitmap* myBitmap = NULL;
	BFile imageFile;
	BPath imagePath;
	int ret = 0;

	if(*FileName=='/')
		imageFile.SetTo(FileName, B_READ_ONLY);
	else
		if(!strcmp(ApplicationDirectory,""))
		{
	 		if(imagePath.SetTo((const char*)ApplicationDirectory, FileName)==B_OK)
				imageFile.SetTo(imagePath.Path(), B_READ_ONLY);
		}

	if(imageFile.InitCheck()!=B_OK)
		imageFile.SetTo(FileName, B_READ_ONLY);
	
	if(imageFile.InitCheck()!=B_OK)
		return 1;

	Roster = BTranslatorRoster::Default();

	if(!Roster)
		return 2;

	BBitmapStream Stream;

	if(Roster->Translate(&imageFile, NULL, NULL, &Stream, B_TRANSLATOR_BITMAP)<B_OK)
		return 3;

	if(Stream.DetachBitmap(&myBitmap)!=B_OK)
		return 4;

	char *t;
	BBitmap *b = new BBitmap(myBitmap->Bounds(), B_RGBA32, true);
	BView *bview = new BView(myBitmap->Bounds(), id, B_FOLLOW_NONE, 0);
	b->AddChild(bview);
	/*
	t = (char*)b->Bits();
	for(int i=0; i<w*h*4; i = i + 4)
	{
		t[i] = t[i+1] = t[i+2] = 255;
		t[i+3] = 0;
	}
	*/

	b->Lock();
	drawing_mode mode = bview->DrawingMode();
	bview->SetDrawingMode(B_OP_ALPHA);
	bview->DrawBitmap(myBitmap, myBitmap->Bounds());
	bview->SetDrawingMode(mode);
	bview->Sync();
	b->Unlock();

	delete Roster;
	delete myBitmap;

	yabbitmaps->AddItem(b);

	return 0;
}

int YabInterface::BitmapGet(const char* id, const char* option)
{
	BString t(option);
	bool isWidth = false;
	if(t.IFindFirst("height") != B_ERROR)
		isWidth = false;
	else if(t.IFindFirst("width") != B_ERROR)
		isWidth = true;
	else 
		ErrorGen("Unknown option");

	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(id);
		if(bview)
		{
			BRect r = b->Bounds();
			if(isWidth)
				return r.IntegerWidth()+1;
			else
				return r.IntegerHeight()+1;
		}
	}
	Error(id, "BITMAP");
}

int YabInterface::BitmapSave(const char* id, const char* filename, const char* type)
{
	uint32 btype;
	BString t(type);
	if(t.IFindFirst("png") != B_ERROR)
		btype = B_PNG_FORMAT;
	else if(t.IFindFirst("jpg") != B_ERROR)
		btype = B_JPEG_FORMAT;
	else if(t.IFindFirst("tga") != B_ERROR)
		btype = B_TGA_FORMAT;
	else if(t.IFindFirst("tiff") != B_ERROR)
		btype = B_TIFF_FORMAT;
	else if(t.IFindFirst("ppm") != B_ERROR)
		btype = B_PPM_FORMAT;
	else if(t.IFindFirst("bmp") != B_ERROR)
		btype = B_BMP_FORMAT;
	else
		ErrorGen("Unknown type");
	
	for(int i=0; i<yabbitmaps->CountItems(); i++)
	{
		BBitmap *b = (BBitmap*)yabbitmaps->ItemAt(i);
		BView *bview = b->FindView(id);
		if(bview)
		{
			BTranslatorRoster *roster = BTranslatorRoster::Default(); 
			BBitmapStream stream(b); // init with contents of bitmap 
			BFile file(filename, B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE); 
			if(roster->Translate(&stream, NULL, NULL, &file, btype) != B_OK)
				return 1;
			else
				return 0;
		}
	}
	Error(id, "BITMAP");
}

void YabInterface::Canvas(BRect frame, const char* id, const char* view)
{
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			YabBitmapView *bmpView = new YabBitmapView(frame, id, B_FOLLOW_NONE, B_WILL_DRAW);
			if(w->layout == -1)
				bmpView->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
			else
				bmpView->SetResizingMode(w->layout);
			myView->AddChild(bmpView);
			yabcanvas->AddItem(bmpView);
			myView->Invalidate(frame);
			w->Unlock();
			return;
		}
	}
	Error(view, "VIEW");
}

int YabInterface::Sound(const char* filename)
{
	entry_ref ref; 
	BEntry entry(filename, true); 

	if (entry.InitCheck() == B_OK) 
		if (entry.GetRef(&ref) == B_OK) 
			return play_sound(&ref, true, false, true);
	return -1;
}

void YabInterface::SoundStop(int32 id)
{
	stop_sound(id);
}

void YabInterface::SoundWait(int32 id)
{
	wait_for_sound(id);
}

void YabInterface::SetOption(const char* id, const char* option, double x, double y)
{
	BString tmp(option);
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BView *theView = cast_as(myView->FindView(id), BView);
				if(theView)
				{
					if(tmp.IFindFirst("ResizeTo")!=B_ERROR)
						theView->ResizeTo(x,y);
					else if(tmp.IFindFirst("ResizeBy")!=B_ERROR)
						theView->ResizeBy(x,y);
					else if(tmp.IFindFirst("MoveTo")!=B_ERROR)
						theView->MoveTo(x,y);
					else if(tmp.IFindFirst("MoveBy")!=B_ERROR)
						theView->MoveBy(x,y);
					else
						ErrorGen("Unknown option");
					theView->Invalidate();
					w->Unlock();
					return;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "VIEW");
}

void YabInterface::DrawSet(const char* option, const char* color,const char* view)
{
	BString tmp(option);
	BString colstr(color);
	rgb_color col;
	if(colstr.IFindFirst("Panel-Background-Color")!=B_ERROR)
		col = ui_color(B_PANEL_BACKGROUND_COLOR);
	else if(colstr.IFindFirst("Panel-Text-Color")!=B_ERROR)
	{
		col = ui_color(B_PANEL_TEXT_COLOR);
	}
	else if(colstr.IFindFirst("Panel-Link-Color")!=B_ERROR)
	{
		col.red = 0; col.green = 0; col.blue = 255; 
	}
	else if(colstr.IFindFirst("Menu-Background-Color")!=B_ERROR)
		col = ui_color(B_MENU_BACKGROUND_COLOR);
	else if(colstr.IFindFirst("Menu-Selection-Background-Color")!=B_ERROR)
		col = ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
	else if(colstr.IFindFirst("Menu-Item-Text-Color")!=B_ERROR)
		col = ui_color(B_MENU_ITEM_TEXT_COLOR);
	else if(colstr.IFindFirst("Menu-Selected-Item-Text-Color")!=B_ERROR)
		col = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
	else if(colstr.IFindFirst("Keyboard-Navigation-Color")!=B_ERROR)
		col = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	else if(colstr.IFindFirst("Jan-Favorite-Color")!=B_ERROR)
	{
		col.red = 220; col.green = 220; col.blue = 250;
	}
	else
		ErrorGen("Invalid color");
	if(colstr.IFindFirst("Lighten-1-Tint")!=B_ERROR)
		col = tint_color(col, B_LIGHTEN_1_TINT);
	else if(colstr.IFindFirst("Lighten-2-Tint")!=B_ERROR)
		col = tint_color(col, B_LIGHTEN_2_TINT);
	else if(colstr.IFindFirst("Lighten-Max-Tint")!=B_ERROR)
		col = tint_color(col, B_LIGHTEN_MAX_TINT);
	else if(colstr.IFindFirst("Darken-1-Tint")!=B_ERROR)
		col = tint_color(col, B_DARKEN_1_TINT);
	else if(colstr.IFindFirst("Darken-2-Tint")!=B_ERROR)
		col = tint_color(col, B_DARKEN_2_TINT);
	else if(colstr.IFindFirst("Darken-3-Tint")!=B_ERROR)
		col = tint_color(col, B_DARKEN_3_TINT);
	else if(colstr.IFindFirst("Darken-4-Tint")!=B_ERROR)
		col = tint_color(col, B_DARKEN_4_TINT);
	else if(colstr.IFindFirst("Darken-Max-Tint")!=B_ERROR)
		col = tint_color(col, B_DARKEN_MAX_TINT);
	else if(colstr.IFindFirst("Disabled-Label-Tint")!=B_ERROR)
		col = tint_color(col, B_DISABLED_LABEL_TINT);
	else if(colstr.IFindFirst("Disabled-Mark-Tint")!=B_ERROR)
		col = tint_color(col, B_DISABLED_MARK_TINT);
	else if(colstr.IFindFirst("Highlight-Background-Tint")!=B_ERROR)
		col = tint_color(col, B_HIGHLIGHT_BACKGROUND_TINT);
	col.alpha = yabAlpha;

	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			if(tmp.IFindFirst("BGColor")!=B_ERROR)
			{
				myView->SetViewColor(col);
				myView->Invalidate();
			}
			else if(tmp.IFindFirst("HighColor")!=B_ERROR)
			{
				if(yabAlpha == 255) 
					myView->SetDrawingMode(B_OP_COPY);
				else
					myView->SetDrawingMode(B_OP_ALPHA);
				myView->SetHighColor(col);
				YabDrawing *t = new YabDrawing();
				t->command = 6;
				t->r = col.red; t->g = col.green;
				t->b = col.blue; t->alpha = yabAlpha;
				myView->drawList->AddItem(t);
			}
			else if(tmp.IFindFirst("LowColor")!=B_ERROR)
			{
				if(yabAlpha == 255) 
					myView->SetDrawingMode(B_OP_COPY);
				else
					myView->SetDrawingMode(B_OP_ALPHA);
				myView->SetLowColor(col);
				YabDrawing *t = new YabDrawing();
				t->command = 7;
				t->r = col.red; t->g = col.green;
				t->b = col.blue; t->alpha = yabAlpha;
				myView->drawList->AddItem(t);
			}
			else
				ErrorGen("Unknown option");
			w->Unlock();
		}
		else
			ErrorGen("Unable to lock window");
	}
	else
        {
                for(int i=0; i<yabbitmaps->CountItems(); i++)
                {
                        BBitmap *bmp = (BBitmap*)yabbitmaps->ItemAt(i);
                        BView *bView = bmp->FindView(view);
                        if(bView)
                        {
				if(tmp.IFindFirst("HighColor")!=B_ERROR)
				{
                                	bmp->Lock();
					if(yabAlpha == 255) 
						bView->SetDrawingMode(B_OP_COPY);
					else
						bView->SetDrawingMode(B_OP_ALPHA);
					bView->SetHighColor(col);
                                	bView->Sync();
                                	bmp->Unlock();
                                	return;
				}
				else if(tmp.IFindFirst("LowColor")!=B_ERROR)
				{
                                	bmp->Lock();
					if(yabAlpha == 255) 
						bView->SetDrawingMode(B_OP_COPY);
					else
						bView->SetDrawingMode(B_OP_ALPHA);
					bView->SetLowColor(col);
                                	bView->Sync();
                                	bmp->Unlock();
                                	return;
				}
				else
					ErrorGen("Unknown option");
                        }
                }
                for(int i=0; i<yabcanvas->CountItems(); i++)
                {
                        YabBitmapView *myView = (YabBitmapView*)yabcanvas->ItemAt(i);
                        if(!strcmp(myView->Name(), view))
                        {
                                YabWindow *w = cast_as(myView->Window(), YabWindow);
                                if(w)
                                {
                                        w->Lock();
                                        BBitmap *bmp = myView->GetBitmap();
                                        BView *bView = myView->GetBitmapView();
					if(tmp.IFindFirst("HighColor")!=B_ERROR)
					{
                                		bmp->Lock();
						if(yabAlpha == 255) 
							bView->SetDrawingMode(B_OP_COPY);
						else
							bView->SetDrawingMode(B_OP_ALPHA);
						bView->SetHighColor(col);
                                		bView->Sync();
                                		bmp->Unlock();
                                        	w->Unlock();
                                		return;
					}
					else if(tmp.IFindFirst("LowColor")!=B_ERROR)
					{
       	                         		bmp->Lock();
						if(yabAlpha == 255) 
							bView->SetDrawingMode(B_OP_COPY);
						else
							bView->SetDrawingMode(B_OP_ALPHA);
						bView->SetLowColor(col);
       		                         	bView->Sync();
       		                         	bmp->Unlock();
                                        	w->Unlock();
       		                         	return;
					}
					else
						ErrorGen("Unknown option");
                                }
                                else
                                        ErrorGen("Unable to lock window");
                        }
                } 
		Error(view, "VIEW, BITMAP or CANVAS");
	}
}

void YabInterface::Treebox13(const char* id,const char* option, int pos)
{
}

int YabInterface::TreeboxGetOpt(const char* id, const char* option, int pos)
{
}

int YabInterface::ListboxGetNum(const char* id)
{
	YabView *myView = NULL;
	BListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(id), BListView);
				if(myList)
				{
					int32 t = myList->CurrentSelection();
					w->Unlock();
					return t+1;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "LISTBOX");
}

int YabInterface::DropboxGetNum(const char* id)
{
	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				BMenuField *myMenuField = cast_as(myView->FindView(id), BMenuField);
				if(myMenuField)
				{
					BMenu *myMenu = (BMenu*)myMenuField->Menu();
					if(myMenu)
					{
						int32 ret;
						ret = myMenu->IndexOf(myMenu->FindMarked());
						w->Unlock();
						return ret;
					}
				}
				w->Unlock();
			}
		}
	}
	Error(id, "DROPBOX");
}

int YabInterface::TreeboxGetNum(const char* id)
{
	YabView *myView = NULL;
	BOutlineListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(id), BOutlineListView);
				if(myList)
				{
					int32 t = myList->FullListCurrentSelection();
					w->Unlock();
					return t+1;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "TREEBOX");
}

int YabInterface::ColumnboxGetNum(const char* id)
{
	YabView *myView = NULL;
	BColumnListView *myList = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{
				w->Lock();
				myList = cast_as(myView->FindView(id), BColumnListView);
				if(myList)
				{
					int32 t = myList->IndexOf(myList->CurrentSelection());
					w->Unlock();
					return t+1;
				}
				w->Unlock();
			}
		}
	}
	Error(id, "COLUMNBOX");
}

void YabInterface::Attribute1(const char* type, const char* name, const char* value, const char* filename)
{
	
	// fix to work properly with string and bool types.
	BNode node(filename);
	if(node.InitCheck() != B_OK)
		ErrorGen("Attribute file not found!");
	BString tempvalue(value);
	
	
	type_code typecode;
	BString typeStr(type);
	if(typeStr.IFindFirst("string")!=B_ERROR)
	{
		int32 x = tempvalue.Length();
		typecode = B_STRING_TYPE;
		x++;
		node.WriteAttr(name, typecode, 0, value, x);
	}
	else if(typeStr.IFindFirst("mime")!=B_ERROR)
	{
		int32 x = tempvalue.Length();
		typecode = B_MIME_STRING_TYPE;
		x++;
		node.WriteAttr(name, typecode, 0, value, x);
	}
	else if(typeStr.IFindFirst("bool")!=B_ERROR)
	{
		typecode = B_BOOL_TYPE;
		int32 x=1;
		char str[1];
		str[0]=1;
		const char* tf = str;
		if (tempvalue.IFindFirst("false") !=B_ERROR)
		tf="";
		node.WriteAttr(name, typecode, 0, tf, x);
	}
		
			
	else if(typeStr.IFindFirst("int")!=B_ERROR)
	{
		int32 x = tempvalue.Length();
		typecode = B_INT32_TYPE;
		node.WriteAttr(name, typecode, 0, value, x);
	}
		
		
	else if(typeStr.IFindFirst("double")!=B_ERROR)
	{
		int32 x = tempvalue.Length();
		typecode = B_DOUBLE_TYPE;
		node.WriteAttr(name, typecode, 0, value, x);
	}
	else if(typeStr.IFindFirst("float")!=B_ERROR)
	{
		int32 x = tempvalue.Length();
		typecode = B_FLOAT_TYPE;
		node.WriteAttr(name, typecode, 0, value, x);
	}
	else if(typeStr.IFindFirst("long")!=B_ERROR)
	{
		int32 x = tempvalue.Length();
		typecode = B_INT64_TYPE;
		node.WriteAttr(name, typecode, 0, value, x);
	}
	else
		ErrorGen("Unknown attribute type!");

		

}

void YabInterface::AttributeClear(const char* name, const char* filename)
{
	BNode node(filename);
	if(node.InitCheck() != B_OK)
		ErrorGen("Attribute file not found!");
	node.RemoveAttr(name);
}

const char* YabInterface::AttributeGet1(const char* name, const char* filename)
{
	BString tempname(name);
	BNode node(filename);
	if(node.InitCheck() != B_OK){
		if (tempname.Length() >0)
			ErrorGen("Attribute file not found!");
		BString appdir = mainFileName;
		return appdir;
	}	
			
	if (tempname.Length() >0)
	{
	attr_info attr;
	if(node.GetAttrInfo(name, &attr) != B_OK)
		ErrorGen("Attribute not found!");

	int size = attr.size;
	if(size>32568)
		size = 32568;
	if(node.ReadAttr(name,attr.type, 0, attrbuffer, size) == 0)
		return "";
		
		if (attr.type == B_BOOL_TYPE)
			{	
				int x = 0;
				if (attrbuffer[0] == x)
				{
				return "false";
				}
				else 
				{
				return "true";
				}	
			}

	return (char*)attrbuffer;
	}
	else
	{
		BString List="";
		BString sp=" | ";
		BString Attrtype;
		char buf[B_ATTR_NAME_LENGTH];
		while (node.GetNextAttrName(buf) == B_OK)
		{
			attr_info attr;
			if(node.GetAttrInfo(buf, &attr) != B_OK)
				ErrorGen("Attribute not found!");
				uint32 attrtype;
		 attrtype=attr.type;
		switch(attrtype)
		{	
		 case B_BOOL_TYPE:
			Attrtype="Bool";
			break;
		case B_STRING_TYPE:
			Attrtype = "String";
			break;
		case B_MIME_STRING_TYPE:
			Attrtype = "Mime";
			break;
		case B_INT32_TYPE:
			Attrtype = "Int";
			break;
		case B_DOUBLE_TYPE:
			Attrtype = "Double";
			break;
		case B_FLOAT_TYPE:
			Attrtype = "Float";
			break;	
		case B_INT64_TYPE:
			Attrtype = "Long";
			break;
		default:	
			Attrtype = "Unsupported";
			break;
		}			
				
				
			List << buf << sp << Attrtype  << sp;	
		}
		
		
		
	return List.String();
		
	}	
}

double YabInterface::AttributeGet2(const char* name, const char* filename)
{
	BNode node(filename);
	if(node.InitCheck() != B_OK)
		ErrorGen("Attribute file not found!");
	
	attr_info attr;
	if(node.GetAttrInfo(name, &attr) != B_OK)
		ErrorGen("Attribute not found!");	
	int size = attr.size;
	if(size>32568)
		size = 32568;
	if(node.ReadAttr(name,attr.type, 0, attrbuffer, size) == 0)
		return 0.0;		
		if (attr.type == B_BOOL_TYPE)
			{	
				int x = 0;
				if (attrbuffer[0] == x)
				{
				return 0.0;
				}
				else 
				{
				return 1.0;
				}	
			}
	
	
	if(node.ReadAttr(name, 0, 0, attrbuffer, size) == 0)
		return 0.0;
	return atof((char*)attrbuffer);	
}
void YabInterface::ShortCut(const char* view, const char* key, const char* msg)
{
	char myShortcut;
	int32 modifiers = 0;
	BString t(key);
	if(t.Length()>1)
	{
		myShortcut = key[t.Length()-1];
		if(t.IFindFirst("s")!=B_ERROR && t.IFindFirst("s")<t.Length()-1)
			modifiers = modifiers|B_SHIFT_KEY;
		if(t.IFindFirst("c")!=B_ERROR && t.IFindFirst("c")<t.Length()-1)
			modifiers = modifiers|B_CONTROL_KEY;
		if(t.IFindFirst("o")!=B_ERROR && t.IFindFirst("o")<t.Length()-1)
			modifiers = modifiers|B_OPTION_KEY;
	}
	else
		myShortcut = key[0];
	YabView *myView = cast_as((BView*)viewList->GetView(view), YabView);
	if(myView)
	{
		YabWindow *w = cast_as(myView->Window(), YabWindow);
		if(w)
		{
			w->Lock();
			BMessage *mesg = new BMessage(YABSHORTCUT);
			mesg->AddString("shortcut", msg);
			w->AddShortcut(myShortcut, modifiers, mesg);
			w->Unlock();
			return;
		}
	}
	Error(view,"VIEW");
}

int YabInterface::IsComputerOn()
{
	return is_computer_on();
}

void YabInterface::MouseSet(const char* opt)
{
	BString t(opt);
	if(t.IFindFirst("Hide")!=B_ERROR)
		HideCursor();
	else if(t.IFindFirst("Show")!=B_ERROR)
		ShowCursor();
	else if(t.IFindFirst("Obscure")!=B_ERROR)
		ObscureCursor();
	else ErrorGen("Unknown option");
}

const char* YabInterface::GetMessageString()
{
	snooze(20000);
	BString tmp("");
	if(exiting)
	{
		tmp += "_QuitRequested|";
		exiting = false;
	}
	tmp += localMessage;
	localMessage = "";
	for(int i=0; i<CountWindows(); i++)
	{
		// if(strcmp(WindowAt(i)->Title(), "_TOOLTIPWINDOW") && strcmp(WindowAt(i)->Title(), "_CALENDARWINDOW"))
		
		
		YabWindow *w = cast_as(WindowAt(i), YabWindow);
		if(w)
		{

			w->Sync();
			if(w->Lock())
			{
				tmp += w->getMessages();
				if (w->WActivated==1)
				{
					tmp+=w->idString;
					tmp+=":_Activated|";
					w->WActivated=-1;
				}
				if (w->WActivated==0)
				{
					tmp+=w->idString;
					tmp+=":_Deactivated|";
					w->WActivated=-1;
				}
//				if (w->WFrameMoved==1)
//				{
//					w->WFrameMoved=-1;
//					tmp+=w->Name();
//					tmp+=":_FrameMoved:";
//					tmp << w->Wpx; 
//					tmp+=":";
//					tmp << w->Wpy; 
//					tmp+="|";
//				}
//				if (w->WFrameResized==1)
//				{
//					w->WFrameResized=-1;
//					tmp+=w->Name();
//					tmp+=":_FrameResized:";
//					tmp << w->Wpw; 
//					tmp+=":";
//					tmp << w->Wph; 
//					tmp+="|";
//				}				
				w->Unlock();
			}
		}
	}


	YabView *myView = NULL;
	for(int i=0; i<viewList->CountItems(); i++)
	{
		myView = cast_as((BView*)viewList->ItemAt(i), YabView);
		if(myView)
		{
			YabWindow *w = cast_as(myView->Window(), YabWindow);
			if(w)
			{			
				w->Lock();
				if (myView->CountChildren())
				{
					for (int o=0; o<myView->CountChildren(); o++)
					{
  					    if (dynamic_cast<YabTabView*>(myView->ChildAt(o)))
  					    {
							YabTabView *target = dynamic_cast<YabTabView*>(myView->ChildAt(o));
							if (target)
							{	
								if (target->FocusChanged!=target->OldTabView)
								{
									tmp+=target->Name();
									tmp+=":_TabChanged:";
									tmp+=target->Name();
									tmp << target->FocusChanged;
									tmp+="|";
									target->OldTabView=target->FocusChanged;
								}
								
							}
  					    }

					}
				}
				w->Unlock();
			}
		}
	}		
	if(tmp.Length()>32766)
		tmp.Remove(32767, tmp.Length()-32766);
	strcpy(messagebuffer, tmp.String());
	return (char*)messagebuffer;
}

int YabInterface::MessageSend(const char* app, const char* msg)
{
	BMessage message, reply; 
	status_t result; 
	
	// set the command constant 
	message.what = B_SET_PROPERTY; 
	
	// construct the specifier stack 
	message.AddSpecifier("YabSendString", msg); // B_NAME_SPECIFIER 
	
	// send the message and fetch the result 
	result = BMessenger(app).SendMessage(&message, &reply); 
	if(result == B_OK) return 0;
	if(result == B_BAD_PORT_ID) return 1;
	if(result == B_WOULD_BLOCK) return 2;
	if(result == B_TIMED_OUT) return 3;
	return 4;
}

void YabInterface::SetLocalize(const char* path)
{
	if(yabCatalog)
		delete yabCatalog;
	//yabCatalog = new BCatalog(path);
}

const int YabInterface::GetErrorCode()
{
	return errorCode;
}

void YabInterface::KillThread(int code)
{
	errorCode = code;
	quitting = true;
	ExitRequested();
	// BMessenger(be_app).SendMessage(new BMessage(B_QUIT_REQUESTED));
	// while(1){}
}

void YabInterface::Error(const char* id, const char* type)
{
	fprintf(stderr, "---Error in %s, line %d: \"%s\" is not of type %s\n", currentLib.String(), currentLineNumber, id, type);
	fprintf(stderr,"---Error: Program stopped due to an error \n");
	KillThread(-1);
	// while(1){}
}

void YabInterface::ErrorGen(const char* msg)
{
	fprintf(stderr, "---Error in %s, line %d: %s\n", currentLib.String(), currentLineNumber, msg);
	fprintf(stderr,"---Error: Program stopped due to an error \n");
	KillThread(-1);
	// while(1){}
}

void YabInterface::SetCurrentLineNumber(int line, const char* libname)
{
	currentLineNumber = line;
	if(!strcmp(libname, "main"))
		currentLib = mainFileName;
	else
		currentLib = libname;
}

void YabInterface::SetMainFileName(const char* name)
{
	mainFileName = strdup(name);
}

/**
 * C interface functions
 */

const char* yi_GetApplicationDirectory(YabInterface *yab)
{
	return yab->GetApplicationDirectory();
}

void yi_OpenWindow(double x1,double y1,double x2,double y2, const char* id, const char* title, YabInterface* yab)
{
	yab->OpenWindow(BRect(x1,y1,x2,y2), id, _L(title));
}

int yi_CloseWindow(const char* view, YabInterface* yab)
{
	return yab->CloseWindow(view);
}

void yi_CreateButton(double x1,double y1,double x2,double y2, const char* id, const char* title, const char* view, YabInterface* yab) 
{
	yab->CreateButton(BRect(x1,y1,x2,y2), id, _L(title), view);
}

int yi_CreateImage(double x,double y,const char* imagefile, const char* window, YabInterface* yab) 
{
	return yab->CreateImage(BPoint(x,y),imagefile,window);
}

int yi_CreateImage2(double x1,double y1,double x2,double y2,const char* imagefile, const char* window, YabInterface* yab) 
{
	return yab->CreateImage(BRect(x1,y1,x2,y2),imagefile,window);
}

int yi_CreateSVG(double x1,double y1,double x2,double y2,const char* imagefile, const char* window, YabInterface* yab) 
{
	return yab->CreateSVG(BRect(x1,y1,x2,y2),imagefile,window);
}

void yi_CreateMenu(const char* menuhead, const char* menuitem, const char *shortcut, const char* window, YabInterface* yab) 
{
	yab->CreateMenu(_L(menuhead),_L(menuitem),shortcut,window);
}

void yi_CreateTextControl(double x1, double y1, double x2, double y2, const char* id, const char* label, const char* text, const char* window, YabInterface *yab)
{
	yab->CreateTextControl(BRect(x1,y1,x2,y2),id,_L(label),_L(text),window);
}

void yi_CreateCheckBox(double x, double y, const char* id, const char* label, int isActivated, const char* window, YabInterface *yab)
{
	yab->CreateCheckBox(x,y,id,_L(label),isActivated,window);
}

void yi_CreateRadioButton(double x, double y, const char* groupID, const char* label, int isActivated, const char* window, YabInterface *yab)
{
	yab->CreateRadioButton(x,y,groupID,_L(label),isActivated,window);
}

void yi_CreateListBox(double x1, double y1, double x2, double y2, const char* title, int scrollbar, const char* window, YabInterface *yab)
{
	yab->CreateListBox(BRect(x1,y1,x2,y2),title,scrollbar,window); 
}

void yi_CreateDropBox(double x1, double y1, double x2, double y2, const char* title, const char* label, const char* window, YabInterface *yab)
{
	yab->CreateDropBox(BRect(x1,y1,x2,y2),title,_L(label), window);
}

void yi_CreateItem(const char* id,const char* item, YabInterface *yab)
{
	yab->CreateItem(id,_L(item));
}

void yi_RemoveItem(const char* title,const char* item, YabInterface *yab)
{
	yab->RemoveItem(title,_L(item));
}

void yi_ClearItems(const char* title, YabInterface *yab)
{
	yab->ClearItems(title);
}

void yi_DrawText(double x, double y, const char* text, const char* window, YabInterface* yab)
{
	yab->DrawText(BPoint(x,y), _L(text), window);
}

void yi_DrawRect(double x1, double y1, double x2, double y2, const char* window, YabInterface* yab)
{
	yab->DrawRect(BRect(x1,y1,x2,y2),window);
}

void yi_DrawClear(const char* window, YabInterface* yab)
{
	yab->DrawClear(window, false);
}

void yi_CreateAlert(const char* text, const char* button1, const char* type, YabInterface* yab)
{
	yab->CreateAlert(_L(text),_L(button1),type);
}

void yi_CreateText(double x, double y, const char* id, const char* text, const char* window, YabInterface* yab)
{
	yab->CreateText(x,y,id,_L(text),window);
}

void yi_Text2(double x1, double y1, double x2, double y2, const char* id, const char* text, const char* window, YabInterface* yab)
{
	yab->Text2(BRect(x1,y1,x2,y2),id,_L(text),window);
}

void yi_TextAlign(const char* txt, const char *option, YabInterface *yab)
{
	yab->TextAlign(txt, option);
}

void yi_Translate(char* text, char result[])
{
	if(yabCatalog)
	{
		result[0] = '\0';
		strcpy(result,yabCatalog->GetString(text, NULL));
	}
	else
		strcpy(result,text);
}

void yi_MenuTranslate(char* text, char result[])
{
	if(yabCatalog)
	{
		result[0] = '\0';
		const char* token;
		const char delimiters[] = ":";

		token = strtok(text, delimiters);
		while(token!=NULL)
		{
			strcat(result,yabCatalog->GetString(token, NULL)); //B_TRANSLATE_CONTEXT));
			token = strtok(NULL, delimiters);
			if(token!=NULL) strcat(result,":");
		}
	}
	else
		strcpy(result,text);
}

void yi_SetLocalize()
{
	localize = true;
}

void yi_StopLocalize()
{
	localize = false;
}

void yi_SetLocalize2(const char* , YabInterface *yab) 
{
	localize = true;
	//yab->SetLocalize(path);
}

const char* yi_LoadFilePanel(const char* mode, const char* title, const char* directory, YabInterface *yab)
{
	return yab->LoadFilePanel(mode, _L(title), directory);
}

const char* yi_SaveFilePanel(const char* mode, const char* title, const char* directory, const char*filename, YabInterface *yab)
{
	return yab->SaveFilePanel(mode, _L(title), directory, filename);
}

void yi_SetLayout(const char* layout, const char* window, YabInterface *yab)
{
	yab->SetLayout(layout, window);
}

void yi_WindowSet1(const char* option, const char* value, const char* window, YabInterface *yab)
{
	yab->WindowSet(option, value, window);
}

void yi_WindowSet2(const char* option, int r, int g, int b, const char* window, YabInterface *yab)
{
	yab->WindowSet(option, r, g, b, window);
}

void yi_WindowSet3(const char* option, double x, double y, const char* window, YabInterface *yab)
{
	yab->WindowSet(option,x,y, window);
}

void yi_WindowSet4(const char* option, const char* window, YabInterface *yab)
{
	yab->WindowSet(option, window);
}

void yi_WindowClear(const char* window, YabInterface *yab)
{
	yab->WindowClear(window);
}

void yi_TextEdit(double x1, double y1, double x2, double y2, const char* title, int scrollbar, const char* window, YabInterface *yab)
{
	yab->TextEdit(BRect(x1,y1,x2,y2), title, scrollbar, window);
}

void yi_TextAdd(const char* title, const char* text, YabInterface *yab)
{
	yab->TextAdd(title,text);
}

void yi_TextSet(const char* title, const char* option, YabInterface *yab)
{
	yab->TextSet(title,option);
}

void yi_TextSet2(const char* title, const char* option, int value, YabInterface *yab)
{
	yab->TextSet(title,option,value);
}

void yi_TextSet3(const char* title, const char* option, const char* value, YabInterface *yab)
{
	yab->TextSet(title,option,value);
}

void yi_TextColor1(const char* title, const char* option, const char* command, YabInterface *yab)
{
	yab->TextColor(title,option,command);
}

void yi_TextColor2(const char* title, const char* option, int r, int g, int b, YabInterface *yab)
{
	yab->TextColor(title,option,r,g,b);
}

int yi_TextGet2(const char* title, const char* option, YabInterface *yab)
{
	return yab->TextGet(title,option);
}

const char* yi_TextGet3(const char* title, int linenum, YabInterface *yab)
{
	return yab->TextGet(title,linenum);
}

double yi_TextGet4(const char* title, const char* option, int linenum, YabInterface *yab)
{
	return yab->TextGet(title,option,linenum);
}

int yi_TextGet5(const char* title, const char* option, const char* option2, YabInterface *yab)
{
	return yab->TextGet(title,option,option2);
}

const char* yi_TextGet6(const char* title, const char* option, YabInterface *yab)
{
	return yab->TextGet6(title, option);
}

void yi_TextClear(const char* title, YabInterface *yab)
{
	yab->TextClear(title);
}

const char* yi_TextGet(const char* title, YabInterface *yab)
{
	return yab->TextGet(title);
}

void yi_DrawSet1(const char* option, const char* window, YabInterface *yab)
{
	return yab->DrawSet1(option, window);
}

void yi_DrawSet2(int fillorstroke, const char* mypattern, YabInterface *yab)
{
	return yab->DrawSet2(fillorstroke, mypattern);
}

void yi_View(double x1, double y1, double x2, double y2, const char* id, const char* view, YabInterface *yab)
{
	yab->View(BRect(x1,y1,x2,y2), id, view);
}

void yi_BoxView(double x1, double y1, double x2, double y2, const char* id, const char* text, int lineType, const char* view, YabInterface *yab)
{
	yab->BoxView(BRect(x1,y1,x2,y2), id, _L(text), lineType, view);
}

void yi_BoxViewSet(const char* id, const char* option,const char* value, YabInterface *yab)
{
	yab->BoxViewSet(id, option,value);
}


void yi_Tab(double x1, double y1, double x2, double y2, const char* id, const char* names, const char* view, YabInterface *yab)
{
	yab->Tab(BRect(x1,y1,x2,y2), id, names, view);
}

void yi_TabSet(const char* id, int num, YabInterface *yab)
{
	yab->TabSet(id, num);
}

void yi_TabAdd(const char* id, const char* tabname, YabInterface *yab)
{
	yab->TabAdd(id, _L(tabname));
}

void yi_TabDel(const char* id, int num, YabInterface *yab)
{
	yab->TabDel(id, num);
}

int yi_TabViewGet(const char* id, YabInterface *yab) 
{
	return yab->TabViewGet(id);
} 

void yi_DrawDot(double x, double y, const char* window, YabInterface *yab)
{
	yab->DrawDot(x,y, window);
}

void yi_DrawLine(double x1, double y1, double x2, double y2, const char* window, YabInterface *yab)
{
	yab->DrawLine(x1,y1,x2,y2, window);
}

void yi_DrawCircle(double x, double y, double r, const char* window, YabInterface *yab)
{
	yab->DrawCircle(x,y,r, window);
}

void yi_DrawEllipse(double x, double y, double r1, double r2, const char* window, YabInterface *yab)
{
	yab->DrawEllipse(x,y,r1,r2, window);
}

void yi_DrawCurve(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, const char* window, YabInterface *yab)
{
	yab->DrawCurve(x1,y1,x2,y2,x3,y3,x4,y4, window);
}

void yi_Slider1(double x1, double y1, double x2, double y2, const char* id, const char* title, int min, int max, const char* view, YabInterface *yab)
{
	yab->Slider(BRect(x1,y1,x2,y2), id, _L(title), min, max, view);
}

void yi_Slider2(double x1, double y1, double x2, double y2, const char* id, const char* title, int min, int max, const char* option, const char* view, YabInterface *yab)
{
	yab->Slider(BRect(x1,y1,x2,y2), id, _L(title), min, max, option, view);
}

void yi_SetSlider1(const char* id, const char* label1, const char* label2, YabInterface *yab)
{
	yab->SetSlider(id, _L(label1), _L(label2));
}

void yi_SetSlider2(const char* id, const char* bottomtop, int count, YabInterface *yab)
{
	yab->SetSlider(id, bottomtop, count);
}

void yi_SetSlider3(const char* id, const char* part, int r, int g, int b, YabInterface *yab)
{
	yab->SetSlider(id, part, r,g,b);
}

void yi_SetSlider4(const char* id, int value, YabInterface *yab)
{
	yab->SetSlider(id, value);
}

void yi_SetOption1(const char* id, const char* option, const char* value, YabInterface *yab)
{
	yab->SetOption(id,option,value);
}

void yi_SetOption2(const char* id, const char* option, int r, int g, int b, YabInterface *yab)
{
	yab->SetOption(id,option,r,g,b);
}

void yi_SetOption3(const char* id, const char* option, double x, double y, YabInterface *yab)
{
	yab->SetOption(id,option,x,y);
}

void yi_SetOption4(const char* id, const char* option, YabInterface *yab)
{
	yab->SetOption(id,option);
}

void yi_SetOption5(const char* id, const char* option, int value, YabInterface *yab)
{
	yab->SetOption(id,option,value);
}

void yi_DropZone(const char* view, YabInterface *yab)
{
	yab->DropZone(view);
}

void yi_ColorControl1(double x, double y, const char* id, const char* view, YabInterface* yab)
{
	yab->ColorControl(x,y,id,view);
}

void yi_ColorControl2(const char* id, int r, int g, int b, YabInterface* yab)
{
	yab->ColorControl(id,r,g,b);
}

void yi_TextControl2(const char* id, const char* text, YabInterface* yab)
{
	yab->TextControl(id,_L(text));
}

void yi_TextControl3(const char* id, int mode, YabInterface* yab)
{
	yab->TextControl(id,mode);
}

void yi_TextControl5(const char* id, YabInterface* yab)
{
	yab->TextControl(id);
}

void yi_TextControl4(const char* id, const char* option, const char* value, YabInterface* yab)
{
	yab->TextControl(id,option,value);
}

void yi_TreeBox1(double x1, double y1, double x2, double y2, const char* id, int scrollbarType, const char* view, YabInterface* yab)
{
	yab->TreeBox1(BRect(x1,y1,x2,y2), id, scrollbarType, view);
}

void yi_TreeBox2(const char* id, const char* item, YabInterface* yab)
{
	yab->TreeBox2(id,_L(item));
}

void yi_TreeBox3(const char* id, const char* head, const char* item, int isExpanded, YabInterface* yab)
{
	yab->TreeBox3(id,_L(head),_L(item),isExpanded);
}

void yi_TreeBox4(const char* id, YabInterface* yab)
{
	yab->TreeBox4(id);
}

void yi_TreeBox5(const char* id, const char* item, YabInterface* yab)
{
	yab->TreeBox5(id,_L(item));
}

void yi_TreeBox7(const char* id, int pos, YabInterface* yab)
{
	yab->TreeBox7(id,pos);
}

void yi_TreeBox8(const char* id, int pos, YabInterface* yab)
{
	yab->TreeBox8(id,pos);
}

void yi_TreeBox9(const char* id, const char* head, const char* item, YabInterface* yab)
{
	yab->TreeBox9(id,_L(head), _L(item));
}

void yi_TreeBox10(const char* id, const char* head, YabInterface* yab)
{
	yab->TreeBox10(id,_L(head));
}

void yi_TreeBox11(const char* id, const char* head, YabInterface* yab)
{
	yab->TreeBox11(id,_L(head));
}

void yi_TreeBox12(const char* id, const char* item, int pos, YabInterface* yab)
{
	yab->TreeBox12(id,_L(item), pos);
}

const char* yi_TreeboxGet(const char* treebox, int pos, YabInterface *yab)
{
	return yab->TreeboxGet(treebox, pos);
}

int yi_TreeboxCount(const char* treebox, YabInterface *yab)
{
	return yab->TreeboxCount(treebox);
}

void yi_ButtonImage(double x,double y,const char* id,const char* enabledon, const char* enabledoff, const char *disabled, const char* view, YabInterface *yab)
{
	yab->ButtonImage(x,y, id, enabledon, enabledoff, disabled, view);
}

void yi_CheckboxImage(double x,double y,const char* id,const char* enabledon, const char* enabledoff, const char *disabledon, const char *disabledoff, int isActivated, const char* view, YabInterface *yab)
{
	yab->CheckboxImage(x,y, id, enabledon, enabledoff, disabledon, disabledoff, isActivated, view);
}

void yi_CheckboxSet(const char* id, int isActivated, YabInterface* yab)
{
	yab->CheckboxSet(id, isActivated);
}

void yi_RadioSet(const char* id, int isActivated, YabInterface* yab)
{
	yab->RadioSet(id, isActivated);
}

void yi_ToolTip(const char* view, const char* text, YabInterface *yab)
{
	yab->ToolTips(view,_L(text));
}

void yi_ToolTipColor(const char* color, int r, int g, int b, YabInterface *yab)
{
	yab->ToolTipsColor(color,r,g,b);
}

void yi_TreeSort(const char* view, YabInterface *yab)
{
	yab->TreeSort(view);
}

void yi_ListSort(const char* view, YabInterface *yab)
{
	yab->ListSort(view);
}

void yi_FileBox(double x1, double y1, double x2, double y2, const char* id, int scrollbartype, const char *option, const char* view, YabInterface *yab)
{
	yab->FileBox(BRect(x1,y1,x2,y2), id, scrollbartype, option, view);
}

void yi_FileBoxAdd2(const char* columnbox, const char* name, int pos, double maxWidth, double minWidth, double width, const char* option, YabInterface *yab)
{
	yab->FileBoxAdd(columnbox, _L(name), pos, maxWidth, minWidth, width, option);
}

void yi_FileBoxClear(const char* view, YabInterface *yab)
{
	yab->FileBoxClear(view);
}

void yi_ColumnBoxAdd(const char* id, int column, int position, int height, const char* item, YabInterface *yab)
{
	yab->ColumnBoxAdd(id, column, position, height, _L(item));
}

void yi_ColumnBoxSelect(const char *columnbox, int position, YabInterface *yab)
{
	yab->ColumnBoxSelect(columnbox, position);
}

void yi_ColumnBoxRemove(const char *columnbox, int position, YabInterface *yab)
{
	yab->ColumnBoxRemove(columnbox, position);
}

void yi_ColumnBoxColor(const char *columnbox, const char* option, int r, int g, int b, YabInterface *yab)
{
	yab->ColumnBoxColor(columnbox, option, r,g,b);
}

const char* yi_ColumnBoxGet(const char *columnbox, int column, int position, YabInterface *yab)
{
	return yab->ColumnBoxGet(columnbox, column, position);
}

int yi_ColumnBoxCount(const char *columnbox, YabInterface *yab)
{
	return yab->ColumnBoxCount(columnbox);
}

const char* yi_TextControlGet(const char* id, YabInterface* yab)
{
	return yab->TextControlGet(id);
}

int yi_DeskbarPosition(YabInterface *yab)
{
	return yab->DeskbarParam("position");
}

int yi_DeskbarExpanded(YabInterface *yab)
{
	return yab->DeskbarParam("expanded");
}

int yi_DeskbarWidth(YabInterface *yab)
{
	return yab->DeskbarParam("width");
}

int yi_DeskbarHeight(YabInterface *yab)
{
	return yab->DeskbarParam("height");
}

int yi_DeskbarX(YabInterface *yab)
{
	return yab->DeskbarParam("x");
}

int yi_DeskbarY(YabInterface *yab)
{
	return yab->DeskbarParam("y");
}

int yi_DesktopWidth(YabInterface *yab)
{
	return yab->DesktopParam(true);
}

int yi_DesktopHeight(YabInterface *yab)
{
	return yab->DesktopParam(false);
}

int yi_WindowGet(const char* view, const char* option, YabInterface *yab)
{
	return yab->WindowGet(view,option);
}

int yi_ViewGet(const char* view, const char* option, YabInterface *yab) //vasper
{
	return yab->ViewGet(view,option);
}

void yi_ClipboardCopy(const char* text, YabInterface *yab)
{
	yab->ClipboardCopy(text);
}

int yi_Printer(const char* docname, const char *view, const char* config, YabInterface *yab)
{
	return yab->Printer(docname, view,config);
}

void yi_PrinterConfig(const char* config, YabInterface *yab)
{
	yab->PrinterConfig(config);
}

const char* yi_ClipboardPaste(YabInterface *yab)
{
	return yab->ClipboardPaste();
}

int yi_NewAlert(const char* text, const char* button1, const char* button2, const char* button3, const char* option, YabInterface *yab)
{
	return yab->NewAlert(_L(text), _L(button1), _L(button2), _L(button3), option);
}

void yi_Calendar1(double x, double y, const char* id, const char* format, const char* date, const char* view, YabInterface *yab)
{
	yab->Calendar(x,y, id, format, date, view);
}

const char* yi_Calendar2(const char* id, YabInterface *yab)
{
	return yab->Calendar(id);
}

void yi_Calendar3(const char* id, const char* date, YabInterface *yab)
{
	yab->Calendar(id, date);
}

const char* yi_ListboxGet(const char* listbox, int pos, YabInterface *yab)
{
	return yab->ListboxGet(listbox, pos);
}

int yi_ListboxCount(const char* listbox, YabInterface *yab)
{
	return yab->ListboxCount(listbox);
}

void yi_ListboxAdd1(const char* listbox, const char* item, YabInterface *yab)
{	
	yab->ListboxAdd(listbox,_L(item));
}

void yi_ListboxAdd2(const char* listbox, int pos,  const char* item, YabInterface *yab)
{	
	yab->ListboxAdd(listbox, pos, _L(item));
}

void yi_ListboxSelect(const char* listbox, int pos, YabInterface *yab)
{	
	yab->ListboxSelect(listbox,pos);
}

void yi_ListboxRemove(const char* listbox, int pos, YabInterface *yab)
{	
	yab->ListboxRemove(listbox,pos);
}

void yi_Scrollbar(const char* id, int format, const char* view, YabInterface *yab)
{
	yab->Scrollbar(id, format, view);
}

void yi_ScrollbarSet1(const char* scrollview, const char* option, double position, YabInterface *yab)
{
	yab->ScrollbarSet(scrollview, option, position);
}

void yi_ScrollbarSet2(const char* scrollview, const char* option, double opt1, double opt2, YabInterface *yab)
{
	yab->ScrollbarSet(scrollview, option, opt1, opt2);
}

void yi_ScrollbarSet3(const char* scrollview, const char* option, YabInterface *yab)
{
	yab->ScrollbarSet(scrollview, option);
}

double yi_ScrollbarGet(const char* scrollview, const char* option, YabInterface *yab)
{
	return yab->ScrollbarGet(scrollview, option);
}

void yi_SplitView1(double x1,double y1,double x2,double y2, const char* id, int isVertical, int style, const char* view, YabInterface *yab)
{
	yab->SplitView(BRect(x1,y1,x2,y2), id, isVertical, style, view);
}

void yi_SplitView2(const char* splitView, const char* option, double position, YabInterface *yab)
{
	yab->SplitView(splitView, option, position);
}

void yi_SplitView3(const char* splitView, const char* option, double left, double right, YabInterface *yab)
{
	yab->SplitView(splitView, option, left, right);
}

double yi_SplitViewGet(const char* splitView, const char* option, YabInterface *yab)
{
	return yab->SplitViewGet(splitView, option);
}

void yi_StackView1(double x1,double y1,double x2,double y2, const char* id, int number, const char* view, YabInterface *yab)
{
	yab->StackViews(BRect(x1,y1,x2,y2), id, number, view);
}

void yi_StackView2(const char* stackView, int num, YabInterface *yab)
{
	yab->StackViews(stackView, num);
}

int yi_StackViewGet(const char* stackView, YabInterface *yab)
{
	return yab->StackViewGet(stackView);
}

void yi_DrawSet3(const char* option, int transparency, YabInterface *yab)
{
	yab->DrawSet3(option, transparency);
}

extern void yi_TextURL1(double x, double y, const char* id, const char* text, const char* url, const char* view, YabInterface *yab)
{
	yab->TextURL(x,y, id, _L(text), url, view);
}

void yi_TextURL2(const char* id, const char* option, int r, int g, int b, YabInterface *yab)
{
	yab->TextURL(id, option, r,g,b);
}

void yi_Menu2(const char* menuHead, int isRadio, const char* view, YabInterface *yab)
{
	yab->Menu(_L(menuHead), isRadio, view);
}

void yi_SubMenu1(const char* menuHead, const char* menuItem, const char* subMenuItem, const char* modifiers, const char* view, YabInterface *yab)
{
	yab->SubMenu(_L(menuHead), _L(menuItem), _L(subMenuItem), modifiers, view);
}

void yi_SubMenu2(const char* menuHead, const char* menuItem, int isRadio, const char* view, YabInterface *yab)
{
	yab->SubMenu(_L(menuHead), _L(menuItem), isRadio, view);
}

void yi_SpinControl1(double x, double y, const char* id, const char* label, int min, int max, int step, const char* view, YabInterface *yab)
{
	yab->SpinControl(x,y, id, _L(label), min, max, step, view);
}

void yi_SpinControl2(const char* spinControl, int value, YabInterface *yab)
{
	yab->SpinControl(spinControl, value);
}

int yi_SpinControlGet(const char *spinControl, YabInterface *yab)
{
	return yab->SpinControlGet(spinControl);
}

const char* yi_PopUpMenu(double x, double y, const char* menuItems, const char* view, YabInterface *yab)
{
	return yab->PopUpMenu(x,y,menuItems,view);
}

void yi_DropBoxSelect(const char* dropbox, int position, YabInterface *yab)
{
	yab->DropBoxSelect(dropbox, position);
}

void yi_DropBoxClear(const char* dropbox, YabInterface *yab)
{
	yab->DropBoxClear(dropbox);
}

void yi_DropBoxRemove(const char* dropbox, int position, YabInterface *yab)
{
	yab->DropBoxRemove(dropbox,position);
}

int yi_DropBoxCount(const char* dropbox, YabInterface *yab)
{
	return yab->DropBoxCount(dropbox);
}

const char* yi_DropBoxGet(const char* dropbox, int position, YabInterface *yab)
{
	return yab->DropBoxGet(dropbox, position);
}

int yi_ColorControlGet(const char* colorcontrol, const char* option, YabInterface *yab)
{
	return yab->ColorControlGet(colorcontrol, option);
}

int yi_SliderGet(const char* slider, YabInterface *yab)
{
	return yab->SliderGet(slider);
}

double yi_DrawGet1(const char* option, const char* txt, const char* view, YabInterface *yab)
{
	return yab->DrawGet(option, txt, view);
}

double yi_DrawGet2(const char* option, const char* view, YabInterface *yab)
{
	return yab->DrawGet(option, "", view);
}

const char* yi_DrawGet3(const char* option, YabInterface *yab)
{
	return yab->DrawGet(option);
}

void yi_SubMenu3(const char* menuHead, const char* menuItem, const char* subMenuItem, const char* option, const char* view, YabInterface *yab)
{
	yab->SubMenu3(_L(menuHead), _L(menuItem), _L(subMenuItem), option, view);
}

void yi_Menu3(const char* menuHead, const char* menuItem, const char* option,const char* view, YabInterface *yab)
{
	yab->Menu3(_L(menuHead), _L(menuItem), option, view);
}

double yi_MenuHeight(YabInterface *yab)
{
	return yab->MenuHeight();
}

double yi_TabHeight(YabInterface *yab)
{
	return yab->TabHeight();
}

double yi_ScrollbarWidth(YabInterface *yab)
{
	return yab->ScrollbarWidth();
}

void yi_exit(int code, YabInterface *yab)
{
	yab->KillThread(code);
}

const int yi_IsMouseIn(const char* view, YabInterface *yab)
{
	return yab->IsMouseIn(view);
}
	
const char* yi_GetMouseIn(YabInterface *yab)
{
	return yab->GetMouseIn();
}
		
	
const char* yi_KeyboardMessages(const char* view, YabInterface* yab)
{
	return yab->KeyboardMessages(view);
}

const char* yi_GetMouseMessages(const char* view, YabInterface* yab)
{
	return yab->GetMouseMessages(view);
}

const char* yi_CheckMessages(YabInterface* yab)
{
	return yab->GetMessageString();
}

int yi_MessageSend(const char* app, const char* msg,YabInterface* yab)
{
	return yab->MessageSend(app,msg);
}

int yi_ThreadKill(const char* option, int id,YabInterface* yab)
{
	return yab->ThreadKill(option, id);
}

int yi_ThreadGet(const char* option, const char* appname,YabInterface* yab)
{
	return yab->ThreadGet(option, appname);
}

void yi_SetCurrentLineNumber(int line, const char* libname, YabInterface* yab)
{
	yab->SetCurrentLineNumber(line, libname);
}

void yi_SetMainFileName(const char* name, YabInterface* yab)
{
	yab->SetMainFileName(name);
}

void yi_beep()
{
	beep();
}

void yi_Bitmap(double w, double h, const char* id,YabInterface* yab)
{
	yab->Bitmap(w,h,id);
}

int yi_BitmapColor(double x, double y, const char* id, const char* option, YabInterface *yab)
{
	yab->BitmapColor(x,y, id, option);
}

void yi_BitmapDraw(double x, double y, const char* bitmap, const char* mode, const char* view,YabInterface* yab)
{
	yab->BitmapDraw(x,y, bitmap, mode, view);
}

void yi_BitmapDraw2(double x1, double y1, double x2, double y2, const char* bitmap, const char* mode, const char* view,YabInterface* yab)
{
	yab->BitmapDraw(BRect(x1,y1,x2,y2), bitmap, mode, view);
}

void yi_BitmapGet(double x1, double y1, double x2, double y2, const char* id, const char* bitmap, YabInterface* yab)
{
	yab->BitmapGet(BRect(x1,y1,x2,y2), id, bitmap);
}

void yi_BitmapGet2(double w, const char* id, const char* path, YabInterface* yab)
{
	yab->BitmapGet(w, id, path);
}

int yi_BitmapGetNum(const char* id, const char* option, YabInterface* yab)
{
	yab->BitmapGet(id, option);
}

int yi_BitmapLoad(const char* filename, const char* bitmap, YabInterface* yab)
{
	yab->BitmapLoad(filename, bitmap);
}

void yi_BitmapGetIcon(const char* id, const char* option, const char* path, YabInterface* yab)
{
	yab->BitmapGetIcon(id, option, path);
}

void yi_BitmapDrag(const char* bitmap,YabInterface* yab)
{
	yab->BitmapDrag(bitmap);
}

void yi_BitmapRemove(const char* bitmap,YabInterface* yab)
{
	yab->BitmapRemove(bitmap);
}

void yi_Screenshot(double x1, double y1, double x2, double y2, const char* bitmap, YabInterface *yab)
{
	return yab->Screenshot(BRect(x1,y1,x2,y2), bitmap);
}

int yi_BitmapSave(const char* id, const char* filename, const char* type, YabInterface* yab)
{
	return yab->BitmapSave(id, filename, type); //, type);
}

void yi_Canvas(double x1, double y1, double x2, double y2, const char* id, const char* view, YabInterface *yab)
{
	yab->Canvas(BRect(x1,y1,x2,y2), id,view);
}

int yi_Sound(const char* filename, YabInterface* yab)
{
	return yab->Sound(filename);
}

void yi_SoundStop(int id, YabInterface* yab)
{
	yab->SoundStop(id);
}

void yi_SoundWait(int id, YabInterface* yab)
{
	yab->SoundWait(id);
}

void yi_ShortCut(const char* view, const char* key, const char* msg, YabInterface *yab)
{
	yab->ShortCut(view,key,msg);
}

int yi_IsComputerOn(YabInterface *yab)
{
	return yab->IsComputerOn();
}

void yi_DrawSet4(const char* option, const char* color,const char* view, YabInterface* yab)
{
	yab->DrawSet(option, color, view);
}

void yi_Treebox13(const char* id,const char* option, int pos, YabInterface* yab)
{
	yab->Treebox13(id, option, pos);
}

int yi_TreeboxGetOpt(const char* id, const char* option, int pos, YabInterface* yab)
{
	return yab->TreeboxGetOpt(id, option, pos);
}

int yi_ListboxGetNum(const char* id, YabInterface* yab)
{
	return yab->ListboxGetNum(id);
}

int yi_DropboxGetNum(const char* id, YabInterface* yab)
{
	return yab->DropboxGetNum(id);
}

int yi_TreeboxGetNum(const char* id, YabInterface* yab)
{
	return yab->TreeboxGetNum(id);
}

int yi_ColumnboxGetNum(const char* id, YabInterface* yab)
{
	return yab->ColumnboxGetNum(id);
}

int yi_DrawGet4(double x, double y, const char* option, const char* view, YabInterface* yab)
{
	return yab->DrawGet(BPoint(x,y),option,view);
}

void yi_MouseSet(const char* opt, YabInterface *yab)
{	
	yab->MouseSet(opt);
}

void yi_StatusBar(double x1, double y1, double x2, double y2, const char* id, const char* label1, const char* label2, const char* view, YabInterface *yab)
{
	yab->StatusBar(BRect(x1, y1, x2, y2), id, label1, label2, view);
}

void yi_StatusBarSet(const char* id, const char* label1, const char* label2, double state, YabInterface *yab)
{
	yab->StatusBarSet(id, label1, label2, state);
}

void yi_StatusBarSet2(double x1, double y1, double x2, double y2, const char* id, const char* view, YabInterface *yab)
{
	yab->StatusBarSet(BRect(x1, y1, x2, y2), id, view);
}

void yi_StatusBarSet3(const char* id, int r, int g, int b, YabInterface *yab)
{
	yab->StatusBarSet(id, r, g, b);
}

void yi_Launch(const char* strg, YabInterface *yab)
{
	yab->Launch(strg);
}

void yi_Attribute1(const char* type, const char* name, const char* value, const char* filename, YabInterface* yab) 
{
	yab->Attribute1(type, name, value, filename);
}

void yi_AttributeClear(const char* name, const char* filename, YabInterface* yab)
{
	yab->AttributeClear(name, filename);
}

const char* yi_AttributeGet1(const char* name, const char* filename, YabInterface* yab)
{
	return yab->AttributeGet1(name, filename);
}

double yi_AttributeGet2(const char* name, const char* filename, YabInterface* yab)
{
	return yab->AttributeGet2(name, filename);
}
