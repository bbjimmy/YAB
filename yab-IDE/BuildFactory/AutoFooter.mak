##
## GCC Options
##

SH=$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)
UH= $(shell finddir B_USER_HEADERS_DIRECTORY)
GCC = gcc
GCC_OPT = $(DBG) $(OPT) -I. $(addprefix -I,$(SH)) $(addprefix -I,$(UH)) -DHAVE_CONFIG -DUNIX $(HAIKUOPT)
GPP = g++
GPP_OPT = $(DBG) $(OPT) -I. -DHAVE_CONFIG -DUNIX $(HAIKUOPT)


##
## Compile and link
##
yab: YabMain.o YabInterface.o YabWindow.o YabView.o YabBitmapView.o YabFilePanel.o YabFilePanelLooper.o YabList.o \
	YabText.o flex.o bison.o symbol.o function.o graphic.o io.o main.o $(COLUMN) column/YabColumnType.o column/ColorTools.o YabStackView.o SplitPane.o URLView.o YabControlLook.o \
	$(HAIKUTAB) Spinner.o column/ColumnListView.o CalendarControl.o
	$(GPP) $(GPP_OPT) -o $(TARGET) YabMain.o YabInterface.o YabWindow.o YabView.o YabBitmapView.o YabText.o YabFilePanel.o \
		YabFilePanelLooper.o YabList.o main.o function.o io.o graphic.o symbol.o bison.o flex.o  $(COLUMN) column/YabColumnType.o column/ColorTools.o \
		YabStackView.o SplitPane.o URLView.o YabControlLook.o $(HAIKUTAB) Spinner.o $(TABLIB) CalendarControl.o \
		$(LIBPATH) $(LIB)
YabMain.o: YabMain.cpp 
	$(GPP) $(GPP_OPT) -c YabMain.cpp -o YabMain.o
YabInterface.o: YabInterface.cpp YabInterface.h global.h YabMenu.h
	$(GPP) $(GPP_OPT) -c YabInterface.cpp -o YabInterface.o
YabWindow.o: YabWindow.cpp YabWindow.h global.h
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
	rm -f core *.o column/*.o yabasic.output
