#include <List.h>
#include <String.h>
#include <View.h>
#include "YabList.h"
#include <stdio.h>

YabList::YabList()
{
	idList = new BList(1);
	viewList = new BList(1);
	typeList = new BList(1);
}

YabList::~YabList()
{
	DelAll();
	delete idList;
	delete viewList;
	delete typeList;
}

int YabList::ViewNum(const char* id)
{
	int tmp=-1;
	if(id)
	{
		for(int i=0; i<idList->CountItems(); i++)
			if(!strcmp(id, ((BString*)(idList->ItemAt(i)))->String() ))
			{
				tmp = i;
				break;
			}
	}
	return tmp;
}

void YabList::AddView(const char* id, const BView* view, int type)
{
	idList->AddItem((void*)new BString(id));
	viewList->AddItem((void*)view);
	typeList->AddItem((void*)(addr_t)type);
}

void YabList::DelView(const char* id)
{
	int i = ViewNum(id);
	if(i!=-1)
	{
		idList->RemoveItem(i);
		viewList->RemoveItem(i);
		typeList->RemoveItem(i);
	}
}

void YabList::DelAll()
{
	idList->MakeEmpty();
	viewList->MakeEmpty();
	typeList->MakeEmpty();
}

const void* YabList::GetView(const char* id)
{
	int t = ViewNum(id);
	if(t>=0)
		return viewList->ItemAt(t);
	else
		return NULL;
}

const int YabList::GetType(const char* id)
{
	return (int)(addr_t)typeList->ItemAt(ViewNum(id));
}

const int YabList::CountItems()
{
	return typeList->CountItems();
}

const void* YabList::ItemAt(int i)
{
	return viewList->ItemAt(i);
}

void YabList::PrintOut()
{
	printf("\n");
	for(int i=0; i<idList->CountItems(); i++)
		printf("\t View %s and the id %d %d \n", ((BString*)(idList->ItemAt(i)))->String() , idList->ItemAt(i), viewList->ItemAt(i));
	printf("\n");
}
