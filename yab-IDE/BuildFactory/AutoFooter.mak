##
## GCC Options
##

SH=$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)
UH= $(shell finddir B_USER_HEADERS_DIRECTORY)
GCC = gcc
GCC_OPT = $(DBG) $(OPT) -I. $(addprefix -I,$(SH)) $(addprefix -I,$(UH)) -DHAVE_CONFIG -DUNIX $(HAIKUOPT) -I/boot/system/non-packaged/develop/headers/yab
GPP = g++
GPP_OPT = $(DBG) $(OPT) -I. -DHAVE_CONFIG -DUNIX $(HAIKUOPT) -I/boot/system/non-packaged/develop/headers/yab

yab: YabMain.o main.o flex.o
	$(GPP) $(GPP_OPT) -o $(TARGET) YabMain.o main.o flex.o $(LIBPATH) $(LIB)

YabMain.o: YabMain.cpp 
	$(GPP) $(GPP_OPT) -c YabMain.cpp -o YabMain.o
flex.o: flex.c
	$(GCC) $(GCC_OPT) -c flex.c -o flex.o
main.o: main.c
	$(GCC) $(GCC_OPT) -c main.c -o main.o

clean:
	rm -f core *.o yabasic.output
