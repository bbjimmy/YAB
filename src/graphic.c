/*  

    YABASIC ---  a simple Basic Interpreter
    written by Marc-Oliver Ihm 1995-2004
    homepage: www.yabasic.de
    
    graphic.c --- code for windowed graphics, printing and plotting
    
    This file is part of yabasic and may be copied only 
    under the terms of either the Artistic License or 
    the GNU General Public License (GPL), both of which 
    can be found at www.yabasic.de

*/


/* ------------- includes ---------------- */

//include "YabInterface.h"

#include "global.h"

#ifndef YABASIC_INCLUDED
#include "yabasic.h"       /* all prototypes and structures */
#endif
#ifdef UNIX
#ifdef BUILD_NCURSES
#include <curses.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef KEY_MAX
#define KEY_MAX 0777
#endif
#endif


/* ------------- global variables ---------------- */

char translationbuffer[8192];

/* mouse and keyboard */
int mousex=0,mousey=0,mouseb=0,mousemod=0; /* last know mouse coordinates */
char *ykey[kLASTKEY+1]; /* keys returned by inkey */

int winopened = 0;

/* ------------- functions ---------------- */

void create_openwin(int fnt) /* create Command 'openwin' */
{
  struct command *cmd;
  
  cmd=add_command(cOPENWIN,FALSE);
  cmd->args=fnt;
}


void openwin(struct command *cmd, YabInterface* yab) /* open a Window */
{
	double x1,y1,x2,y2;
	char *id, *title;
	title = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2=pop(stNUMBER)->value;
	x2=pop(stNUMBER)->value; 
	y1=pop(stNUMBER)->value;
	x1=pop(stNUMBER)->value; 
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_OpenWindow(x1,y1,x2,y2, id, title, yab);

	winopened++;
}

void closewin(struct command *cmd, YabInterface* yab) /* close the window */
{
	char *id;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	if (yi_CloseWindow(id, yab) == 1)
		winopened--;
}

int numwindows()
{
	return winopened;
}
    
void createbutton(struct command *cmd, YabInterface* yab) /* set a button */
{
        double x1,y1,x2,y2;
        char *id, *title, *view;

        view = pop(stSTRING)->pointer;
        title = pop(stSTRING)->pointer;
        id = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
        yi_CreateButton(x1,y1,x2,y2, id, title, view, yab);   
}

void createmenu(struct command *cmd, YabInterface* yab) /* add a menu */
{
	char *menuhead, *menuitem, *shortcut, *window;
	window = pop(stSTRING)->pointer;
	shortcut = pop(stSTRING)->pointer;
	menuitem = pop(stSTRING)->pointer;
	menuhead = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateMenu(menuhead, menuitem, shortcut, window, yab);
}

void createcheckbox(struct command *cmd, YabInterface* yab) /* add a checkbox */
{
	char *label, *window, *id;
	double x,y,isActivated;
	
	window = pop(stSTRING)->pointer;
        isActivated = pop(stNUMBER)->value;
	label = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
        y = pop(stNUMBER)->value;
        x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateCheckBox(x,y,id, label,isActivated,window, yab);
}

void createradiobutton(struct command *cmd, YabInterface* yab) /* add a radio button*/
{
	char *label, *groupid, *window;
	double x,y,isActivated;
	
        
	window = pop(stSTRING)->pointer;
        isActivated = pop(stNUMBER)->value;
	label = pop(stSTRING)->pointer;
	groupid = pop(stSTRING)->pointer;
        y = pop(stNUMBER)->value;
        x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateRadioButton(x,y,groupid,label,isActivated,window, yab);
}

void createtextcontrol(struct command *cmd, YabInterface* yab) /* add a textcontrol */
{
	char *text, *label, *window, *id;
	double x1,y1,x2,y2;
        
	window = pop(stSTRING)->pointer;
	text = pop(stSTRING)->pointer;
	label = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateTextControl(x1,y1,x2,y2,id, label, text, window, yab);
}

void createlistbox(struct command *cmd, YabInterface* yab) /* list box */
{
	char *title, *window;
	double x1,y1,x2,y2,scrollbar;
        
	window = pop(stSTRING)->pointer;
	scrollbar = pop(stNUMBER)->value;
	title = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateListBox(x1,y1,x2,y2, title, scrollbar, window, yab);
}

void createdropbox(struct command *cmd, YabInterface* yab) /* drop box */
{
	char *title, *label, *window;
	double x1,y1,x2,y2;
        
	window = pop(stSTRING)->pointer;
	label = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateDropBox(x1,y1,x2,y2, title, label, window, yab);
}

void createitem(struct command *cmd, YabInterface* yab) /* item add */
{
	char *item, *view;
        
	item = pop(stSTRING)->pointer;
	view = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateItem(view,item,yab);
}

void removeitem(struct command *cmd, YabInterface* yab) /* item add */
{
	char *title, *item;
        
	item = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_RemoveItem(title,item,yab);
}

void clearitems(struct command *cmd, YabInterface* yab) /* clear item list*/
{
	char *title;
        
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ClearItems(title,yab);
}

int createimage(double x, double y, const char* imagefile, const char* window, YabInterface* yab, int line, const char* libname) /* set an image */
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_CreateImage(x,y, imagefile, window, yab);
}

int createimage2(double x1, double y1, double x2, double y2, const char* imagefile, const char* window, YabInterface* yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_CreateImage2(x1,y1,x2,y2, imagefile, window, yab);
}

int createsvg(double x1, double y1, double x2, double y2, const char* imagefile, const char* window, YabInterface* yab, int line, const char* libname)
{
 
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_CreateSVG(x1,y1,x2,y2, imagefile, window, yab);
}

void mouseset(struct command *cmd, YabInterface* yab) 
{
        char *opt;
        opt = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_MouseSet(opt, yab);
}

void drawtext(struct command *cmd, YabInterface* yab) /* draw text */
{
        double x,y;
        char *text, *window;

        window = pop(stSTRING)->pointer;
        text = pop(stSTRING)->pointer;
        y=pop(stNUMBER)->value;
        x=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawText(x,y,text, window, yab);
}

void drawrect(struct command *cmd, YabInterface* yab) /* draw rect */
{
        double x1,y1,x2,y2;
        char *window;

        window = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawRect(x1,y1,x2,y2,window, yab);
}

void drawdot(struct command *cmd, YabInterface* yab) /* draw dot */
{
        double x1,y1;
        char *window;

        window = pop(stSTRING)->pointer;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawDot(x1,y1,window, yab);
}

void drawline(struct command *cmd, YabInterface* yab) /* draw line */
{
        double x1,y1,x2,y2;
        char *window;

        window = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawLine(x1,y1,x2,y2,window, yab);
}

void drawcircle(struct command *cmd, YabInterface* yab) /* draw circle */
{
        double x1,y1,r;
        char *window;

        window = pop(stSTRING)->pointer;
        r=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawCircle(x1,y1,r,window, yab);
}

void drawellipse(struct command *cmd, YabInterface* yab) /* draw ellipse */
{
        double x1,y1,r1,r2;
        char *window;

        window = pop(stSTRING)->pointer;
        r2=pop(stNUMBER)->value;
        r1=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawEllipse(x1,y1,r1,r2,window, yab);
}

void drawcurve(struct command *cmd, YabInterface* yab) /* draw a curve */
{
        double x1,y1,x2,y2,x3,y3,x4,y4;
        char *window;

        window = pop(stSTRING)->pointer;
        y4=pop(stNUMBER)->value;
        x4=pop(stNUMBER)->value;
        y3=pop(stNUMBER)->value;
        x3=pop(stNUMBER)->value;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawCurve(x1,y1,x2,y2,x3,y3,x4,y4,window, yab);
}

void drawclear(struct command *cmd, YabInterface* yab) /* clear canvas */
{
	char *window;


        window = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawClear(window, yab);
}

void drawset1(struct command *cmd, YabInterface *yab)
{
	char *option, *window;

        window = pop(stSTRING)->pointer;
        option = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawSet1(option,window,yab);
}

void drawset2(struct command *cmd, YabInterface *yab)
{
	char *pattern;
	int mode;

        pattern = pop(stSTRING)->pointer;
	mode = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawSet2(mode,pattern,yab);
}

void drawset3(struct command *cmd, YabInterface *yab)
{
	char *option;
	int transparency;

	transparency = pop(stNUMBER)->value;
        option = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawSet3(option, transparency,yab);
}

void createtext(struct command *cmd, YabInterface* yab) /* text */
{
        double x,y;
        char *text, *window, *id;


        window = pop(stSTRING)->pointer;
        text = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
        y=pop(stNUMBER)->value;
        x=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CreateText(x,y,id,text, window, yab);
}

void text2(struct command *cmd, YabInterface* yab) /* text */
{
        double x1,y1,x2,y2;
        char *text, *window, *id;


        window = pop(stSTRING)->pointer;
        text = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
        y2=pop(stNUMBER)->value;
        x2=pop(stNUMBER)->value;
        y1=pop(stNUMBER)->value;
        x1=pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Text2(x1,y1,x2,y2,id,text, window, yab);
}

void textalign(struct command *cmd, YabInterface* yab) /* align text */
{
        char *option, *id;


        option = pop(stSTRING)->pointer;
        id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextAlign(id, option, yab);
}

void createalert(struct command *cmd, YabInterface* yab) /* open an alert window (one button only)*/
{
	char *text, *button1, *type;
        
        type = pop(stSTRING)->pointer;
        button1 = pop(stSTRING)->pointer;
        text = pop(stSTRING)->pointer;
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
        yi_CreateAlert(text,button1,type, yab);   
}

void setlayout(struct command *cmd, YabInterface *yab)
{
        char *text, *window;
	
 
        window = pop(stSTRING)->pointer;
        text = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetLayout(text, window, yab);
}

void winset1(struct command *cmd, YabInterface *yab)
{
	char *option, *value, *window;
	

        value = pop(stSTRING)->pointer;
        option = pop(stSTRING)->pointer;
        window = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_WindowSet1(option,value,window,yab);
}

void winset2(struct command *cmd, YabInterface *yab)
{
	char *option, *window;
	int r, g, b;
	
        window = pop(stSTRING)->pointer;
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
        option = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_WindowSet2(option,r,g,b,window,yab);
}

void winset3(struct command *cmd, YabInterface *yab)
{
	char *option, *window;
	double x,y;
	
        
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;
        option = pop(stSTRING)->pointer;
	window = pop(stSTRING)->pointer;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_WindowSet3(option,x,y,window,yab);
}

void winset4(struct command *cmd, YabInterface *yab)
{
	char *option, *window;
	
        
        option = pop(stSTRING)->pointer;
	window = pop(stSTRING)->pointer;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_WindowSet4(option,window,yab);
}

void winclear(struct command *cmd, YabInterface *yab)
{
	char *window;
	

	window = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_WindowClear(window,yab);
}

char* gettranslation(const char* text, YabInterface* yab, int line, const char* libname) /* get translation string */
{
	yi_SetCurrentLineNumber(line, libname, yab);
	yi_Translate((char*)text, translationbuffer);
	return my_strdup((char*) translationbuffer);
}

char* getmenutranslation(const char* text, YabInterface* yab, int line, const char* libname) /* get menu translation string */
{
	yi_SetCurrentLineNumber(line, libname, yab);
	yi_MenuTranslate((char*)text, translationbuffer);
	return my_strdup((char*) translationbuffer);
}

void localize()
{
	yi_SetLocalize();
}

void localizestop()
{
	yi_StopLocalize();
}

void localize2(struct command *cmd, YabInterface *yab) 
{
	char *path;

	path = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetLocalize2(path, yab);
}

char* getloadfilepanel(const char* mode, const char* title, const char* dir,  YabInterface *yab, int line, const char* libname) /* load panel */
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_LoadFilePanel(mode, title, dir,yab));
}

char* getsavefilepanel(const char* mode, const char* title, const char* dir, const char* filename, YabInterface *yab, int line, const char* libname) /* save panel */
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_SaveFilePanel(mode, title, dir, filename, yab));
}

void textedit(struct command *cmd, YabInterface *yab) 
{
	char *window, *title;
	double x1,y1,x2,y2;
	int scrollbar;
	

	window = pop(stSTRING)->pointer;
	scrollbar = pop(stNUMBER)->value;
	title = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextEdit(x1,y1,x2,y2,title,scrollbar,window,yab);
}

void textadd(struct command *cmd, YabInterface *yab) 
{
	char *title, *text;
	
	
	text = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextAdd(title,text,yab);
}

void textset(struct command *cmd, YabInterface *yab) 
{
	char *title, *option;
	
	
	option = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextSet(title,option,yab);
}

void textset2(struct command *cmd, YabInterface *yab) 
{
	char *title, *option;
	int value;
	

	value = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextSet2(title,option,value,yab);
}

void textset3(struct command *cmd, YabInterface *yab) 
{
	char *title, *option, *option2;
	
	
	option2 = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextSet3(title,option,option2,yab);
}

void textcolor1(struct command *cmd, YabInterface *yab) 
{
	char *title, *option, *command;
	
	
	command = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextColor1(title,option,command,yab);
}

void textcolor2(struct command *cmd, YabInterface *yab) 
{
	char *title, *option;
	int r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextColor2(title,option,r,g,b,yab);
}

void textclear(struct command *cmd, YabInterface *yab) 
{
	char *title;
	
	
	title = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextClear(title,yab);
}

char* textget(const char* title, YabInterface *yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_TextGet(title,yab));
}

int textget2(const char* title, const char* option, YabInterface *yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TextGet2(title,option,yab);
}

char* textget3(const char* title, int linenum, YabInterface *yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return (char*)yi_TextGet3(title,linenum,yab);
}

double textget4(const char* title, const char* option, int linenum, YabInterface *yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TextGet4(title,option,linenum,yab);
}

int textget5(const char* title, const char* option, const char* option2, YabInterface *yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TextGet5(title,option,option2,yab);
}

char* textget6(const char* title, const char* option, YabInterface *yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return (char*)yi_TextGet6(title,option,yab);
}

void view(struct command *cmd, YabInterface *yab) 
{
	char *id, *view;
	double x1, y1, x2, y2;
	
	
	view = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_View(x1,y1,x2,y2, id, view, yab);
}

void boxview(struct command *cmd, YabInterface *yab) 
{
	char *id, *view, *text;
	double x1, y1, x2, y2;
	int linetype;
	
	
	view = pop(stSTRING)->pointer;
	linetype = pop(stNUMBER)->value;
	text = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BoxView(x1,y1,x2,y2, id, text, linetype, view, yab);
}

void boxviewset(struct command *cmd, YabInterface *yab) 
{
	char *id, *value, *option;
	
	
	
	value= pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BoxViewSet(id, option, value, yab);
}


	
void tab(struct command *cmd, YabInterface *yab) 
{
	char *id, *names, *view;
	double x1, y1, x2, y2;
	
	
	view = pop(stSTRING)->pointer;
	names = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Tab(x1,y1,x2,y2, id, names, view, yab);
}

void tabset(struct command *cmd, YabInterface *yab) 
{
	char *tabname;
	int num;
	
	
	num = pop(stNUMBER)->value;
	tabname = pop(stSTRING)->pointer;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TabSet(tabname, num, yab);
}

void tabadd(struct command *cmd, YabInterface *yab) 
{
	char *id, *tabname;
	
	
	tabname = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TabAdd(id, tabname, yab);
}

void tabdel(struct command *cmd, YabInterface *yab) 
{
	char *id;
	int num;
	
	
	num = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TabDel(id, num, yab);
}

void slider1(struct command *cmd, YabInterface *yab)
{
	char *view, *id, *title;
	double x1,y1,x2,y2,min,max;
	
	
	view = pop(stSTRING)->pointer;
	max = pop(stNUMBER)->value;
	min = pop(stNUMBER)->value;
	title = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Slider1(x1,y1,x2,y2,id,title,min,max,view,yab);
}


void slider2(struct command *cmd, YabInterface *yab)
{
	char *view, *id, *title, *option;
	double x1,y1,x2,y2,min,max;
	
	
	view = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	max = pop(stNUMBER)->value;
	min = pop(stNUMBER)->value;
	title = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Slider2(x1,y1,x2,y2,id,title,min,max,option,view,yab);
}

void slider3(struct command *cmd, YabInterface *yab)
{
	char *id, *label1, *label2;
	
	
	label2 = pop(stSTRING)->pointer;
	label1 = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetSlider1(id,label1,label2,yab);
}


void slider4(struct command *cmd, YabInterface *yab)
{
	char *id, *bottomtop;
	int count;
	
	
	count = pop(stNUMBER)->value;
	bottomtop = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetSlider2(id,bottomtop,count,yab);
}

void slider5(struct command *cmd, YabInterface *yab)
{
	char *id, *part;
	int r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	part = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetSlider3(id,part,r,g,b,yab);
}

void slider6(struct command *cmd, YabInterface *yab)
{
	char *id;
	int value;
	

	value = pop(stNUMBER)->value;	
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetSlider4(id,value,yab);
}

void option1(struct command *cmd, YabInterface *yab)
{
	char *id, *option, *value;
	
	
	value = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetOption1(id,option,value,yab);
}

void option2(struct command *cmd, YabInterface *yab)
{
	char *id, *part;
	int r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	part = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetOption2(id,part,r,g,b,yab);
}

void option3(struct command *cmd, YabInterface *yab)
{
	char *id, *option;
	double x,y;
	
	
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetOption3(id,option,x,y,yab);
}

void option4(struct command *cmd, YabInterface *yab)
{
	char *id, *option;
	
	
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetOption4(id,option,yab);
}

void option5(struct command *cmd, YabInterface *yab)
{
	char *id, *option;
	int value;
	
	
	value = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SetOption5(id,option,value,yab);
}

void dropzone(struct command *cmd, YabInterface *yab)
{
	char *view;
	
	
	view = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DropZone(view,yab);
}

void colorcontrol1(struct command *cmd, YabInterface *yab)
{
	char *view, *id;
	double x,y;
	
	
	view = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ColorControl1(x,y,id,view,yab);
}

void colorcontrol2(struct command *cmd, YabInterface *yab)
{
	char *id;
	double r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ColorControl2(id,r,g,b,yab);
}

void textcontrol2(struct command *cmd, YabInterface *yab)
{
	char *id, *text;
	
	
	text = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextControl2(id,text,yab);
}

void textcontrol3(struct command *cmd, YabInterface *yab)
{
	char *id;	
	int mode;
	mode = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextControl3(id,mode,yab);
}

void textcontrol5(struct command *cmd, YabInterface *yab)
{
	char *id;
	
	
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextControl5(id,yab);
}


void textcontrol4(struct command *cmd, YabInterface *yab)
{
	char *id, *option, *value;
	
	
	value = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextControl4(id,option,value,yab);
}




void treebox1(struct command *cmd, YabInterface *yab)
{
	char *id, *view;
	double x1,y1,x2,y2;
	int scrollbarType;
	
	
	view = pop(stSTRING)->pointer;
	scrollbarType = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox1(x1,y1,x2,y2,id,scrollbarType,view,yab);
}

void treebox2(struct command *cmd, YabInterface *yab)
{
	char *id, *item;
	
	
	item = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox2(id,item,yab);
}

void treebox3(struct command *cmd, YabInterface *yab)
{
	char *id, *head, *item;
	int isExpanded;
	
	
	isExpanded = pop(stNUMBER)->value;
	item = pop(stSTRING)->pointer;
	head = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox3(id,head,item,isExpanded,yab);
}

void treebox4(struct command *cmd, YabInterface *yab)
{
	char *id;
	
	
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox4(id,yab);
}

void treebox5(struct command *cmd, YabInterface *yab)
{
	char *id,*item;
	
	
	item = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox5(id,item,yab);
}

void treebox7(struct command *cmd, YabInterface *yab)
{
	char *id;
	int pos;
	
	
	pos = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox7(id,pos,yab);
}

void treebox8(struct command *cmd, YabInterface *yab)
{
	char *id;
	int pos;
	
	
	pos = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox8(id,pos,yab);
}

void treebox9(struct command *cmd, YabInterface *yab)
{
	char *id, *head, *item;
	
	
	item = pop(stSTRING)->pointer;
	head = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox9(id,head,item,yab);
}

void treebox10(struct command *cmd, YabInterface *yab)
{
	char *id, *head;
	
	
	head = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox10(id,head,yab);
}

void treebox11(struct command *cmd, YabInterface *yab)
{
	char *id, *head;
	
	
	head = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox11(id,head,yab);
}

void treebox12(struct command *cmd, YabInterface *yab)
{
	char *id, *item;
	int pos;
	

	pos = pop(stNUMBER)->value;
	item = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeBox12(id,item,pos,yab);
}

void buttonimage(struct command *cmd, YabInterface *yab)
{
	char *id, *enabledon, *enabledoff, *disabled, *view;
	double x,y;
	
	
	view = pop(stSTRING)->pointer;
	disabled = pop(stSTRING)->pointer;
	enabledoff = pop(stSTRING)->pointer;
	enabledon = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ButtonImage(x,y,id,enabledon, enabledoff, disabled, view,yab);
}

void checkboximage(struct command *cmd, YabInterface *yab)
{
	char *id, *enabledon, *enabledoff, *disabledon, *disabledoff, *view;
	double x,y;
	int isActivated;
	
	
	view = pop(stSTRING)->pointer;
	isActivated = pop(stNUMBER)->value;
	disabledoff = pop(stSTRING)->pointer;
	disabledon = pop(stSTRING)->pointer;
	enabledoff = pop(stSTRING)->pointer;
	enabledon = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CheckboxImage(x,y,id,enabledon, enabledoff, disabledon, disabledoff, isActivated, view,yab);
}

void checkboxset(struct command *cmd, YabInterface *yab)
{
	char *id;
	int isActivated;
	
	
	isActivated = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_CheckboxSet(id,isActivated,yab);
}

void radioset(struct command *cmd, YabInterface *yab)
{
	char *id;
	int isActivated;
	
	
	isActivated = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_RadioSet(id,isActivated,yab);
}

char* textcontrolget(const char* textcontrol, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return  my_strdup((char*)yi_TextControlGet(textcontrol,yab));
}

void tooltip(struct command *cmd, YabInterface *yab)
{
	char *view,*text;
	
	
	text = pop(stSTRING)->pointer;
	view = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ToolTip(view,text,yab);
}
void tooltipnew(struct command *cmd, YabInterface *yab)
{
	char *view, *text, *color;
	int r,g,b;
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	
	color = pop(stSTRING)->pointer;
	text = pop(stSTRING)->pointer;
	view = pop(stSTRING)->pointer;
	
	
		
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ToolTipNew(view,text,color,r,g,b,yab);	
}
void tooltipcolor(struct command *cmd, YabInterface *yab)
{
	char *color;
	int r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	color = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ToolTipColor(color,r,g,b,yab);
}

void listsort(struct command *cmd, YabInterface *yab)
{
	char *id;
	
	
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ListSort(id,yab);
}

void treesort(struct command *cmd, YabInterface *yab)
{
	char *id;
	
	
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TreeSort(id,yab);
}

void filebox(struct command *cmd, YabInterface *yab)
{
	char *id,*view, *option;
	int scrollbar;
	double x1,y1,x2,y2;
	
	
	view = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	scrollbar = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_FileBox(x1,y1,x2,y2,id,scrollbar,option,view,yab);
}

void fileboxadd2(struct command *cmd, YabInterface *yab)
{
	char *columnbox, *name, *option;
	double width, maxWidth, minWidth;
	int pos;
	
	
	option = pop(stSTRING)->pointer;
	width = pop(stNUMBER)->value;
	minWidth = pop(stNUMBER)->value;
	maxWidth = pop(stNUMBER)->value;
	pos = pop(stNUMBER)->value;
	name = pop(stSTRING)->pointer;
	columnbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_FileBoxAdd2(columnbox, name, pos, maxWidth, minWidth, width, option, yab);
}

void fileboxclear(struct command *cmd, YabInterface *yab)
{
	char *id;
	
	
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_FileBoxClear(id,yab);
}

void columnboxadd(struct command *cmd, YabInterface *yab)
{
	char *id, *item ;
	int position, height, column;
	
	
	item = pop(stSTRING)->pointer;
	height = pop(stNUMBER)->value;
	position = pop(stNUMBER)->value;
	column = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ColumnBoxAdd(id,column,position,height,item,yab);
}

void columnboxremove(struct command *cmd, YabInterface *yab)
{
	char *columnbox;
	int pos;
	
	
	pos = pop(stNUMBER)->value;
	columnbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ColumnBoxRemove(columnbox,pos,yab);
}

void columnboxselect(struct command *cmd, YabInterface *yab)
{
	char *columnbox;
	int pos;
	
	
	pos = pop(stNUMBER)->value;
	columnbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ColumnBoxSelect(columnbox,pos,yab);
}

void columnboxcolor(struct command *cmd, YabInterface *yab)
{
	char *columnbox, *option;
	int r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	columnbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ColumnBoxColor(columnbox,option,r,g,b,yab);
}

void dropboxselect(struct command *cmd, YabInterface *yab)
{
	char *dropbox;
	int num;
	
	
	num = pop(stNUMBER)->value;
	dropbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DropBoxSelect(dropbox,num,yab);
}

void menu2(struct command *cmd, YabInterface *yab)
{
	char *menuhead, *view;
	int radio;
	

	view = pop(stSTRING)->pointer;
	radio = pop(stNUMBER)->value;
	menuhead = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Menu2(menuhead,radio,view,yab);
}

void submenu1(struct command *cmd, YabInterface *yab)
{
	char *menuhead, *menuitem, *submenuitem, *modifier, *view;
	
	
	view = pop(stSTRING)->pointer;
	modifier = pop(stSTRING)->pointer;
	submenuitem = pop(stSTRING)->pointer;
	menuitem = pop(stSTRING)->pointer;
	menuhead = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SubMenu1(menuhead,menuitem,submenuitem,modifier,view,yab);
}

void submenu2(struct command *cmd, YabInterface *yab)
{
	char *menuhead, *menuitem, *view;
	int radio;
	
	
	view = pop(stSTRING)->pointer;
	radio = pop(stNUMBER)->value;
	menuitem = pop(stSTRING)->pointer;
	menuhead = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SubMenu2(menuhead,menuitem,radio,view,yab);
}

void submenu3(struct command *cmd, YabInterface *yab)
{
	char *menuhead, *menuitem, *submenuitem, *option, *view;
	
	
	view = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	submenuitem = pop(stSTRING)->pointer;
	menuitem = pop(stSTRING)->pointer;
	menuhead = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SubMenu3(menuhead,menuitem,submenuitem,option,view,yab);
}

void menu3(struct command *cmd, YabInterface *yab)
{
	char *menuhead, *menuitem, *option, *view;
	
	
	view = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	menuitem = pop(stSTRING)->pointer;
	menuhead = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Menu3(menuhead,menuitem,option,view,yab);
}

int sliderget(const char *slider, YabInterface *yab, int line, const char* libname)
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_SliderGet(slider, yab);
}

int colorcontrolget(const char* colorcontrol, const char* option, YabInterface *yab, int line, const char* libname)
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ColorControlGet(colorcontrol, option, yab);
}

void spincontrol1(struct command *cmd, YabInterface *yab)
{
	char *id, *view, *label;
	int min, max, step;
	double x,y;
	
	
	view = pop(stSTRING)->pointer;
	step = pop(stNUMBER)->value;
	max = pop(stNUMBER)->value;
	min = pop(stNUMBER)->value;
	label = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SpinControl1(x,y,id,label,min,max,step,view,yab);
}

void spincontrol2(struct command *cmd, YabInterface *yab)
{
	char *spincontrol;
	int value;
	
	
	value = pop(stNUMBER)->value;
	spincontrol = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SpinControl2(spincontrol,value,yab);
}

int spincontrolget(const char* view, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_SpinControlGet(view,yab);
}

char* popupmenu(double x, double y, const char* menuitems, const char* view, YabInterface *yab, int line, const char* libname)
{
	
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_PopUpMenu(x,y,menuitems,view,yab));
}

void dropboxremove(struct command *cmd, YabInterface *yab)
{
	char *dropbox;
	int value;
	
	
	value = pop(stNUMBER)->value;
	dropbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DropBoxRemove(dropbox,value,yab);
}

void dropboxclear(struct command *cmd, YabInterface *yab)
{
	char *dropbox;
	
	
	dropbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DropBoxClear(dropbox,yab);
}

int dropboxcount(const char* id, YabInterface *yab, int line, const char* libname)
{
	
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_DropBoxCount(id,yab);
}

char* dropboxget(const char* id, int position, YabInterface *yab, int line, const char* libname)
{
	
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_DropBoxGet(id,position,yab));
}

void splitview1(struct command *cmd, YabInterface *yab)
{
	char *id, *view;
	double x1,y1,x2,y2;
	int isVertical, NormalStyle;
	
	
	view = pop(stSTRING)->pointer;
	NormalStyle = pop(stNUMBER)->value;
	isVertical = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SplitView1(x1,y1,x2,y2, id, isVertical, NormalStyle, view, yab);
}

void splitview2(struct command *cmd, YabInterface *yab)
{
	char *id, *option;
	double value;
	
	
	value = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SplitView2(id, option, value, yab);
}

void splitview3(struct command *cmd, YabInterface *yab)
{
	char *id, *option;
	double left, right;
	
	
	right = pop(stNUMBER)->value;
	left = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SplitView3(id, option, left, right, yab);
}

double splitviewget(const char* id, const char* option, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_SplitViewGet(id, option, yab);
}

void stackview1(struct command *cmd, YabInterface *yab)
{
	char *id, *view;
	double x1,y1,x2,y2;
	int number;
	
	
	view = pop(stSTRING)->pointer;
	number = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_StackView1(x1,y1,x2,y2, id, number, view, yab);
}

void stackview2(struct command *cmd, YabInterface *yab)
{
	char *id;
	int num;
	
	
	num = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_StackView2(id, num, yab);
}

int stackviewget(const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_StackViewGet(id, yab);
}

int tabviewget(const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TabViewGet(id, yab);
}

void calendar1(struct command *cmd, YabInterface *yab)
{
	char *format, *id, *date, *view;
	double x,y;
	
	
	view = pop(stSTRING)->pointer;
	date = pop(stSTRING)->pointer;
	format = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Calendar1(x,y, id, format, date, view, yab);
}

char* calendar2(const char* id, YabInterface *yab, int line, const char* libname)
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_Calendar2(id, yab));
}

void calendar3(struct command *cmd, YabInterface *yab)
{
	char *id, *date;
	
	
	date = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Calendar3(id, date, yab);
}

void scrollbar(struct command *cmd, YabInterface *yab)
{
	char *view, *id;
	int format;
	
	view = pop(stSTRING)->pointer;
	format = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Scrollbar(id, format, view, yab);
}

void scrollbarset1(struct command *cmd, YabInterface *yab)
{
	char *scrollbar, *option;
	double position;
	

	position = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	scrollbar = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ScrollbarSet1(scrollbar, option, position, yab);
}

void scrollbarset2(struct command *cmd, YabInterface *yab)
{
	char *scrollbar, *option;
	double a,b;
	

	b = pop(stNUMBER)->value;
	a = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	scrollbar = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ScrollbarSet2(scrollbar, option, a, b, yab);
}

void scrollbarset3(struct command *cmd, YabInterface *yab)
{
	char *scrollbar, *option;
	double a,b;
	

	option = pop(stSTRING)->pointer;
	scrollbar = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ScrollbarSet3(scrollbar, option, yab);
}

void texturl1(struct command *cmd, YabInterface *yab)
{
	char *id, *text, *url, *view;
	double x,y;
	
	
	view = pop(stSTRING)->pointer;
	url = pop(stSTRING)->pointer;
	text = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextURL1(x,y, id, text, url, view, yab);
}

void texturl2(struct command *cmd, YabInterface *yab)
{
	char *id, *color;
	int r,g,b;
	
	
	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	color = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_TextURL2(id, color,r,g,b,yab);
}

double scrollbarget(const char* scrollbar, const char* option, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ScrollbarGet(scrollbar, option, yab);
}

void clipboardcopy(struct command *cmd, YabInterface *yab)
{
	char *text;
	text = pop(stSTRING)->pointer;
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ClipboardCopy(text,yab);
}

int printer(const char* docname, const char *view, const char* config, YabInterface *yab,int line, const char* libname) 
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_Printer(docname, view,config,yab);
}

void printerconfig(struct command *cmd, YabInterface *yab)
{
	char *config, *docname;

	config = pop(stSTRING)->pointer;
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_PrinterConfig(config,yab);
}

void listboxadd1(struct command *cmd, YabInterface *yab)
{
	char *listbox, *item;


	item = pop(stSTRING)->pointer;
	listbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ListboxAdd1(listbox, item, yab);
}

void listboxadd2(struct command *cmd, YabInterface *yab)
{
	char *listbox, *item;
	int position;


	item = pop(stSTRING)->pointer;
	position = pop(stNUMBER)->value;
	listbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ListboxAdd2(listbox, position, item, yab);
}

void listboxselect(struct command *cmd, YabInterface *yab)
{
	char *listbox;
	int position;


	position = pop(stNUMBER)->value;
	listbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ListboxSelect(listbox, position, yab);
}

void listboxremove(struct command *cmd, YabInterface *yab)
{
	char *listbox;
	int position;


	position = pop(stNUMBER)->value;
	listbox = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ListboxRemove(listbox, position, yab);
}

char* clipboardpaste(YabInterface* yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_ClipboardPaste(yab));
}

char* keyboardmessages(const char* view, YabInterface* yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_KeyboardMessages(view,yab));
}

char* columnboxget(const char* columnbox, int column, int position, YabInterface* yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_ColumnBoxGet(columnbox,column,position,yab));
}

int columnboxcount(const char* columnbox, YabInterface* yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ColumnBoxCount(columnbox, yab);
}

int windowgetnum(const char* view, const char *option, YabInterface* yab, int line, const char* libname) 
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_WindowGet(view,option,yab);
}

int viewgetnum(const char* view, const char *option, YabInterface* yab, int line, const char* libname) //vasper
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ViewGet(view,option,yab);
}

double drawget1(const char* option, const char* txt, const char* view, YabInterface* yab, int line, const char* libname)
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_DrawGet1(option, txt, view, yab);
}

double drawget2(const char* option, const char* view, YabInterface* yab, int line, const char* libname)
{

	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_DrawGet2(option, view, yab);
}

char* drawget3(const char* option, YabInterface* yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_DrawGet3(option, yab));
}

int drawget4(double x, double y, const char* rgb, const char* view, YabInterface* yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_DrawGet4(x,y,rgb, view, yab);
}

int newalert(const char* text, const char* button1, const char* button2, const char* button3, const char* option, YabInterface* yab, int line, const char* libname) 
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_NewAlert(text,button1,button2,button3,option,yab);
}

char* listboxget(const char* listbox, int position, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_ListboxGet(listbox,position,yab));
}

int listboxcount(const char* listbox, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ListboxCount(listbox, yab);
}

char* treeboxget(const char* treebox, int position, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_TreeboxGet(treebox,position,yab));
}

int treeboxcount(const char* treebox, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TreeboxCount(treebox, yab);
}

int ismousein(const char* view, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_IsMouseIn(view,yab);
}

char* getmousein(YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return  my_strdup((char*)yi_GetMouseIn(yab));	
}

char* getmousemessages(const char* view, YabInterface* yab, int line, const char* libname) /* get a mouse message string */
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return my_strdup((char*)yi_GetMouseMessages(view, yab));
}

char* getmessages(YabInterface* yab, int line, const char* libname) /* get message string */
{
	// char tmp[1024]; 
	//strcpy(tmp, yi_CheckMessages(yab));
	yi_SetCurrentLineNumber(line, libname, yab);
	return  my_strdup((char*)yi_CheckMessages(yab));
}

int messagesend(const char* app, const char *msg, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_MessageSend(app, msg, yab);
}

int threadkill(const char* option, int id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ThreadKill(option, id, yab);
}

int threadget(const char* option, const char* appname, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ThreadGet(option, appname, yab);
}

void bitmap(struct command *cmd, YabInterface *yab)
{
	char *id;
	double w,h;

	id = pop(stSTRING)->pointer;
	h = pop(stNUMBER)->value;
	w = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Bitmap(w, h, id, yab);
}

void bitmapdraw(struct command *cmd, YabInterface *yab)
{
	char *id, *mask, *view;
	double x,y;

	view = pop(stSTRING)->pointer;
	mask = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y = pop(stNUMBER)->value;
	x = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapDraw(x,y, id, mask, view, yab);
}

void bitmapdraw2(struct command *cmd, YabInterface *yab)
{
	char *id, *mask, *view;
	double x1,y1,x2,y2;

	view = pop(stSTRING)->pointer;
	mask = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapDraw2(x1,y1,x2,y2, id, mask, view, yab);
}

void bitmapget(struct command *cmd, YabInterface *yab)
{
	char *id, *bitmap;
	double x1,y1,x2,y2;

	bitmap = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapGet(x1,y1, x2,y2, id, bitmap, yab);
}

void bitmapget2(struct command *cmd, YabInterface *yab)
{
	char *id, *path;
	double w;

	path = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	w = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapGet2(w, id, path, yab);
}

void bitmapgeticon(struct command *cmd, YabInterface *yab)
{
	char *id, *option, *path;

	path = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapGetIcon(id, option, path, yab);
}

void bitmapdrag(struct command *cmd, YabInterface *yab)
{
	char *id;

	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapDrag(id, yab);
}

void screenshot(struct command *cmd, YabInterface *yab)
{
	char *bitmap;
	double x1,y1,x2,y2;

	bitmap = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Screenshot(x1,y1,x2,y2, bitmap, yab);
}

void statusbar(struct command *cmd, YabInterface *yab)
{
	char *view, *label2, *label1, *id;
	double x1, y1, x2, y2;

	view = pop(stSTRING)->pointer;
	label2 = pop(stSTRING)->pointer;
	label1 = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_StatusBar(x1,y1,x2,y2, id, label1, label2, view, yab);
}

void statusbarset(struct command *cmd, YabInterface *yab)
{
	double state;
	char *label2, *label1, *id;

	state = pop(stNUMBER)->value;
	label2 = pop(stSTRING)->pointer;
	label1 = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_StatusBarSet(id, label1, label2, state, yab);
}

void statusbarset2(struct command *cmd, YabInterface *yab)
{
	double x1, y1, x2, y2;
	char *id, *view;

	view = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_StatusBarSet2(x1,y1,x2,y2, id, view, yab);
}

void statusbarset3(struct command *cmd, YabInterface *yab)
{
	int r,g,b;
	char *id;

	b = pop(stNUMBER)->value;
	g = pop(stNUMBER)->value;
	r = pop(stNUMBER)->value;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_StatusBarSet3(id, r, g, b, yab);
}

void bitmapremove(struct command *cmd, YabInterface *yab)
{
	char *id;

	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_BitmapRemove(id, yab);
}

int bitmapsave(const char* id, const char* filename, const char* type, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_BitmapSave(id, filename, type, yab);
}

int bitmapload(const char* filename, const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_BitmapLoad(filename, id, yab);
}

int bitmapgetnum(const char* id, const char* option, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_BitmapGetNum(id, option, yab);
}

int bitmapcolor(double x, double y, const char* id, const char* option, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_BitmapColor(x,y,id,option,yab);
}

void canvas(struct command *cmd, YabInterface *yab)
{
	char *id, *view;
	double x1,y1,x2,y2;


	view = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;
	y2 = pop(stNUMBER)->value;
	x2 = pop(stNUMBER)->value;
	y1 = pop(stNUMBER)->value;
	x1 = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Canvas(x1,y1,x2,y2,id,view, yab);
}

int listboxgetnum(const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ListboxGetNum(id, yab);
}

int dropboxgetnum(const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_DropboxGetNum(id, yab);
}

int columnboxgetnum(const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_ColumnboxGetNum(id, yab);
}

int treeboxgetnum(const char* id, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TreeboxGetNum(id, yab);
}

int treeboxgetopt(const char* id, const char* option, int pos, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_TreeboxGetOpt(id, option, pos, yab);
}

void treebox13(struct command *cmd, YabInterface *yab)
{
	char *id, *option;
	int pos;

	pos = pop(stNUMBER)->value;
	option = pop(stSTRING)->pointer;
	id = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Treebox13(id,option,pos, yab);
}

void drawset4(struct command *cmd, YabInterface *yab)
{
	char *option, *color, *view;

	view = pop(stSTRING)->pointer;
	color = pop(stSTRING)->pointer;
	option = pop(stSTRING)->pointer;
	
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_DrawSet4(option, color, view, yab);
}

void launch(struct command *cmd, YabInterface *yab)
{
	char *strg;
	strg = pop(stSTRING)->pointer;
	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Launch(strg, yab);
}

int sound(const char* filename, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_Sound(filename, yab);
}

void soundstop(struct command *cmd, YabInterface *yab)
{
	int id;

	id = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SoundStop(id, yab);
}

void soundwait(struct command *cmd, YabInterface *yab)
{
	int id;

	id = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_SoundWait(id, yab);
}

int mediasound(const char* filename, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_MediaSound(filename, yab);
}

void mediasoundstop(struct command *cmd, YabInterface *yab)
{
	int id;

	id = pop(stNUMBER)->value;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_MediaSoundStop(id, yab);
}
int iscomputeron(YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_IsComputerOn(yab);
}

void attribute1(struct command *cmd, YabInterface *yab)
{
	char *type, *name, *value, *filename;

	filename = pop(stSTRING)->pointer;
	value = pop(stSTRING)->pointer;
	name = pop(stSTRING)->pointer;
	type = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_Attribute1(type, name, value, filename, yab);
}

void attributeclear(struct command *cmd, YabInterface *yab)
{
	char *name, *filename;

	filename = pop(stSTRING)->pointer;
	name = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_AttributeClear(name, filename, yab);
}


char* attributeget1(const char* name, const char* filename, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return  my_strdup((char*)yi_AttributeGet1(name, filename, yab));	
}

double attributeget2(const char* name, const char* filename, YabInterface *yab, int line, const char* libname)
{
	yi_SetCurrentLineNumber(line, libname, yab);
	return yi_AttributeGet2(name, filename, yab);	
}

void shortcut(struct command *cmd, YabInterface *yab)
{
	char *view, *key, *msg;

	msg = pop(stSTRING)->pointer;
	key = pop(stSTRING)->pointer;
	view = pop(stSTRING)->pointer;

	yi_SetCurrentLineNumber(cmd->line, (const char*)cmd->lib->s, yab);
	yi_ShortCut(view, key, msg, yab);
}

void gettermkey(char *keybuff) /* read a key from terminal */
{
#ifdef BUILD_NCURSES
  char *skey=NULL;
  int key; /* returned key */
  
  do key=getch(); while(key==ERR);
  switch(key) {
  case KEY_UP: skey=ykey[kUP];break;
  case KEY_DOWN: skey=ykey[kDOWN];break;
  case KEY_LEFT: skey=ykey[kLEFT];break;
  case KEY_RIGHT: skey=ykey[kRIGHT];break;
  case KEY_DC: skey=ykey[kDEL];break;
  case KEY_IC: skey=ykey[kINS];break;
  case KEY_IL: skey=ykey[kINS];break;
  case KEY_CLEAR: skey=ykey[kCLEAR];break;
  case KEY_HOME: skey=ykey[kHOME];break;
#ifdef KEY_END
  case KEY_END: skey=ykey[kEND];break;
#endif
  case KEY_F0: skey=ykey[kF0];break;
  case KEY_F(1): skey=ykey[kF1];break;
  case KEY_F(2): skey=ykey[kF2];break;
  case KEY_F(3): skey=ykey[kF3];break;
  case KEY_F(4): skey=ykey[kF4];break;
  case KEY_F(5): skey=ykey[kF5];break;
  case KEY_F(6): skey=ykey[kF6];break;
  case KEY_F(7): skey=ykey[kF7];break;
  case KEY_F(8): skey=ykey[kF8];break;
  case KEY_F(9): skey=ykey[kF9];break;
  case KEY_F(10): skey=ykey[kF10];break;
  case KEY_F(11): skey=ykey[kF11];break;
  case KEY_F(12): skey=ykey[kF12];break;
  case KEY_F(13): skey=ykey[kF13];break;
  case KEY_F(14): skey=ykey[kF14];break;
  case KEY_F(15): skey=ykey[kF15];break;
  case KEY_F(16): skey=ykey[kF16];break;
  case KEY_F(17): skey=ykey[kF17];break;
  case KEY_F(18): skey=ykey[kF18];break;
  case KEY_F(19): skey=ykey[kF19];break;
  case KEY_F(20): skey=ykey[kF20];break;
  case KEY_F(21): skey=ykey[kF21];break;
  case KEY_F(22): skey=ykey[kF22];break;
  case KEY_F(23): skey=ykey[kF23];break;
  case KEY_F(24): skey=ykey[kF24];break;
  case KEY_BACKSPACE: skey=ykey[kBACKSPACE];break;
  case KEY_NPAGE: skey=ykey[kSCRNDOWN];break;
  case KEY_PPAGE: skey=ykey[kSCRNUP];break;
  case KEY_ENTER: skey=ykey[kENTER];break;
  default:
    if (isprint(key)) {
      keybuff[0]=key;
      keybuff[1]='\0';
    } else if (key<0 || key>=KEY_MAX) {
      keybuff[0]='\0';
    } else {
      switch(key) {
      case 0x1b:skey=ykey[kESC];break;
      case 0x7f:skey=ykey[kDEL];break;
      case 0xa:skey=ykey[kENTER];break;
      case 0x9:skey=ykey[kTAB];break;
      default:
        sprintf(keybuff,"key%x",key);
      }
    }
    break;
  }
  if (skey) strcpy(keybuff,skey);
  snooze(20000);
#endif
}

