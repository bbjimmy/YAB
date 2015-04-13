#ifndef YABCONTROLLOOK
#define YABCONTROLLOOK

#include <ControlLook.h>

class YabControlLook : public BControlLook
{
	public:
		YabControlLook();
		~YabControlLook();

		virtual void DrawActiveTabBottom(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags = 0, uint32 borders = BControlLook::B_ALL_BORDERS);
		virtual void DrawInactiveTabBottom(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags = 0, uint32 borders = BControlLook::B_ALL_BORDERS);
			void				_DrawRoundCornerLeftBottom(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& edgeColor,
									const rgb_color& frameColor,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);
			void				_DrawRoundCornerRightBottom(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& edgeTopColor,
									const rgb_color& edgeRightColor,
									const rgb_color& frameTopColor,
									const rgb_color& frameRightColor,
									const rgb_color& bevelTopColor,
									const rgb_color& bevelRightColor,
									const BGradientLinear& fillGradient);
};

#endif
