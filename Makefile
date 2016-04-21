CC       = gcc
CXX      = g++
CFLAGS   = -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
CXXFLAGS = -std=c++0x -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
INCPATH  = -I./src
LINK     = g++
LFLAGS   =
LIBS     = $(SUBLIBS)  /usr/local/lib/libevent.a /usr/local/lib/libevent_pthreads.a -lrt -pthread /usr/local/lib/libjemalloc.a
####### Output directory

OBJECTS_DIR = tmp/

####### Files

HEADERS = src/eventloop.h \
		src/util/logger.h \
                src/util/objectpool.h \
		src/util/vector.h \
		src/util/string.h \
		src/util/queue.h \
		src/util/hash.h \
		src/redisproto.h \
		src/redisproxy.h \
		src/redisservant.h \
		src/command.h \
		src/util/tcpsocket.h \
		src/util/tcpserver.h \
		src/util/thread.h \
		src/tinyxml/tinystr.h \
		src/tinyxml/tinyxml.h \
		src/redis-proxy-config.h \
		src/util/iobuffer.h \
		src/redis-servant-select.h \
		src/redisservantgroup.h \
		src/util/locker.h \
		src/monitor.h \
		src/top-key.h \
		src/non-portable.h \
		src/proxymanager.h \
		src/cmdhandler.h

SOURCES = src/eventloop.cpp \
		src/util/logger.cpp \
		src/main.cpp \
                src/util/hash.cpp \
		src/redisproto.cpp \
		src/redisproxy.cpp \
		src/redisservant.cpp \
		src/command.cpp \
		src/util/tcpsocket.cpp \
		src/util/tcpserver.cpp \
		src/util/thread.cpp \
		src/tinyxml/tinystr.cpp \
		src/tinyxml/tinyxml.cpp \
		src/tinyxml/tinyxmlerror.cpp \
		src/tinyxml/tinyxmlparser.cpp \
		src/redis-proxy-config.cpp \
		src/util/iobuffer.cpp \
		src/redis-servant-select.cpp \
		src/redisservantgroup.cpp \
		src/util/locker.cpp \
		src/monitor.cpp \
		src/top-key.cpp \
		src/non-portable.cpp \
		src/cmdhandler.cpp   \
		src/util/md5.cpp    \
		src/util/crc16.cpp  \
		src/util/crc32.cpp  \
		src/util/hsieh.cpp  \
		src/util/jenkins.cpp \
		src/util/fnv.cpp \
		src/util/murmur.cpp

OBJECTS = tmp/eventloop.o \
		tmp/logger.o \
		tmp/main.o \
    tmp/hash.o \
		tmp/redisproto.o \
		tmp/redisproxy.o \
		tmp/redisservant.o \
		tmp/command.o \
		tmp/tcpsocket.o \
		tmp/tcpserver.o \
		tmp/thread.o \
		tmp/tinystr.o \
		tmp/tinyxml.o \
		tmp/tinyxmlerror.o \
		tmp/tinyxmlparser.o \
		tmp/redis-proxy-config.o \
		tmp/iobuffer.o \
		tmp/redis-servant-select.o \
		tmp/redisservantgroup.o \
		tmp/locker.o \
		tmp/monitor.o \
		tmp/top-key.o \
		tmp/non-portable.o \
		tmp/proxymanager.o \
		tmp/cmdhandler.o   \
		tmp/md5.o \
		tmp/crc16.o \
		tmp/crc32.o \
		tmp/hsieh.o \
		tmp/jenkins.o \
		tmp/fnv.o \
		tmp/murmur.o 


DESTDIR  =
TARGET   = onecache

first: all
####### Implicit rules

.SUFFIXES: .c .o .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(UICDECLS) $(OBJECTS) $(OBJMOC)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(OBJCOMP) $(LIBS)


clean:
	rm -f $(OBJECTS)
	rm -f *.core


####### Compile

tmp/eventloop.o: src/eventloop.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/eventloop.o src/eventloop.cpp

tmp/logger.o: src/util/logger.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/logger.o src/util/logger.cpp

tmp/main.o: src/main.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/main.o src/main.cpp

tmp/hash.o: src/util/hash.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/hash.o src/util/hash.cpp

tmp/redisproto.o: src/redisproto.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redisproto.o src/redisproto.cpp

tmp/redisproxy.o: src/redisproxy.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redisproxy.o src/redisproxy.cpp

tmp/redisservant.o: src/redisservant.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redisservant.o src/redisservant.cpp

tmp/command.o: src/command.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/command.o src/command.cpp

tmp/tcpsocket.o: src/util/tcpsocket.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tcpsocket.o src/util/tcpsocket.cpp

tmp/tcpserver.o: src/util/tcpserver.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tcpserver.o src/util/tcpserver.cpp

tmp/thread.o: src/util/thread.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/thread.o src/util/thread.cpp

tmp/tinystr.o: src/tinyxml/tinystr.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinystr.o src/tinyxml/tinystr.cpp

tmp/tinyxml.o: src/tinyxml/tinyxml.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinyxml.o src/tinyxml/tinyxml.cpp

tmp/tinyxmlerror.o: src/tinyxml/tinyxmlerror.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinyxmlerror.o src/tinyxml/tinyxmlerror.cpp

tmp/tinyxmlparser.o: src/tinyxml/tinyxmlparser.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinyxmlparser.o src/tinyxml/tinyxmlparser.cpp

tmp/redis-proxy-config.o: src/redis-proxy-config.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redis-proxy-config.o src/redis-proxy-config.cpp

tmp/iobuffer.o: src/util/iobuffer.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/iobuffer.o src/util/iobuffer.cpp

tmp/redisservantgroup.o: src/redisservantgroup.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redisservantgroup.o src/redisservantgroup.cpp

tmp/redis-servant-select.o: src/redis-servant-select.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redis-servant-select.o src/redis-servant-select.cpp

tmp/locker.o: src/util/locker.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/locker.o src/util/locker.cpp

tmp/monitor.o: src/monitor.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/monitor.o src/monitor.cpp

tmp/top-key.o: src/top-key.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/top-key.o src/top-key.cpp

tmp/non-portable.o: src/non-portable.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/non-portable.o src/non-portable.cpp

tmp/proxymanager.o: src/proxymanager.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/proxymanager.o src/proxymanager.cpp

tmp/cmdhandler.o: src/cmdhandler.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/cmdhandler.o src/cmdhandler.cpp

tmp/md5.o: src/util/md5.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/md5.o src/util/md5.cpp

tmp/crc16.o: src/util/crc16.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/crc16.o src/util/crc16.cpp

tmp/crc32.o: src/util/crc32.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/crc32.o src/util/crc32.cpp

tmp/hsieh.o: src/util/hsieh.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/hsieh.o src/util/hsieh.cpp

tmp/jenkins.o: src/util/jenkins.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/jenkins.o src/util/jenkins.cpp 

tmp/fnv.o: src/util/fnv.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/fnv.o src/util/fnv.cpp

tmp/murmur.o: src/util/murmur.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/murmur.o src/util/murmur.cpp
