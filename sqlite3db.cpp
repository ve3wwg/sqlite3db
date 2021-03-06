//////////////////////////////////////////////////////////////////////
// sqlite3db.cpp -- Implementation of Sqlite3 Class
// Date: Sat May 17 08:12:22 2014
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "sqlite3db.hpp"

Sqlite3Db::Sqlite3Db() {
	sqldb = 0;
	stmt = 0;
	needs_reset = false;
	bindx = 0;
	trunc = false;
	vfs_module_name = 0;
}

bool
Sqlite3Db::open(const char *filename,bool readonly,bool create) {
	int flags = 0;
	
	if ( sqldb )
		close();		// Close and then reopen

	if ( readonly )
		flags = SQLITE_OPEN_READONLY;
	else	{
		flags = SQLITE_OPEN_READWRITE;
		if ( create )
			flags |= SQLITE_OPEN_CREATE;
	}

	status = sqlite3_open_v2(filename,&sqldb,flags,vfs_module_name);
	if ( status != SQLITE_OK ) {
		errmsg = sqlite3_errmsg(sqldb);
		sqlite3_close(sqldb);
		sqldb = 0;
		return false;
	}		
	return true;
}

void
Sqlite3Db::close() {
	if ( sqldb ) {
		if ( stmt ) {
			sqlite3_finalize(stmt);
			needs_reset = false;
			bindx = 0;
			stmt = 0;
		}
		status = sqlite3_close(sqldb);
		errmsg = "Closed";
		sqldb = 0;
	}
}

bool
Sqlite3Db::prepare(const char *sql) {

	if ( !sqldb ) {
		status = SQLITE_CANTOPEN;
		errmsg = "No database open";
		return false;
	}

	if ( stmt ) {
		sqlite3_finalize(stmt);
		stmt = 0;
	}

	needs_reset = false;
	trunc = false;
	bindx = 0;
	rvec.clear();

	{
		int lcl_status = sqlite3_prepare_v2(sqldb,sql,-1,&stmt,0);
		if ( lcl_status != SQLITE_OK ) {
			std::string lcl_errmsg = sqlite3_errmsg(sqldb);
			sqlite3_finalize(stmt);
			stmt = 0;
			status = lcl_status;
			errmsg = lcl_errmsg;
			return false;
		}
	}

	return true;
}

bool
Sqlite3Db::execute(const char *sql) {
	bool ok, r = false;

	rclear();
	ok = prepare(sql);
	if ( !ok )
		goto xit;

	status = step();

	switch ( status ) {
	case SQLITE_DONE:
	case SQLITE_ROW:
		r = true;
		break;

	case SQLITE_BUSY:
	case SQLITE_ERROR:
		r = false;
		errmsg = sqlite3_errmsg(sqldb);
		break;
	case SQLITE_MISUSE:
	default :
		errmsg = sqlite3_errmsg(sqldb);
		assert(0);
	}

xit:	rclear();
	return r;
}

void
Sqlite3Db::rclear() {
	if ( !sqldb )
		return;

	if ( stmt ) {
		sqlite3_finalize(stmt);
		stmt = 0;
		trunc = false;
		bindx = 0;
		rvec.clear();
	}
}

int
Sqlite3Db::step() {

	if ( !sqldb || !stmt ) {
		errmsg = "Not open/ready.";
		status = SQLITE_CANTOPEN;
	}

	if ( needs_reset ) {
		sqlite3_reset(stmt);
		needs_reset = false;
	}

	status = sqlite3_step(stmt);

	switch ( status ) {
	case SQLITE_BUSY:
		errmsg = sqlite3_errmsg(sqldb);
		needs_reset = false;
		return status;

	case SQLITE_DONE:
		needs_reset = true;
		return status;

	case SQLITE_ROW:
		needs_reset = false;
		trunc = false;

		for ( int rx=0; size_t(rx) < rvec.size() && rx < sqlite3_column_count(stmt); ++rx ) {
			s_result& r = rvec[rx];
			int stype = sqlite3_column_type(stmt,rx);

			r.is_null = stype == SQLITE_NULL;
			r.rbytes = sqlite3_column_bytes(stmt,rx);

			switch ( r.type ) {
			case RT_int :
				{
					int &col = *(int *)r.loc;
					
					if ( !r.is_null )
						col = sqlite3_column_int(stmt,rx);
					else	col = 0;
				}
				break;
			case RT_int64 :
				{
					sqlite3_int64 &col = *(sqlite3_int64 *)r.loc;
					
					if ( !r.is_null )
						col = sqlite3_column_int64(stmt,rx);
					else	col = 0;
				}
				break;
			case RT_double :
				{
					double &col = *(double *)r.loc;
					
					if ( !r.is_null )
						col = sqlite3_column_double(stmt,rx);
					else	col = 0;
				}
				break;
			case RT_text :
				{
					char *col = (char *)r.loc;
					
					if ( !r.is_null ) {
						const char *text = (const char *) sqlite3_column_text(stmt,rx);
						size_t tlen = strlen(text);

						strncpy(col,text,r.length);
						if ( tlen >= r.length ) {
							if ( r.length > 0 )
								col[r.length-1] = 0;
							trunc = true;
						}
					} else	*col = 0;
				}
				break;
			case RT_string :
				{
					std::string *col = (std::string *)r.loc;
					
					if ( !r.is_null ) {
						const char *text = (const char *) sqlite3_column_text(stmt,rx);
						*col = text;
					} else	col->clear();
				}
				break;
			case RT_blob :
				{
					char *col = (char *)r.loc;
					
					if ( !r.is_null ) {
						void *blob = (void *) sqlite3_column_blob(stmt,rx);
						unsigned blen = sqlite3_column_bytes(stmt,rx);

						if ( r.length < blen ) {
							blen = r.length;
							trunc = true;
						}
						r.rbytes = blen;		// Keep the actual blob size around
						memcpy(col,blob,r.rbytes);	// Copy to user's buffer
					} else	memset(col,0,r.length);
				}
				break;
			}
		}

		if ( int(rvec.size()) != sqlite3_column_count(stmt) )
			return SQLITE_MISMATCH;		// # of bound result columns does not match query

		return status;

	case SQLITE_ERROR:
		needs_reset = true;
		return status;

	case SQLITE_MISUSE:
	default :
		assert(0);
	}

	return status;
}

bool
Sqlite3Db::qbind_null() {
	status = sqlite3_bind_null(stmt,++bindx);
	if ( status != SQLITE_OK )
		errmsg = sqlite3_errmsg(sqldb);

	return status == SQLITE_OK;
}

bool
Sqlite3Db::qbind(int qv) {
	status = sqlite3_bind_int(stmt,++bindx,qv);
	if ( status != SQLITE_OK )
		errmsg = sqlite3_errmsg(sqldb);

	return status == SQLITE_OK;
}

bool
Sqlite3Db::qbind(sqlite3_int64 qv) {
	status = sqlite3_bind_int64(stmt,++bindx,qv);
	if ( status != SQLITE_OK )
		errmsg = sqlite3_errmsg(sqldb);

	return status == SQLITE_OK;
}

bool
Sqlite3Db::qbind(double qv) {
	status = sqlite3_bind_double(stmt,++bindx,qv);
	if ( status != SQLITE_OK )
		errmsg = sqlite3_errmsg(sqldb);

	return status == SQLITE_OK;
}

bool
Sqlite3Db::qbind(const char *qv) {
	status = sqlite3_bind_text(stmt,++bindx,qv,-1,0);
	if ( status != SQLITE_OK )
		errmsg = sqlite3_errmsg(sqldb);

	return status == SQLITE_OK;
}

//////////////////////////////////////////////////////////////////////
// IMPORTANT: Use this only if the string does not vary in value
//////////////////////////////////////////////////////////////////////

bool
Sqlite3Db::qbind(const std::string& qv) {
	return sqlite3_bind_text(stmt,++bindx,qv.c_str(),-1,0) == SQLITE_OK;
}

bool
Sqlite3Db::qbind(const void *qblob,int nbytes) {
	status = sqlite3_bind_blob(stmt,++bindx,qblob,nbytes,0);
	if ( status != SQLITE_OK )
		errmsg = sqlite3_errmsg(sqldb);

	return status == SQLITE_OK;
}

void
Sqlite3Db::rbind(int& rv) {
	s_result r;

	r.type = RT_int;
	r.loc = &rv;
	r.length = sizeof rv;
	r.rbytes = 0;				// Filled in later
	rvec.push_back(r);
}

void
Sqlite3Db::rbind(sqlite3_int64& rv) {
	s_result r;

	r.type = RT_int64;
	r.loc = &rv;
	r.length = sizeof rv;
	r.rbytes = 0;				// Filled in later
	rvec.push_back(r);
}

void
Sqlite3Db::rbind(double& rv) {
	s_result r;

	r.type = RT_double;
	r.loc = &rv;
	r.length = sizeof rv;
	r.rbytes = 0;				// Filled in later
	rvec.push_back(r);
}

void
Sqlite3Db::rbind(char *rv,unsigned maxbytes) {
	s_result r;

	r.type = RT_text;
	r.loc = rv;
	r.length = maxbytes;
	r.rbytes = 0;				// Filled in later
	rvec.push_back(r);
}

void
Sqlite3Db::rbind(std::string& rv) {
	s_result r;

	r.type = RT_string;
	r.loc = &rv;
	r.length = 0;
	r.rbytes = 0;				// Filled in later
	rvec.push_back(r);

}

void
Sqlite3Db::rbind(void *qblob,int maxbytes) {
	s_result r;

	r.type = RT_blob;
	r.loc = qblob;
	r.length = maxbytes;
	r.rbytes = 0;				// Filled in later
	rvec.push_back(r);
}

unsigned
Sqlite3Db::rlength(void *loc) {
	for ( int x=0; size_t(x) < rvec.size(); ++x ) {
		s_result& r = rvec[x];

		if ( r.loc == loc )
			return r.rbytes;
	}
	return 0;
}

bool
Sqlite3Db::is_rnull(void *loc) {
	for ( int x=0; size_t(x) < rvec.size(); ++x ) {
		s_result& r = rvec[x];

		if ( r.loc == loc )
			return r.is_null;
	}
	return true;
}

bool
Sqlite3Db::is_table(const char *table_name) {
	bool ok, r = false;
	int res_count = 0;

	ok = prepare(
		"select count(*) \n"
		"from sqlite_master \n"
		"where name = ? and type = 'table' \n"
	);
	if ( !ok )
		goto xit;

	ok = qbind(table_name);
	if ( !ok )
		goto xit;

	rbind(res_count);

	if ( step() == SQLITE_ROW ) {
		r = res_count > 0;
	} else	r = false;

xit:	rclear();
	return r;
}

//////////////////////////////////////////////////////////////////////
// Open a blob for read or read/write
//////////////////////////////////////////////////////////////////////

Sqlite3Db::Blob *
Sqlite3Db::open_blob(const char *table,const char *column,sqlite3_int64 rowid,bool readonly) {
	sqlite3_blob *sqblob = 0;

	status = sqlite3_blob_open(sqldb,0,table,column,rowid,readonly ? 0 : 1,&sqblob);

	switch ( status ) {
	case SQLITE_OK :
		break;
	default :
		errmsg = sqlite3_errmsg(sqldb);
		return 0;
	}

	return new Blob(*this,sqblob);
}

//////////////////////////////////////////////////////////////////////
// Blob constructor
//////////////////////////////////////////////////////////////////////

Sqlite3Db::Blob::Blob(Sqlite3Db &database,sqlite3_blob *sqblob) : db(database), blob(sqblob) {
	db.status = SQLITE_OK;
}

//////////////////////////////////////////////////////////////////////
// Blob destructor
//////////////////////////////////////////////////////////////////////

Sqlite3Db::Blob::~Blob() {

	if ( blob ) {
		db.status = sqlite3_blob_close(blob);
		if ( db.status != SQLITE_OK )
			db.errmsg = sqlite3_errmsg(db.sqldb);
		blob = 0;
	} else	db.status = SQLITE_OK;
}

//////////////////////////////////////////////////////////////////////
// Return the size of the blob
//////////////////////////////////////////////////////////////////////

size_t
Sqlite3Db::Blob::size() {
	return static_cast<size_t>(sqlite3_blob_bytes(blob));
}

//////////////////////////////////////////////////////////////////////
// Read from the blob
//////////////////////////////////////////////////////////////////////

bool
Sqlite3Db::Blob::read(void *buffer,size_t bytes,size_t offset) {

	db.status = sqlite3_blob_read(
		blob,
		buffer,
		static_cast<int>(bytes),
		static_cast<int>(offset)
	);

	if ( db.status != SQLITE_OK ) {
		db.errmsg = sqlite3_errmsg(db.sqldb);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Write to the blob
//////////////////////////////////////////////////////////////////////

bool
Sqlite3Db::Blob::write(const void *buffer,size_t bytes,size_t offset) {

	db.status = sqlite3_blob_write(
		blob,
		buffer,
		static_cast<int>(bytes),
		static_cast<int>(offset)
	);

	if ( db.status != SQLITE_OK ) {
		db.errmsg = sqlite3_errmsg(db.sqldb);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Move an open blob from the current rowid, to a new rowid
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_BLOB_REOPEN
bool
Sqlite3Db::Blob::move(sqlite3_int64 new_rowid) {

	db.status = sqlite3_blob_reopen(blob,new_rowid);

	if ( db.status != SQLITE_OK ) {
		db.errmsg = sqlite3_errmsg(db.sqldb);
		return false;
	}

	return true;
}
#endif

bool
Sqlite3Db::integrity_check() {
	char buf[40];

	if ( !prepare("pragma integrity_check") )
		return false;

	rbind(buf,sizeof buf);

	if ( step() != SQLITE_ROW )
		return false;

	return !strcasecmp(buf,"ok");
}

bool
Sqlite3Db::quick_check() {
	char buf[40];

	if ( !prepare("pragma quick_check") )
		return false;

	rbind(buf,sizeof buf);

	if ( step() != SQLITE_ROW )
		return false;

	return !strcasecmp(buf,"ok");
}

bool
Sqlite3Db::vacuum() {
	return execute("vacuum");
}

bool
Sqlite3Db::qreset() {
	if ( stmt ) {
		bindx = 0;
		needs_reset = false;
		status = SQLITE_OK;
		return true;
	} else
		return false;
}

bool
Sqlite3Db::query_set_pragma(const char *pragma,const char *mode,std::string& result) {
	std::string sql = "pragma ";
	char res[1024];

	result.clear();
	sql += pragma;

	if ( mode ) {
		sql += " = ";
		sql += mode;
	}

	if ( !prepare(sql.c_str()) )
		return false;

	rbind(res,sizeof res);
	if ( step() != SQLITE_ROW )
		return false;

	result = res;
	return true;
}

bool
Sqlite3Db::journal_mode(const char *mode,std::string& result) {
	return query_set_pragma("journal_mode",mode,result);
}

bool
Sqlite3Db::locking_mode(const char *mode,std::string& result) {
	return query_set_pragma("locking_mode",mode,result);
}

// End sqlite3db.cpp
