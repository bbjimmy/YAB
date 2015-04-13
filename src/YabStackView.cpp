// #include <string.h>
#include "YabStackView.h"


YabStackView::YabStackView(BRect frame, const char *name, int32 number_of_views, uint32 resizingMode, uint32 flags, const BFont *labelFont) : BView(frame, name, resizingMode, flags)
{
	myViews = new BView*[number_of_views];
	// init
	for(int i=0; i < number_of_views; i++)
	{
		myViews[i] = NULL;
	}
	myCurrent = 0;
	myBounds = Bounds();
	myNumOfViews = number_of_views;
}

YabStackView::~YabStackView()
{
	delete[] myViews;
}

void YabStackView::AddViews(BView** stackedViews)
{
	for(int32 i = 0; i < myNumOfViews; i++)
	{
		myViews[i] = stackedViews[i];
		if(i != myCurrent) myViews[i]->Hide();
		AddChild(myViews[i]);
	}
}


void YabStackView::SelectView(int32 index)
{
	if(index != myCurrent && index >= 0 && index < myNumOfViews)
	{
		Invalidate(myBounds);
		myViews[myCurrent]->Hide();
		myCurrent = index;
		Invalidate(myBounds);
		myViews[myCurrent]->Show();
	}
}

int32 YabStackView::CurrentView()
{
	return myCurrent;
}

