#include <GradientLinear.h>
#include <Region.h>
#include "YabControlLook.h"

YabControlLook::YabControlLook()
{ }

YabControlLook::~YabControlLook()
{ }

void YabControlLook::DrawActiveTabBottom(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color edgeShadowColor;
	rgb_color edgeLightColor;
	rgb_color frameShadowColor;
	rgb_color frameLightColor;
	rgb_color bevelShadowColor;
	rgb_color bevelLightColor;
	BGradientLinear fillGradient;
	fillGradient.SetStart(rect.LeftBottom() + BPoint(3, -3));
	fillGradient.SetEnd(rect.LeftTop() + BPoint(3, 3));

	if (flags & B_DISABLED) {
		edgeShadowColor = base;
		edgeLightColor = base;
		frameShadowColor = tint_color(base, 1.30);
		frameLightColor = tint_color(base, 1.25);
		bevelShadowColor = tint_color(base, 1.07);
		bevelLightColor = tint_color(base, 0.8);
		fillGradient.AddColor(tint_color(base, 0.85), 0);
		fillGradient.AddColor(base, 255);
	} else {
		edgeShadowColor = tint_color(base, 1.03);
		edgeLightColor = tint_color(base, 0.80);
		frameShadowColor = tint_color(base, 1.30);
		frameLightColor = tint_color(base, 1.30);
		bevelShadowColor = tint_color(base, 1.07);
		bevelLightColor = tint_color(base, 0.6);
		fillGradient.AddColor(tint_color(base, 0.75), 0);
		fillGradient.AddColor(tint_color(base, 1.03), 255);
	}

	static const float kRoundCornerRadius = 4;

	// left/top corner
	BRect cornerRect(rect);
	cornerRect.right = cornerRect.left + kRoundCornerRadius;
	cornerRect.top = cornerRect.bottom - kRoundCornerRadius;

	BRegion clipping(rect);
	clipping.Exclude(cornerRect);

	_DrawRoundCornerLeftBottom(view, cornerRect, updateRect, base, edgeShadowColor,
		frameLightColor, bevelLightColor, fillGradient);

	// left/top corner
	cornerRect.right = rect.right;
	cornerRect.left = cornerRect.right - kRoundCornerRadius;

	clipping.Exclude(cornerRect);

	_DrawRoundCornerRightBottom(view, cornerRect, updateRect, base, edgeShadowColor,
		edgeLightColor, frameLightColor, frameShadowColor, bevelLightColor,
		bevelShadowColor, fillGradient);

	// rest of frame and fill
	view->ConstrainClippingRegion(&clipping);

	_DrawFrame(view, rect, edgeShadowColor, edgeLightColor, edgeLightColor,
		edgeShadowColor,
		borders & (B_LEFT_BORDER | B_BOTTOM_BORDER | B_RIGHT_BORDER));
	if ((borders & B_LEFT_BORDER) == 0)
		rect.left++;
	if ((borders & B_RIGHT_BORDER) == 0)
		rect.right--;

	_DrawFrame(view, rect, frameLightColor, frameShadowColor, frameShadowColor,
		frameLightColor, B_LEFT_BORDER | B_BOTTOM_BORDER | B_RIGHT_BORDER);

	_DrawFrame(view, rect, bevelLightColor, bevelShadowColor, bevelShadowColor,
		bevelLightColor);

	view->FillRect(rect, fillGradient);

	view->SetHighColor(216,216,216);
	view->StrokeLine(BPoint(rect.left, rect.top), BPoint(rect.right, rect.top));

	view->ConstrainClippingRegion(NULL);
}

void YabControlLook::DrawInactiveTabBottom(BView* view, BRect& rect,const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color edgeShadowColor;
	rgb_color edgeLightColor;
	rgb_color frameShadowColor;
	rgb_color frameLightColor;
	rgb_color bevelShadowColor;
	rgb_color bevelLightColor;
	BGradientLinear fillGradient;
	fillGradient.SetStart(rect.LeftBottom() + BPoint(3, -3));
	fillGradient.SetEnd(rect.LeftTop() + BPoint(3, 3));

	if (flags & B_DISABLED) {
		edgeShadowColor = base;
		edgeLightColor = base;
		frameShadowColor = tint_color(base, 1.30);
		frameLightColor = tint_color(base, 1.25);
		bevelShadowColor = tint_color(base, 1.07);
		bevelLightColor = tint_color(base, 0.8);
		fillGradient.AddColor(tint_color(base, 0.85), 0);
		fillGradient.AddColor(base, 255);
	} else {
		edgeShadowColor = tint_color(base, 1.03);
		edgeLightColor = tint_color(base, 0.80);
		frameShadowColor = tint_color(base, 1.30);
		frameLightColor = tint_color(base, 1.30);
		bevelShadowColor = tint_color(base, 1.17);
		bevelLightColor = tint_color(base, 1.10);
		fillGradient.AddColor(tint_color(base, 1.12), 0);
		fillGradient.AddColor(tint_color(base, 1.08), 255);
	}

	// active tabs stand out at the top, but this is an inactive tab
	view->SetHighColor(base);
	view->FillRect(BRect(rect.left, rect.bottom - 4, rect.right, rect.bottom));
	rect.bottom -= 4;

	// frame and fill
	_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor, edgeLightColor,
		edgeLightColor,
		borders & (B_LEFT_BORDER | B_BOTTOM_BORDER | B_RIGHT_BORDER));

	_DrawFrame(view, rect, frameLightColor, frameLightColor, frameShadowColor,
		frameShadowColor,
		borders & (B_LEFT_BORDER | B_BOTTOM_BORDER | B_RIGHT_BORDER));

	if (rect.IsValid()) {
		_DrawFrame(view, rect, bevelShadowColor, bevelShadowColor,
			bevelLightColor, bevelLightColor, B_LEFT_BORDER & ~borders);
	} else {
		if ((B_LEFT_BORDER & ~borders) != 0)
			rect.left++;
	}

	view->FillRect(rect, fillGradient);
}

void
YabControlLook::_DrawRoundCornerLeftBottom(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& edgeColor,
	const rgb_color& frameColor, const rgb_color& bevelColor,
	const BGradientLinear& fillGradient)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	BRegion clipping(rect);
	view->ConstrainClippingRegion(&clipping);

	// background
	view->SetHighColor(base);
	view->FillRect(rect);

	// outer edge
	BRect ellipseRect(rect);
	ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
	ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

	view->SetHighColor(edgeColor);
	view->FillEllipse(ellipseRect);

	// frame
	ellipseRect.InsetBy(1, 1);
	view->SetHighColor(frameColor);
	view->FillEllipse(ellipseRect);

	// bevel
	ellipseRect.InsetBy(1, 1);
	view->SetHighColor(bevelColor);
	view->FillEllipse(ellipseRect);

	// fill
	ellipseRect.InsetBy(1, 1);
	view->FillEllipse(ellipseRect, fillGradient);

	view->ConstrainClippingRegion(NULL);
}

void
YabControlLook::_DrawRoundCornerRightBottom(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& edgeTopColor, const rgb_color& edgeRightColor,
	const rgb_color& frameTopColor, const rgb_color& frameRightColor,
	const rgb_color& bevelTopColor, const rgb_color& bevelRightColor,
	const BGradientLinear& fillGradient)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	BRegion clipping(rect);
	view->ConstrainClippingRegion(&clipping);

	// background
	view->SetHighColor(base);
	view->FillRect(rect);

	// outer edge
	BRect ellipseRect(rect);
	ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
	ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

	BGradientLinear gradient;
	gradient.AddColor(edgeTopColor, 0);
	gradient.AddColor(edgeRightColor, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.RightBottom());
	view->FillEllipse(ellipseRect, gradient);

	// frame
	ellipseRect.InsetBy(1, 1);
	rect.right--;
	rect.top++;
	if (frameTopColor == frameRightColor) {
		view->SetHighColor(frameTopColor);
		view->FillEllipse(ellipseRect);
	} else {
		gradient.SetColor(0, frameTopColor);
		gradient.SetColor(1, frameRightColor);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.RightBottom());
		view->FillEllipse(ellipseRect, gradient);
	}

	// bevel
	ellipseRect.InsetBy(1, 1);
	rect.right--;
	rect.top++;
	gradient.SetColor(0, bevelTopColor);
	gradient.SetColor(1, bevelRightColor);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.RightBottom());
	view->FillEllipse(ellipseRect, gradient);

	// fill
	ellipseRect.InsetBy(1, 1);
	view->FillEllipse(ellipseRect, fillGradient);

	view->ConstrainClippingRegion(NULL);
}


