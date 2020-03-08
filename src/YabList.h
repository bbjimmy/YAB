#ifndef YABLIST_H
#define YABLIST_H

#include <String.h>
#include <View.h>

class YabList
{
public:
	YabList();
	~YabList();
	void AddView(const char* id, const BView* view, int type);
	void DelView(const char* id);
	void DelAll();
	const void* GetView(const char* id);
	const int GetType(const char* id);
	const int CountItems();
	const void* ItemAt(int i);
	void PrintOut();
	
private:
	int ViewNum(const char* id);
	BList* idList;
	BList* viewList;
	BList* typeList;
};

#endif
