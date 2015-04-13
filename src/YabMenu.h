#ifndef YABMENU_H
#define YABMENU_H

class YabMenu : public BMenu
{
public:
	YabMenu(const char* name) : BMenu(name)
	{
	}

	void MyHide()
	{
		Hide();
	}
};

#endif
