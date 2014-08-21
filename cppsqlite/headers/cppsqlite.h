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


namespace sqlite {
	

/*
=================
 Exception
=================
*/
struct SqlException 
{
	int error;	// sqlite error code
	std::string message;

	SqlException( const int result, const char* msg ) :
		error	(result),
		message	(msg)
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

	sqlite3* data() const 
	{ 
		return m_db.get();		
	}	

	long long int rowid()
	{
		return sqlite3_last_insert_rowid(m_db.get());
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

		const int result = sqlite3_prepare_v2(connection.data(), statement.c_str(), statement.size(), &rawStmt, nullptr);

		auto localHandle = std::unique_ptr<sqlite3_stmt, int(*)(sqlite3_stmt*)>(rawStmt, sqlite3_finalize);

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(connection.data()));
		}

		m_stmt = std::move(localHandle);
	}

	void bind(const int index, const int value)
	{
		assert(m_stmt);

		const int result = sqlite3_bind_int(m_stmt.get(), index, value);

		if (result != SQLITE_OK)
		{
			throw SqlException(result, sqlite3_errmsg(sqlite3_db_handle(m_stmt.get())));
		}
	}

	void bind(const int index, const std::string& value)
	{
		assert(m_stmt);

		const int result = sqlite3_bind_text(m_stmt.get(), index, value.c_str(), value.size(), SQLITE_TRANSIENT);  // This relies on string being present at execution
		// SQLITE_TRANSIENT will make a copy instead

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

		// Look at boost optional
	}
	
	// More getters

private:
	std::unique_ptr<sqlite3_stmt, int(*)(sqlite3_stmt*)> m_stmt;
};


/*
=================
 Transaction
=================
*/
class Transaction 
{
		
};

} // namespace sqlite