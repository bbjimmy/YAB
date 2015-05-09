/*  

YABASIC ---  a simple Basic Interpreter
written by Marc-Oliver Ihm 1995-2004
homepage: www.yabasic.de
    
main.c --- main() and auxilliary functions
    
This file is part of yabasic and may be copied only 
under the terms of either the Artistic License or 
the GNU General Public License (GPL), both of which 
can be found at www.yabasic.de

*/


/* ------------- includes ---------------- */

// include "YabInterface.h"

#ifndef YABASIC_INCLUDED
#include "yabasic.h"       /* all prototypes and structures */
#endif


/* ------------- defines ---------------- */

#define DONE {current=current->next;break;}  /* reduces type-work */
#define COPYRIGHT "Original yabasic Copyright 1995-2006 by Marc-Oliver Ihm\n\tyab improvements Copyright 2006-2014 by Jan Bungeroth\n\tyab improvements Copyright 2013-2015 by Jim Saxton\n"
#define BANNER \
"\n        yab is yabasic for Haiku. This is version " VERSION ",\n       built on "\
ARCHITECTURE " on " BUILD_TIME "\n\n      " COPYRIGHT "\n\n"
#define BANNER_VERSION \
"yab " VERSION ", built on " ARCHITECTURE "\n" COPYRIGHT
#define YABFORHELP "(type 'yab -help' for help)"
#define YABMAGIC "__YaBaSiC_MaGiC_CoOkIe__"

/* ------------- external references ---------------- */

extern int mylineno;   /* current line number */
extern int yyparse();  /* call bison parser */


/* ------------- local functions ---------------- */

static void std_diag(char *,int,char *); /* produce standard diagnostic */
static void parse_arguments(int,char *argv[]); /* parse cmd line arguments */
static void initialize(void); /* give correct values to pointers etc ... */
static void run_it(YabInterface* yab); /* execute the compiled code */
static void end_it(void); /* perform shutdown operations */
#ifdef WINDOWS
static void chop_command(char *,int *,char ***); /* chops WIN95-commandline */
#endif
void create_docu_array(void); /* create array with documentation */
int equal(char *,char *,int); /* helper for processing options */
void do_help(char *); /* process help option */ 
static int mybind(char *); /* bind a program to the interpreter and save it */
char *find_interpreter(char *); /* find interpreter with full path */

/* ------------- global variables ---------------- */

struct command *cmdroot; /* first command */
struct command *cmdhead; /* next command */
struct command *lastcmd; /* last command */
struct command *current; /* currently executed command */
int infolevel; /* controls issuing of error messages */
int errorlevel; /* highest level of error message seen til now */
static int debug_count; /* number of debug messages */
static int note_count; /* number of notes */
static int warning_count; /* number of warning messages */
static int error_count; /* number of error messages */
int interactive; /* true, if commands come from stdin */
int is_bound; /* true, if this executable is bound */
char* appdirectory;
static char *to_bind=NULL; /* name bound program to be written */
FILE *bound_program=NULL; /* points to embedded yabasic program (if any) */
char *string; /* for trash-strings */
char *errorstring; /* for error-strings */
int errorcode; /* error-codes */
int enterkeyflag = 0; /* press enter to end program */
static int commandcount; /* total number of commands */
int program_state;  /* state of program */
char *progname=NULL; /* name of yabasic-program */
int print_docu=FALSE; /* TRUE, if only docu should be printed */
int hold_docu=FALSE; /* TRUE, if docu should be printed in portions */
#ifdef WINDOWS
DWORD InitialConsole; /* initial state of console window */
#endif
char *explanation[cLAST_COMMAND-cFIRST_COMMAND+1]; /* explanations of commands */
char *explicit=NULL; /* yabasic commands given on the command line */
char **yabargv; /* arguments for yabasic */
int yabargc; /* number of arguments in yabargv */
static int endreason=erNONE; /* reason for termination */
static int exitcode=0;
static int signal_arrived=0;
/* timing */
time_t compilation_start,compilation_end,execution_end;
char library_path[200]; /* full path to search libraries */
char library_default[200]; /* default full path to search libraries */
static struct command *docuhead=NULL; /* first docu in main */
static int docucount=0; /* number of docu-lines in array */
int check_compat=0; /* true, if compatibility should be checked */
static char *interpreter_path=NULL; /* name of interpreter executing; i.e. ARGV[0] */
static char *main_file_name=NULL; /* name of program to be executed */

YabInterface *yab;

/* ------------- main program ---------------- */

int mmain(int argc,char **argv, YabInterface* y)
{
#ifdef WINDOWS
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  char *lp;
  int fromlibpath;
#endif
  int len;

  yab = y;
  appdirectory = yi_GetApplicationDirectory(yab);

  // appdirectory=(char *)my_malloc(sizeof(argv[0]));
  // strcpy(appdirectory, argv[0]);

  string=(char *)my_malloc(sizeof(char)*INBUFFLEN);
  errorstring=(char *)my_malloc(sizeof(char)*INBUFFLEN);
  *errorstring='\0';
  errorcode=0;
  
  program_state=HATCHED;
  infolevel=WARNING; /* set the initial Infolevel */
  
#ifdef WINDOWS
  /* get handle for current thread */
  DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),
		  GetCurrentProcess(),&mainthread,THREAD_ALL_ACCESS,FALSE,0);
  
  /* get handle of instance */
  this_instance=GetModuleHandle(NULL);
  
  /* define my window class */
  myclass.style=0;
  myclass.lpfnWndProc=(LPVOID)mywindowproc;
  myclass.cbClsExtra=0; /* no extra bytes */
  myclass.cbWndExtra=0;
  myclass.hInstance=this_instance;
  myclass.hIcon=LoadIcon(this_instance,"yabasicIcon");
  myclass.hCursor=LoadCursor(NULL,IDC_ARROW); /*  standard cursor */
  myclass.hbrBackground=(HBRUSH)COLOR_WINDOW; /* default-background */
  myclass.lpszMenuName=NULL;
  myclass.lpszClassName=my_class;
  
  RegisterClass(&myclass);
  
  /* get console handles */
  ConsoleInput=GetStdHandle(STD_INPUT_HANDLE);
  ConsoleOutput=GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleMode(ConsoleInput,&InitialConsole);
  
  /* find out, if launched from commandline */
  GetConsoleScreenBufferInfo(ConsoleOutput,&csbi);
  Commandline=!((csbi.dwCursorPosition.X==0) && (csbi.dwCursorPosition.Y==0));
  if ((csbi.dwSize.X<=0) || (csbi.dwSize.Y <= 0)) Commandline=TRUE;
  
#endif
  /* get library path */
  library_path[0]='\0';
  library_default[0]='\0';
#ifdef UNIX
  strcpy(library_default,LIBRARY_PATH);
#else
  fromlibpath=TRUE;
  if (lp=getreg("librarypath")) {
    strcpy(library_default,lp);
    fromlibpath=TRUE;
  } else if (lp=getreg("path")) {
    strcpy(library_default,lp);
    fromlibpath=FALSE;
  } else {
    library_default[0]='\0';
    fromlibpath=FALSE;
  }
#endif
  
  /* find out, if this executable is bound to a yabasic program */
  interpreter_path=find_interpreter(argv[0]);
  is_bound=isbound();

  /* parse arguments */
  parse_arguments(argc,argv);
  
  /* brush up library path */
  if (!library_path[0]) strcpy(library_path,library_default);
  len=strlen(library_path);
#ifdef UNIX
  if (library_path[len-1]=='/' || library_path[len-1]=='\\') library_path[len-1]='\0';
#else
  if (library_path[0]) {
    if (library_path[len-1]!='/' && library_path[len-1]!='\\') strcat(library_path,"\\");
    if (!fromlibpath) strcat(library_path,"lib\\");
  }
#endif
    
  time(&compilation_start);
  error(DEBUG,"this is yab " VERSION);
  initialize();
  program_state=INITIALIZED;

  error(NOTE,"calling parser/compiler");
      
  if (interactive) {
    printf("%s",BANNER);
    printf("Enter your program and type RETURN twice when done.\n\n");
    printf("Your program will execute immediately and CANNOT BE SAVED;\n");
    printf("create your program with an EXTERNAL EDITOR, if you want to keep it.\n");
#ifdef UNIX
#ifndef BEOS
    printf("Type 'man yabasic' or see the file yabasic.htm for more information.\n\n");
#else
    printf("See the documentation for more information.\n\n");
#endif
#else
    printf("See the documentation within the start-menu for more information.\n\n");
#endif
  }
  program_state=COMPILING;
  if (yyparse() && errorlevel>ERROR) error(ERROR,"Couldn't parse program");

  if (errorlevel>ERROR) create_docu_array();

  add_command(cEND,NULL);
  sprintf(string,"read %d line(s) and generated %d command(s)",mylineno,commandcount);
  error(NOTE,string);
  
  time(&compilation_end);

  if (to_bind) {
    struct libfile_name *lib;
    if (mybind(to_bind)) {
      sprintf(string,"Successfully bound '%s' and '%s' into '%s'",interpreter_path,main_file_name,to_bind);
      error(INFO,string);
      end_it();
    } else {
      sprintf(string,"could not bind '%s' and '%s' into '%s'",interpreter_path,main_file_name,to_bind);
      error(ERROR,string);
      end_it();
    }
  }
  
  if (errorlevel>ERROR && !check_compat) {
    program_state=RUNNING;
    run_it(yab);
  } else {
    program_state=FINISHED;
    if (check_compat) 
      printf("Check for possible compatibility problems done\nProgram will not be executed, %d possible problem(s) reported\n",warning_count);
    else
      error(ERROR,"Program not executed");
  }
  
  program_state=FINISHED;
  sprintf(string,"%d debug(s), %d note(s), %d warning(s), %d error(s)",
	  debug_count,note_count,warning_count,error_count);
  error(NOTE,string);
  time(&execution_end);
  sprintf(string,"compilation time %g second(s), execution %g",
	  (double)(compilation_end-compilation_start),(double)(execution_end-compilation_end));
  error(NOTE,string);
  end_it();
  return !(errorlevel>ERROR);
}



/* ------------- subroutines ---------------- */


static void std_diag(char *head,int type,char *name) /* produce standard diagnostic */
{
  int n,i;
  char *s;
  struct stackentry *sp;
  
  if (infolevel>=DEBUG) {
    s=string;
    if (type>cLAST_COMMAND || type<cFIRST_COMMAND) {
      sprintf(s,"%s Illegal %d %n",head,type,&n);
    }
    else {
      if (name) 
	sprintf(s,"%s '%s' (%s) %n",head,explanation[type],name?name:"NULL",&n);
      else
	sprintf(s,"%s '%s' %n",head,explanation[type],&n);
    }
    s+=n;
    if (stackhead->prev!=stackroot) {
      sprintf(s,"t[");
      s+=2;
      sp=stackhead;
      for(i=0;TRUE;i++) {
        sp=sp->prev;
        if (sp==stackroot) break;
	if (i>=5) continue;
        if (i>0) {
          sprintf(s,",");
          s++;
        }
        switch(sp->type) {
        case stGOTO:
          sprintf(s,"goto%n",&n);
          break;
        case stSTRINGARRAYREF:
        case stNUMBERARRAYREF:
	  if (sp->pointer) 
	    sprintf(s,"%s()%n",(char *)sp->pointer,&n);
	  else
	    sprintf(s,"ARRAY()%n",&n);
          break;
        case stSTRING:
          sprintf(s,"'%s'%n",(char *)sp->pointer,&n);
          break;
        case stNUMBER:
          sprintf(s,"%g%n",sp->value,&n);
          break;
        case stLABEL:
          sprintf(s,"label%n",&n);
          break;
        case stRETADD:
          sprintf(s,"retadd%n",&n);
          break;
        case stRETADDCALL:
          sprintf(s,"retaddcall%n",&n);
          break;
        case stSWITCH_MARK:
          sprintf(s,"switch_mark%n",&n);
          break;
        case stFREE:
          sprintf(s,"free%n",&n);
          break;
        case stROOT:
          sprintf(s,"root%n",&n);
          break;
        case stSWITCH_STRING:
          sprintf(s,"switch_string%n",&n);
          break;
        case stSWITCH_NUMBER:
          sprintf(s,"switch_number%n",&n);
          break;
        default:
          sprintf(s,"unknown%n",&n);
	  break;
        }
        s+=n;
      }
      if (i>5) {
	sprintf(s,";+%d%n",i-5,&n);
	s+=n;
      }
      strcat(s,"]b");
    }
    error(DEBUG,string);
  }
}


struct command *add_command(int type,char *name) 
     /* get room for new command, and make a link from old one */
{
  struct command *new;
  
  if (infolevel>=DEBUG) std_diag("creating",type,name);
  cmdhead->type=type;  /* store command */
  cmdhead->line=mylineno;
  cmdhead->lib=currlib;
  cmdhead->cnt=commandcount;
  if (!name || !*name) 
    cmdhead->name=NULL;
  else
    cmdhead->name=my_strdup(name);
  commandcount++;
  cmdhead->pointer=NULL;  /* no data yet */ 
  cmdhead->jump=NULL;
  cmdhead->nextassoc=NULL;
  cmdhead->switch_id=get_switch_id();
  if (!currlib->datapointer && cmdhead->type==cDATA) currlib->firstdata=currlib->datapointer=cmdhead;

  /* link into chain of commands referencing a symbol */
  if (name) {
    if (lastref) lastref->nextref=cmdhead;
    lastref=cmdhead;
  }

  /* create new command */
  new=(struct command *)my_malloc(sizeof(struct command)); 
  /* and initialize */
  new->next=NULL;
  new->prev=cmdhead;
  new->pointer=NULL;
  new->symbol=NULL;
  new->nextref=NULL;
  new->nextassoc=NULL;
  new->name=NULL;

  cmdhead->next=new;
  lastcmd=cmdhead;
  cmdhead=cmdhead->next;
  return lastcmd;
}


static void parse_arguments(int cargc,char *cargv[])
     /* parse arguments from the command line */
{
  char **argv;
  int argc,larg;
#ifdef UNIX
  char *parg;
  int i;
#endif
  int ar;
  FILE *inputfile=NULL;
  char *option;
  char *info;
  int options_done=FALSE;
  
  if (cargc>1) larg=strlen(cargv[1]);
  else larg=0;
#ifdef UNIX
  if (cargc>0) {
    /* get room for arguments */
    argv=(char **)my_malloc((larg+cargc+1)*sizeof(char *));
    /* copy zero argument */
    argv[0]=cargv[0];
    argc=0;
    /* chop first argument into pieces */
    if (cargc>=2) {
      parg=strtok(my_strdup(cargv[1])," \t");
      for(argc=1;parg;argc++) {
        argv[argc]=parg;
        parg=strtok(NULL," \t");
      }
    }
    /* copy other arguments */
    for(i=2;i<cargc;i++) {
      argv[argc]=cargv[i];
      argc++;
    }
  } else {
    argv=cargv;
    argc=cargc;
  }
#else
  argv=cargv;
  argc=cargc;
#endif
  yabargv=(char **)my_malloc((larg+cargc+1)*sizeof(char *));
  yabargc=0;
  
  if (is_bound) options_done=1;
  /* process options */
  for(ar=1;ar<argc;ar++) {
    option=argv[ar];
    if (!options_done) {
      if (equal("-help",option,-2) || 
	  equal("-?",option,2) || 
	  equal("-version",option,2) ||
	  equal("-licence",option,4) || 
	  equal("-license",option,4)) {
	do_help(option);
	end_it();
      } else if (equal("-check",option,2)) {
	check_compat=1;
      } else if (!strcmp("--",option)) {
	options_done=TRUE;
	continue;
      } else if (equal("-enter",option,2)) {
	enterkeyflag = 1;
      } else if (equal("-infolevel",option,2)) {
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no infolevel specified " YABFORHELP);
	  end_it();
	}
	info=argv[ar];
	if (!strncmp(info,"debug",strlen(info))) infolevel=DEBUG;
	else if (!strncmp(info,"note",strlen(info))) infolevel=NOTE;
	else if (!strncmp(info,"warning",strlen(info))) infolevel=WARNING;
	else if (!strncmp(info,"error",strlen(info))) infolevel=ERROR;
	else if (!strncmp(info,"fatal",strlen(info))) infolevel=FATAL;
	else if (!strncmp(info,"bison",strlen(info))) {yydebug=1;infolevel=DEBUG;}
	else {
	  sprintf(string,"there's no infolevel '%s' " YABFORHELP,argv[ar]);
	  error(ERROR,string);
	  end_it();
	}
      }
      /*else if (equal("-fg",option,3) || equal("-foreground",option,4)) {   
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no foreground colour specified " YABFORHELP);
	  end_it();
	}
	foreground=my_strdup(argv[ar]);
      }
      else if (equal("-bg",option,2) || equal("-background",option,2)) {           
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no background colour specified (-h for help)");
	  end_it();
	}
	background=my_strdup(argv[ar]);   
      }
      else if (equal("-geometry",option,2)) {             
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no geometry string specified (-h for help)");
	  end_it();
	}
	geometry=my_strdup(argv[ar]);     
      }*/
      else if (equal("-bind",option,3)) {
	ar++;
	if (ar>=argc) {
	  error(ERROR,"name of bound program to be written is missing");
	  end_it();
	}
	to_bind=my_strdup(argv[ar]);
      }
      else if (equal("-execute",option,2)) {             
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no commands specified (-h for help)");
	  end_it();
	}
	explicit=my_strdup(argv[ar]);     
      }
      else if (equal("-librarypath",option,4)) {             
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no library path specified (-h for help)");
	  end_it();
	}
	strcpy(library_path,argv[ar]);
      }/*
      else if (equal("-display",option,3)) {              
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no display name specified (-h for help)");
	  end_it();
	}
	displayname=my_strdup(argv[ar]);
      }
      else if (equal("-font",option,4)) {         
	ar++;
	if (ar>=argc) {
	  error(ERROR,"no font specified (-h for help)");
	  end_it();
	}
	font=my_strdup(argv[ar]);
      }*/ else if (!print_docu &&
		 (!strcmp("-doc",option) || !strncmp("-doc_",option,5) ||
		  !strcmp("-docu",option) || !strncmp("-docu_",option,6))) {   
	print_docu=TRUE;
	if (!strncmp("-doc_",option,5)) {
	  ar--;
	  hold_docu=TRUE;
	  main_file_name=my_strdup(option+5);
	} else if (!strncmp("-docu_",option,6)) {
	  ar--;
	  hold_docu=TRUE;
	  main_file_name=my_strdup(option+6);
	} else {
	  if (ar>=argc-1) {
	    error(ERROR,"no filename specified (-h for help)");
	    end_it();
	  }
	  hold_docu=FALSE;
	  main_file_name=my_strdup(argv[ar+1]);
	}
      } else if (!print_docu && *option=='-') {
	sprintf(string,"unknown or ambigous option '%s' " YABFORHELP,option);
	error(ERROR,string);
	end_it();
      } else if (!is_bound && !inputfile && !explicit) { /* not an option */
	if (!main_file_name) main_file_name=my_strdup(argv[ar]);
	inputfile=fopen(main_file_name,"r");
	if (inputfile==NULL) {
	  sprintf(string,"could not open '%s': %s",main_file_name,my_strerror(errno));
	  error(ERROR,string);
	  endreason=erERROR;
	  exitcode=1;
	  end_it();
	} else {
	  progname=strrchr(main_file_name,'\\');
	  if (!progname) progname=strrchr(main_file_name,'/');
	  if (progname) 
	    progname++;
	  else 
	    progname=main_file_name;
	  if (!progname) progname="yab";
	}
      } else {    /* option for yabasic program */
	yabargv[yabargc]=my_strdup(argv[ar]);
	yabargc++;
      }
    } else {
      yabargv[yabargc]=my_strdup(argv[ar]);
      yabargc++;
    }
  }
  
  interactive=FALSE;
  
#ifdef WINDOWS
  if (is_bound || !progname) {
    SetConsoleTitle("");
  } else {
    SetConsoleTitle(progname);
  }
#endif
  
  if (is_bound) {
    inputfile=bound_program;
    main_file_name=my_strdup(interpreter_path);
  } else if (!inputfile && !explicit) {
    interactive=TRUE;
    inputfile=stdin;
    main_file_name="standard input";
  }
  if (explicit) main_file_name="command line";

  yi_SetMainFileName(main_file_name, yab);

  /* open a flex buffer for the initial file */
  open_main(inputfile,explicit,main_file_name);
  return;

}

void do_help(char *op) /* process help option */ 
{
  char *ooop=op;
  char *oop=op;
  if (op[0]=='-' && op[1]=='-') op++;
  oop=op;
  op=strchr(++op,'-');
  if (!op) {
    if (equal("-version",oop,2)) {
      fprintf(stderr,"%s\n",BANNER_VERSION);
    } else if (equal("-license",oop,2) || equal("-licence",oop,2)) {
      fprintf(stderr,"\n%s\n",YABLICENSE);
    } else {
      fprintf(stderr,"%s",BANNER);
      fprintf(stderr,"For more help try one of these options:\n");
      fprintf(stderr,"  -help-license : show license and conditions of copying\n");
      fprintf(stderr,"  -help-usage   : invoking yabasic and commandline options\n\n");
#ifdef UNIX
#ifndef BEOS
      fprintf(stderr,"Type 'man yabasic' or see the file yabasic.htm for a description\n");
#else
      fprintf(stderr,"Read the documentation for a description of the language.\n\n");
#endif
#else
      fprintf(stderr,"See the file yabasic.htm (accessible from the start-menu) for a\ndescription");
#endif    
//      fprintf(stderr,"of the language, or go to www.yabasic.de for the latest\n");
//      fprintf(stderr,"information and other yabasic resources.\n\n");
    }
  } else if (equal("-license",op,2) || equal("-licence",op,2)) {
    fprintf(stderr,"\n%s\n",YABLICENSE);
  } else if (equal("-usage",op,2)) {
    fprintf(stderr,"\nUsage:        yab [OPTIONS] [FILENAME [ARGUMENTS]]\n\n");
    fprintf(stderr,"FILENAME  : file, which contains the yab program; omit it to type\n");
    fprintf(stderr,"            in your program on the fly (terminated by a double newline)\n");
    fprintf(stderr,"ARGUMENTS : strings, that are available from within the yab program\n\n");
    fprintf(stderr,"available OPTIONS:\n");
    fprintf(stderr,"   -help               : print help message and other help options\n");
    fprintf(stderr,"   -version            : show version of yab\n");
    fprintf(stderr,"   -infolevel [dnwefb] : set infolevel to debug,note,warning,error or fatal\n");
    fprintf(stderr,"   -execute COMMANDS   : execute yab COMMANDS right away\n");
    fprintf(stderr,"   -bind BOUND         : bind interpreter with FILENAME into BOUND\n");
//    fprintf(stderr,"   -geometry x+y       : position graphic window at x,y\n");
#ifdef UNIX
//    fprintf(stderr,"   -fg,-bg COL         : specify fore/background color of graphic window\n");
//    fprintf(stderr,"   -display DISP       : display, where window will show up\n");
//    fprintf(stderr,"   -font FONT          : font for graphic window\n");
#else
//    fprintf(stderr,"   -font FONT          : font for graphic, supply style (decorative,dontcare,\n");
//    fprintf(stderr,"                         modern,roman,script or swiss) and size, e.g. swiss10\n");
#endif
    fprintf(stderr,"   -docu NAME          : print embedded docu of program or library\n");
    fprintf(stderr,"   -check              : check for possible compatibility problems\n");
    fprintf(stderr,"   --                  : pass any subsequent words as arguments to yab\n");
    fprintf(stderr,"   -librarypath PATH   : directory to search libraries not found in\n");
    fprintf(stderr,"                         current dir (default %s)\n",library_default);
    fprintf(stderr,"   -enter              : waits for the user to press the return key after\n");
    fprintf(stderr,"                         the program finished\n");
    fprintf(stderr,"\n");
  } else {
    sprintf(string,"unknown or ambigous option '%s' " YABFORHELP,ooop);
    error(ERROR,string);
  }
}


int equal(char *a,char *b,int min) /* helper for processing options */
{
  int len;
  if (b[0]=='-' && b[1]=='-' && b[2]) b++;
  if (min<0) {
    min=-min;
    len=min;
  } else {
    len=strlen(b);
  }

  return (!strncmp(a,b,len) && len>=min);
}


#ifdef WINDOWS
static void chop_command(char *command,int *argc,char ***argv)
     /* chop the WIN95-commandline into seperate strings */
{
  int i,j,count;
  int quote;
  char c,last;
  char *curr;
  char **list;
  
  /* count, how many arguments */
  count=i=0;
  last=' ';
  quote=FALSE;
  while((c=*(command+i))!='\0') {
    if (!quote && c!=' ' && last==' ') count++;
    if (c=='\"') quote=!quote;
    last=c;
    i++;
  }
  
  /* fill yabasic into argv[0] */
  *argv=my_malloc((count+1)*sizeof(char *));
  list=*argv;
  *argc=count+1;
  *list=my_strdup("yabasic");
  
  /* fill in other strings */
  i=0;
  count=1;
  last=' ';
  quote=FALSE;
  do {
    c=*(command+i);
    if (!quote && c!=' ' && last==' ') j=i;
    if (c=='\"') {
      quote=!quote;
      if (quote) j++;
    }
    if (((c==' ' && !quote) || c=='\0') && last!=' ') {
      *(list+count)=my_malloc((i-j+1)*sizeof(char));
      strncpy(*(list+count),command+j,i-j);
      curr=*(list+count)+i-j;
      *curr='\0';
      if (*(curr-1)=='\"') *(curr-1)='\0';
      count++;
    }
    last=c;
    i++;
  } while(c!='\0');
}
#endif


static void end_it(void) /* perform shutdown-operations */
{
  char l[2];
#ifdef UNIX
  int status;
#ifndef BEOS
  if (winpid==0 || termpid==0 || backpid==0) 
  	yi_exit(1,yab);
  if (backpid>0) {
    kill(backpid,SIGTERM);
    waitpid(backpid,&status,0);
    backpid=-1;
  }
#else
  if(enterkeyflag)
  {
	  printf("---Press RETURN to continue ");
	  getchar();
	  // fgets(1,2,stdin);
  }
  if (winpid==0 || termpid==0) yi_exit(1,yab);
#endif
  if ((curinized || winopened) && endreason!=erREQUEST) {
#else
    if (!Commandline && endreason!=erREQUEST) {
#endif
      myswitch(STDIO_STREAM);
      onestring("---Program done, press RETURN---\n");
#ifdef WINDOWS
      SetConsoleMode(ConsoleInput,InitialConsole & (~ENABLE_ECHO_INPUT));
      FlushConsoleInputBuffer(ConsoleInput);
#endif
      fgets(l,2,stdin);
#ifdef WINDOWS
      if (wthandle!=INVALID_HANDLE_VALUE) TerminateThread(wthandle,0);
#endif
#ifdef UNIX
    }
#else
  }
#endif

#ifdef UNIX
#ifdef BUILD_NCURSES
  if (curinized) endwin();
#endif
#else
  if (printerfont) DeleteObject(printerfont);
  if (myfont) DeleteObject(myfont);
  if (printer) DeleteDC(printer);
#endif
  yi_exit(exitcode,yab);
}


static void initialize(void) 
     /* give correct values to pointers etc ... */
{
  struct symbol *s;
  struct stackentry *base;
  int i;

  /* install exception handler */
  /*signal(SIGFPE,signal_handler);
  signal(SIGSEGV,signal_handler);
  signal(SIGINT,signal_handler);
#ifdef SIGHUP
  signal(SIGHUP,signal_handler);
#endif
#ifdef SIGQUIT
  signal(SIGQUIT,signal_handler);
#endif
#ifdef SIGABRT
  signal(SIGABRT,signal_handler);
#endif */
  
  /* initialize error handling: no errors seen 'til now */
  errorlevel=DEBUG;  
  debug_count=0;
  note_count=0;
  warning_count=0;
  error_count=0;
  
  /* initialize stack of symbol lists */
  pushsymlist();

  /* initialize random number generator */
  srand((unsigned)time(NULL));
  
  /* initialize numeric stack */
  /* create first : */
  stackroot=(struct stackentry *)my_malloc(sizeof(struct stackentry)); 
  stackroot->next=NULL;
  stackroot->prev=NULL;
  stackhead=stackroot; /* stack of double values */
  
  /* initialize command stack */
  /* create first: */
  cmdroot=(struct command *)my_malloc(sizeof(struct command)); 
  cmdroot->next=cmdroot->prev=NULL;
  
  /* initialize random number generator */
  srand((unsigned int)time(NULL));
  
  /* specify default text-alignement and window origin */
  /*
  text_align=my_strdup("lb");
  winorigin=my_strdup("lt");*/
  
  /* initialize stack */
  base=push();
  base->type=stROOT; /* push nil, so that pop will not crash */
  cmdhead=cmdroot; /* list of commands */;
  commandcount=0;
  
  /* add internal string variables */
  s=get_sym("yabos$",sySTRING,amADD_GLOBAL);
  if (s->pointer) my_free(s->pointer);
#ifdef UNIX
#ifndef BEOS
  s->pointer=my_strdup("unix");
#elseifdef HAIKU
  s->pointer=my_strdup("Haiku");
#else
  s->pointer=my_strdup("BeOS");
#endif
#else
  s->pointer=my_strdup("windows");
#endif

  /* set default-scales for grafics */
  /*
  fontheight=10;
  winheight=100;
  winwidth=100;
#ifdef UNIX
  calc_psscale();
#endif*/

    
  /* file stuff */
  for(i=1;i<=9;i++) {
    streams[i]=NULL;
    stream_modes[i]=smCLOSED;
  }
  streams[0]=stdin;
  stream_modes[0]=smREAD | smWRITE | smREADWRITE;
#ifdef UNIX
  /* printerfile=NULL; /* no ps-file yet */
#endif
  
  /* array with explanation */
  for(i=cFIRST_COMMAND;i<=cLAST_COMMAND;i++) explanation[i]="???";
  explanation[cFIRST_COMMAND]="FIRST_COMMAND";
  explanation[cFINDNOP]="FINDNOP";
  explanation[cEXCEPTION]="EXCEPTION";
  explanation[cLABEL]="LABEL";
  explanation[cSUBLINK]="cSUBLINK";
  explanation[cTOKEN]="TOKEN";
  explanation[cTOKEN2]="TOKEN2";
  explanation[cTOKENALT]="TOKENALT";
  explanation[cTOKENALT2]="TOKENALT2";
  explanation[cSPLIT]="SPLIT";
  explanation[cSPLIT2]="SPLIT2";
  explanation[cSPLITALT]="SPLITALT";
  explanation[cSPLITALT2]="SPLITALT2";
  explanation[cGOTO]="GOTO";
  explanation[cQGOTO]="QGOTO";
  explanation[cGOSUB]="GOSUB";
  explanation[cQGOSUB]="QGOSUB";
  explanation[cCALL]="CALL";
  explanation[cQCALL]="QCALL";
  explanation[cRETURN]="RETURN";
  explanation[cRET_FROM_FUN]="RET_FROM_FUN";
  explanation[cRETVAL]="RETVAL";
  explanation[cSWAP]="SWAP";
  explanation[cDECIDE]="DECIDE";
  explanation[cANDSHORT]="ANDSHORT";
  explanation[cORSHORT]="ORSHORT";
  explanation[cSKIPPER]="SKIPPER";
  explanation[cSKIPONCE]="SKIPONCE";
  explanation[cRESETSKIPONCE]="RESETSKIPONCE";
  explanation[cNOP]="NOP";
  explanation[cEND_FUNCTION]="END_FUNCTION";
  explanation[cDIM]="DIM";
  explanation[cFUNCTION]="FUNCTION";
  explanation[cDOARRAY]="DOARRAY";
  explanation[cDBLADD]="DBLADD";
  explanation[cDBLMIN]="DBLMIN";
  explanation[cDBLMUL]="DBLMUL";
  explanation[cDBLDIV]="DBLDIV";
  explanation[cDBLPOW]="DBLPOW";
  explanation[cNEGATE]="NEGATE";
  explanation[cPUSHDBLSYM]="PUSHDBLSYM";
  explanation[cREQUIRE]="REQUIRE";
  explanation[cCLEARREFS]="CLEARREFS";
  explanation[cPUSHSYMLIST]="PUSHSYMLIST";
  explanation[cPOPSYMLIST]="POPSYMLIST";
  explanation[cMAKELOCAL]="MAKELOCAL";
  explanation[cNUMPARAM]="NUMPARAM";
  explanation[cMAKESTATIC]="MAKESTATIC";
  explanation[cARRAYLINK]="ARRAYLINK";
  explanation[cPUSHARRAYREF]="PUSHARRAYREF";
  explanation[cARDIM]="ARRAYDIMENSION";
  explanation[cARSIZE]="ARRAYSIZE";
  explanation[cUSER_FUNCTION]="USER_FUNCTION";
  explanation[cFUNCTION_OR_ARRAY]="FUNCTION_OR_ARRAY";
  explanation[cSTRINGFUNCTION_OR_ARRAY]="STRINGFUNCTION_OR_ARRAY";
  explanation[cPUSHFREE]="PUSHFREE";
  explanation[cPOPDBLSYM]="POPDBLSYM";
  explanation[cPOP]="POP";
  explanation[cPUSHDBL]="PUSHDBL";
  explanation[cPOKE]="POKE";
  explanation[cPOKEFILE]="POKEFILE";
  explanation[cAND]="AND";
  explanation[cOR]="OR";
  explanation[cNOT]="NOT";
  explanation[cLT]="LT";
  explanation[cGT]="GT";
  explanation[cLE]="LE";
  explanation[cGE]="GE";
  explanation[cEQ]="EQ";
  explanation[cNE]="NE";
  explanation[cSTREQ]="STREQ";
  explanation[cSTRNE]="STRNE";
  explanation[cPUSHSTRSYM]="PUSHSTRSYM";
  explanation[cPOPSTRSYM]="POPSTRSYM";
  explanation[cPUSHSTR]="PUSHSTR";
  explanation[cCONCAT]="CONCAT";
  explanation[cPUSHSTRPTR]="PUSHSTRPTR";
  explanation[cCHANGESTRING]="CHANGESTRING";
  explanation[cGLOB]="GLOB";
  explanation[cPRINT]="PRINT";
  explanation[cREAD]="READ";
  explanation[cRESTORE]="RESTORE";
  explanation[cQRESTORE]="QRESTORE";
  explanation[cREADDATA]="READDATA";
  explanation[cONESTRING]="ONESTRING";
  explanation[cDATA]="DATA";
  explanation[cOPEN]="OPEN";
  explanation[cCHECKOPEN]="CHECKOPEN";
  explanation[cCHECKSEEK]="CHECKSEEK";
  explanation[cCOMPILE]="COMPILE";
  explanation[cEXECUTE]="EXECUTE";
  explanation[cEXECUTE2]="EXECUTE$";
  explanation[cCLOSE]="CLOSE";
  explanation[cSEEK]="SEEK";
  explanation[cSEEK2]="SEEK2";
  explanation[cPUSHSTREAM]="cPUSHSTREAM";
  explanation[cPOPSTREAM]="cPOPSTREAM";
  explanation[cWAIT]="WAIT";
  explanation[cBELL]="BELL";
  explanation[cMOVE]="MOVE";
  explanation[cCLEARSCR]="CLEARSCR";
  explanation[cOPENWIN]="OPENWIN";
  explanation[cDOT]="DOT";
  explanation[cPUTCHAR]="PUTCHAR";
  explanation[cLINE]="LINE";
  explanation[cCIRCLE]="CIRCLE";
  explanation[cTEXT]="TEXT";
  explanation[cCLOSEWIN]="CLOSEWIN";
  explanation[cTESTEOF]="TESTEOF";
  explanation[cCOLOUR]="COLOUR";
  explanation[cDUPLICATE]="DUPLICATE";
  explanation[cDOCU]="DOCU";
  explanation[cEND]="END";
  explanation[cEXIT]="EXIT";
  explanation[cBIND]="BIND";
  explanation[cERROR]="ERROR";
  explanation[cSTARTFOR]="STARTFOR";
  explanation[cFORCHECK]="FORCHECK";
  explanation[cFORINCREMENT]="FORINCREMENT";
  explanation[cBREAK_MARK]="BREAK_MARK";
  explanation[cPUSH_SWITCH_MARK]="PUSH_SWITCH_MARK";
  explanation[cCLEAN_SWITCH_MARK]="CLEAN_SWITCH_MARK";
  explanation[cSWITCH_COMPARE]="SWITCH_COMPARE";
  explanation[cNEXT_CASE]="NEXT_CASE";
  explanation[cBREAK]="BREAK";
  explanation[cMINOR_BREAK]="MINOR_BREAK";
  explanation[cCONTINUE]="CONTINUE";
  explanation[cBREAK_HERE]="BREAK_HERE";
  explanation[cCONTINUE_HERE]="CONTINUE_HERE";
  explanation[cBREAK_MARK]="BREAK_MARK";
  explanation[cCONTINUE_CORRECTION]="CONTINUE_CORRECTION";
  explanation[cLAST_COMMAND]="???";
  
  ykey[kERR]="error";
  ykey[kUP]="up";
  ykey[kDOWN]="down";
  ykey[kLEFT]="left";
  ykey[kRIGHT]="right";
  ykey[kDEL]="del";
  ykey[kINS]="ins";
  ykey[kCLEAR]="clear";
  ykey[kHOME]="home";
  ykey[kEND]="end";
  ykey[kF0]="f0";
  ykey[kF1]="f1";
  ykey[kF2]="f2";
  ykey[kF3]="f3";
  ykey[kF4]="f4";
  ykey[kF5]="f5";
  ykey[kF6]="f6";
  ykey[kF7]="f7";
  ykey[kF8]="f8";
  ykey[kF9]="f9";
  ykey[kF10]="f10";
  ykey[kF11]="f11";
  ykey[kF12]="f12";
  ykey[kF13]="f13";
  ykey[kF14]="f14";
  ykey[kF15]="f15";
  ykey[kF16]="f16";
  ykey[kF17]="f17";
  ykey[kF18]="f18";
  ykey[kF19]="f19";
  ykey[kF20]="f20";
  ykey[kF21]="f21";
  ykey[kF22]="f22";
  ykey[kF23]="f23";
  ykey[kF24]="f24";
  ykey[kBACKSPACE]="backspace";
  ykey[kSCRNDOWN]="scrndown";
  ykey[kSCRNUP]="scrnup";
  ykey[kENTER]="enter";
  ykey[kESC]="esc";
  ykey[kTAB]="tab";
  ykey[kLASTKEY]="";
}

void signal_handler(int sig)   /* handle signals */
{
  signal(sig,SIG_DFL);
  if (program_state==FINISHED) {
    yi_exit(1,yab);
  }
  signal_arrived=sig;

#ifdef WINDOWS
  if (signal_arrived) {
    SuspendThread(mainthread);
    if (wthandle!=INVALID_HANDLE_VALUE) SuspendThread(wthandle);
    if (kthandle!=INVALID_HANDLE_VALUE) TerminateThread(kthandle,0);
  }
#endif
  
  switch(sig) {
  case SIGFPE:
    error(FATAL,"floating point exception, cannot proceed.");
  case SIGSEGV:
    error(FATAL,"segmentation fault, cannot proceed.");
  case SIGINT:
#ifdef UNIX
#ifndef BEOS
    if (winpid==0 || termpid==0 || backpid==0) yi_exit(1,yab);
#endif
#endif
    error(FATAL,"keyboard interrupt, cannot proceed.");
#ifdef SIGHUP
  case SIGHUP:
    error(FATAL,"received signal HANGUP, cannot proceed.");
#endif
#ifdef SIGQUIT
  case SIGQUIT:
    error(FATAL,"received signal QUIT, cannot proceed.");
#endif
#ifdef SIGABRT
  case SIGABRT:
    error(FATAL,"received signal ABORT, cannot proceed.");
#endif
  default:
    break;
  }
}


static void run_it(YabInterface* yab)
     /* execute the compiled code */
{
  int l=0;

  current=cmdroot; /* start with first comand */
  if (print_docu) { /* don't execute program, just print docu */
    while(current!=cmdhead) {
      if (current->type==cDOCU) {
	if (infolevel>=DEBUG) std_diag("executing",current->type,current->name); 
	printf("%s\n",(char *)current->pointer);
        l++;
        if (hold_docu && !(l%24)) {
	  printf("---Press RETURN to continue ");
	  fgets(string,20,stdin);
        }
      } else {
	if (infolevel>=DEBUG) std_diag("skipping",current->type,current->name); 
      }
      current=current->next;
    }
    if (!l) printf("---No embbeded documentation\n");
    if (hold_docu) {
      printf("---End of embbedded documentation, press RETURN ");
      fgets(string,20,stdin);
    }
  } else {
    while(current!=cmdhead && endreason==erNONE) {
      if (infolevel>=DEBUG) std_diag("executing",current->type,current->name); 
      switch(current->type) {
      case cGOTO:case cQGOTO:case cGOSUB:case cQGOSUB:case cCALL:case cQCALL:
	jump(current); DONE;
      case cEXCEPTION:
	exception(current); DONE;
      case cSKIPPER: 
	skipper(); break;
      case cSKIPONCE:
	skiponce(current); DONE;
      case cRESETSKIPONCE:
	resetskiponce(current); DONE;
      case cNEXT_CASE:
	next_case(); DONE;
      case cBREAK:case cMINOR_BREAK: 
	mybreak(current); DONE;
      case cSWITCH_COMPARE:
	switch_compare(); DONE;
      case cPUSH_SWITCH_MARK:
	push_switch_mark(); DONE;
      case cCLEAN_SWITCH_MARK:
	clean_switch_mark(current); DONE;
      case cCONTINUE:
	mycontinue(current); DONE;
      case cFINDNOP:
	findnop(); DONE;
      case cFUNCTION_OR_ARRAY: case cSTRINGFUNCTION_OR_ARRAY:
	function_or_array(current);
	break; /* NOT 'DONE' ! */
      case cLABEL:case cDATA:case cNOP:case cUSER_FUNCTION:
      case cSUBLINK:case cEND_FUNCTION: case cDOCU: case cBREAK_HERE: 
      case cCONTINUE_HERE:case cBREAK_MARK:case cCONTINUE_CORRECTION:
	DONE;
      case cERROR:
	do_error(current); DONE;
      case cCOMPILE:
	compile(); DONE;
      case cEXECUTE: case cEXECUTE2:
	execute(current); DONE;
      case cRETURN:case cRET_FROM_FUN:
	myreturn(current); DONE;
      case cRETVAL:
	retval(current); DONE;
      case cPUSHDBLSYM: 
	pushdblsym(current); DONE;
      case cPUSHDBL:
	pushdbl(current); DONE;
      case cPOPDBLSYM:
	popdblsym(current); DONE;
      case cPOP:
	pop(stANY); DONE;
      case cPOPSTRSYM:
	popstrsym(current); DONE;
      case cPUSHSTRSYM: 
	pushstrsym(current); DONE;
      case cPUSHSTR:
	pushstr(current); DONE;
      case cCLEARREFS:
	clearrefs(current); DONE;
      case cPUSHSYMLIST:
	pushsymlist(); DONE;
      case cPOPSYMLIST:
	popsymlist(); DONE;
      case cREQUIRE:
	require(current); DONE;
      case cMAKELOCAL:
	makelocal(current); DONE;
      case cNUMPARAM:
	numparam(current); DONE;
      case cMAKESTATIC:
	makestatic(current); DONE;
      case cARRAYLINK:
	arraylink(current); DONE;
      case cPUSHARRAYREF:
	pusharrayref(current); DONE;
      case cTOKEN: case cTOKEN2: case cSPLIT: case cSPLIT2:
	token(current); DONE;
      case cTOKENALT: case cTOKENALT2: case cSPLITALT: case cSPLITALT2: 
	tokenalt(current); DONE;
      case cARDIM: case cARSIZE:
	query_array(current); DONE;
      case cPUSHFREE:
	push()->type=stFREE; DONE;
      case cCONCAT:
	concat(); DONE;
      case cPRINT:
	print(current); DONE;
      case cMOVE:
	mymove(); DONE;
      case cCOLOUR:
	colour(current); DONE;
      case cCLEARSCR:
	clearscreen(); DONE;
      case cONESTRING:
	onestring(current->pointer); DONE;
      case cTESTEOF:
	testeof(current); DONE;
      case cOPEN:
	myopen(current); DONE;
      case cCHECKOPEN:case cCHECKSEEK:
	checkopen(); DONE;
      case cCLOSE:
	myclose(); DONE;
      case cSEEK:case cSEEK2:
	myseek(current); DONE;
      case cPUSHSTREAM:
	push_switch(current); DONE;
      case cPOPSTREAM:
	pop_switch(); DONE;
      case cCHKPROMPT:
	chkprompt(); DONE;
      case cREAD:
	myread(current); DONE;
      case cRESTORE:case cQRESTORE:
	restore(current); DONE;
      case cREADDATA:
	readdata(current); DONE;
      case cDBLADD:case cDBLMIN:case cDBLMUL:case cDBLDIV:case cDBLPOW:
	dblbin(current); DONE;
      case cNEGATE:
	negate(); DONE;
      case cEQ:case cNE:case cGT:case cGE:case cLT:case cLE:
	dblrelop(current); DONE;
      case cSTREQ:case cSTRNE:case cSTRLT:case cSTRLE:case cSTRGT:case cSTRGE:
	strrelop(current); DONE;
      case cAND:case cOR:case cNOT:
	boole(current); DONE;
      case cFUNCTION:
	function(current, yab); DONE;
      case cGLOB:
	glob(); DONE;
      case cDOARRAY:
	doarray(current); DONE;
      case cCHANGESTRING:
	changestring(current); DONE;
      case cPUSHSTRPTR:
	pushstrptr(current); DONE;
      case cDIM:
	dim(current); DONE;
      case cDECIDE:
	decide(); DONE;
      case cANDSHORT:case cORSHORT:
	logical_shortcut(current); DONE;
      case cOPENWIN:
	openwin(current, yab); DONE;
      case cBUTTON:
	createbutton(current, yab); DONE;
      case cALERT:
	createalert(current, yab); DONE;
      case cWINSET1:
	winset1(current, yab); DONE;
      case cWINSET2:
	winset2(current, yab); DONE;
      case cWINSET3:
	winset3(current, yab); DONE;
      case cWINSET4:
	winset4(current, yab); DONE;
      case cWINCLEAR:
	winclear(current, yab); DONE;
      case cLAYOUT:
	setlayout(current,yab); DONE;
      case cTEXTEDIT:
	textedit(current,yab); DONE;
      case cTEXTADD:
	textadd(current,yab); DONE;
      case cTEXTSET:
	textset(current,yab); DONE;
      case cTEXTSET2:
	textset2(current,yab); DONE;
      case cTEXTSET3:
	textset3(current,yab); DONE;
      case cTEXTCOLOR1:
	textcolor1(current,yab); DONE;
      case cTEXTCOLOR2:
	textcolor2(current,yab); DONE;
      case cTEXTCLEAR:
	textclear(current,yab); DONE;
      case cDRAWSET1:
	drawset1(current,yab); DONE;
      case cDRAWSET2:
	drawset2(current,yab); DONE;
      case cDRAWSET3:
	drawset3(current,yab); DONE;
      case cTEXT:
	createtext(current, yab); DONE;
      case cTEXT2:
        text2(current, yab); DONE;
      case cTEXTALIGN:
        textalign(current, yab); DONE;
      case cTEXTURL1:
		texturl1(current, yab); DONE;
      case cTEXTURL2:
		texturl2(current, yab); DONE;
      case cLOCALIZE:
		localize(); DONE;
      case cLOCALIZESTOP:
		localizestop(); DONE;
      case cLOCALIZE2:
		localize2(current, yab); DONE;
      case cMENU:
		createmenu(current, yab); DONE;
      case cCHECKBOX:
		createcheckbox(current, yab); DONE;
      case cRADIOBUTTON:
		createradiobutton(current, yab); DONE;
      case cTEXTCONTROL:
		createtextcontrol(current, yab); DONE;
      case cLAUNCH:
		launch(current, yab); DONE;
      case cLISTBOX:
        createlistbox(current, yab); DONE;
      case cDROPBOX:
        createdropbox(current, yab); DONE;
      case cITEMADD:
        createitem(current, yab); DONE;
      case cITEMDEL:
        removeitem(current, yab); DONE;
      case cITEMCLEAR:
        clearitems(current, yab); DONE;
      case cDRAWRECT:
		drawrect(current, yab); DONE;
      case cDRAWTEXT:
		drawtext(current, yab); DONE;
      case cDRAWCLEAR:
		drawclear(current, yab); DONE;
      case cDOT:
		drawdot(current,yab); DONE;
      case cPUTCHAR:
		putchars(); DONE;
      case cLINE:
		drawline(current,yab); DONE;
      case cCIRCLE:
		drawcircle(current,yab); DONE;
      case cCURVE:
		drawcurve(current,yab); DONE;
      case cELLIPSE:
		drawellipse(current,yab); DONE;
/*      case cTEXT:
	text(current); DONE;*/
      case cCLOSEWIN:
		closewin(current,yab); DONE;
      case cVIEW:
		view(current, yab); DONE;
      case cBOXVIEW:
		boxview(current, yab); DONE;
	  case cBOXVIEWSET:
		boxviewset(current, yab); DONE;	
	  case cTAB:
		tab(current, yab); DONE;
      case cTABSET:
		tabset(current, yab); DONE;
      case cTABADD:
		tabadd(current, yab); DONE;
      case cTABDEL:
		tabdel(current, yab); DONE;
      case cSLIDER1:
		slider1(current, yab); DONE;
      case cSLIDER2:
		slider2(current, yab); DONE;
      case cSLIDER3:
		slider3(current, yab); DONE;
      case cSLIDER4:
		slider4(current, yab); DONE;
      case cSLIDER5:
		slider5(current, yab); DONE;
      case cSLIDER6:
		slider6(current, yab); DONE;
      case cOPTION1:
		option1(current, yab); DONE;
      case cOPTION2:
		option2(current, yab); DONE;
      case cOPTION3:
		option3(current, yab); DONE;
      case cOPTION4:
		option4(current, yab); DONE;
      case cOPTION5:
		option5(current, yab); DONE;
      case cDROPZONE:
		dropzone(current, yab); DONE;
      case cCOLORCONTROL1:
		colorcontrol1(current, yab); DONE;
      case cCOLORCONTROL2:
		colorcontrol2(current, yab); DONE;
      case cTEXTCONTROL2:
		textcontrol2(current, yab); DONE;
      case cTEXTCONTROL3:
		textcontrol3(current, yab); DONE;
      case cTEXTCONTROL4:
		textcontrol4(current, yab); DONE;
	  case cTEXTCONTROL5:
		textcontrol5(current, yab); DONE;		
      case cTREEBOX1:
      	treebox1(current, yab); DONE;
      case cTREEBOX2:
      	treebox2(current, yab); DONE;
      case cTREEBOX3:
      	treebox3(current, yab); DONE;
      case cTREEBOX4:
      	treebox4(current, yab); DONE;
      case cTREEBOX5:
      	treebox5(current, yab); DONE;
      case cTREEBOX7:
      	treebox7(current, yab); DONE;
      case cTREEBOX8:
      	treebox8(current, yab); DONE;
      case cTREEBOX9:
      	treebox9(current, yab); DONE;
      case cTREEBOX10:
      	treebox10(current, yab); DONE;
      case cTREEBOX11:
      	treebox11(current, yab); DONE;
      case cTREEBOX12:
      	treebox12(current, yab); DONE;
      case cBUTTONIMAGE:
      	buttonimage(current, yab); DONE;
      case cCHECKBOXIMAGE:
        checkboximage(current, yab); DONE;
      case cCHECKBOXSET:
      	checkboxset(current, yab); DONE;
      case cRADIOSET:
      	radioset(current, yab); DONE;
      case cTOOLTIP:
        tooltip(current, yab); DONE;
      case cTOOLTIPCOLOR:
        tooltipcolor(current, yab); DONE;
      case cFILEBOX:
        filebox(current, yab); DONE;
      case cFILEBOXADD2:
        fileboxadd2(current, yab); DONE;
      case cFILEBOXCLEAR:
        fileboxclear(current, yab); DONE;
      case cCOLUMNBOXADD:
        columnboxadd(current, yab); DONE;
      case cCOLUMNBOXREMOVE:
      	columnboxremove(current, yab); DONE;
      case cCOLUMNBOXSELECT:
      	columnboxselect(current, yab); DONE;
      case cCOLUMNBOXCOLOR:
      	columnboxcolor(current, yab); DONE;
      case cDROPBOXSELECT:
      	dropboxselect(current, yab); DONE;
      case cDROPBOXREMOVE:
      	dropboxremove(current, yab); DONE;
      case cDROPBOXCLEAR:
      	dropboxclear(current, yab); DONE;
      case cPRINTERCONFIG:
      	printerconfig(current, yab); DONE;
      case cMENU2:
      	menu2(current, yab); DONE;
      case cMENU3:
      	menu3(current, yab); DONE;
      case cSUBMENU1:
      	submenu1(current, yab); DONE;
      case cSUBMENU2:
      	submenu2(current, yab); DONE;
      case cSUBMENU3:
      	submenu3(current, yab); DONE;
      case cCLIPBOARDCOPY:
      	clipboardcopy(current, yab); DONE;
      case cLISTSORT:
        listsort(current, yab); DONE;
      case cTREESORT:
        treesort(current, yab); DONE;
      case cCALENDAR:
      	calendar1(current, yab); DONE;
      case cCALENDARSET:
      	calendar3(current, yab); DONE;
      case cSCROLLBAR:
		scrollbar(current, yab); DONE;
      case cSCROLLBARSET1:
		scrollbarset1(current, yab); DONE;
      case cSCROLLBARSET2:
		scrollbarset2(current, yab); DONE;
      case cSCROLLBARSET3:
		scrollbarset3(current, yab); DONE;
      case cLISTBOXADD1:
        listboxadd1(current, yab); DONE;
      case cLISTBOXADD2:
        listboxadd2(current, yab); DONE;
      case cLISTBOXSELECT:
        listboxselect(current, yab); DONE;
      case cLISTBOXDEL2:
        listboxremove(current, yab); DONE;
      case cSOUNDSTOP:
      	soundstop(current, yab); DONE;
      case cSOUNDWAIT:
      	soundwait(current, yab); DONE;
      case cSHORTCUT:
        shortcut(current, yab); DONE;
      case cTREEBOX13:
      	treebox13(current, yab); DONE;
      case cDRAWSET4:
      	drawset4(current, yab); DONE;
      case cSPLITVIEW1:
      	splitview1(current, yab); DONE;
      case cSPLITVIEW2:
      	splitview2(current, yab); DONE;
      case cSPLITVIEW3:
      	splitview3(current, yab); DONE;
      case cSTACKVIEW1:
      	stackview1(current, yab); DONE;
      case cSTACKVIEW2:
      	stackview2(current, yab); DONE;
      case cSPINCONTROL1:
      	spincontrol1(current, yab); DONE;
      case cSPINCONTROL2:
      	spincontrol2(current, yab); DONE;
      case cBITMAP:
      	bitmap(current, yab); DONE;
      case cBITMAPDRAW:
      	bitmapdraw(current, yab); DONE;
      case cBITMAPDRAW2:
      	bitmapdraw2(current, yab); DONE;
      case cBITMAPGET:
      	bitmapget(current, yab); DONE;
      case cBITMAPGET2:
      	bitmapget2(current, yab); DONE;
      case cBITMAPGETICON:
        bitmapgeticon(current, yab); DONE;
      case cBITMAPDRAG:
      	bitmapdrag(current, yab); DONE;
      case cSCREENSHOT:
      	screenshot(current, yab); DONE;
      case cBITMAPREMOVE:
      	bitmapremove(current, yab); DONE;
      case cSTATUSBAR:
      	statusbar(current, yab); DONE;
      case cSTATUSBARSET:
      	statusbarset(current, yab); DONE;
      case cSTATUSBARSET2:
      	statusbarset2(current, yab); DONE;
      case cSTATUSBARSET3:
      	statusbarset3(current, yab); DONE;
      case cCANVAS:
      	canvas(current, yab); DONE;
      case cMOUSESET:
        mouseset(current, yab); DONE;
      case cATTRIBUTE1:
	attribute1(current, yab); DONE;
      case cATTRIBUTECLEAR:
	attributeclear(current, yab); DONE;
      case cWAIT:
	mywait(); DONE;
      case cRESTORE2: {
	struct command *c;
	char *s;
	c = add_command(cRESTORE, FALSE);
	s = pop(stSTRING)->pointer;
	if(!strcmp(s, ""))
		c->pointer=my_strdup(s);
	else
		c->pointer=my_strdup(dotify(s, TRUE));
	restore(c);
	} DONE;
      case cBELL:
	mybell(); DONE;
      case cPOKE:
	poke(current); DONE;
      case cPOKEFILE:
	pokefile(current); DONE;
      case cSWAP:
	swap(); DONE;
      case cDUPLICATE:
	duplicate(); DONE;
      case cFORCHECK:
	forcheck(); DONE;
      case cFORINCREMENT:
	forincrement(); DONE;
      case cSTARTFOR:
	startfor(); DONE;
      case cBIND:
	mybind(pop(stSTRING)->pointer); DONE;
      case cEND:
	endreason=erEOF; break;
      case cEXIT:
	exitcode=(int)pop(stNUMBER)->value;endreason=erREQUEST; break;
      default:
	sprintf(string,"Command %s (%d, right before '%s') not implemented",
		explanation[current->type],current->type,explanation[current->type+1]);
	error(ERROR,string);
      }
    }
  }
  program_state=FINISHED;
  switch(errorlevel) {
  case NOTE:case DEBUG: 
    error(NOTE,"Program ended normally."); break;
  case WARNING:
    error(WARNING,"Program ended with a warning"); break;
  case ERROR:
    error(ERROR,"Program stopped due to an error"); break;
  case FATAL: /* should not come here ... */
    error(FATAL,"Program terminated due to FATAL error");
    break;
  }
}


void error(int severity, char *messageline) 
     /* reports an basic error to the user and possibly exits */
{
  if (program_state==COMPILING)
    error_with_line(severity,messageline,mylineno);
  else if (program_state==RUNNING && current->line>0) 
    error_with_line(severity,messageline,current->line);
  else 
    error_with_line(severity,messageline,-1);
}


void error_with_line(int severity, char *message,int line) 
     /* reports an basic error to the user and possibly exits */
{
  char *callstack;
  char *stext;
  char *f=NULL;
  int l;
  static int lastline;
  static int first=TRUE;

  if (severity<=infolevel) {
#ifdef UNIX
#ifdef BUILD_NCURSES
    if (curinized) reset_shell_mode();
#endif
#endif
    
    switch(severity) {
    case(INFO): 
      stext="---Info"; 
      break;
    case(DUMP): 
      stext="---Dump"; 
      break;
    case(DEBUG): 
      stext="---Debug"; 
      debug_count++;
      break;
    case(NOTE): 
      stext="---Note"; 
      note_count++;
      break;
    case(WARNING): 
      stext="---Warning"; 
      warning_count++;
      break;
    case(ERROR): 
      stext="---Error"; 
      error_count++;
      break;
    case(FATAL): 
      stext="---Fatal"; 
      break;
    }
    fprintf(stderr,"%s",stext);
    if (line>=0) {
      if (program_state==COMPILING) {
	f=currlib->l;
	l=mylineno;
      } else if (program_state==RUNNING && current->line>0) {
	f=current->lib->l;
	l=current->line;
      }
      if (f) {
	if (first || lastline!=l) {
	  fprintf(stderr," in %s, line %d",f,l);
	}
	lastline=l;
	first=FALSE;
      }
    }
    fprintf(stderr,": %s\n",message);
    if (program_state==RUNNING && severity<=ERROR && severity!=DUMP) dump_sub(1);
  }
  if (severity<errorlevel) errorlevel=severity;
  if (severity<=ERROR) {
    program_state=FINISHED;
    endreason=erERROR;
    exitcode=1;
  }
  if (severity<=FATAL) {
    program_state=FINISHED;
    fprintf(stderr,"---Immediate exit to system, due to a fatal error.\n");
    end_it();
  }
#ifdef UNIX
#ifdef BUILD_NCURSES
  if (curinized && severity<=infolevel) reset_prog_mode();
#endif
#endif
}


char *my_strndup(char *arg,int len) /*  own version of strndup */
{
  char *copy;
  
  copy=my_malloc(len+1);
  
  strncpy(copy,arg,len);
  copy[len]='\0';
  
  return copy;
}


char *my_strdup(char *arg)  /* my own version of strdup, checks for failure */
{
  int l;
  
  if (!arg) return my_strndup("",0);
  l=strlen(arg);
  return my_strndup(arg,l);
}


void *my_malloc(unsigned num) /* Alloc memory and issue warning on failure */
{
  void *room;
  
  room=malloc(num+sizeof(int));
  if (room==NULL) {
    sprintf(string,"Can't malloc %d bytes of memory",num);
    error(FATAL,string);
  }
  return room;
}


void my_free(void *mem) /* free memory */
{
  free(mem);
}

      
struct libfile_name *new_file(char *l,char *s) /* create a new structure for library names */
{
  struct libfile_name *new;
  struct libfile_name *curr;
  static struct libfile_name *last=NULL;
  int start,end;
  
  /* check, if library has already been included */
  for(curr=libfile_stack[0];curr;curr=curr->next) {
    if (!strcmp(curr->l,l)) {
      if (is_bound) 
	return curr;
      else
	return NULL;
    }
  }

  new=my_malloc(sizeof(struct libfile_name));
  new->next=NULL;
  new->lineno=1;
  if (last) last->next=new;
  last=new;
  
  new->l=my_strdup(l);
  new->llen=strlen(new->l);

  if (s) {
    new->s=my_strdup(s);
  } else { 
    /* no short name supplied get piece from l */
    end=strlen(l);
    for(start=end;start>0;start--) {
      if (l[start-1]=='\\' || l[start-1]=='/') break;
      if (l[start]=='.') end=start;
    }
    end--;
    new->s=my_malloc(end-start+2);
    strncpy(new->s,new->l+start,end-start+1);
    new->s[end-start+1]='\0';
  }
  new->slen=strlen(new->s);
  new->datapointer=new->firstdata=NULL;

  return new;
}


char *dotify(char *name,int addfun) /* add library name, if not already present */
{
  static char buff[200];
  if (!strchr(name,'.')) {
    strcpy(buff,currlib->s);
    strcat(buff,".");
    strcat(buff,name);
  } else {
    strcpy(buff,name);
  }
  if (addfun && !strchr(name,'@')) {
    strcat(buff,"@");
    strcat(buff,current_function);
  }
  return buff;
}


char *strip(char *name) /* strip down to minimal name */
{
  static char buff[300];
  char *at,*dot;
  
  if (infolevel>=DEBUG) return name;
  dot=strchr(name,'.');
  if (dot)
    strcpy(buff,dot+1);
  else
    strcpy(buff,name);
  at=strchr(buff,'@');
  if (at) *at='\0';
    
  return buff;
}


void do_error(struct command *cmd) /* issue user defined error */
{
  struct stackentry *s;
  struct command *r;
  
  s=stackhead;
  while(s!=stackroot) {
    if (s->type==stRETADDCALL) {
      r=s->pointer;
      cmd->line=r->line;
      cmd->lib=r->lib;
      break;
    }
    s=s->prev;
  }
  error(ERROR,pop(stSTRING)->pointer);
}


void compile() /* create s subroutine at runtime */
{
  open_string(pop(stSTRING)->pointer);
  yyparse();
  add_command(cEND,NULL);
}


void create_execute(int string) /* create command 'cEXECUTESUB' */
{
  struct command *cmd;
  
  cmd=add_command(string?cEXECUTE2:cEXECUTE,NULL);
  cmd->pointer=my_strdup(dotify("",FALSE));
}


void execute(struct command *cmd) /* execute a subroutine */
{
  struct stackentry *st,*ret;
  char *fullname,*shortname;
  struct command *newcurr;
  
  st=stackhead;
  do {st=st->prev;} while(st->type!=stFREE);
  st=st->next;
  if (st->type!=stSTRING) {
    error(ERROR,"need a string as a function name");
    return;
  }
  shortname=st->pointer;
  if ((shortname[strlen(shortname)-1]=='$')!=(cmd->type==cEXECUTE2)) {
    if (cmd->type==cEXECUTE2) 
      sprintf(string,"expecting the name of a string function (not '%s')",shortname);
    else
      sprintf(string,"expecting the name of a numeric function (not '%s')",shortname);
    error(ERROR,string);
    return;
  }
  fullname=my_malloc(strlen(cmd->pointer)+strlen(shortname)+2);
  strcpy(fullname,cmd->pointer);
  strcat(fullname,shortname);
  free(st->pointer);
  st->type=stFREE;
  newcurr=search_label(fullname,smSUB);
  if (!newcurr) {
    sprintf(string,"subroutine '%s' not defined",fullname);
    error(ERROR,string);
    return;
  }
  ret=push();
  ret->pointer=current;
  ret->type=stRETADDCALL;
  reshufflestack(ret);
  current=newcurr;
  free(fullname);
}


void create_docu(char *doc) /* create command 'docu' */
{
  struct command *cmd;
  static struct command *previous=NULL;

  if (inlib) return;
  cmd=add_command(cDOCU,NULL);
  cmd->pointer=doc;
  if (previous) 
    previous->nextassoc=cmd;
  else
    docuhead=cmd;
  previous=cmd;
  docucount++;
}


void create_docu_array(void) /* create array with documentation */
{  
  struct array *ar;
  struct command *doc;
  int i;

  /* create and prepare docu-array */
  ar=create_array('s',1);
  ar->bounds[0]=docucount+1;
  ar->pointer=my_malloc((docucount+1)*sizeof(char *));
  ((char **)ar->pointer)[0]=my_strdup("");
  doc=docuhead;
  i=1;
  while(doc) {
    ((char **)ar->pointer)[i]=doc->pointer;
    doc=doc->nextassoc;
    i++;
  }
  get_sym("main.docu$",syARRAY,amADD_GLOBAL)->pointer=ar;
} 


int isbound(void) /* check if this interpreter is bound to a program */
{
/*
  if (!interpreter_path || !interpreter_path[0]) {
    error(FATAL,"interpreter_path is not set !");
    return 0;
  }
  if (!(interpreter=fopen(interpreter_path,"r"))) {
    sprintf(string,"Couldn't open '%s' to check, if it is bound: %s",interpreter_path,my_strerror(errno));
    error(WARNING,string);
    return 0;
  }

  if(sizeof(myProg)>1)
  	return 1;
  else
  	return 0;
  */
  FILE *interpreter;
  int i;
  int c;
  int proglen=0;
  int bound=1;
  
  if (!interpreter_path || !interpreter_path[0]) {
    error(FATAL,"interpreter_path is not set !");
    return 0;
  }
  if (!(interpreter=fopen(interpreter_path,"r"))) {
    sprintf(string,"Couldn't open '%s' to check, if it is bound: %s",interpreter_path,my_strerror(errno));
    error(WARNING,string);
    return 0;
  }

  if (fseek(interpreter,0-strlen(YABMAGIC)-1,SEEK_END)) {
    sprintf(string,"Couldn't seek within '%s': %s",interpreter_path,my_strerror(errno));
    error(WARNING,string);
    return 0;
  }
  for(i=0;i<strlen(YABMAGIC);i++) {
    c=fgetc(interpreter);
    if (c==EOF || c!=(YABMAGIC)[i]) bound=0;
  }
  if (!bound) {
    fclose(interpreter);
    return bound;
  }
  
  if (fseek(interpreter,0-strlen(YABMAGIC)-5-8-1,SEEK_END)) {
    sprintf(string,"Couldn't seek within '%s': %s",interpreter_path,my_strerror(errno));
    error(WARNING,string);
    return 0;
  }
  if (!fscanf(interpreter,"%d",&proglen)) {
    error(WARNING,"Could not read length of embedded program");
    return 0;
  }

  if (fseek(interpreter,0-strlen(YABMAGIC)-5-8-5-5-proglen,SEEK_END)) {
    sprintf(string,"Couldn't seek within '%s': %s",interpreter_path,my_strerror(errno));
    error(WARNING,string);
    return 0;
  }

  if (infolevel>=NOTE) {
    error(NOTE,"Dumping the embedded program, that will be executed:");
    fprintf(stderr,"     ");
    for(i=0;i<proglen;i++) {
      c=fgetc(interpreter);
      fprintf(stderr,"%c",c);
      if (c=='\n' && i<proglen-1) fprintf(stderr,"     ");
    }
    error(NOTE,"End of program, that will be executed");
    if (fseek(interpreter,0-strlen(YABMAGIC)-5-8-5-5-proglen,SEEK_END)) {
      sprintf(string,"Couldn't seek within '%s': %s",interpreter_path,my_strerror(errno));
      error(WARNING,string);
      return 0;
    }
  }
  bound_program=interpreter;
  return 1;
}


static int mybind(char *bound) /* bind a program to the interpreter and save it */
{
  FILE *fyab;
  FILE *fprog;
  FILE *fbound;
  FILE *flib;
  int c;
  char *pc;
  int i;
  int proglen=0;

  if (interactive) {
    error(ERROR,"cannot bind a program when called interactive");
    return 0;
  }

  if (!strcmp(interpreter_path,bound)) {
    sprintf(string,"will not overwrite '%s' with '%s'",bound,interpreter_path);
    error(ERROR,string);
    return 0;
  }
  if (!strcmp(main_file_name,bound)) {
    sprintf(string,"will not overwrite '%s' with '%s'",bound,main_file_name);
    error(ERROR,string);
    return 0;
  }

  if (!(fyab=fopen(interpreter_path,"rb"))) {
    sprintf(string,"could not open '%s' for reading: %s",interpreter_path,my_strerror(errno));
    error(ERROR,string);
    return 0;
  }
  if (!(fprog=fopen(main_file_name,"rb"))) {
    sprintf(string,"could not open '%s' for reading: %s",main_file_name,my_strerror(errno));
    error(ERROR,string);
    fclose(fyab);
    return 0;
  }
  if (!(fbound=fopen(bound,"wb"))) {
    sprintf(string,"could not open '%s' for writing: %s",bound,my_strerror(errno));
    error(ERROR,string);
    fclose(fyab);
    fclose(fprog);
    return 0;
  }

  if (infolevel>=DEBUG) {
    sprintf(string,"binding %s and %s into %s",interpreter_path,main_file_name,bound);
    error(NOTE,string);
  }
  
  while((c=fgetc(fyab))!=EOF) {
    fputc(c,fbound);
  }
  for (i=1;i<libfile_chain_length;i++) {
     if (!(flib=fopen(libfile_chain[i]->l,"rb"))) {
      sprintf(string,"could not open '%s' for reading: %s",libfile_chain[i]->l,my_strerror(errno));
      error(ERROR,string);
      fclose(flib);
      return 0;
    }
    sprintf(string,"\nimport %s\n",libfile_chain[i]->s);
    for (pc=string;*pc;pc++) {
      fputc(*pc,fbound);
      proglen++;
    }
    while((c=fgetc(flib))!=EOF) {
      fputc(c,fbound);
      proglen++;
    }
  }

  for (pc="\nimport main\n";*pc;pc++) {
    fputc(*pc,fbound);
    proglen++;
  }
  for (pc="\nimport __END_OF_IMPORT\n";*pc;pc++) {
    fputc(*pc,fbound);
    proglen++;
  }    
  while((c=fgetc(fprog))!=EOF) {
    fputc(c,fbound);
    proglen++;
  }
  fprintf(fbound,"\nend\n");
  fprintf(fbound,"rem %08d\n",proglen);
  fprintf(fbound,"rem %s\n",YABMAGIC);
  fclose(fyab);
  fclose(fprog);
  fclose(fbound);
  
  return 1;
}

char *find_interpreter(char *name) /* find interpreter with full path */
{
  FILE *f;
  char *path=NULL;

#ifdef WINDOWS
  path=my_malloc(1000);
  GetModuleFileName(NULL,path,1000);
  if (f=fopen(path,"r")) {
    fclose(f);
    return path;
  } else {
    my_free(path);
    return my_strdup(name);
  }
  
#else	
  if (f=fopen(name,"r")) {
    fclose(f);
    path=my_strdup(name);
    return path;
  } else {
    char *from,*to,*try;
    from=to=path=getenv("PATH");
    try=my_malloc(strlen(path)+strlen(name)+2);
    to=strchr(to+1,':');
    while(to) {
      strncpy(try,from,to-from);
      try[to-from]=0;
      if (try[to-from-1]!='/') strcat(try,"/");
      strcat(try,name);
      if (f=fopen(try,"r")) {
	fclose(f);
	return try;
      }
      from=to+1;
      to=strchr(to+1,':');
    }
    return name;
  }
#endif
}

	
char *my_strerror(int err) { /* return description of error */
#ifdef WINDOWS
  return strerror(err);
#else
#ifdef HAVE_STRERROR
  return strerror(err);
#else
  char buff[100];
  sprintf(buff,"errno=%d",err);
  return buff;
#endif
#endif
}
