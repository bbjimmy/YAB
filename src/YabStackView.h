#ifndef YAB_STACKVIEW_H_
#define YAB_STACKVIEW_H_

#include <View.h>

class YabStackView : public BView
{
	public:
		YabStackView(BRect frame, const char *name, int32 number_of_views, uint32 resizingMode =  B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS, const BFont *labelFont = be_plain_font);
		~YabStackView();

		void AddViews(BView** stackedViews);
		int32 CurrentView();
		virtual void SelectView(int32 index);

	private:
		BView** myViews;
		int32 myCurrent;
		BRect myBounds;
		int32 myNumOfViews;
};

#endif
