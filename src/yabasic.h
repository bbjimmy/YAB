/*  

    YABASIC ---  a simple Basic Interpreter
    written by Marc-Oliver Ihm 1995-2004
    homepage: www.yabasic.de
    
    yabasic.h --- function prototypes and global variables
    
    This file is part of yabasic and may be copied only 
    under the terms of either the Artistic License or 
    the GNU General Public License (GPL), both of which 
    can be found at www.yabasic.de

*/


#define YABLICENSE \
"\n"\
"  Yab may only be copied under the terms of the Artistic                 \n"\
"  License which can be found at yab-interpreter.sourceforge.net.         \n"\
"\n"\
"  The Artistic License gives you the right to use and distribute         \n"\
"  yab in a more-or-less customary fashion, plus the right to make        \n"\
"  reasonable modifications and distribute (or even sell) such a          \n"\
"  modified version under a different name.                               \n"\
"  However, the original author and copyright holder still reserves       \n"\
"  himself some sort of artistic control over the development             \n"\
"  of yab itself.                                                         \n"\



#define YABASIC_INCLUDED

/* ------------- defines ---------------- */

/*
  Define one and only one of the following symbols, depending on your
  System:
  - UNIX: uses some UNIX-features and X11
  - WINDOWS: uses WIN32-features
  Define UNIX and BEOS for a BeOS build
*/

#if defined(UNIX) && defined(WINDOWS)
UNIX and WINDOWS are defined at once; check your compiler settings
#endif


/* ------------- includes ---------------- */

#include "global.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#ifdef WINDOWS
#include <string.h>
#include <windows.h>
#include <io.h>
#define ARCHITECTURE "windows"
#ifdef __LCC__  /* fix for lccwin32 */
#include <winspool.h>
#endif
#endif

#include "YabInterface.h"


#ifdef UNIX
#define ARCHITECTURE UNIX_ARCHITECTURE
#ifdef HAS_STRING_HEADER
#include <string.h>
#elif HAS_STRINGS_HEADER
#include <strings.h>
#endif

#include <sys/time.h>
#include <sys/wait.h>

#ifndef BEOS
  #include <sys/ipc.h>
  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
  #include <X11/Intrinsic.h>
  #define XK_LATIN1
  #define XK_MISCELLANY
  #include <X11/keysymdef.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#ifdef BUILD_NCURSES
#include <ncurses.h>
// #else
// #ifdef HAVE_CURSES_HEADER
// #include <curses.h>
// #endif
#endif
#ifndef KEY_MAX
#define KEY_MAX 0777
#endif
#endif

#ifndef FOPEN_MAX
#define FOPEN_MAX 24
#endif

#include <signal.h>
#include <ctype.h>

#ifdef UNIX
#ifndef LIBRARY_PATH
#define LIBRARY_PATH "/boot/home/config/settings/yab"
#endif
#endif

#define OPEN_HAS_STREAM 1
#define OPEN_HAS_MODE 2
#define OPEN_PRINTER 8
#define STDIO_STREAM 1234

/* -------- variables needed in all files and defined in ... -------- */

/* main.c */
extern struct command *current; /* currently executed command */
extern struct command *cmdroot; /* first command */
extern struct command *cmdhead; /* next command */
extern struct command *lastcmd; /* last command */
extern int infolevel; /* controls issuing of error messages */
extern int errorlevel; /* highest level of error message seen til now */
extern int interactive; /* true, if commands come from stdin */
extern char *progname; /* name of yabasic-program */
extern char *explanation[];  /* explanations of commands */
extern char **yabargv; /* arguments for yabasic */
extern int yabargc; /* number of arguments in yabargv */
extern time_t compilation_start,compilation_end,execution_end;
extern char *string; /* for trash-strings */
extern char *errorstring; /* for error-strings */
extern int errorcode; /* error-codes */
extern char library_path[]; /* full path to search libraries */
extern int program_state;  /* state of program */
extern int check_compat; /* true, if compatibility should be checked */
extern int is_bound; /* true, if this executable is bound */
extern char* appdirectory; /* appdir */


/* io.c */
extern FILE *streams[]; /* file streams */
extern int read_controls; /* TRUE, if input should read control characters */
extern int stream_modes[]; /* modes for streams */
extern int curinized; /* true, if curses has been initialized */
extern int badstream(int,int); /* test for valid stream id */
void myseek(struct command *); /* reposition file pointer */
#ifdef WINDOWS
extern HANDLE wantkey; /* mutex to signal key desire */
extern HANDLE gotkey; /* mutex to signal key reception */
extern HANDLE wthandle; /* handle of win thread */
extern HANDLE kthandle; /* handle of inkey thread */
extern DWORD ktid; /* id of inkey thread */
extern int LINES; /* number of lines on screen */
extern int COLS; /* number of columns on screen */
extern HANDLE ConsoleInput; /* handle for console input */
extern HANDLE ConsoleOutput; /* handle for console output */
#else
extern int winpid; /* pid of process waiting for window keys */
extern int termpid; /* pid of process waiting for terminal keys */
#ifndef BUILD_NCURSES
extern int LINES; /* number of lines on screen */
extern int COLS; /* number of columns on screen */
#endif
#endif

/* graphic.c */
/* printing and plotting */
extern int print_to_file; /* print to file ? */
#ifdef WINDOWS
extern HFONT printerfont; /* handle of printer-font */
extern HDC printer; /* handle of printer */
#endif
extern FILE *printerfile; /* file to print on */
extern double xoff; /* offset for x-mapping */
extern double xinc; /* inclination of x-mapping */
extern double yoff; /* offset for y-mapping */
extern double yinc; /* inclination for y-mapping */
/* window coordinates */
extern int winopened; /* flag if window is open already */
extern char *winorigin; /* e.g. "lt","rc"; defines origin of grafic window */
extern int winwidth,winheight;  /* size of window */
/* mouse, console and keyboard */
/* extern int mousex,mousey,mouseb,mousemod; */ /* last know mouse coordinates */
extern char *ykey[]; /* keys returned by inkey */
/* text and font */
extern char *getreg(char *); /* get defaults from Registry */
extern char *text_align; /* specifies alignement of text */
extern int fontheight; /* height of font in pixel */
#ifdef WINDOWS
extern HFONT myfont; /* handle of font for screen */
#endif
/* general window stuff */
extern char *foreground;
extern char *background;
extern char *geometry;
extern char *displayname;
extern char *font;
extern int drawmode;
#ifdef WINDOWS
extern HWND window;   /* handle of my window */
extern HANDLE mainthread; /* handle to main thread */
extern HANDLE this_instance;
extern WNDCLASS myclass; /* window class for my program */
extern char *my_class;
extern BOOL Commandline; /* true if launched from command line */
#else
extern int backpid; /* pid of process waiting for redraw events */
#endif


/* function.c */
extern struct command *datapointer; /* current location for read-command */

/* symbol.c */
extern struct stackentry *stackroot; /* first element of stack */
extern struct stackentry *stackhead; /* last element of stack */
extern void query_array(struct command *cmd); /* query array */
extern struct command *lastref; /* last command in UDS referencing a symbol */
extern struct command *firstref; /* first command in UDS referencing a symbol */
extern int labelcount; /* count self-generated labels */


/* flex.c */
extern int include_stack_ptr; /* Lex buffer for any imported file */
extern struct libfile_name *libfile_stack[]; /* stack for library file names */
extern struct libfile_name *currlib; /* current libfile as relevant to bison */
extern int inlib; /* true, while in library */
extern int fi_pending; /* true, if within a short if */
extern int libfile_chain_length; /* length of libfile_chain */
extern struct libfile_name *libfile_chain[]; /* list of all library file names */


/* bison.c */
extern char *current_function; /* name of currently parsed function */
extern int yydebug;
extern int missing_endif;
extern int missing_endif_line;
extern int in_loop;

/*-------------------------- defs and undefs ------------------------*/

/* undef symbols */
#undef FATAL
#undef ERROR
#undef WARNING
#undef NOTE
#undef DEBUG
#undef DUMP

#if !defined(TRUE)
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (1!=1)
#endif

/* I've been told, that some symbols are missing under SunOs ... */
#ifndef RAND_MAX
#define RAND_MAX 32767
#endif

/* length of buffers for system() and input */
#define SYSBUFFLEN 100
#define INBUFFLEN 10000


/* ---------------------- enum types ------------------------------- */

enum error {  /* error levels  */
  FATAL,ERROR,INFO,DUMP,WARNING,NOTE,DEBUG
};

enum endreasons { /* ways to end the program */
  erNONE,erERROR,erREQUEST,erEOF
};

enum streammodes { /* ways to access a stream */
  smCLOSED=0,smREAD=1,smWRITE=2,smPRINT=4
};

enum functions { /* functions in yabasic (sorted by number of arguments) */
  fRAN2,fDATE,fTIME,fMESSAGE,fNUMWINDOWS,fCLIPBOARDPASTE,fISCOMPUTERON,fMOUSEMOVE, 
  fZEROARGS,
  fINKEY,/* fMOUSEX,fMOUSEY,fMOUSEB,fMOUSEMOD,*/
  fSIN,fASIN,fCOS,fACOS,fTAN,
  fATAN,fSYSTEM,fSYSTEM2,fPEEK,fPEEK2,fPEEK4,fTELL,fEXP,fLOG,fLEN,fSTR,
  fSQRT,fSQR,fFRAC,fABS,fSIG,fRAN,fINT,fVAL,fASC,fHEX,fBIN,fDEC,fUPPER,fLOWER,
  fLTRIM,fRTRIM,fTRIM,fCHR,fTRANSLATE,fMENUTRANSLATE,fMOUSE, fISMOUSEIN,fTEXTCONTROLGET,
  fKEYBOARD,fCOLUMNBOXCOUNT, fCALENDAR, fLISTBOXCOUNT, fTREEBOXCOUNT, fSTACKVIEWGET,
  fSPINCONTROLGET, fDROPBOXCOUNT, fSLIDERGET, fTEXTGET, fDRAWGET3, fTABVIEWGET,
  fLISTBOXGETNUM, fDROPBOXGETNUM, fCOLUMNBOXGETNUM, fTREEBOXGETNUM, fSOUND,
  fONEARGS, 
  fDEC2,fATAN2,fLEFT,fAND,fOR,fEOR,fLOG2,
  fRIGHT,fINSTR,fRINSTR,fSTR2,fMOD,fMIN,fMAX,fPEEK3,fMID2,fWINDOWGET, fVIEWGET /* vasper */,
  fLISTBOXGET, fTREEBOXGET, fSCROLLBARGET, fSPLITVIEWGET, fDROPBOXGET, fCOLORCONTROLGET,
  fTEXTGET2,fTEXTGET6,fDRAWGET2, fTEXTGET3, fMESSAGESEND, fTHREADKILL, fTHREADGET, fBITMAPGET, 
  fBITMAPLOAD, fATTRIBUTEGET1, fATTRIBUTEGET2,
  fTWOARGS,
  fMID,fINSTR2,fRINSTR2,fSTR3,fCOLUMNBOXGET,fDRAWGET1,fTEXTGET4,fTEXTGET5,fPRINTER,
  fLOAD, fTREEBOXGETOPT,fBITMAPSAVE, 
  fTHREEARGS,
  fGETCHAR,fDRAWIMAGE,fPOPUPMENU,fSAVE,fDRAWGET4,fBITMAPCOLOR,
  fFOURARGS,
  fALERT,
  fFIVEARGS,
  fDRAWSVG,fDRAWIMAGE2,
  fSIXARGS
};

enum arraymode { /* type of array access */
  CALLARRAY,ASSIGNARRAY,CALLSTRINGARRAY,ASSIGNSTRINGARRAY,GETSTRINGPOINTER
};

enum drawing_modes { /* various ways to draw */
  dmNORMAL=0,dmCLEAR=1,dmFILL=2
};

enum cmd_type { /* type of command */
  cFIRST_COMMAND, /* no command, just marks start of list */
  
  cLABEL,cSUBLINK,cGOTO,cQGOTO,cGOSUB,cQGOSUB,cRETURN,  /* flow control */
  cEND,cEXIT,cBIND,cDECIDE,cSKIPPER,cNOP,cFINDNOP,cEXCEPTION,cANDSHORT,
  cORSHORT,cSKIPONCE,cRESETSKIPONCE,cCOMPILE,cEXECUTE,cEXECUTE2,
  
  cDIM,cFUNCTION,cDOARRAY,cARRAYLINK,cPUSHARRAYREF,cCLEARREFS,   /* everything with "()" */
  cARDIM,cARSIZE,cTOKEN,cTOKEN2,cTOKENALT,cTOKENALT2,
  cSPLIT,cSPLIT2,cSPLITALT,cSPLITALT2,
  cSTARTFOR,cFORCHECK,cFORINCREMENT,              /* for for-loops */

  cSWITCH_COMPARE,cNEXT_CASE,cBREAK,cMINOR_BREAK,       /* break-continue-switch */
  cCONTINUE,cBREAK_HERE,cCONTINUE_HERE,cCONTINUE_CORRECTION,
  cBREAK_MARK,cPUSH_SWITCH_MARK,cCLEAN_SWITCH_MARK,

  cDBLADD,cDBLMIN,cDBLMUL,cDBLDIV,cDBLPOW,            /* double operations */
  cNEGATE,cPUSHDBLSYM,cPOP,cPOPDBLSYM,cPUSHDBL,

  cREQUIRE,cPUSHFREE,cMAKELOCAL,cMAKESTATIC,cNUMPARAM,     /* functions and procedures */
  cCALL,cQCALL,cPUSHSYMLIST,cPOPSYMLIST,cRET_FROM_FUN,
  cUSER_FUNCTION,cRETVAL,cEND_FUNCTION,
  cFUNCTION_OR_ARRAY,cSTRINGFUNCTION_OR_ARRAY,

  cPOKE,cPOKEFILE,cSWAP,cDUPLICATE,cDOCU,                  /* internals */
  
  cAND,cOR,cNOT,cLT,cGT,cLE,cGE,cEQ,cNE,            /* comparisons */
  cSTREQ,cSTRNE,cSTRLT,cSTRLE,cSTRGT,cSTRGE,
  
  cPUSHSTRSYM,cPOPSTRSYM,cPUSHSTR,cCONCAT,           /* string operations */
  cPUSHSTRPTR,cCHANGESTRING,cGLOB,
  
  cPRINT,cREAD,cRESTORE,cQRESTORE,cONESTRING,         /* i/o operations */
  cREADDATA,cDATA,cOPEN,cCHECKOPEN,cCHECKSEEK,cCLOSE,cPUSHSTREAM,cPOPSTREAM,
  cSEEK,cSEEK2,cTESTEOF,cWAIT,cBELL,cMOVE,
  cCLEARSCR,cCOLOUR,cCHKPROMPT,cERROR,

 /*
  cDOT,cLINE,cCIRCLE,cCLEARWIN,
  cOPENPRN,cCLOSEPRN,cMOVEORIGIN,cRECT,
  cPUTBIT, */
  
  cPUTCHAR, 

  cOPENWIN, cCLOSEWIN, cLAYOUT, cWINSET1, cWINSET2, cWINSET3, cWINSET4, /* Be Graphics */
  cBUTTON, cALERT, cMENU, cTEXTCONTROL, cCHECKBOX, cRADIOBUTTON, cWINCLEAR,
  cLISTBOX, cDROPBOX, cITEMADD, cITEMDEL, cITEMCLEAR, cLOCALIZE, cLOCALIZE2, cLOCALIZESTOP, cTEXT, cTEXT2, cTEXTALIGN,
  cTEXTEDIT, cTEXTADD, cTEXTSET, cTEXTSET2, cTEXTCOLOR1, cTEXTCOLOR2, cTEXTSET3,  cTEXTCLEAR,  
  cVIEW, cBOXVIEW, cBOXVIEWSET, cTAB, cSLIDER1, cSLIDER2, cSLIDER3, cSLIDER4, cSLIDER5, cSLIDER6,
  cOPTION1, cOPTION2, cOPTION3, cDROPZONE, cTEXTCONTROL2, cTEXTCONTROL3, cTEXTCONTROL4, cTEXTCONTROL5, 
  cCOLORCONTROL1, cCOLORCONTROL2, cTREEBOX1, cTREEBOX2, cTREEBOX3, cTREEBOX4, cTREEBOX5,
  cBUTTONIMAGE, cCHECKBOXIMAGE, cCHECKBOXSET, cRADIOSET, cTOOLTIP, cTOOLTIPCOLOR, cTREESORT,
  cLISTSORT, cFILEBOX, cFILEBOXADD2, cFILEBOXCLEAR, cCOLUMNBOXREMOVE,
  cCOLUMNBOXSELECT, cCOLUMNBOXADD, cDROPBOXSELECT, cMENU2, cSUBMENU1, cSUBMENU2, cCLIPBOARDCOPY,
  cCOLUMNBOXCOLOR, cPRINTERCONFIG, cCALENDAR, cLISTBOXSELECT, cLISTBOXADD1, cLISTBOXADD2, 
  cLISTBOXDEL2, cSCROLLBAR, cSCROLLBARSET1, cSCROLLBARSET2, cSCROLLBARSET3, cTREEBOX7, cTREEBOX8,
  cTREEBOX9, cTREEBOX10, cTREEBOX11, cSPLITVIEW1, cSPLITVIEW2, cSPLITVIEW3, 
  cSTACKVIEW1, cSTACKVIEW2, cTEXTURL1, cTEXTURL2, cDRAWSET3, cSPINCONTROL1, cTABSET, cTABDEL, cTABADD,
  cSPINCONTROL2, cDROPBOXREMOVE, cDROPBOXCLEAR, cSUBMENU3, cMENU3, cCALENDARSET,
  cDOT, cLINE, cCIRCLE,  cDRAWTEXT, cDRAWRECT, cTREEBOX12, cOPTION4, cOPTION5,
  cDRAWCLEAR, cDRAWSET1, cDRAWSET2, cELLIPSE, cCURVE,   /* Drawing */
  cBITMAP, cBITMAPDRAW, cBITMAPDRAW2, cBITMAPGET, cBITMAPGET2, cBITMAPGETICON, cBITMAPDRAG, cBITMAPREMOVE, cCANVAS, /* Bitmaps */
  cSOUNDSTOP, cSOUNDWAIT, /* Sound */
  cTREEBOX13, cDRAWSET4, cSHORTCUT, cMOUSESET,
  cSCREENSHOT, cSTATUSBAR, cSTATUSBARSET, cSTATUSBARSET2, cSTATUSBARSET3, cLAUNCH, cRESTORE2, cRESTORE3,
  cATTRIBUTE1, cATTRIBUTE2, cATTRIBUTECLEAR,
  cLAST_COMMAND /* no command, just marks end of list */
};

enum stackentries { /* different types of stackentries */
  stGOTO,stSTRING,stSTRINGARRAYREF,stNUMBER,stNUMBERARRAYREF,stLABEL,stRETADD,stRETADDCALL,stFREE,stROOT,
  stANY,stSTRING_OR_NUMBER,stSTRING_OR_NUMBER_ARRAYREF, /* these will never appear on stack but are used to check with pop */
  stSWITCH_MARK, /* used to clean up stack after switch-statement */
  stSWITCH_STRING,stSWITCH_NUMBER /* only used in switch statement, compares true to every string or number */
};

enum symbols { /* different types of symbols */
  sySTRING,syNUMBER,syFREE,syARRAY
};

enum function_type { /* different types of functions */
  ftNONE,ftNUMBER,ftSTRING
};

enum addmodes { /* different modes for adding symbols */
  amSEARCH,amSEARCH_PRE,amADD_LOCAL,amADD_GLOBAL,amSEARCH_VERY_LOCAL
};

enum states { /* current state of program */
  HATCHED,INITIALIZED,COMPILING,RUNNING,FINISHED
};

enum yabkeys { /* recognized special keys */
  kERR,kUP,kDOWN,kLEFT,kRIGHT,kDEL,kINS,kCLEAR,kHOME,kEND,
  kF0,kF1,kF2,kF3,kF4,kF5,kF6,kF7,kF8,kF9,kF10,kF11,kF12,
  kF13,kF14,kF15,kF16,kF17,kF18,kF19,kF20,kF21,kF22,kF23,kF24,
  kBACKSPACE,kSCRNDOWN,kSCRNUP,kENTER,kESC,kTAB,kLASTKEY
};

enum searchmodes { /* modes for searching labels */
  smSUB=1,smLINK=2,smLABEL=4,smGLOBAL=8
};

/* ------------- global types ---------------- */ 

struct stackentry { /* one element on stack */ 
  int type;     /* contents of entry */
  struct stackentry *next;
  struct stackentry *prev;
  void *pointer; /* multiuse ptr */
  double value;  /* double value, only one of pointer or value is used */
};

/*
  symbols are organized as a stack of lists: for every procedure call 
  a new list is pushed onto the stack; all local variables of this
  function are chained into this list. After return from this procedure,
  the whole list is discarded and one element is popped from
  the stack.
*/

struct symstack { /* stack of symbol lists */
  struct symbol *next_in_list;
  struct symstack *next_in_stack;
  struct symstack *prev_in_stack;
};

struct symbol {   /* general symbol; either variable, string */
  int type;
  struct symbol *link; /* points to linked symbol, if any */
  struct symbol *next_in_list; /* next symbol in symbollist */
  char *name; /* name of symbol */
  void *pointer;   /* pointer to string contents (if any) */
  char *args;      /* used to store number of arguments for functions/array */
  double value;
};

struct command { /* one interpreter command */
  int type;    /* type of command */
  int cnt; /* count of this command */
  struct command *prev;  /* link to previous command */
  struct command *next;  /* link to next command */
  void *pointer;  /* pointer to data */
  void *symbol;  /* pointer to symbol (or data within symbol) associated with command */
  struct command *jump;  /* pointer to jump destination */
  char *name; /* name of symbol associated with command */
  struct command *nextref; /* next cmd within function referencing a symbol */
  struct command *nextassoc; /* next cmd within within chain of associated commands */
  int args;  /* number of arguments for function/array call */
             /* or stream number for open/close             */
  int tag;  /* char/int to pass some information */
  int line; /* line this command has been created for */
  struct libfile_name *lib; /* associated library */
  int switch_id; /* TRUE, if in switch, FALSE else; used to check gotos */
};

struct array { /* data structure for arrays */
  int bounds[10];  /* index boundaries */
  int dimension; /* dimension of array */
  void *pointer; /* contents of array */
  char type;  /* decide between string- ('s') and double-Arrays ('d') */
};

struct buff_chain { /* buffer chain for system-input */
  char buff[SYSBUFFLEN+1]; /* content of buffer */
  int len; /* used length of buff */
  struct buff_chain *next; /* next buffer in chain */
};

struct libfile_name { /* used to store library names */
  char *l; /* long version, including path */
  int llen; /* length of l */
  char *s; /* short version */
  int slen; /* length of s */
  int lineno; /* linenumber within file */
  struct command *datapointer; /* data pointer of this library */
  struct command *firstdata; /* first data-command in library */
  struct libfile_name *next; /* next in chain */
};

/* ------------- function prototypes defined in ... ---------------- */

/* main.c */
void error(int,char *); /* reports an error and possibly exits */
void error_with_line(int,char *,int); /* reports an error and possibly exits */
void *my_malloc(unsigned); /* my own version of malloc */
void my_free(void *); /* free memory */
char *my_strerror(int); /* return description of error messages */
struct command *add_command(int,char *); /* get room for new command */
void signal_handler(int);  /* handle various signals */
char *my_strdup(char *); /* my own version of strdup */
char *my_strndup(char *,int); /*  own version of strndup */
struct libfile_name *new_file(char *,char *);  /* create a new structure for library names */
char *dotify(char *,int); /* add library name, if not already present */
char *strip(char *); /* strip off to minimal name */
void do_error(struct command *); /* issue user defined error */
void create_docu(char *); /* create command 'docu' */
extern void add_variables(char *); /* add pi and e to symbol table */
void compile(void); /* create a subroutine at runtime */
void create_execute(int); /* create command 'cEXECUTE' */
void execute(struct command *); /* execute a subroutine */
int isbound(void); /* check if this interpreter is bound to a program */


/* io.c */
void checkopen(void); /* check, if open has been sucessfull */
void create_colour(int); /* create command 'reverse' */
void colour(struct command *cmd); /* switch reverse-printing */
void create_print(char); /* create command 'print' */
void print(struct command *); /* print on screen */
void create_myread(char,int); /* create command 'read' */
void myread(struct command *); /* read from file or stdin */
void create_onestring(char *); /* write string to file */
void onestring(char *); /* write string to file */
void chkprompt(void); /* print an intermediate prompt if necessary */
void create_myopen(int); /* create command 'myopen' */
void myopen(struct command *); /* open specified file for given name */
void testeof(struct command *); /* close the specified stream */
void myclose(); /* close the specified stream */
void create_pps(int,int); /* create command pushswitch or popswitch */
void push_switch(struct command *); /* push current stream on stack and switch to new one */
void pop_switch(void); /* pop current stream from stack and switch to it */
void mymove(); /* move to specific position on screen */
void clearscreen(); /* clear entire screen */
char *inkey(double); /* gets char from keyboard, blocks and doesn´t print */
char *replace(char *); /* replace \n,\a, etc. */

/* graphic.c */
void create_openwin(int); /* create Command 'openwin' */
void openwin(struct command *, YabInterface* yab); /* open a Window */
void closewin(struct command *, YabInterface* yab); /* close the window */
int numwindows(); /* number of windows opened */
void createbutton(struct command *, YabInterface* yab); /* create a Button */
int createimage(double x, double y, const char* imagefile, const char* window, YabInterface* yab, int line, const char* libname); /* insert an image*/
int createimage2(double x1, double y1, double x2, double y2, const char* imagefile, const char* window, YabInterface* yab, int line, const char* libname); /* insert an image*/
int createsvg(double x1, double y1, double x2, double y2, const char* imagefile, const char* window, YabInterface* yab, int line, const char* libname); /* insert a svg image*/
int ismousein(const char* view, YabInterface *yab, int line, const char* libname); /* check if mouse is in view */
char* getmousein(YabInterface *yab, int line, const char* libname); /* check which view the mouse is in */ //vasper
void drawtext(struct command *, YabInterface* yab); /* draw text */
void drawrect(struct command *, YabInterface* yab); /* draw rectangle */
void drawclear(struct command *, YabInterface* yab); /* clears canvas */
void createmenu(struct command *, YabInterface* yab); /* add a menu */
void createalert(struct command *, YabInterface* yab); /* alert */
void createtext(struct command *, YabInterface* yab); /* text */
void text2(struct command *, YabInterface* yab); /* text */
void textalign(struct command *, YabInterface* yab); /* align text */
void createtextcontrol(struct command *, YabInterface* yab); /* textcontrol */
void createcheckbox(struct command *, YabInterface* yab); /* checkbox*/
void createradiobutton(struct command *, YabInterface* yab); /* radio button */
void createlistbox(struct command *, YabInterface* yab); /* list box */
void createdropbox(struct command *, YabInterface* yab); /* drop box */
void createitem(struct command *, YabInterface* yab); /* item add*/
void removeitem(struct command *, YabInterface* yab); /* item del*/
void clearitems(struct command *, YabInterface* yab); /* clears itemlist */
void localize();
void localizestop();
void localize2(struct command *, YabInterface* yab);
void setlayout(struct command *, YabInterface* yab); /* set layout */
void winset1(struct command *, YabInterface* yab); 
void winset2(struct command *, YabInterface* yab); 
void winset3(struct command *, YabInterface* yab); 
void winset4(struct command *, YabInterface* yab); 
void winclear(struct command *, YabInterface* yab); 
void textedit(struct command *, YabInterface* yab);
void textadd(struct command *, YabInterface* yab);
void textset(struct command *, YabInterface* yab);
void textset2(struct command *, YabInterface* yab);
void textset3(struct command *, YabInterface* yab);
void textcolor1(struct command *, YabInterface* yab);
void textcolor2(struct command *, YabInterface* yab);
void textclear(struct command *, YabInterface* yab);
char* textget(const char*, YabInterface* yab, int line, const char* libname);
int textget2(const char*, const char*, YabInterface* yab, int line, const char* libname);
char* textget3(const char*, int, YabInterface* yab, int line, const char* libname);
double textget4(const char*, const char*, int, YabInterface* yab, int line, const char* libname);
int textget5(const char*, const char*, const char*, YabInterface* yab, int line, const char* libname);
char* textget6(const char*, const char*, YabInterface *yab, int line, const char* libname);
char* textcontrolget(const char*, YabInterface* yab, int line, const char* libname);
void drawset1(struct command *, YabInterface* yab);
void drawset2(struct command *, YabInterface* yab);
char* getmessages(YabInterface* yab, int line, const char* libname); /* get message string */
char* getmousemessages(const char* view, YabInterface* yab, int line, const char* libname); /* get mouse message string */
char* gettranslation(const char*,YabInterface* yab, int line, const char* libname); /* get translation string */
char* getmenutranslation(const char*,YabInterface* yab, int line, const char* libname); /* get translation string */
char* getloadfilepanel(const char*, const char*, const char*, YabInterface *yab, int line, const char* libname); /* open a load filepanel */
char* getsavefilepanel(const char*, const char*, const char*, const char*, YabInterface *yab, int line, const char* libname); /* open a save filepanel */
void view(struct command *, YabInterface *yab); /* add a view */
void boxview(struct command *, YabInterface *yab); /* add a boxview */
void boxviewset(struct command *, YabInterface *yab);/*boxview options*/
void tab(struct command *, YabInterface *yab); /* add a tab */
void tabset(struct command *, YabInterface *yab); /* set a tab */
void tabadd(struct command *, YabInterface *yab);
void tabdel(struct command *, YabInterface *yab); 
int tabviewget(const char* tab, YabInterface *yab, int line, const char* libname); /* get a tab */
void drawdot(struct command *, YabInterface *yab); /* draw a dot */
void drawline(struct command *, YabInterface *yab); /* draw a line */
void drawcircle(struct command *, YabInterface *yab); /* draw a circle */
void drawellipse(struct command *, YabInterface *yab); /* draw a ellipse */
void drawcurve(struct command *, YabInterface *yab); /* draw a curve */
void slider1(struct command *, YabInterface *yab);
void slider2(struct command *, YabInterface *yab);
void slider3(struct command *, YabInterface *yab);
void slider4(struct command *, YabInterface *yab);
void slider5(struct command *, YabInterface *yab);
void slider6(struct command *, YabInterface *yab);
void option1(struct command *, YabInterface *yab);
void option2(struct command *, YabInterface *yab);
void option3(struct command *, YabInterface *yab);
void dropzone(struct command *, YabInterface *yab);
void colorcontrol1(struct command *, YabInterface *yab);
void colorcontrol2(struct command *, YabInterface *yab);
void textcontrol2(struct command *, YabInterface *yab);
void textcontrol3(struct command *, YabInterface *yab);
void textcontrol4(struct command *, YabInterface *yab);
void textcontrol5(struct command *, YabInterface *yab);
void treebox1(struct command *, YabInterface *yab);
void treebox2(struct command *, YabInterface *yab);
void treebox3(struct command *, YabInterface *yab);
void treebox4(struct command *, YabInterface *yab);
void treebox5(struct command *, YabInterface *yab);
void treebox7(struct command *, YabInterface *yab);
void treebox8(struct command *, YabInterface *yab);
void treebox9(struct command *, YabInterface *yab);
void treebox10(struct command *, YabInterface *yab);
void treebox11(struct command *, YabInterface *yab);
void buttonimage(struct command *, YabInterface *yab);
void checkboximage(struct command *, YabInterface *yab);
void checkboxset(struct command *, YabInterface *yab);
void radioset(struct command *, YabInterface *yab);
void tooltip(struct command *, YabInterface *yab);
void tooltipcolor(struct command *, YabInterface *yab);
void listsort(struct command *, YabInterface *yab);
void treesort(struct command *, YabInterface *yab);
void filebox(struct command *, YabInterface *yab);
void fileboxadd2(struct command *, YabInterface *yab);
void fileboxclear(struct command *, YabInterface *yab);
void columnboxadd(struct command *, YabInterface *yab);
void columnboxremove(struct command *, YabInterface *yab);
void columnboxselect(struct command *, YabInterface *yab);
void columnboxcolor(struct command *, YabInterface *yab);
void dropboxselect(struct command *, YabInterface *yab);
void menu2(struct command *, YabInterface *yab);
void submenu1(struct command *, YabInterface *yab);
void submenu2(struct command *, YabInterface *yab);
void clipboardcopy(struct command *, YabInterface *yab);
void launch(struct command *, YabInterface *yab);
int printer(const char* docname, const char *view, const char* config, YabInterface *yab, int line, const char* libname);
void printerconfig(struct command *, YabInterface *yab);
char* keyboardmessages(const char* view, YabInterface* yab, int line, const char* libname);
char* clipboardpaste(YabInterface* yab, int line, const char* libname);
char* columnboxget(const char* columnbox, int column, int position, YabInterface* yab, int line, const char* libname);
int columnboxcount(const char* columnbox, YabInterface* yab, int line, const char* libname);
int windowgetnum(const char* view, const char *option, YabInterface* yab, int line, const char* libname);
int viewgetnum(const char* view, const char *option, YabInterface* yab, int line, const char* libname); //vasper
int newalert(const char* text, const char* button1, const char* button2, const char* button3, const char* option, YabInterface* yab, int line, const char* libname);
void calendar1(struct command *, YabInterface *yab);
char* calendar2(const char* calendar, YabInterface *yab, int line, const char* libname);
void calendar3(struct command *, YabInterface *yab);
void listboxadd1(struct command *, YabInterface *yab);
void listboxadd2(struct command *, YabInterface *yab);
void listboxselect(struct command *, YabInterface *yab);
void listboxremove(struct command *, YabInterface *yab);
int listboxcount(const char* listbox, YabInterface *yab, int line, const char* libname);
char* treeboxget(const char* treebox, int position, YabInterface *yab, int line, const char* libname);
int treeboxcount(const char* treebox, YabInterface *yab, int line, const char* libname);
char* listboxget(const char* listbox, int position, YabInterface *yab, int line, const char* libname);
void scrollbar(struct command *, YabInterface *yab);
void scrollbarset1(struct command *, YabInterface *yab);
void scrollbarset2(struct command *, YabInterface *yab);
void scrollbarset3(struct command *, YabInterface *yab);
double scrollbarget(const char* scrollbar, const char* option, YabInterface *yab, int line, const char* libname);
void splitview1(struct command *, YabInterface *yab);
void splitview2(struct command *, YabInterface *yab);
void splitview3(struct command *, YabInterface *yab);
double splitviewget(const char* splitview, const char* option, YabInterface *yab, int line, const char* libname);
void stackview1(struct command *, YabInterface *yab);
void stackview2(struct command *, YabInterface *yab);
int stackviewget(const char* stackview, YabInterface *yab, int line, const char* libname);
void texturl1(struct command *, YabInterface *yab);
void texturl2(struct command *, YabInterface *yab);
void drawset3(struct command *, YabInterface *yab);
void spincontrol1(struct command *, YabInterface *yab);
void spincontrol2(struct command *, YabInterface *yab);
int spincontrol(const char* spincontrol, YabInterface *yab, int line, const char* libname);
char* popupmenu(double x, double y, const char* messages, const char* id, YabInterface *yab, int line, const char* libname);
void dropboxremove(struct command *, YabInterface *yab);
void dropboxclear(struct command *, YabInterface *yab);
int dropboxcount(const char* id, YabInterface *yab, int line, const char* libname);
char* dropboxget(const char* id, int position, YabInterface *yab, int line, const char* libname);
int sliderget(const char *slider, YabInterface *yab, int line, const char* libname);
int colorcontrolget(const char* colorcontrol, const char* option, YabInterface *yab, int line, const char* libname);
void submenu3(struct command *, YabInterface *yab);
void menu3(struct command *, YabInterface *yab);
double drawget1(const char*, const char*, const char*, YabInterface *yab, int line, const char* libname);
double drawget2(const char*, const char*, YabInterface *yab, int line, const char* libname);
char* drawget3(const char*, YabInterface *yab, int line, const char* libname);
int drawget4(double, double, const char*, const char*, YabInterface *yab, int line, const char* libname);
void option4(struct command *, YabInterface *yab);
void option5(struct command *, YabInterface *yab);
void treebox12(struct command *, YabInterface *yab);
int messagesend(const char*, const char*, YabInterface *yab, int line, const char* libname);
int threadkill(const char*, int, YabInterface *yab, int line, const char* libname);
int threadget(const char*, const char*, YabInterface *yab, int line, const char* libname);
void bitmap(struct command *, YabInterface *yab);
void bitmapdraw(struct command *, YabInterface *yab);
void bitmapdraw2(struct command *, YabInterface *yab);
void bitmapget(struct command *, YabInterface *yab);
void bitmapget2(struct command *, YabInterface *yab);
void bitmapgeticon(struct command *, YabInterface *yab);
void bitmapdrag(struct command *, YabInterface *yab);
void bitmapremove(struct command *, YabInterface *yab);
void screenshot(struct command *, YabInterface *yab);
void statusbar(struct command *, YabInterface *yab);
void statusbarset(struct command *, YabInterface *yab);
void statusbarset2(struct command *, YabInterface *yab);
void statusbarset3(struct command *, YabInterface *yab);
void canvas(struct command *, YabInterface *yab);
int bitmapsave(const char*, const char*, const char*, YabInterface *yab, int line, const char* libname);
int bitmapload(const char*, const char*, YabInterface *yab, int line, const char* libname);
int bitmapgetnum(const char*, const char*, YabInterface *yab, int line, const char* libname);
int bitmapcolor(double x, double y, const char* id, const char* option, YabInterface *yab, int line, const char* libname);
int listboxgetnum(const char*, YabInterface *yab, int line, const char* libname);
int dropboxgetnum(const char*, YabInterface *yab, int line, const char* libname);
int columnboxgetnum(const char*, YabInterface *yab, int line, const char* libname);
int treeboxgetnum(const char*, YabInterface *yab, int line, const char* libname);
int treeboxgetopt(const char*, const char*, int, YabInterface *yab, int line, const char* libname);
void treebox13(struct command *, YabInterface *yab);
void drawset4(struct command *, YabInterface *yab);
int sound(const char*, YabInterface *yab, int line, const char* libname);
void soundstop(struct command *, YabInterface *yab);
void soundwait(struct command *, YabInterface *yab);
void shortcut(struct command *, YabInterface *yab);
int iscomputeron(YabInterface *yab, int line, const char* libname);
void mouseset(struct command *, YabInterface *yab);
void gettermkey(char *); /* read a key from terminal */
void attribute1(struct command *, YabInterface *yab);
void attributeclear(struct command *, YabInterface *yab);
char* attributeget1(const char*, const char*, YabInterface *yab, int line, const char* libname);
double attributeget2(const char*, const char*, YabInterface *yab, int line, const char* libname);

/* function.c */
void create_exception(int); /* create command 'exception' */
void exception(struct command *); /* change handling of exceptions */
void create_poke(char); /* create Command 'POKE' */
void poke(); /* poke in internals */
void pokefile(struct command *); /* poke into file */
void create_dblrelop(char); /* create command dblrelop */ 
void dblrelop(struct command *);  /* compare topmost double-values */
void concat(void); /* concetenates two strings from stack */
void create_strrelop(char); /* create command strrelop */ 
void strrelop(struct command *);  /* compare topmost string-values */
void create_changestring(int); /* create command 'changestring' */
void changestring(struct command *); /* changes a string */
void glob(void); /* check, if pattern globs string */
void create_boole(char); /* create command boole */ 
void boole(struct command *);  /* perform and/or/not */
void create_function(int); /* create command 'function' */
void function(struct command *, YabInterface* yab); /* performs a function */
int myformat(char *,double,char *,char *); /* format number */
void create_restore(char *); /* create command 'restore' */
void restore(struct command *); /* reset data pointer to given label */
void create_dbldata(double);  /* create command dbldata */
void create_strdata(char *);  /* create command strdata */
void create_readdata(char); /* create command readdata */
void readdata(struct command *); /* read data items */
void mywait(); /* wait given number of seconds */
void mybell(); /* ring ascii bell */
void getmousexybm(char *,int *,int *,int *,int *); /* get mouse coordinates */
void token(struct command *); /* extract token from variable */
void tokenalt(struct command *); /* extract token from variable with alternate semantics */


/* symbol.c */
struct array *create_array(int,int); /* create an array */
void clearrefs(struct command *); /* clear references for commands within function */
void duplicate(void); /* duplicate topmost element of stack */
void negate(void);  /* negates top of stack */
void create_require(int); /* create command 'cREQUIRE' */
void require(struct command *); /* check item on stack has right type */
void create_makelocal(char *,int); /* create command 'cMAKELOCAL' */
void create_makestatic(char *,int); /* create command 'cMAKESTATIC' */
void create_arraylink(char *,int); /* create command 'cARRAYLINK' */
void create_pusharrayref(char *,int); /* create command 'cPUSHARRAYREF' */
void pusharrayref(struct command *); /* push an array reference onto stack */
void arraylink(struct command *); /* link a local symbol to a global array */
void makestatic(struct command *); /* makes symbol static */
void makelocal(struct command *); /* makes symbol local */
void create_numparam(void); /* create command 'cNUMPARAM' */
void numparam(struct command *); /* count number of function parameters */
void pushdblsym(struct command *); /* push double symbol onto stack */
void popdblsym(struct command *); /* pop double from stack */
void create_pushdbl(double); /* create command 'pushdbl' */
void pushdbl(struct command *); /* push double onto stack */
void create_dblbin(char); /* create binary expression calculation */
void dblbin(struct command *); /* compute with two numbers from stack */
void pushstrsym(struct command *);   /* push string symbol onto stack */
void popstrsym(struct command *); /* pop string from stack */
void create_pushstr(char *); /* creates command pushstr */
void pushstr(struct command *); /* push string onto stack */
void pushname(char *); /* push a name on stack */
void pushstrptr(struct command *);  /* push string-pointer onto stack */
void forcheck(void); /* check, if for-loop is done */
void forincrement(void); /* increment value on stack */
void startfor(void); /* compute initial value of for-variable */
void create_goto(char *); /* creates command goto */
void create_gosub(char *); /* creates command gosub */
void create_call(char *); /* creates command function call */
void create_label(char *,int); /* creates command label */
void create_sublink(char *); /* create link to subroutine */
void pushgoto(void); /* generate label and push goto on stack */
void popgoto(void); /* pops a goto and generates the matching command */
void jump(struct command *); /* jump to specific Label */
void myreturn(struct command *); /* return from gosub */
void findnop(); /* used for on_gosub, find trailing nop command */
void skipper(void); /* used for on_goto/gosub, skip commands */
void skiponce(struct command *); /* skip next command once */
void resetskiponce(struct command *); /* find and reset next skip */
void decide(void); /*  skips next command, if not 0 on stack */
void logical_shortcut(struct command *type); /* shortcut and/or if possible */ 
void create_doarray(char *,int); /* creates array-commands */ 
void doarray(struct command *);  /* call an array */
void create_dim(char *,char); /* create command 'dim' */
void dim(struct command *); /* get room for array */
void pushlabel(void); /* generate goto and push label on stack */
void poplabel(void); /* pops a label and generates the matching command */
void storelabel(); /* push label on stack */
void matchgoto(); /* generate goto matching label on stack */
void swap(void); /*swap topmost elements on stack */
struct stackentry *push(void); /* push element on stack and enlarge it*/
struct stackentry *pop(int); /* pops element to memory */
struct symbol *get_sym(char *,int,int); /* find and/or add a symbol */
void link_symbols(struct symbol *,struct symbol *); /* link one symbol to the other */
void pushsymlist(void); /* push a new list on symbol stack */
void popsymlist(void); /* pop list of symbols and free symbol contents */
void dump_sym(); /* dump the stack of lists of symbols */
void dump_sub(int); /* dump the stack of subroutine calls */
void create_retval(int,int); /* create command 'cRETVAL' */
void retval(struct command *); /* check return value of function */
void create_endfunction(void); /* create command cEND_FUNCTION */
void function_or_array(struct command *); /* decide whether to do perform function or array */
struct command *search_label(char *,int); /* search label */
void reshufflestack(struct stackentry *); /* reorganize stack for function call */
void push_switch_mark(void); /* push a switch mark */
void create_clean_switch_mark(int,int); /* add command clean_switch_mark */
void clean_switch_mark(struct command *); /* pop everything up to (and including) first switch_mark from stack */
void push_switch_id(void); /* generate a new switch id */
void pop_switch_id(void); /* get previous switch id */
int get_switch_depth(void); /* get current depth of switch id stack */


/* flex.c */
void yyerror(char *); /* yyerror message */
void open_main(FILE *,char *,char *); /* switch to file */
void open_string(char *); /* open string with commands */
FILE *open_library(char *,char **,int); /* search and open a library */
void switchlib(void); /* switch library, called by bison */
