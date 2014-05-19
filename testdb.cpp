//////////////////////////////////////////////////////////////////////
// testdb.cpp -- Test Suite for Sqlite3Db class
// Date: Sat May 17 20:29:33 2014
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "sqlite3db.hpp"

int
main(int argc,char **argv) {
	Sqlite3Db db;
	bool ok;

	//////////////////////////////////////////////////////////////
	// Open/Create test.db
	//////////////////////////////////////////////////////////////

	if ( !db.open("test.db",false,true) ) {
		fprintf(stderr,"ERROR %s: Opening/creating test.db\n",
			db.errormsg().c_str());
		exit(1);
	}

	//////////////////////////////////////////////////////////////
	// If table xyzzy exists, drop it
	//////////////////////////////////////////////////////////////

	if ( db.is_table("xyzzy") ) {
		puts("Table xyzzy already exists.. dropping it:");
		ok = db.execute("drop table xyzzy");
		if ( !ok )
			printf("%s: drop table xyzzy\n",db.errormsg().c_str());
		assert(ok);
	}

	//////////////////////////////////////////////////////////////
	// Create fresh new table xyzzy
	//////////////////////////////////////////////////////////////

	ok = db.execute(
		"create table xyzzy ( \n"
		"  name varchar(40) not null, \n"
		"  ivalue int(10) not null \n"
		")\n"
	);
	if ( !ok )
		printf("%s: create table xyzzy\n",db.errormsg().c_str());
	assert(ok);
		
	//////////////////////////////////////////////////////////////
	// Check if table xyzzy exists (it should)
	//////////////////////////////////////////////////////////////

	if ( !db.is_table("xyzzy") ) {
		puts("Table xyzzy does not exist.");
		assert(0);
	}

	//////////////////////////////////////////////////////////////
	// Insert a row
	//////////////////////////////////////////////////////////////

	ok = db.execute(
		"insert into xyzzy(name,ivalue) \n"
		"values('One',1) \n"
	);
	if ( !ok )
		printf("%s: insert\n",db.errormsg().c_str());
	assert(ok);

	//////////////////////////////////////////////////////////////
	// Report rows inserted count and row ID
	//////////////////////////////////////////////////////////////

	unsigned count = db.rows_affected();
	uint64_t rowid = db.last_rowid();

	printf("Rows affected: %u, last_rowid %llu\n",count,rowid);

	assert(count == 1);
	assert(rowid == 1);

	//////////////////////////////////////////////////////////////
	// Close database
	//////////////////////////////////////////////////////////////

	puts("Closing database.");
	db.close();

	puts("Success.");

	return 0;
}

// End testdb.cpp
