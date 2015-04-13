#include <Input.h>
#include "YabText.h"

const uint32 YABTEXT_ANALYSE		= 'YTan';
const uint32 YABTEXT_FILECHANGED	= 'YTfc';
const uint32 YABTEXT_PARSE_LINE		= 'YTpl';
const uint32 YABTEXT_UNDO_HIGHLIGHTING	= 'YTuh';

const rgb_color Blue = {0,0,255,255};
const rgb_color Red = {255,0,0,255};
const rgb_color Grey = {185,185,185,255};
const rgb_color Green = {0,200,000,255};
const rgb_color Magenta = {200,0,255,255};

YabText::YabText(BRect frame, const char* name, BRect textRect, uint32 resizeMode, uint32 flags)
	: BTextView(frame, name, textRect, resizeMode, flags)
{
	isCaseSensitive = false;
	hasChanged = false;

	// Standard definitions
	bgcolor.blue = bgcolor.red = bgcolor.green = bgcolor.alpha = 255; // background
	textcolor.blue = textcolor.red = textcolor.green = 0; textcolor.alpha = 255; 

	generic_cmd_color = Blue;
	format_cmd_color = Red;
	special_cmd_color = Green;
	comment_color = Grey;
	punc_symbol_color = Magenta;

	SetStylable(true);
	// BFont myFont(be_fixed_font);
	// myFontSize = 12;
	// f = myFont;
	// f.SetSize(myFontSize);
	// SetFontAndColor(0,1,&f,B_FONT_ALL,&textcolor);

	SearchOffset = 0;
	SetDoesUndo(true);

	hasAutoCompletion = true;
	words = new BList(0);
	number_of_letters = 3;
	isJapanese = false;
}

YabText::~YabText()
{
	BString *anItem;
	for ( int32 i = 0; (anItem = (BString*)words->ItemAt(i)); i++ )
		delete anItem;
	delete words; 
}

void YabText::AddWord(BString *word)
{
	words->AddItem((void*)word);
}

void YabText::HasAutoCompletion(bool flag)
{
	hasAutoCompletion = flag;
}

void YabText::SetAutoCompleteStart(int num)
{
	number_of_letters = num;
}

void YabText::AddCommand(const char* command, int colorGroup)
{
	switch(colorGroup)
	{
		case 0: generic_matches.push_back(command);
			break;
		case 1: green_matches.push_back(command);
			break;
		case 2: purple_matches.push_back(command);
			break;
		case 3: comment_matches.push_back(command);
			break;
		case 4: punctuation.push_back(command[0]);
			break;
		default:
			break;
	}
}

void YabText::SetColors(int id, int r, int g, int b)
{
	switch(id)
	{
		case 0: generic_cmd_color.red = r;
			generic_cmd_color.green = g;
			generic_cmd_color.blue = b;
			ParseAll(0,TextLength()-1,true);	
			break;
		case 1: format_cmd_color.red = r;
			format_cmd_color.green = g;
			format_cmd_color.blue = b;
			ParseAll(0,TextLength()-1,true);	
			break;
		case 2: special_cmd_color.red = r;
			special_cmd_color.green = g;
			special_cmd_color.blue = b;
			ParseAll(0,TextLength()-1,true);	
			break;
		case 3: comment_color.red = r;
			comment_color.green = g;
			comment_color.blue = b;
			ParseAll(0,TextLength()-1,true);	
			break;
		case 4: punc_symbol_color.red = r;
			punc_symbol_color.green = g;
			punc_symbol_color.blue = b;
			ParseAll(0,TextLength()-1,true);	
			break;
		case 5: SetViewColor(r,g,b);
			Invalidate();
			break;
		case 6: {
				textcolor.red = r;
				textcolor.green = g;
				textcolor.blue = b;
				BFont default_font;
				GetFontAndColor(0, &default_font);
				// BFont default_font(be_fixed_font);
				// default_font.SetSize(myFontSize);
				SetFontAndColor(0,TextLength()-1,&default_font,B_FONT_ALL,&textcolor);		
				ParseAll(0,TextLength()-1,true);	
			}
			break;
		default:
			break;
	}
}

void YabText::AttachedToWindow()
{
	SetViewColor(bgcolor);
	SetColorSpace(B_RGB32);
	BTextView::AttachedToWindow();
}

void YabText::Select(int32 start,int32 finish)
{
	BTextView::Select(start,finish);
}

int32 YabText::CountPhysicalLines()
{
	return BTextView::CountLines();
}

void YabText::KeyDown(const char* bytes, int32 numBytes)
{
	isAutoComplete = false;
	bool shouldBeChanged = true;
	bool passon = true;
	switch(bytes[0])
	{
		case B_ENTER:
		{
			//update previous line on enter
			BMessage msg(YABTEXT_PARSE_LINE);
			int32 start,finish;
			GetSelection(&start,&finish);
			if(msg.AddInt32("start",start) == B_OK && msg.AddInt32("finish",finish) == B_OK)
			{
				BMessenger msgr(this);
				msgr.SendMessage(&msg);
			}
		}
		break;
		case B_LEFT_ARROW:
		{
			shouldBeChanged = false;
			if(modifiers() & B_CONTROL_KEY)
			{
				passon = false;
				int32 startoffset, endoffset;
				GetSelection(&startoffset, &endoffset);
				bool inloop = true;
				while(inloop)
				{
					startoffset--;
					if(startoffset < 0) 
					{
						if(modifiers() & B_SHIFT_KEY)
							Select(0,endoffset);
						else
							Select(0,0);
						ScrollToSelection();
						inloop = false;
					}
					else
					{
						char tmp = ByteAt(startoffset);

						if(tmp == ' ' || tmp == ':' ||  tmp == '/' || tmp == '\n' || tmp == '.' || tmp == '(' || tmp == ')' || tmp == '"' || tmp == '\t' || tmp == '-' || tmp == '+' || tmp == '*' || tmp == '^' || tmp == ',' || tmp == ';' || tmp == '=' || tmp == '\r')
						{
							if(modifiers() & B_SHIFT_KEY)
								Select(startoffset,endoffset);
							else
								Select(startoffset,startoffset);
							ScrollToSelection();
							inloop = false;
						}
					}

				}

			}
		}
		break;
		case B_RIGHT_ARROW:
		{
			shouldBeChanged = false;
			int cur = CurrentLine();
			if(modifiers() & B_CONTROL_KEY)
			{
				// passon = false;
				int32 startoffset, endoffset;
				GetSelection(&startoffset, &endoffset);
				bool inloop = true;
				while(inloop)
				{
					endoffset++;
					if(endoffset > TextLength() ) 
					{
						if(modifiers() & B_SHIFT_KEY)
							Select(startoffset,endoffset);
						else
							Select(endoffset, endoffset);
						ScrollToSelection();
						inloop = false;
					}
					else
					{
						char tmp = ByteAt(endoffset);
						int a = LineAt(endoffset);

						if(tmp == ' ' || tmp == ':' ||  tmp == '/' || tmp == '\n' || tmp == '.' || tmp == '(' || tmp == ')' || tmp == '"' || tmp == '\t' || tmp == '-' || tmp == '+' || tmp == '*' || tmp == '^' || tmp == ',' || tmp == ';' || tmp == '=' || tmp == '\r' || a!=cur)
						{
							if(a!=cur) endoffset --;
							if(modifiers() & B_SHIFT_KEY)
								Select(startoffset,endoffset);
							else
								Select(endoffset,endoffset);
							ScrollToSelection();
							inloop = false;
						}
					}

				}

			}
		}
		break;
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
		case B_HOME:
		case B_END:
		case B_INSERT:
		case B_FUNCTION_KEY:
		case B_ESCAPE:
			shouldBeChanged = false;
			break;
		case B_BACKSPACE:
			{
				int32 a,b;
				GetSelection(&a, &b);
				if(a == b && a == 0) shouldBeChanged = false;
			}
			break;
		case B_DELETE:
			{
				int32 a,b;
				GetSelection(&a, &b);
				if(a == b && a == TextLength()) shouldBeChanged = false;
			}
			break;
	}

	if(shouldBeChanged && !hasChanged) hasChanged = true;

	if(passon) BTextView::KeyDown(bytes,numBytes);

	if(isAutoComplete) 
	{
		Select(autoOffset, autoEnd);
	}
}

int YabText::FindFirstOnLine(char c,int offset,int eol)
{
	for(int i=offset;i<eol;i++)
	{
		if(ByteAt(i) == c)
			return i;
	}
	return -1;
}

int32 YabText::OffsetAtIndex(int32 index) //const
{
	//int text_length = TextLength();
	int i;
	//if(index <= 0)
	//	return 0;
	int count = 0;
	int last=0;
	for(i=0;count < index;i++)
	{
		if(ByteAt(i) == '\n')
		{
			count++;
			last = i+1;
		}
	}
	return last;	
}

int32 YabText::LineAt(int32 offset) //const
{
	std::vector<int> sols;
	std::vector<int> eols;
	FillSolEol(sols,eols,0,TextLength()-1);

	for(int i=0;i<sols.size();i++)
	{
		if(offset >= sols[i] && offset <= eols[i])
			return i;
	}
	return -1;
}

void YabText::FillSolEol(std::vector<int>& s,std::vector<int>& e,int start,int finish)
{
	int i=start;
	int text_length = TextLength();
	for(i=start;i>=0;i--)
	{
		if(ByteAt(i) == '\n')
		{
			break;
		}
	}
	start=i+1;
	
	i = finish;	
	for(i=finish;i<text_length;i++)
	{
		if(ByteAt(i) == '\n')
		{
			break;
		}
	}
	finish=i;
	
	for(i=start;i<=finish;i++)
	{
		if(ByteAt(i) == '\n')
		{
			e.push_back(i);
		}
	}
	
	e.push_back(i);
	
	s.push_back(start);
	for(i=0;i<e.size()-1;i++)
	{
		s.push_back(e[i]+1);
	}
	//s.push_back(e[i]);
}

/*
void YabText::GoToLine(int32 index)
{
	if(TextLength()<=0)
		return;
	if(index<=0) index = 0;
	if(index > CountLines()) index = CountLines();
	
//	if(index < 0 || index > CountLines() || TextLength() <= 0)
//		return;
		
	std::vector<int> eols;
	std::vector<int> sols;
	
	FillSolEol(sols,eols,0,TextLength()-1);
	Select(sols[index],eols[index]);
}*/

int32 YabText::CountLines()
{
	std::vector<int> eols;
	std::vector<int> sols;
	FillSolEol(sols,eols,0,TextLength()-1);	
	return eols.size();
}

void YabText::ParseAll(int start,int finish,bool IsInteractive)
{
	// BFont font(be_fixed_font);
	// font.SetSize(myFontSize);
	BFont default_font;
	GetFontAndColor(0, &default_font);
	
	int text_length = TextLength();
	if(text_length > 0)
	{
		std::vector<int> eols;
		std::vector<int> sols;
		FillSolEol(sols,eols,start,finish);
		
		int i;
		int size;
	/*	
		if(!IsInteractive)
		{
			size = text_length;
			std::vector<rgb_color> colorVec(size,textcolor);

			for(i=0;i<sols.size();i++)
			{
				ParseLine(sols[i],eols[i],colorVec);		
			}
			
			int offset_size=1;
			std::vector<int> offsets;
			offsets.push_back(0);
			
			for(i=1;i<size;i++)
			{
				if(colorVec[i-1].blue != colorVec[i].blue ||
				   colorVec[i-1].green != colorVec[i].green ||
				   colorVec[i-1].red != colorVec[i].red)
				{
					offsets.push_back(i);	
					offset_size++;
				}
			}
			text_run_array* tra = (text_run_array*)malloc(sizeof(text_run_array)*(size));
			tra->count = offset_size;
			for(i=0;i<offset_size;i++)
			{
				tra->runs[i].color=colorVec[offsets[i]];
				tra->runs[i].font=font;
				tra->runs[i].offset=offsets[i];
			}
			SetRunArray(0,text_length-1,tra);
		}		
		else
		{*/
			for(i=0;i<sols.size();i++)
			{
				IParseLine(sols[i],eols[i]);		
			}
	}
}

void YabText::IParseLine(int sol,int eol)
{
	int size = eol-sol+1;
	std::vector<rgb_color> colorVec(size,textcolor);
		
	for(int k=0;k<size;k++)
		colorVec[k] = textcolor;

	int i,j;
	int pos;
	int text_length = TextLength();

	// punctuation first
	for(i=sol;i<eol;i++)
	{
		for(int j=0; j<punctuation.size(); j++)
			if(ByteAt(i) == punctuation[j])
				colorVec[i-sol] = punc_symbol_color;
	}

	// words second
	ICheckWordLists(sol,eol,colorVec);

/*
	for(i=sol;i<eol;i++)
	{
		BString word("");
		for(j=i; j<eol; j++)
			word << (char)ByteAt(j);
			if(isIgnoring)
			{
				for(int k=i;k<j;k++)
					colorVec[k-sol] = comment_color;
			}
			else
			{
				printf("%s\n", word.String());
				if(Contains(comment_matches,word))
				{
					printf("\t%s\n", word.String());
					isIgnoring = true;
					for(int k=i;k<j;k++)
						colorVec[k-sol] = comment_color;
				}
			}
	}
*/
/*
	if(FindFirstOnLine('#',sol,eol) == 0)
		for(i=0;i<eol;i++)
			colorVec[i-sol] = comment_color;
			*/
		
	//START COLOURING***********************************
	
	// BFont default_font(be_fixed_font);
	// default_font.SetSize(myFontSize);

	BFont default_font;
	GetFontAndColor(0, &default_font);

// INS HERE

	int plLength=0;
	int plStart=0;
	for(i=sol;i<eol;i++)
	{
		if(i == sol)
		{
			plLength = 1;
			plStart = i;
		}
		else if(colorVec[i-sol-1].blue != colorVec[i-sol].blue ||
			colorVec[i-sol-1].green != colorVec[i-sol].green ||
			colorVec[i-sol-1].red != colorVec[i-sol].red)
		{
			if(!isJapanese) SetFontAndColor(plStart,plStart+plLength,&default_font,B_FONT_ALL,&colorVec[i-sol-1]);		
			plLength = 1;
			plStart = i;
		}
		else
		{
			plLength++;
		}	
	}
				
	if(plLength > 0)
		if(!isJapanese) SetFontAndColor(plStart,plStart+plLength,&default_font,B_FONT_ALL,&colorVec[i-sol-1]);	
}

void YabText::ParseLine(int sol,int eol,std::vector<rgb_color>& colorVec)//,std::vector<rgb_color>& colorVec)
{
	int i;
	int offset = sol;
	int pos;
	int text_length = TextLength();
	//assert(sol >=0 && eol >=0 && sol < text_length && eol < text_length);
	//Setup some defaults....
	/*
	TwoColorPlateau('\'',sol,eol,comment_color,colorVec);//,displaced);//-displaced
	TwoColorPlateau('`',sol,eol,comment_color,colorVec);//,displaced);
	TwoColorPlateau('\\',sol,eol,comment_color,colorVec);//,displaced);*/

	for(i=sol;i<eol;i++)
	{
		if(ByteAt(i) == '[' || ByteAt(i) == ']')
		{
			if(i-1 >= 0 && ByteAt(i-1) == '\\')
			{
				colorVec[i-1] = punc_symbol_color;
			}
				
			colorVec[i] = punc_symbol_color;			
			
		}
		else if(ByteAt(i) == '&' || ByteAt(i) == '{' || ByteAt(i) == '}')//
		{
			if(i-1 >= 0 && ByteAt(i-1) == '\\')
			{
				colorVec[i-1] = punc_symbol_color;
			}
				
			colorVec[i] = punc_symbol_color;			
			
		}
		else if(ByteAt(i) == '$')
		{
			if(i-1 >= 0 && ByteAt(i-1) == '\\')
			{
				colorVec[i-1] = textcolor;
				colorVec[i] = textcolor;
			}
		}
		else if(ByteAt(i) == '\\' && i+1 < eol)
		{
			if(ByteAt(i+1) == '#')
			{	
				colorVec[i] = punc_symbol_color;
				colorVec[i+1] = punc_symbol_color;
			}else if(ByteAt(i+1) == '\'' || ByteAt(i+1) == '`')
			{
				colorVec[i] = textcolor;
				colorVec[i+1] = textcolor;
			}
		}
		/*if(toupper((char)ByteAt(i)) == 'B')
		{
			if(i+3 < eol && toupper((char)ByteAt(i+1)) == 'E' && 
			toupper((char)ByteAt(i+2)) == 'O' && toupper((char)ByteAt(i+3)) == 'S')
			{
				 colorVec[i] = Blue;
				 colorVec[i+1] = Red;
			}
			else if(i+4 < eol && toupper((char)ByteAt(i+1)) == 'E' && 
			toupper((char)ByteAt(i+2)) == 'T' && toupper((char)ByteAt(i+3)) == 'E'
			&& toupper((char)ByteAt(i+3)) == 'X')
			{
				 colorVec[i] = Blue;
				 colorVec[i+1] = Red;				
			}
		}*/	
	}
	offset = sol;
	while((pos = FindFirstOnLine('%',offset,eol))>= 0 && offset < eol)
	{
		
		if(pos - 1 >= 0 && ByteAt(pos-1) == '\\')
		{
				colorVec[pos-1] = punc_symbol_color;
				colorVec[pos] = punc_symbol_color;
		}
		else 
		{
			for(i=pos;i<eol;i++)
				colorVec[i] = comment_color;
			break;
		}
		offset= pos+1;
	}	
}

void YabText::ICheckWordLists(int sol,int eol,std::vector<rgb_color>& colorVec)
{
	int i;
	for(i=sol;i<eol;i++)
	{
		BString match="";
				
		int j=i;
		for(j=i;j < eol;j++)
		{
			if(isalpha(ByteAt(j)) || (char)ByteAt(j) == ':' || (char)ByteAt(j) == '$' || ((char)ByteAt(j)>='0' && (char)ByteAt(j)<='9')) 
			// if(ByteAt(j)>32)
			{
				match << (char)ByteAt(j);
			}
			else
				break;
		}
		if((match.Length() > 0) && (i==sol || !isalpha(ByteAt(i-1))))
		{			
			if(Contains(green_matches,match))
			{
				for(int k=i;k<j;k++)
					colorVec[k-sol] = format_cmd_color;
			}
			else if(Contains(purple_matches,match))
			{
				for(int k=i;k<j;k++)
					colorVec[k-sol] = special_cmd_color;
			}
			else if(Contains(generic_matches,match))
			{
				for(int k=i;k<j;k++)
					colorVec[k-sol] = generic_cmd_color;
			}
			else if(Contains(comment_matches,match))
			{
				for(int k=i;k<j;k++)
					colorVec[k-sol] = comment_color;
			}
		}
	}
}

void YabText::InsertText(const char* text,int32 length,int32 offset,const text_run_array* runner)
{
	hasChanged = true;
        BString replace = text;
        if(length == 1 && hasAutoCompletion)
        {
                BString itext(""); //Text();
                // BString lastWord = "";
                int32 myOffset = offset - 1;
                while(myOffset>=0)
                {
                        if(ByteAt(myOffset) == ' ' || ByteAt(myOffset) == '\n' || ByteAt(myOffset) == '\t')
                                break;
                        myOffset --;
                }

                if(offset-myOffset>number_of_letters)
                {
                        for(int i=myOffset+1; i<offset; i++)
                                itext << (char)ByteAt(i);

                        itext << text;

                        BString *anItem;
                        for ( int32 i = 0; (anItem = (BString*)words->ItemAt(i)); i++ )
                        {
                                if(anItem->Compare(itext, offset-myOffset+length-1) == 0)
                                {
                                        autoOffset = offset + 1;
                                        isAutoComplete = true;
                                        BString sleepy(anItem->String());
                                        sleepy.CopyInto(replace, offset-myOffset-1, anItem->Length());
                                        length = replace.Length();
                                        autoEnd = anItem->Length()+myOffset+1;
                                        break;
                                }

                        }
                }  
	}
	BTextView::InsertText(replace.String(),length,offset,NULL);

	if(text[0] != B_ENTER)
	{
		BMessage msg(YABTEXT_ANALYSE);
		if(msg.AddInt32("offset",offset)==B_OK && msg.AddInt32("length",length)==B_OK)
		//&& msg.AddString("text",text) == B_OK)
		{
			BMessenger msgr(this);
			msgr.SendMessage(&msg);
		}
	}
}

void YabText::DeleteText(int32 start, int32 finish)
{
	BMessage msg(YABTEXT_ANALYSE);
	if(msg.AddInt32("start",start)==B_OK && msg.AddInt32("finish",finish)==B_OK)
	{
		BMessenger msgr(this);
		msgr.SendMessage(&msg);
	}
	BTextView::DeleteText(start,finish);

}

void YabText::SetText(const char* text,int32 length,const text_run_array* runs )
{
	hasChanged = true;
	BTextView::SetText(text,length,runs);
	// ParseAll(0,length-1,false);
}

void YabText::UpdateColors()
{
	SetFontAndColor(0,TextLength(),&f,B_FONT_ALL,&textcolor);
	ParseAll(0,TextLength()-1,true);	
	//const char* text = Text();
	//Delete(0,TextLength()-1);
	//SetText(text,strlen(text));//,TextLength()-1);
}

void YabText::UpdateFontSize()
{
	f.SetSize(myFontSize);
	SetFontAndColor(0,TextLength(),&f,B_FONT_ALL,&textcolor);
	ParseAll(0,TextLength()-1,true);	
}

void YabText::SetText(BFile* file,int32 offset,int32 length,const text_run_array* runs )
{
	hasChanged = true;
	BTextView::SetText(file,offset,length,runs);
	// ParseAll(offset,length-1,false);
}

bool YabText::Contains(std::vector<BString>& v,BString s)
{
	for(int i=0;i<v.size();i++)
	{
		if(isCaseSensitive)
		{
			if(v[i].Compare(s) == 0)
				return true;
		}
		else
		{
			if(v[i].ICompare(s) == 0)
				return true;
		}
	}
	return false;
}

/*
bool YabText::CanEndLine(int32 offset)
{
	if(ByteAt(offset) == B_ENTER || )
		return true;
	else
		return false;
}*/

void YabText::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_INPUT_METHOD_EVENT:
		{
			int32 be_op;
			if(msg->FindInt32("be:opcode", &be_op) == B_OK)
			{
				if(be_op == B_INPUT_METHOD_STARTED) isJapanese = true;
				if(be_op == B_INPUT_METHOD_STOPPED) isJapanese = false;
			}
			BTextView::MessageReceived(msg);
		}
		break;
		case YABTEXT_UNDO_HIGHLIGHTING:
		{
			int32 start,finish;
			if(msg->FindInt32("start",&start)==B_OK && msg->FindInt32("finish",&finish)==B_OK)
			{			
				Select(start,finish);
			}
		}break;
		
		case YABTEXT_PARSE_LINE:
		{
			int32 start,finish;
			if(msg->FindInt32("start",&start)==B_OK && msg->FindInt32("finish",&finish)==B_OK)
			{	
				int32 sel_start,sel_finish;
				GetSelection(&sel_start,&sel_finish);
				ParseAll(min(sel_start,start),max(sel_finish,finish),true);
			}	
		}break;
		case YABTEXT_ANALYSE:
		{		
			int32 start,finish;
			if(msg->FindInt32("start",&start)==B_OK && msg->FindInt32("finish",&finish)==B_OK)
			{			
				ParseAll(start,finish,true);
			}
			
			int32 offset,length;
			if(msg->FindInt32("offset",&offset)==B_OK
			&& msg->FindInt32("length",&length)==B_OK)
			{
				GetSelection(&start,&finish);
				ParseAll(offset,finish,true);			
			}

		}break;
	/*	
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		{
			BMessage msg(UPDATE_CLIPBOARD_MENU_STATUS);
			BMessenger msgr(Window());
			msgr.SendMessage(&msg);
		}*/
		default:
			BTextView::MessageReceived(msg);
			
	}
}

/*
void YabText::LoadFile (entry_ref *ref)
{
	if (ref == NULL) {
		return;
	}
	
	BFile file(ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		return;
	}
	
	off_t length;
	file.GetSize(&length);
	if (length == 0) {
		return;
	}
	
	SetText (&file, 0, length);
}*/

bool YabText::HasChanged()
{
	return hasChanged;
}

void YabText::SetChanged(bool changed)
{
	hasChanged = changed;
}

bool YabText::IsCaseSensitive()
{
	return isCaseSensitive;
}

void YabText::SetCaseSensitive(bool caseSensitive)
{
	isCaseSensitive = caseSensitive;
}
