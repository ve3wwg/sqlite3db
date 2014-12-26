######################################################################
#  sqlite3db/Makefile - Generic POSIX Makefile
#  Warren Gay ve3wwg	Sat Apr  5 08:47:58 2014
######################################################################

-include Makefile.conf

PREFIX	= /usr/local

ARCH	= 
DEFNS	= 
STD	= -std=gnu++0x

GCOPTS	= -Wall -g -Os 
GLIBS	= -lstdc++ -lc -lm 

INCL	+= -I.

CPPFLAGS = $(GOPTS) $(INCL) $(OPTIONS) $(INCL) $(DEFNS)
CXXFLAGS = $(STD) $(GCOPTS) $(CPPFLAGS)

CXX	= $(ARCH)c++
OBJSIZE	= $(ARCH)size
OBJCOPY	= $(ARCH)objcopy
AR	= $(ARCH)ar
MAKE	= gmake

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $*.o

CPP_FILES 	:= $(wildcard *.cpp)
OBJS 		:= $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all:	libsqlite3db.a

sqlite3db.o: sqlite3db.cpp sqlite3db.hpp Makefile

libsqlite3db.a: sqlite3db.o
	$(AR) r libsqlite3db.a sqlite3db.o

testdb: testdb.o libsqlite3db.a
	$(CXX) testdb.o -o testdb -L. -lsqlite3db -L$(PREFIX)/lib -lsqlite3

check:	testdb
	./testdb	# Creates test.db if non-existing
	./testdb	# Re-opens test.db
	rm -f test.db

install: libsqlite3db.a
	install -m 444 libsqlite3db.a $(PREFIX)/lib 
	install -m 444 sqlite3db.hpp $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/libsqlite3db.a $(PREFIX)/include/sqlite3db.hpp

clean:
	rm -f *.o testdb

distclean: clean
	rm -f libsqlite3db.a testdb errs.t

clobber: distclean

# End sqlite3db/Makefile
