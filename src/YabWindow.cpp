#include "global.h"
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <RadioButton.h>
#include <Slider.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>
#include <stdio.h>
#include "YabWindow.h"
#include "column/ColumnListView.h"

const uint32 YABBUTTON                  = 'YBbu'; 
const uint32 YABMENU			= 'YBme'; 
const uint32 YABSUBMENU			= 'YBsu'; 
const uint32 YABTEXTCONTROL		= 'YBtc'; 
const uint32 YABCHECKBOX                = 'YBcb';
const uint32 YABRADIOBUTTON             = 'YBrb'; 
const uint32 YABLISTBOXSELECT           = 'YBls';
const uint32 YABLISTBOXINVOKE           = 'YBli';
const uint32 YABDROPBOX                 = 'YBdb';  
const uint32 YABSLIDER			= 'YBsl';  
const uint32 YABCOLORCONTROL		= 'YBco';  
const uint32 YABTREEBOXSELECT           = 'YBts';
const uint32 YABTREEBOXINVOKE           = 'YBti';
const uint32 YABFILEBOXSELECT           = 'YBfs';
const uint32 YABFILEBOXINVOKE           = 'YBfi';
const uint32 YABSPINCONTROL		= 'YBsp';
const uint32 YABSHORTCUT		= 'YBsh';


YabWindow::YabWindow(BRect frame, const char* title, const char* id, window_look winlook, window_feel winfeel, uint32 flags)
	: BWindow (frame, title, winlook, winfeel, flags)
{
	messageString.SetTo("");
	idString.SetTo(id);
	showing = 1;
	dropMsg.SetTo("");
	WActivated=-1;
	WFrameMoved=-1;
	WFrameResized=-1;
}

YabWindow::~YabWindow()
{
}

void YabWindow::WindowActivated(bool state)
{
	if (state) 
		WActivated=1;
	else
		WActivated=0;
}

void YabWindow::FrameMoved(BPoint new_position)
{
	WFrameMoved=1;
	Wpx=(int)new_position.x;
	Wpy=(int)new_position.y;
}

void YabWindow::FrameResized(float new_width, float new_height)
{
	WFrameResized=1;
	Wph=(int)new_height;
	Wpw=(int)new_width;
}

void YabWindow::MessageReceived(BMessage *message)
{
	// if(message) message->PrintToStream();
	switch(message->what)
	{
		case YABBUTTON:
			{
				BString tmpMessage("");
                        	BButton *myButtonPressed;
                        	message->FindPointer("source",(void **) &myButtonPressed);
				tmpMessage += myButtonPressed->Name();
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABMENU:
			{
				BString tmpMessage("");
				BMenuItem *mySelectedMenu;
                        	message->FindPointer("source",(void **) &mySelectedMenu);
				BMenu *myMenu = mySelectedMenu->Menu();
				tmpMessage += ((BMenuBar*)myMenu->Supermenu())->Parent()->Name();
				tmpMessage += ":";
				tmpMessage += myMenu->Name();
				tmpMessage += ":";
                        	tmpMessage += mySelectedMenu->Label(); 
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABSUBMENU:
			{
				BString tmpMessage("");
				BMenuItem *mySelectedMenu;
                        	message->FindPointer("source",(void **) &mySelectedMenu);
				BMenu *myMenu = mySelectedMenu->Menu();
				tmpMessage += ((BMenuBar*)myMenu->Supermenu()->Supermenu())->Parent()->Name();
				tmpMessage += ":";
				tmpMessage += myMenu->Supermenu()->Name();
				tmpMessage += ":";
				tmpMessage += myMenu->Name();
				tmpMessage += ":";
                        	tmpMessage += mySelectedMenu->Label(); 
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABTEXTCONTROL:
			{
				BString tmpMessage("");
				BTextControl *myTextControl;
                        	message->FindPointer("source",(void **) &myTextControl);
				tmpMessage += myTextControl->Name();
				tmpMessage += ":";
				tmpMessage += myTextControl->Text();
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABCHECKBOX:
			{
				BString tmpMessage("");
				BCheckBox *myCheckBox;
                        	message->FindPointer("source",(void **) &myCheckBox);
				tmpMessage += myCheckBox->Name();
				tmpMessage += ":";
				if(myCheckBox->Value()==B_CONTROL_ON)
					tmpMessage += "ON|";
				else
					tmpMessage += "OFF|";
				messageString += tmpMessage;
			}
			break;
		case YABRADIOBUTTON:
			{
				BString tmpMessage("");
				BRadioButton *myRadioButton;
                        	message->FindPointer("source",(void **) &myRadioButton);
				tmpMessage += myRadioButton->Name();
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABLISTBOXINVOKE:
			{
				BListView *myList;
				message->FindPointer("source",(void **) &myList);
				int i = myList->CurrentSelection();
				if(i>=0)
				{
					BString tmpMessage("");
					tmpMessage += myList->Name();
					tmpMessage += ":_Invoke:";
					// tmpMessage += ((BStringItem*)(myList->ItemAt(i)))->Text();
					tmpMessage << i+1;
					tmpMessage += "|";
					messageString += tmpMessage;
				}
			}
			break;
		case YABLISTBOXSELECT:
			{
				BListView *myList;
				message->FindPointer("source",(void **) &myList);
				int i = myList->CurrentSelection();
				if(i>=0)
				{
					BString tmpMessage("");
					tmpMessage += myList->Name();
					tmpMessage += ":_Select:";
					// tmpMessage += ((BStringItem*)(myList->ItemAt(i)))->Text();
					tmpMessage << i+1;
					tmpMessage += "|";
					messageString += tmpMessage;
				}
			}
			break;
		case YABDROPBOX:
			{
				BString tmpMessage("");
				BMenuItem *myMenuItem;
				message->FindPointer("source",(void **) &myMenuItem);
				tmpMessage += (myMenuItem->Menu())->Supermenu()->Parent()->Name();
				tmpMessage += ":";
				tmpMessage += myMenuItem->Label();
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABSLIDER:
			{
				BString tmpMessage("");
                        	BSlider *mySlider;
                        	message->FindPointer("source",(void **) &mySlider);
				tmpMessage += mySlider->Name();
				tmpMessage += ":";
				tmpMessage << mySlider->Value();
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABCOLORCONTROL:
			{
				rgb_color col;
				BString tmpMessage("");
                        	BColorControl *myCControl;
                        	message->FindPointer("source",(void **) &myCControl);
				tmpMessage += myCControl->Name();
				tmpMessage += ":";
				col = myCControl->ValueAsColor();
				tmpMessage << col.red << ":" << col.green << ":" << col.blue;
				tmpMessage += "|";
				messageString += tmpMessage;
			}
			break;
		case YABTREEBOXINVOKE:
			{
				BOutlineListView *myList;
				message->FindPointer("source",(void **) &myList);
				int i = myList->FullListCurrentSelection();
				if(i>=0)
				{
					BString tmpMessage("");
					const char* txt = ((BStringItem*)(myList->FullListItemAt(i)))->Text();
					tmpMessage += myList->Name();
					tmpMessage += ":_Invoke:";
					tmpMessage << i+1;
					/*
					int n = tmpMessage.Length();
					BListItem *superitem = myList->Superitem(myList->FullListItemAt(i));
					while(superitem)
					{
						BString t("");
						t << ((BStringItem*)superitem)->Text() << ":";
						tmpMessage.Insert(t,n);
						superitem = myList->Superitem(superitem);
					}
					tmpMessage += txt;*/
					tmpMessage += "|";
					messageString += tmpMessage;
				}
			}
			break;
		case YABTREEBOXSELECT:
			{
				BOutlineListView *myList;
				message->FindPointer("source",(void **) &myList);
				int i = myList->FullListCurrentSelection();
				if(i>=0)
				{
					BString tmpMessage("");
					const char* txt = ((BStringItem*)(myList->FullListItemAt(i)))->Text();
					tmpMessage += myList->Name();
					tmpMessage += ":_Select:";
					tmpMessage << i+1;
					/*
					int n = tmpMessage.Length();
					BListItem *superitem = myList->Superitem(myList->FullListItemAt(i));
					while(superitem)
					{
						BString t("");
						t << ((BStringItem*)superitem)->Text() << ":";
						tmpMessage.Insert(t,n);
						superitem = myList->Superitem(superitem);
					}
					tmpMessage += txt;*/
					tmpMessage += "|";
					messageString += tmpMessage;
				}
			}
			break;
		case YABFILEBOXSELECT:
			{
				BColumnListView *myList;
				message->FindPointer("source",(void **) &myList);
				BRow *myRow = NULL;
				if(myList) myRow = myList->CurrentSelection();
				if(myRow)
				{
				//	if(!myList->IsFocus()) myList->MakeFocus();
					BString tmpMessage("");
					tmpMessage += myList->Name();
					tmpMessage += ":_Select:";
					tmpMessage << myList->IndexOf(myRow)+1; 
					tmpMessage += "|";
					messageString += tmpMessage;
				}
			}
			break;
		case YABFILEBOXINVOKE:
			{
				BColumnListView *myList;
				message->FindPointer("source",(void **) &myList);
				BRow *myRow = NULL;
				if(myList) myRow = myList->CurrentSelection();
				if(myRow)
				{
				//	if(!myList->IsFocus()) myList->MakeFocus();
					BString tmpMessage("");
					tmpMessage += myList->Name();
					tmpMessage += ":_Invoke:";
					tmpMessage << myList->IndexOf(myRow)+1; 
					tmpMessage += "|";
					messageString += tmpMessage;
				}
			}
			break;
		case YABSHORTCUT:
			{
				const char* myMsg;
				if(message->FindString("shortcut", &myMsg) == B_OK)
				{
					messageString += myMsg;
					messageString += "|";
				}
			}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

const BString YabWindow::getMessages()
{
	BString tmp(messageString);
	tmp += dropMsg;
	messageString.SetTo("");
	dropMsg.SetTo("");
	return (const BString)tmp;
}

bool YabWindow::QuitRequested()
{
	messageString += idString;
	messageString += ":_QuitRequested|";
	return false;
}

bool YabWindow::IsPaper(uint8* a)
{
	if(a[32] != 0) return true;
	return false;
}

