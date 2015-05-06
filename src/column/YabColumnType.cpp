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

#include "YabColumnType.h"
#include <stdio.h>
#include <Application.h>
#include <BitmapStream.h> 
#include <File.h>
#include <Path.h>
#include <Roster.h>
#include <TranslatorRoster.h>
#include <Translator.h>
#include <View.h>
#include <NodeInfo.h>
#ifdef ZETA
	#include <sys_apps/Tracker/Icons.h>
#endif

#define kTEXT_MARGIN	8
#define kIMG_MARGIN	2


//=====================================================================

BTitledColumn::BTitledColumn(const char* title, float width, float minWidth,
							 float maxWidth, alignment align)
	:BColumn(width, minWidth, maxWidth, align),
	fTitle(title)
{
	font_height	fh;

	be_plain_font->GetHeight(&fh);
	fFontHeight = fh.descent + fh.leading;
}


//--------------------------------------------------------------------

void BTitledColumn::DrawTitle(BRect rect, BView* parent)
{
	float		width = rect.Width() - (2 * kTEXT_MARGIN);
	BString		out_string(fTitle);

	parent->TruncateString(&out_string, B_TRUNCATE_END, width + 2);
	DrawString(out_string.String(), parent, rect);
}


//--------------------------------------------------------------------

void BTitledColumn::GetColumnName(BString* into) const
{
	*into = fTitle;
}


//--------------------------------------------------------------------

void BTitledColumn::DrawString(const char* string, BView* parent, BRect rect)
{
	float		width = rect.Width() - (2 * kTEXT_MARGIN);
	float		y;
	BFont		font;
	font_height	finfo;

	parent->GetFont(&font);
	font.GetHeight(&finfo);
	y = rect.top + ((rect.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
					(finfo.ascent + finfo.descent) - 2;

	switch (Alignment())
	{
		case B_ALIGN_LEFT:
			parent->MovePenTo(rect.left + kTEXT_MARGIN, y);
			break;

		case B_ALIGN_CENTER:
			parent->MovePenTo(rect.left + kTEXT_MARGIN + ((width - font.StringWidth(string)) / 2), y);
			break;

		case B_ALIGN_RIGHT:
			parent->MovePenTo(rect.right - kTEXT_MARGIN - font.StringWidth(string), y);
			break;
	}
	parent->DrawString(string);
}


//--------------------------------------------------------------------

void BTitledColumn::SetTitle(const char* title)
{
	fTitle.SetTo(title);
}


//--------------------------------------------------------------------

void BTitledColumn::Title(BString* forTitle) const
{
	if (forTitle)
		forTitle->SetTo(fTitle.String());
}


//--------------------------------------------------------------------

float BTitledColumn::FontHeight() const
{
	return fFontHeight;
}


//=====================================================================

BYabField::BYabField(const char* string)
	:fWidth(0), fString(string), fClippedString(string)
{
	int n = fString.FindFirst("__Icon__=");
	fBitmap = NULL;
	if(n==0)
	{
		BString myPath;
		fString.CopyInto(myPath, 9, fString.Length()-9);
		BPath AppDirectory;

	        // app directory
		BString ApplicationDirectory("");
	        app_info appinfo;
	
		if(be_roster->GetRunningAppInfo(be_app->Team(), &appinfo) == B_OK)
	        {
	                BEntry ApplicationEntry( &appinfo.ref);
			BEntry ApplicationDirectoryEntry;

			if( ApplicationEntry.GetParent( &ApplicationDirectoryEntry) == B_OK)
			{
				if( AppDirectory.SetTo( &ApplicationDirectoryEntry) == B_OK)
				{
					// strcpy(ApplicationDirectory, AppDirectory.Path());
					ApplicationDirectory.SetTo(AppDirectory.Path());
				}
			}
		}  
	        BFile ImageFile;
	        BPath ImagePath;
	
	        if(myPath[0] == '/')
	                ImageFile.SetTo( myPath.String(), B_READ_ONLY);
	        else
	                // App directory.
	                if(ApplicationDirectory != "")
	                {
	                        if( ImagePath.SetTo(ApplicationDirectory.String(), myPath.String()) == B_OK)
	                                ImageFile.SetTo( ImagePath.Path(), B_READ_ONLY);
	                }
	
	        if( ImageFile.InitCheck() != B_OK)
	                ImageFile.SetTo( myPath.String(), B_READ_ONLY);
	
	        if( ImageFile.InitCheck() == B_OK)
		{
	        	BTranslatorRoster *Roster = BTranslatorRoster::Default();
	        	if( Roster)
			{
	        		BBitmapStream Stream;
	        		if( Roster->Translate( &ImageFile, NULL, NULL, &Stream, B_TRANSLATOR_BITMAP) == B_OK)
	        			Stream.DetachBitmap( &fBitmap);
				delete Roster;
			}
		}
	}
}


void BYabField::SetString(const char* val, int height)
{
	fString = val;
	fClippedString = "";
	fWidth = 0;

	fBitmap = NULL;
	if( ! fString.FindFirst("__Icon__=") )
	{
		BString myPath;
		fString.CopyInto(myPath, 9, fString.Length()-9);
		BPath AppDirectory;

		// app directory
		BString ApplicationDirectory("");
	        app_info appinfo;
	
		if(be_roster->GetRunningAppInfo(be_app->Team(), &appinfo) == B_OK)
		{
			BEntry ApplicationEntry( &appinfo.ref);
			BEntry ApplicationDirectoryEntry;

			if( ApplicationEntry.GetParent( &ApplicationDirectoryEntry) == B_OK)
			{
				if( AppDirectory.SetTo( &ApplicationDirectoryEntry) == B_OK)
				{
					// strcpy(ApplicationDirectory, AppDirectory.Path());
					ApplicationDirectory.SetTo(AppDirectory.Path());
				}
			}
		}  
        BFile ImageFile;
        BPath ImagePath;

        if(myPath[0] == '/')
                ImageFile.SetTo( myPath.String(), B_READ_ONLY);
        else
			// App directory.
			if(ApplicationDirectory != "")
			{
				if( ImagePath.SetTo(ApplicationDirectory.String(), myPath.String()) == B_OK)
					ImageFile.SetTo( ImagePath.Path(), B_READ_ONLY);
			}

		if( ImageFile.InitCheck() != B_OK)
                ImageFile.SetTo( myPath.String(), B_READ_ONLY);

        if( ImageFile.InitCheck() == B_OK)
		{
        	BTranslatorRoster *Roster = BTranslatorRoster::Default();
        	if( Roster)
			{
        		BBitmapStream Stream;
        		if( Roster->Translate( &ImageFile, NULL, NULL, &Stream, B_TRANSLATOR_BITMAP) == B_OK)
        			Stream.DetachBitmap( &fBitmap);
			delete Roster;
			}
		}
	}
	else if( ! fString.FindFirst("__Mime__=") )
	{
		BString myPath;
		fString.CopyInto(myPath, 9, fString.Length()-9);
		fBitmap = new BBitmap(BRect(0, 0, 15,15), B_CMAP8);
		BMimeType mime(myPath.String());
		mime.GetIcon(fBitmap, B_MINI_ICON);
	}
	else if( ! fString.FindFirst("__Path__=") )
	{
		BString myPath;
		fString.CopyInto(myPath, 9, fString.Length()-9);
#ifdef ZETA
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		BEntry fEntry = BEntry(myPath.String());
		BBitmap icon = &GetTrackerIcon(fEntry, 16);
		*fBitmap = icon;
#else
		fBitmap = new BBitmap(BRect(0, 0,31, 31), B_RGBA32);
		BNode *fNode = new BNode(myPath.String());
		BNodeInfo fInfo(fNode);
		int i;
		i=32;
		icon_size ics;
		ics=(icon_size)i;
	
		fInfo.GetTrackerIcon( fBitmap, ics );
		//fInfo.GetTrackerIcon(fBitmap, B_MINI_ICON);
		delete fNode;
#endif
	}
	
	
	else if( ! fString.FindFirst("__SmIC__=") )
	{
		BString myPath;
		fString.CopyInto(myPath, 9, fString.Length()-9);
#ifdef ZETA
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		BEntry fEntry = BEntry(myPath.String());
		BBitmap icon = &GetTrackerIcon(fEntry, 16);
		*fBitmap = icon;
#else
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		BNode *fNode = new BNode(myPath.String());
		BNodeInfo fInfo(fNode);
		int i;
		i=16;
		icon_size ics;
		ics=(icon_size)i;
	
		fInfo.GetTrackerIcon( fBitmap, ics );
		//fInfo.GetTrackerIcon(fBitmap, B_MINI_ICON);
		delete fNode;
#endif
	}
} 


const char* BYabField::String() const
{
	return fString.String();
}


void BYabField::SetWidth(float width)
{
	fWidth = width;
}

float BYabField::Width()
{
	return fWidth;
}


void BYabField::SetClippedString(const char* val)
{
	fClippedString = val;
} 


const char* BYabField::ClippedString()
{
	return fClippedString.String();
}

const BBitmap* BYabField::Bitmap()
{
	return fBitmap;
}

void BYabField::SetBitmap(BBitmap* bitmap)
{
	fBitmap = bitmap;
}

bool BYabField::HasBitmap()
{
	if(fBitmap) return true;
	return false;
}


//=====================================================================

BYabColumn::BYabColumn(const char* title, float width, float minWidth,
							 float maxWidth, uint32 truncate, alignment align)
	:BTitledColumn(title, width, minWidth, maxWidth, align),
	fTruncate(truncate)
{
}


void BYabColumn::DrawField(BField* _field, BRect rect, BView* parent)
{
	if(!((BYabField*)_field)->HasBitmap())
	{
		float			width = rect.Width() - (2 * kTEXT_MARGIN);
		BYabField*	field = static_cast<BYabField*>(_field);

		if (width != field->Width())
		{
			BString		out_string(field->String());

			parent->TruncateString(&out_string, fTruncate, width + 2);
			field->SetClippedString(out_string.String());
			field->SetWidth(width);
		}
		DrawString(field->ClippedString(), parent, rect);
	}
	else
	{
		BYabField *bitmapField = static_cast<BYabField *>(_field);
		const BBitmap *bitmap = bitmapField->Bitmap();

		if (bitmap != NULL)
		{
			float	x = 0.0;
			float	y;
			BRect	r = bitmap->Bounds();
	
			y = rect.top + ((rect.Height() - r.Height()) / 2);
	
			switch (Alignment())
			{
				case B_ALIGN_LEFT:
					x = rect.left + kIMG_MARGIN;
					break;
	
				case B_ALIGN_CENTER:
					x = rect.left + ((rect.Width() - r.Width()) / 2);
					break;
	
				case B_ALIGN_RIGHT:
					x = rect.right - kIMG_MARGIN - r.Width();
					break;
			}
			parent->SetDrawingMode(B_OP_ALPHA);
			parent->DrawBitmap(bitmap, BPoint(x, y));
			parent->SetDrawingMode(B_OP_OVER);
		}
	}
}


int BYabColumn::CompareFields(BField* field1, BField* field2)
{
	return(ICompare(((BYabField*)field1)->String(),
				   (((BYabField*)field2)->String())));
}


bool 
BYabColumn::AcceptsField(const BField *field) const
{
	return static_cast<bool>(dynamic_cast<const BYabField*>(field));
}

