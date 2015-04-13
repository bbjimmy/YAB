// Calendar Control version 2.5
// by Al.V. Sarikov.
// Kherson, Ukraine, 2006.
// E-mail: avix@ukrpost.net.
// Home page: http://avix.pp.ru.

// Control which allows to work with dates:
// enter date to text field and choose it from calendar.

// Distributed under BSD license (see LICENSE file).

#include <Clipboard.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <String.h>
#include <TextView.h>
#include <stdlib.h>
#include <time.h>


#define LAST_FORMAT 1 // quantity of formats - 1
#define LAST_DIVIDER 2 // quantity of dividers - 1


class DateTextView: public BTextView
{
 public:
  DateTextView(int day, int month, int year, uint32 flags, uint32 look);
  virtual void Cut(BClipboard *clip);
  virtual void KeyDown(const char *bytes, int32 numBytes);
  virtual void MakeFocus(bool focused);
  virtual void Paste(BClipboard *clip);
  virtual void SetEnabled(bool enabled);
  
  void GetDate(int *day, int *month, int *year);
  void SetDate(int day, int month, int year);
  void SetDate(const char *tdate);
  void GetYearRange(int *first_year, int *last_year);
  uint32 GetDivider();
  void SetDivider(uint32 dvder);
  uint32 GetDateFlags();
  void SetDateFlags(uint32 flags);
  
 protected:
  virtual void DeleteText(int32 start, int32 finish);
 private:
  virtual bool AcceptsDrop(const BMessage *message);
  virtual bool AcceptsPaste(BClipboard *clip);
  
  void DrawDate(int day, int month, int year);
  bool VerifyDate(int *day, int *month, int *year);
  
  bool is_ins; // is it necessar to insert symbol
  uint32 flags;
  char *div[LAST_DIVIDER+1]; // сstrings of dividers
  uint32 divider;
  
  bool enabled;
  
  int first_year;
  int last_year; // first and last year which control accepts
  
  int textlen; // length of text string (10 or 8 symbols)
  
  uint32 interface; // the same as control variable
};


//////////////////////////////////////////////////////////////////////////
DateTextView::DateTextView(int day, int month, int year,
                           uint32 flags, uint32 look)
             :BTextView(BRect(0,0,110,15),"DateTextViewAViX",
                        BRect(0,0,110,15),B_FOLLOW_LEFT | B_FOLLOW_TOP,
                        B_WILL_DRAW | B_NAVIGABLE)
{
 enabled=true;
 
 is_ins=false;
 
 interface=look;
 
 div[0]=(char*)".";
 div[1]=(char*)"/";
 div[2]=(char*)"-";
 
 divider=look & CC_ALL_DIVIDERS;
 if(divider>LAST_DIVIDER || divider<0) divider=0; // index of divider
 
 this->flags=(flags & (LAST_FORMAT | CC_SHORT_YEAR | CC_HALF_CENTURY));
 if((this->flags & (CC_SHORT_YEAR | CC_HALF_CENTURY))==CC_HALF_CENTURY)
  this->flags=this->flags^CC_HALF_CENTURY; // XOR; CC_FULL_YEAR and CC_HALF_CENTURY
                                           // at the same time may not be used
 
 first_year=1;
 last_year=9999;
 
 if((this->flags & CC_SHORT_YEAR)==CC_SHORT_YEAR)
 {
  textlen=8;
  
  // Changing first and last acceptable years
  // Working in range of century defined by year variable
  
  if(year<first_year || year>last_year)
  {
   // Range is set relative to today's year
   struct tm *dat;
   time_t tmp;
   time(&tmp);
   dat=localtime(&tmp);
   
   year=dat->tm_year+1900; // today's year
  }
  
  first_year=(year/100)*100+1; // dividing with loosing rest
  last_year=first_year+99;
  if(last_year==10000) last_year--; // full century
  
  if((this->flags & CC_HALF_CENTURY)==CC_HALF_CENTURY)
  {
   if(year<51 || year>9950)
    this->flags=this->flags^CC_HALF_CENTURY; // HALF_CENTURY may nor be set
   else
   {
    if((year%100)>50)
    {
     first_year+=50;
     last_year+=50;
    }
    else
    {
     first_year-=50;
     last_year-=50;
    }
   }
  }
 }
 else textlen=10; // length of text string
 
 SetDate(day,month,year);
 
 float width=0;
 BString s("");
 for(char i='0'; i<='9'; i++)
 {
  s<<i;
  if(StringWidth(s.String())>width) width=StringWidth(s.String());
  s.SetTo("");
 }
 
 ResizeTo(width*(textlen-2)+StringWidth("/")*2.5, LineHeight()-1);
 
 SetWordWrap(false);
 SetMaxBytes(textlen);
 SetTextRect(Bounds());
 SetDoesUndo(false);
 for(int32 i=0;i<256;i++) DisallowChar(i);
 for(int32 i='0';i<='9';i++) AllowChar(i);
}


///////////////////////////////////////////////////////
bool DateTextView::AcceptsDrop(const BMessage *message)
{
 return false;
}


/////////////////////////////////////////////////
bool DateTextView::AcceptsPaste(BClipboard *clip)
{
 return false;
}


////////////////////////////////////////
void DateTextView::Cut(BClipboard *clip)
{
 Copy(clip);
}


////////////////////////////////////////////////////////
void DateTextView::DeleteText(int32 start, int32 finish)
{
 BTextView::DeleteText(start,finish);
 if(is_ins)
 {
  if(start==2 || start==5) InsertText(div[divider],1,start,NULL);
  else InsertText("0",1,start,NULL);
 }
}


/////////////////////////////////////////////////////////////
void DateTextView::KeyDown(const char *bytes, int32 numBytes)
{
 if(!enabled) if(bytes[0]!=B_TAB) return;
 
 int32 i1,i2;
 GetSelection(&i1,&i2);
 
 if(bytes[0]>='0' && bytes[0]<='9')
 {
  if(i1>(textlen-1)) return; // not to insert after end of string
  Select(i1,i1);
  if(i1==2 || i1==5)
  {
   i1++;
   Select(i1,i1);
  }
  DeleteText(i1,i1+1);
  BTextView::KeyDown(bytes, numBytes);
 }
 else if(bytes[0]==B_DELETE)
 {
  if(i1>(textlen-1)) return; // not to insert after end of string
  Select(i1,i1);
  is_ins=true; // symbol "0" or divider will be inserted
  BTextView::KeyDown(bytes, numBytes);
  is_ins=false;
 }
 else if(bytes[0]==B_BACKSPACE)
 {
  Select(i1,i1);
  is_ins=true;
  BTextView::KeyDown(bytes, numBytes);
  is_ins=false;
 }
 else if(bytes[0]==B_TAB)
 {
  Parent()->KeyDown(bytes, numBytes);
 }
 else if(bytes[0]==B_DOWN_ARROW)
 {
  // Is Ctrl+DownArrow pressed?
  if(modifiers() & B_CONTROL_KEY)
  {
   // yes
   BMessage msg(myButtonMessage);
   Parent()->MessageReceived(&msg);
  }
  else
   BTextView::KeyDown(bytes, numBytes);
 }
 else 
  BTextView::KeyDown(bytes, numBytes);
}


///////////////////////////////////////////////
void DateTextView::MakeFocus(bool focused=true)
{
 BTextView::MakeFocus(focused);
 
 int day, month, year;
 GetDate(&day, &month, &year);
 Parent()->Draw(Parent()->Bounds());
}


//////////////////////////////////////////
void DateTextView::Paste(BClipboard *clip)
{
 return;
}


///////////////////////////////////////////
void DateTextView::SetEnabled(bool enabled)
{
 this->enabled=enabled;
 SetFlags(Flags()^B_NAVIGABLE);
 MakeEditable(enabled);
 
 BFont font;
 rgb_color color;
 GetFontAndColor((int32) 0, &font, &color);
 color.alpha=0;
 if(enabled)
 {
   SetViewColor(255,255,255,255);
  color.red=color.green=color.blue=0;
 }
 else
 {
   SetViewColor(239,239,239,255);
   color.red=color.green=color.blue=128;
 }
 
 SetFontAndColor(&font,B_FONT_ALL,&color);
 Invalidate();
}


/////////////////////////////////////////////////////////
void DateTextView::DrawDate(int day, int month, int year)
{
 // It is assumed that date is correct
 BString s;
 s.SetTo("");
 
 if(!((flags & CC_MM_DD_YYYY_FORMAT)==CC_MM_DD_YYYY_FORMAT))
 {
  if(day<10) s.Append("0");
  s<<day;
  
  s.Append(div[divider]);
  
  if(month<10) s.Append("0");
  s<<month;
 }
 else // CC_MM_DD_YYYY_FORMAT
 {
  if(month<10) s.Append("0");
  s<<month;
  
  s.Append(div[divider]);
  
  if(day<10) s.Append("0");
  s<<day;
 }
 s.Append(div[divider]);
 
 if((flags & CC_SHORT_YEAR)==CC_SHORT_YEAR)
 {
  int year1=year%100;
  if(year1<10) s.Append("0");
  s<<year1;
 }
 else // FULL_YEAR
 {
  if(year<10) s.Append("000");
  else if(year<100) s.Append("00");
  else if(year<1000) s.Append("0");
  s<<year;
 }
 
 SetText(s.String());
}


///////////////////////////////////////////////////////////
void DateTextView::GetDate(int *day, int *month, int *year)
{
 int mday=*day;
 int mmonth=*month;
 int myear=*year;
 
 BString s(Text());
 char n1[11];
 char n2[11];
 char n3[11];
 
 s.CopyInto(n1,0,2);
 n1[2]='\0';
 s.CopyInto(n2,3,2);
 n2[2]='\0';
 
 if((flags & CC_SHORT_YEAR)==CC_SHORT_YEAR)
 {
  s.CopyInto(n3,6,2);
  n3[2]='\0';
 }
 else // FULL_YEAR
 {
  s.CopyInto(n3,6,4);
  n3[4]='\0';
 }
 
 if(!((flags & CC_MM_DD_YYYY_FORMAT)==CC_MM_DD_YYYY_FORMAT))
 {
  mday=atoi(n1);
  mmonth=atoi(n2);
 }
 else
 {
  mday=atoi(n2);
  mmonth=atoi(n1);
 }
 
 myear=atoi(n3);
 if((flags & CC_SHORT_YEAR)==CC_SHORT_YEAR)
 {
  if((flags & CC_HALF_CENTURY)==CC_HALF_CENTURY) 
  {
   if(myear<51) myear+=50; else myear-=50;
  }
  else if(myear==0) myear=100;
  
  myear+=(first_year-1);
 }
 
 if(!VerifyDate(&mday,&mmonth,&myear)) SetDate(mday,mmonth,myear);
 
 *day=mday;
 *month=mmonth;
 *year=myear;
 
 return;
}


////////////////////////////////////////////////////////
void DateTextView::SetDate(int day, int month, int year)
{
 int mday=day;
 int mmonth=month;
 int myear=year;
 VerifyDate(&mday, &mmonth, &myear);
 DrawDate(mday, mmonth, myear);
}


/////////////////////////////////////////////
void DateTextView::SetDate(const char *tdate)
{
 // Almost the same as GetDate. May be to combine them.
 
 // Changes text using current settings of control.
 int day;
 int month;
 int year;
 
 int k;
 
 bool short_year=false;
 if((flags & CC_SHORT_YEAR)==CC_SHORT_YEAR) short_year=true;
 
 char n1[3]="00";
 char n2[3]="00";
 char n3[5]="0000";
 if(short_year) n3[2]='\0';
 
 bool zero=false; // was the end of tdate string?
 
 int c=0;
 while (!(c==2 || zero))
 {
  if(tdate[c]=='\0') zero=true;
  else n1[c]=tdate[c];
  c++;
 }

 if(zero) goto L1;
 if(tdate[2]=='\0') goto L1;
 
 c=0;
 while (!(c==2 || zero))
 {
  if(tdate[c+3]=='\0') zero=true;
  else n2[c]=tdate[c+3];
  c++;
 }

 if(zero) goto L1;
 if(tdate[5]=='\0') goto L1;
 
 k=short_year ? 2 : 4;
 
 c=0;
 while (!(c==k || zero))
 {
  if(tdate[c+6]=='\0') zero=true;
  else n3[c]=tdate[c+6];
  c++;
 }
 
L1:
 if(!((flags & CC_MM_DD_YYYY_FORMAT)==CC_MM_DD_YYYY_FORMAT))
 {
  day=atoi(n1);
  month=atoi(n2);
 }
 else
 {
  day=atoi(n2);
  month=atoi(n1);
 }
 
 year=atoi(n3);
 if(short_year)
 {
  if((flags & CC_HALF_CENTURY)==CC_HALF_CENTURY) 
  {
   if(year<51) year+=50; else year-=50;
  }
  else if(year==0) year=100;
  
  year+=(first_year-1);
 }
 SetDate(day,month,year);
}


//////////////////////////////////////////////////////////////
bool DateTextView::VerifyDate(int *day, int *month, int *year)
{
 // Function verifies date to be correct and changes it if it's needed
 // Returns true if date was correct (and wasn't changed)
 
 struct tm *dat;
 time_t tmp;
 time(&tmp);
 dat=localtime(&tmp);
 
 bool flag=true; // date is correct
 
 if((flags & CC_SHORT_YEAR)==CC_SHORT_YEAR)
 {
  if(*year<first_year || *year>last_year)
  {
   int year1=*year%100;
   if((flags & CC_HALF_CENTURY)==CC_HALF_CENTURY) 
   {
    if(year1<51) year1+=50; else year1-=50;
   }
   else if(year1==0) year1=100;
   
   *year=year1+first_year-1;
   flag=false;
  }
 }
 else // FULL_YEAR
 {
  if(*year<1 || *year>9999)
  {
   *year=dat->tm_year+1900;
   flag=false;
  }
 }
 
 if(*month<1 || *month>12)
 {
  *month=dat->tm_mon+1;
  flag=false;
 }
 
 if(*day<1 || *day>31)
 {
  *day=dat->tm_mday;
  flag=false;
 }
 
 if((*month==4 || *month==6 || *month==9 || *month==11) && *day>30)
 {
  if((*day=dat->tm_mday)>30) *day=30;
  flag=false;
 }
 else if (*month==2)
 {
  int tmpday;
  
  if((*year)%4==0) // leap year?
  {
   if((*year)%100==0 && (*year)%400!=0) tmpday=28; // no
   else tmpday=29; // yes
  }
  else tmpday=28;
  
  if(*day>tmpday)
  {
   if((*day=dat->tm_mday)>tmpday) *day=tmpday;
   flag=false;
  }
 }
 
 return flag;
}


////////////////////////////////////////////////////////////////
void DateTextView::GetYearRange(int *first_year, int *last_year)
{
 *first_year=this->first_year;
 *last_year=this->last_year;
}


/////////////////////////////////
uint32 DateTextView::GetDivider()
{
 return divider;
}


///////////////////////////////////////////
void DateTextView::SetDivider(uint32 dvder)
{
 if(dvder<0 || dvder>LAST_DIVIDER) dvder=0;
 
 BString s(Text());
 SetText((s.ReplaceAll(div[divider],div[dvder])).String());
 this->divider=dvder;
}


///////////////////////////////////
uint32 DateTextView::GetDateFlags()
{
 return flags;
}


///////////////////////////////////////////
void DateTextView::SetDateFlags(uint32 fmt)
{
 int mday, mmonth, myear;
 GetDate(&mday, &mmonth, &myear);
 
 // Blocking changing of parameters of year
 // (full/short year, full/half century)
 fmt=fmt & 0xFFE7;
 
 if(fmt<0 || fmt>LAST_FORMAT) fmt=0;
 
 flags=fmt | (flags & 0xFFFE); // LAST_FORMAT==1, 1 bit
 
 SetDate(mday, mmonth, myear);
}
