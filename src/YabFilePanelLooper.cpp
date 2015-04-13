#include <Entry.h>
#include <Directory.h>
#include "YabFilePanelLooper.h"

YabFilePanelLooper::YabFilePanelLooper(sem_id semaphore) : BLooper("YabFilePanelLooper")
{
	myEntry=new BEntry();
	mySemaphore = semaphore;
}

BEntry *YabFilePanelLooper::GetChosenFile()
{
	return myEntry;
}

void YabFilePanelLooper::MessageReceived(BMessage *msg)
{
	switch(msg->what) 
	{
		case B_REFS_RECEIVED: 
		{
			entry_ref ref;
			if (msg->FindRef("refs", 0, &ref)==B_OK) 
				myEntry->SetTo(&ref);
			else 
				myEntry->Unset();
		}
		break;
		
		case B_SAVE_REQUESTED: 
		{
			const char *selected;
			entry_ref ref;
			
			if (msg->FindString("name", &selected)!=B_OK) 
				myEntry->Unset();
			else
			{
				if (msg->FindRef("directory", 0, &ref)==B_OK) 
				{
					BDirectory *myDirectory = new BDirectory(&ref);
					myEntry->SetTo(myDirectory, selected);
					myDirectory->Unset();
					delete myDirectory;
				}
				else 
					myEntry->Unset();
			}
		}
		break;

		case B_CANCEL: 
			release_sem(mySemaphore);
			break;
	}
}

