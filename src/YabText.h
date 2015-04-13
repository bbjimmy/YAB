#ifndef YABTEXT_H
#define YABTEXT_H

#include <TextView.h>
#include <Font.h>
#include <String.h>	
#include <vector>
#include <string>
#include <stdio.h>
#include <Window.h>
#include <Message.h>
#include <Messenger.h>

class YabText : public BTextView
{
	public:
		YabText(BRect frame, const char* name, BRect textRect, uint32 resizeMode, uint32 flags);
		~YabText();
		void AddCommand(const char* command, int colorGroup);
		virtual void AttachedToWindow();
		virtual void Select(int32 start,int32 finish);
		void SetColors(int, int, int, int);
		int32 CountPhysicalLines(); //?
		virtual void KeyDown(const char* bytes, int32 numBytes);
		int FindFirstOnLine(char c,int offset,int eol);
		int32 OffsetAtIndex(int32 index);// const;
		int32 LineAt(int32 offset);// const;	
		void FillSolEol(std::vector<int>& s,std::vector<int>& e,int start,int finish);	
		// void GoToLine(int32 index);
		int32 CountLines();
		void ParseAll(int start,int finish,bool IsInteractive);
		void IParseLine(int sol,int eol); // TODO!
		void ParseLine(int sol,int eol,std::vector<rgb_color>& colorVec); // TODO!
		void ICheckWordLists(int sol,int eol,std::vector<rgb_color>& colorVec);
		void SetText(const char* text,int32 length,const text_run_array* runs = NULL);
		void UpdateColors();
		void UpdateFontSize();
		void SetText(BFile* file,int32 offset,int32 length,const text_run_array* runs = NULL);
		bool Contains(std::vector<BString>& v,BString s);
		// virtual bool CanEndLine(int32 offset);
		// void LoadFile(entry_ref *ref);
		virtual void MessageReceived(BMessage* msg);

		bool HasChanged();
		void SetChanged(bool changed);
		bool IsCaseSensitive();
		void SetCaseSensitive(bool caseSensitive);
		
		void SetNormalFocus();
		void SetAttachedFocus();

		void AddWord(BString *word);
		void SetAutoCompleteStart(int num);
		void HasAutoCompletion(bool flag);

		int SearchOffset;

	private:
		bool isCaseSensitive, hasChanged, isJapanese;
		int myFontSize;
		rgb_color bgcolor;
		rgb_color textcolor, punc_symbol_color, comment_color, ignore_color;
		rgb_color format_cmd_color, special_cmd_color, generic_cmd_color;
		BFont f;

		std::vector <BString> green_matches;
		std::vector <BString> purple_matches;
		std::vector <BString> generic_matches;
		std::vector <BString> comment_matches;
		std::vector <char> punctuation;

		BList *words;
		bool hasAutoCompletion, isAutoComplete;
		int32 autoOffset, autoEnd;
		int number_of_letters;

		bool min(int32 a, int32 b)
		{
			return (a<=b?a:b);
		};
		bool max(int32 a, int32 b)
		{
			return (a>=b?a:b);
		};

	protected:
		virtual void InsertText(const char* text,int32 length,int32 offset,const text_run_array* runner);
		virtual void DeleteText(int32 start, int32 finish);
};
#endif
