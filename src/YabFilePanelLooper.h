#ifndef YABFPLOOPER_H
#define YABFPLOOPER_H

#include <Looper.h>
#include <Message.h>
#include <Entry.h>

class YabFilePanelLooper : public BLooper
{
	public:
		YabFilePanelLooper(sem_id semaphore);
		void MessageReceived(BMessage *msg);
		BEntry *GetChosenFile();
	private:
		BEntry *myEntry;
		sem_id mySemaphore;
};

#endif
