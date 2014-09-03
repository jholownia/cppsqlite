/*
================================
 cppsqlite.h

 Created on: 21 August 2014
 Author: Jan Holownia <jan@fuel-3d.com>

 Copyright (c) 2014 Fuel3D Ltd. All rights reserved.

 Description: 
	A C++ wrapper for sqlite3 library.
================================
*/

#pragma once


#include <string>
#include <memory>
#include <cassert>
#include "../sqlite/sqlite3.h"


// Enable foreign keys support
#ifdef SQLITE_OMIT_FOREIGN_KEY
#undef SQLITE_OMIT_FOREIGN_KEY
#endif

#ifdef SQLITE_OMIT_TRIGGER
#undef SQLITE_OMIT_TRIGGER
#endif


namespace sqlite {


/*
=================
 SqlException
=================
*/
struct SqlException 
{
	int errorCode;	// sqlite error code
	std::string errorMessage;

	SqlException( const int result, const char* msg ) :
		errorCode    (result),
		errorMessage (msg)
	{
	}
};


/*
=================
 Connection
=================
*/
class Connection
{
public:
	Connection() :
		m_db(nullptr, sqlite3_close)
	{
	}

	~Connection() 
	{
	}

	void open(const std::string& fileName)
	{
		sqlite3* rawConnection;

		const int result = sqlite3_open(fileName.c_str(), &rawConnection);

		auto localHandle = std::unique_ptr<sqlite3, int(*)(sqlite3*)>(rawConnection, sqlite3_close);

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(localHandle.get()));
		}

		m_db = std::move(localHandle);		
	}

	void createInMemory()
	{
		open(":memory:");
	}

	void execute(const std::string& statement)
	{
		assert(m_db);

		const int result = sqlite3_exec(m_db.get(), statement.c_str(), nullptr, nullptr, nullptr);

		if (result != SQLITE_OK)
		{
			throw SqlException( result, sqlite3_errmsg(m_db.get()));
		}
	}

	sqlite3* ptr() const 
	{ 
		return m_db.get();		
	}	

	long long int lastRowId()
	{
		return sqlite3_last_insert_rowid(m_db.get());
	}

	void beginTransaction()
	{
		execute("BEGIN");
	}

	void rollbackTransaction()
	{
		execute("ROLLBACK");
	}

	void commitTransaction()
	{
		execute("COMMIT");
	}
	
private:
	std::unique_ptr<sqlite3, int(*)(sqlite3*)> m_db;
};


/*
=================
 Statement
=================
*/
class Statement
{
public:
	Statement() :
		m_stmt(nullptr, sqlite3_finalize)
	{
	}

	~Statement()
	{
	}

	void prepare(const Connection& connection, const std::string& statement)
	{
		sqlite3_stmt* rawStmt;

		const int result = sqlite3_prepare_v2(connection.ptr(), statement.c_str(), statement.size(), &rawStmt, nullptr);

		auto localHandle = std::unique_ptr<sqlite3_stmt, int(*)(sqlite3_stmt*)>(rawStmt, sqlite3_finalize);

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(connection.ptr()));
		}

		m_stmt = std::move(localHandle);
	}

	void bind(int index, const int value)
	{
		assert(m_stmt);

		++index; // sqlite uses 1-based indexing for bind, we change it to 0-based for consistency

		const int result = sqlite3_bind_int(m_stmt.get(), index, value);

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(sqlite3_db_handle(m_stmt.get())));
		}
	}

	void bind(int index, const long long int value)
	{
		assert(m_stmt);

		++index;

		const int result = sqlite3_bind_int64(m_stmt.get(), index, value);

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(sqlite3_db_handle(m_stmt.get())));
		}
	}

	void bind(int index, const std::string& value)
	{
		assert(m_stmt);

		++index;
				
		const int result = sqlite3_bind_text(m_stmt.get(), index, value.c_str(), value.size(), SQLITE_TRANSIENT);
		
		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(sqlite3_db_handle(m_stmt.get())));
		}
	}

	bool step()
	{
		assert(m_stmt);

		const int result = sqlite3_step(m_stmt.get());

		if (result == SQLITE_ROW)
		{
			return true;
		}
		else if (result == SQLITE_DONE)
		{
			return false;
		}
		else
		{
			throw SqlException(result, sqlite3_errmsg(sqlite3_db_handle(m_stmt.get())));
		}			
	}

	void reset()
	{
		const int result = sqlite3_reset(m_stmt.get());

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(sqlite3_db_handle(m_stmt.get())));
		}
	}

	int getInt(const int column)
	{
		return sqlite3_column_int(m_stmt.get(), column);
	}

	long long int getInt64(const int column)
	{
		return sqlite3_column_int64(m_stmt.get(), column);		
	}

	std::string getString(const int column)
	{
		const char* str = reinterpret_cast<const char*>(sqlite3_column_text(m_stmt.get(), column));

		return str ? str : std::string();

		// Look at boost::optional
	}

	std::string getBlob(const int column)
	{		
		int dataSize = 0;

		dataSize = sqlite3_column_bytes(m_stmt.get(), column);

		const char* data = reinterpret_cast<const char*>(sqlite3_column_blob(m_stmt.get(), column));
		
		return data ? std::string(data, dataSize) : std::string();
	}

private:
	std::unique_ptr<sqlite3_stmt, int(*)(sqlite3_stmt*)> m_stmt;
};


} // namespace sqlite