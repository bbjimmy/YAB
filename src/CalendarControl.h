// Calendar Control version 2.5
// by Al.V. Sarikov.
// Kherson, Ukraine, 2006.
// E-mail: avix@ukrpost.net.
// Home page: http://avix.pp.ru.

// Control which allows to work with dates:
// enter date to text field and choose it from calendar.

// Distributed under BSD license (see LICENSE file).

#include <Control.h>
#include <PictureButton.h>
#include <Button.h>
#include <TextView.h>

class DateTextView;

class CalendarButton : public BButton
{
	public:
		CalendarButton(BRect frame, const char* name, const char* label,
			BMessage* message, uint32 resizingMode, uint32 flags)
			: BButton(frame, name, label, message, resizingMode, flags)
		{};
		~CalendarButton() {};
		void Draw(BRect update);
};

// Formats of date

enum date_format {
	CC_DD_MM_YYYY_FORMAT = 0, 
	CC_MM_DD_YYYY_FORMAT
};

enum full_short_year {
	CC_FULL_YEAR = 0,  // DD.MM.YYYY
	CC_SHORT_YEAR = 8 // DD.MM.YY
};

enum century_begin {
	CC_FULL_CENTURY = 0,  // first year is first year of century (01-00)
	CC_HALF_CENTURY = 16 // first year is 51th year of century (51-50)
};

enum divider_format {
	CC_DOT_DIVIDER = 0, // .
	CC_SLASH_DIVIDER,   // /
	CC_MINUS_DIVIDER,   // -
	CC_ALL_DIVIDERS     // 2 bits, and some one bit is reserved
};


class CalendarControl: public BControl
{
	public:
		CalendarControl(BPoint p,
               		   const char* name,
		                  int day=0,
                  		int month=0,
                  		int year=0,
                  		uint32 flags=CC_DD_MM_YYYY_FORMAT | CC_FULL_YEAR,
                  		uint32 look=CC_DOT_DIVIDER);
		~CalendarControl();
		virtual void AttachedToWindow(void);
		virtual void Draw(BRect r);
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void MakeFocus(bool focused=true);
		virtual void MessageReceived(BMessage *msg);
		virtual void SetEnabled(bool enabled);
		virtual void SetViewColor(rgb_color color);
		void SetViewColor(uchar red, uchar green, uchar blue, uchar alpha=255);
		virtual void WindowActivated(bool active);
  		
		void GetDate(int *day, int *month, int *year);
		void SetDate(int day=0, int month=0, int year=0);
		void SetDate(const char *tdate);
		void GetYearRange(int *first_year, int *last_year);
		uint32 GetLook();
		void SetLook(uint32 look);
		uint32 GetFlags();
		void SetFlags(uint32 flags);
		const char* Text() const;
		BTextView *TextView(void) const;
		const char* Version();

	private:
		void MakeButton();
  		
		DateTextView *myDateTextView;
		CalendarButton *myButton;
		bool myWindowActive;
		BPoint myOrigin;
		rgb_color view_color;
};

