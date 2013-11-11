#############################################################################

# Makefile for building thPollServd
# Generated by tmake at 23:04, 2010/08/16
#     Project: main
#    Template: app
#############################################################################

####### Compiler, tools and options

QTDIR	=	/usr
CC	=	gcc
CXX	=	g++
CFLAGS	=	-g -pipe -Wall -W -O2 -DNO_DEBUG -std=C99 -lpthread
CXXFLAGS=	-g -pipe -Wall -W -O2 -DNO_DEBUG
LINK	=	g++
LFLAGS	=	
#LIBS	=	$(SUBLIBS) -L$(QTDIR)/lib -L/usr/X11R6/lib -lqt -lXext -lX11 -lm
LIBS	=	$(SUBLIBS) /usr/lib/libboost_regex-gcc42-1_34_1.a /usr/lib/libpthread.so 
MOC	=	$(QTDIR)/bin/moc
UIC	=	$(QTDIR)/bin/uic

TAR	=	tar -cf
GZIP	=	gzip -9f

####### Files

HEADERS =	main.h pthreadpool.h config.h
SOURCES =	main.cpp \
		threadpool.cpp config.cpp
OBJECTS =	main.o \
		threadpool.o config.o
INTERFACES =	
UICDECLS =	
UICIMPLS =	
SRCMOC	=	
OBJMOC	=	
DIST	=	
TARGET	=	thPollServd
INTERFACE_DECL_PATH = .

####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules


all: $(TARGET)

$(TARGET): $(UICDECLS) $(OBJECTS) $(OBJMOC) 
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(LIBS)

moc: $(SRCMOC)

tmake: Makefile

Makefile: main.pro
	tmake main.pro -o Makefile

dist:
	$(TAR) main.tar main.pro $(SOURCES) $(HEADERS) $(INTERFACES) $(DIST)
	$(GZIP) main.tar

clean:
	-rm -f $(OBJECTS) $(OBJMOC) $(SRCMOC) $(UICIMPLS) $(UICDECLS) $(TARGET)
	-rm -f *~ core

####### Sub-libraries


###### Combined headers


####### Compile

main.o: main.cpp \
		main.h \
		threadpool.h

threadpool.o: threadpool.cpp \
		threadpool.h

config.o: config.cpp \
	      config.h \
		  main.h