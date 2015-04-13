/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#ifndef YAB_COLUMN_TYPES_H
#define YAB_COLUMN_TYPES_H

#include "../global.h"
#include <String.h>
#include <Font.h>
#include <Bitmap.h>
#include "ColumnListView.h"

//=====================================================================
// Common base-class: a column that draws a standard title at its top.

class BTitledColumn : public BColumn
{
	public:
		BTitledColumn (const char *title, float width, float minWidth, float maxWidth, alignment align = B_ALIGN_LEFT);

		virtual void DrawTitle(BRect rect, BView* parent);
		virtual void GetColumnName(BString* into) const;
		void DrawString(const char*, BView*, BRect);
		void SetTitle(const char* title);
		void Title(BString* forTitle) const; // sets the BString arg to be the title
		float FontHeight() const;
	private:
		float fFontHeight;
		BString	fTitle;
};


//=====================================================================
// Field and column classes for strings.

class BYabField : public BField
{
	public:
		BYabField (const char* string);
		BYabField (BBitmap* bitmap);

		void SetString(const char* string, int height);
		const char* String() const;
		void SetClippedString(const char* string);
		const char*ClippedString();
		void SetWidth(float);
		float Width();
	
		const BBitmap* Bitmap();
		void SetBitmap(BBitmap* bitmap);
		bool HasBitmap();

	private:
		BBitmap* fBitmap;
		float fWidth;
		BString	fString;
		BString	fClippedString;
};


//--------------------------------------------------------------------

class BYabColumn: public BTitledColumn
{
	public:
		BYabColumn(const char *title, float width, float maxWidth, float minWidth, uint32 truncate, alignment align = B_ALIGN_LEFT);

		virtual void DrawField(BField* field, BRect rect, BView* parent);
		virtual int CompareFields(BField* field1, BField* field2);
		virtual	bool AcceptsField(const BField* field) const;

	private:
		uint32 fTruncate;
};

#endif
