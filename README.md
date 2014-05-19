sqlite3db
=========

Posix Sqlite3Db C++ class, implementing easy to use prepare, qbind and rbind.

See testdb.cpp for an example program.


class Sqlite3Db {
	...
public:	Sqlite3Db();

	bool open(const char *filename,bool readonly=false,bool create=false);
	void close();

	inline const std::string& errormsg() { return errmsg; }
	inline int error() { return status; }

	bool execute(const char *sql);			// Query with no results

	bool prepare(const char *sql);			// Perpare query (1st) with results..

	bool qbind_null();				// Bind NULL to query (2nd)
	bool qbind(int qv);				// Bind int to query
	bool qbind(sqlite3_int64 qv);			// Bind int64 to query
	bool qbind(double qv);				// Bind double to query
	bool qbind(const char *qv);			// Bind text to query
	bool qbind(const std::string& qv);		// Bind std::string to query (use only when qv does not change!)
	bool qbind(const char *qblob,int nbytes);	// Bind blob to query

	void rbind(int& rv);				// Bind int result (3rd)
	void rbind(sqlite3_int64& rv);			// Bind int64 result
	void rbind(double& rv);				// Bind double result
	void rbind(char *rv,unsigned maxbytes);		// Bind text result
	void rbind(std::string& rv);			// Bind text to std::string object
	void rbind(char *rblob,int maxbytes);		// Bind blob result

	unsigned rlength(void *loc);			// Length of returned result

	int step();					// Execute statement/fetch (4th)

	bool is_rnull(void *loc);			// Returns true if the indicated value was NULL
	inline bool is_rtrunc() { return trunc; }	// Returns true if any result was truncated

	inline sqlite3_int64 last_rowid()		{ return sqlite3_last_insert_rowid(sqldb); }
	inline unsigned rows_affected() 		{ return unsigned(sqlite3_changes(sqldb)); }

	void rclear();					// Clear last prepared query & results (5th)

	bool is_table(const char *table_name);		// Return true if table_name exists
};
