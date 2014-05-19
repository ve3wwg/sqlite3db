######################################################################
#  teensy3_lib/posix/sqlite3db/Makefile
#  Warren Gay ve3wwg	Sat Apr  5 08:47:58 2014
######################################################################

include ../Makefile.conf

C_FILES 	:= $(wildcard *.c)
CPP_FILES 	:= $(wildcard *.cpp)
OBJS 		:= $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all:	$(TOP_DIR)/lib/libteensy.a

sqlite3db.o: sqlite3db.cpp sqlite3db.hpp

$(TOP_DIR)/lib/libteensy.a: sqlite3db.o
	$(AR) r $(TOP_DIR)/lib/libteensy.a sqlite3db.o

testdb: testdb.o $(TOP_DIR)/lib/libteensy.a
	$(CXX) testdb.o -o testdb $(LDFLAGS) -lteensy -lsqlite3

check:	testdb
	./testdb	# Creates test.db if non-existing
	./testdb	# Re-opens test.db
	rm -f test.db

clean:
	rm -f *.o testdb

distclean: clean
	rm -f $(TOP_DIR)/libteensy.a testdb errs.t

# End posix/sqlite3/Makefile
