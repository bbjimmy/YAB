#include <Directory.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Messenger.h>
#include <Window.h>
#include "YabFilePanel.h"
#include "YabFilePanelLooper.h"

BEntry *YabFilePanel::MyFilePanel(const char *name, const char *directory, const char* filename, int mode)
{
	BEntry *myEntry = NULL;
	entry_ref ref;

	sem_id semaphore = create_sem(0, "yabfilepanel");
	YabFilePanelLooper *myLooper = new YabFilePanelLooper(semaphore);
	myLooper->Run();
	
	if(directory)
	{
		myEntry=new BEntry(directory);
		if(myEntry->GetRef(&ref)!=B_OK)
		{
			myEntry->Unset();
			myEntry->SetTo("/boot/home/");
			myEntry->GetRef(&ref);
		}
		myEntry->Unset();
		delete myEntry;
	}

	BFilePanel *myFilePanel = NULL; 
	switch(mode)
	{
		case 0:
			myFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(myLooper, myLooper), &ref, B_FILE_NODE, false, NULL, NULL, true, true);
			break;
		case 1:
			myFilePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(myLooper, myLooper), &ref, B_FILE_NODE, false, NULL, NULL, true, true);
			if (filename) myFilePanel->SetSaveText(filename);
			break;
		case 2:
			myFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(myLooper, myLooper), &ref, B_DIRECTORY_NODE, false, NULL, NULL, true, true);
			break;
		case 3:
			myFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(myLooper, myLooper), &ref, B_FILE_NODE|B_DIRECTORY_NODE, false, NULL, NULL, true, true);
			break;
	}

	if(name) myFilePanel->Window()->SetTitle(name);
	myFilePanel->Show();

	bool inloop = true;
	while(inloop)
	{
		while(acquire_sem_etc(semaphore, 1, B_RELATIVE_TIMEOUT, 10000)==B_TIMED_OUT) ;

		myEntry = myLooper->GetChosenFile();
		inloop = false;
/*
		if(mode!=2) 
			inloop = false;
		else
		{
			if(myEntry->IsDirectory())
				inloop = false;
			else
			{
				myFilePanel->Show();
			}
		}
*/
	}
	myLooper->Lock();
	myLooper->Quit();

	delete_sem(semaphore);
	delete myFilePanel;
	return myEntry;
}
