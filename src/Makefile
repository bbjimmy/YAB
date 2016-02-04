##
## yab Haiku Makefile
##
## (c) Jan Bungeroth 2009 - 2012
## Artistic License. 
##
## Use 
##   make 
## to compile yab for Haiku.
##
## Needs a valid installation of at least: gcc, flex, bison, perl, ncurses
##

##
## Haiku stuff
##
HAIKUTAB = YabTabView.o
HAIKUOPT = -DHAIKU -DLIBRARY_PATH=\"`finddir B_USER_SETTINGS_DIRECTORY`/yab\"


##
## Use our own column list view
## 
COLUMN = column/ColumnListView.o

##
## enable debug
##
# DBG = -g 
# 

##
## enable optimization
##
OPT = -O
#

##
## GCC Options
##
GCC = gcc
GCC_OPT = $(DBG) $(OPT) -I. -I/boot/home/config/include/ -I/boot/home/config/include/ncurses/ -DHAVE_CONFIG -DUNIX $(HAIKUOPT)
GPP = g++
GPP_OPT = $(DBG) $(OPT) -I. -DHAVE_CONFIG -DUNIX $(HAIKUOPT)
LD = g++
LD_OPT = -shared

##
## Libraries
##
##LIBPATH = -L/boot/home/config/lib
LIBPATHS = $(shell findpaths -a `getarch` B_FIND_PATH_LIB_DIRECTORY) .
LIBPATH=$(addprefix -L,$(LIBPATHS))
##LIBPATH = -L`finddir B_SYSTEM_LIB_DIRECTORY` ##/boot/system/lib    
LIB = -lbe -lroot -ltranslation -ltracker -lmedia  

## flags for flex (-d for debugging)
FLEXFLAGS = -i -I -L -s

## flags for bison (-t -v for debugging)
BISONFLAGS = -d -l -t -v

YAB_OBJECTS = YabInterface.o YabWindow.o YabView.o YabBitmapView.o YabText.o YabFilePanel.o YabFilePanelLooper.o YabList.o \
	function.o io.o graphic.o symbol.o bison.o \
	$(COLUMN) column/YabColumnType.o column/ColorTools.o \
	YabStackView.o SplitPane.o URLView.o YabControlLook.o $(HAIKUTAB) Spinner.o $(TABLIB) CalendarControl.o

##
## Compile and link
##
yab: libyab.so YabMain.o main.o bison.o flex.o RdefApply YAB.rdef
	$(GPP) $(GPP_OPT) -o $@ YabMain.o main.o bison.o flex.o $(LIBPATH) libyab.so $(LIB)
	LIBRARY_PATH=$$LIBRARY_PATH:. $@ RdefApply YAB.rdef $@
	addattr -t mime BEOS:TYPE application/x-vnd.be-elfexecutable $@

libyab.so: $(YAB_OBJECTS)
	$(LD) $(LD_OPT) -o $@ $+ $(LIBPATH) $(LIB)

YabMain.o: YabMain.cpp 
	$(GPP) $(GPP_OPT) -c YabMain.cpp -o YabMain.o
YabInterface.o: YabInterface.cpp YabInterface.h YabMenu.h
	$(GPP) $(GPP_OPT) -c YabInterface.cpp -o YabInterface.o
YabWindow.o: YabWindow.cpp YabWindow.h
	$(GPP) $(GPP_OPT) -c YabWindow.cpp -o YabWindow.o
YabView.o: YabView.cpp YabView.h
	$(GPP) $(GPP_OPT) -c YabView.cpp -o YabView.o
YabBitmapView.o: YabBitmapView.cpp YabBitmapView.h
	$(GPP) $(GPP_OPT) -c YabBitmapView.cpp -o YabBitmapView.o
YabFilePanel.o: YabFilePanel.cpp YabFilePanel.h
	$(GPP) $(GPP_OPT) -c YabFilePanel.cpp -o YabFilePanel.o
YabFilePanelLooper.o: YabFilePanelLooper.cpp YabFilePanelLooper.h
	$(GPP) $(GPP_OPT) -c YabFilePanelLooper.cpp -o YabFilePanelLooper.o
YabList.o: YabList.cpp YabList.h
	$(GPP) $(GPP_OPT) -c YabList.cpp -o YabList.o
YabText.o: YabText.cpp YabText.h
	$(GPP) $(GPP_OPT) -c YabText.cpp -o YabText.o
bison.o: bison.c yabasic.h config.h 
	$(GCC) $(GCC_OPT) -c bison.c -o bison.o
flex.o: flex.c bison.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c flex.c -o flex.o
function.o: function.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c function.c -o function.o
io.o: io.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c io.c -o io.o
graphic.o: graphic.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c graphic.c -o graphic.o
symbol.o: symbol.c yabasic.h config.h
	$(GCC) $(GCC_OPT) -c symbol.c -o symbol.o
main.o: main.c yabasic.h config.h 
	$(GCC) $(GCC_OPT) -c main.c -o main.o
flex.c: yabasic.flex
	flex $(FLEXFLAGS) -t yabasic.flex >flex.c
bison.c: yabasic.bison 
	bison $(BISONFLAGS) --output-file bison.c yabasic.bison  
YabStackView.o: YabStackView.cpp YabStackView.h
	$(GPP) $(GPP_OPT) -c YabStackView.cpp -o YabStackView.o
SplitPane.o: SplitPane.cpp SplitPane.h 
	$(GPP) $(GPP_OPT) -c SplitPane.cpp -o SplitPane.o
URLView.o: URLView.cpp URLView.h
	$(GPP) $(GPP_OPT) -c URLView.cpp -o URLView.o
Spinner.o: Spinner.cpp Spinner.h
	$(GPP) $(GPP_OPT) -c Spinner.cpp -o Spinner.o
column/ColumnListView.o: column/ColumnListView.cpp column/ColumnListView.h column/ObjectList.h 
	$(GPP) $(GPP_OPT) -c column/ColumnListView.cpp -o column/ColumnListView.o
column/ColorTools.o: column/ColorTools.cpp column/ColorTools.h 
	$(GPP) $(GPP_OPT) -c column/ColorTools.cpp -o column/ColorTools.o
column/YabColumnType.o: column/YabColumnType.cpp column/YabColumnType.h
	$(GPP) $(GPP_OPT) -c column/YabColumnType.cpp -o column/YabColumnType.o
$(HAIKUTAB): YabTabView.cpp YabTabView.h
	$(GPP) $(GPP_OPT) -c YabTabView.cpp -o YabTabView.o
CalendarControl.o: CalendarControl.cpp CalendarControl.h DateTextView.cpp MonthWindow.cpp MonthView.cpp MouseSenseStringView.cpp
	$(GPP) $(GPP_OPT) -c CalendarControl.cpp -o CalendarControl.o
YabControlLook.o: YabControlLook.h YabControlLook.cpp
	$(GPP) $(GPP_OPT) -c YabControlLook.cpp -o YabControlLook.o

clean:
	rm -f core *.o column/*.o flex.* bison.* yab yabasic.output libyab.so

install: yab libyab.so
	mkdir -p /boot/system/non-packaged/bin
	mkdir -p /boot/system/non-packaged/lib
	mkdir -p /boot/system/non-packaged/develop/headers/yab/column
	mkdir -p /boot/system/non-packaged/develop/lib
	cp -f yab /boot/system/non-packaged/bin/
	cp -f libyab.so /boot/system/non-packaged/lib/
	cp -f libyab.so /boot/system/non-packaged/develop/lib
	cp -f *.h /boot/system/non-packaged/develop/headers/yab/
	cp -f column/*.h /boot/system/non-packaged/develop/headers/yab/column/
