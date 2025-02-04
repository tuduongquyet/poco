//
// Transaction.cpp
//
// Library: Data
// Package: DataCore
// Module:  Transaction
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Data/Transaction.h"
#include "Poco/Exception.h"


namespace Poco {
namespace Data {


Transaction::Transaction(Poco::Data::Session& rSession, Poco::Logger* pLogger):
	_rSession(rSession),
	_autoCommit(_rSession.hasFeature("autoCommit") ? _rSession.getFeature("autoCommit") : false),
	_pLogger(pLogger)
{
	begin();
}


Transaction::Transaction(Poco::Data::Session& rSession, bool start):
	_rSession(rSession),
	_autoCommit(_rSession.hasFeature("autoCommit") ? _rSession.getFeature("autoCommit") : false),
	_pLogger(0)
{
	if (start) begin();
}


Transaction::~Transaction()
{
	try
	{
		if (_rSession.isTransaction())
		{
			try
			{
				if (_pLogger)
					_pLogger->debug("Rolling back transaction.");

				_rSession.rollback();
			}
			catch (Poco::Exception& exc)
			{
				if (_pLogger)
					_pLogger->error("Error while rolling back database transaction: %s", exc.displayText());
			}
			catch (...)
			{
				if (_pLogger)
					_pLogger->error("Error while rolling back database transaction.");
			}
		}
	}
	catch (...)
	{
		poco_unexpected();
	}
}


void Transaction::begin()
{
	if (!_rSession.isTransaction())
	{
		if (_autoCommit)
			_rSession.setFeature("autoCommit", false);
		_rSession.begin();
	}
	else
		throw InvalidAccessException("Transaction in progress.");
}


void Transaction::execute(const std::string& sql, bool doCommit)
{
	if (!_rSession.isTransaction()) _rSession.begin();
	_rSession << sql, Keywords::now;
	if (doCommit) commit();
}


bool Transaction::execute(const std::vector<std::string>& sql)
{
	return execute(sql, nullptr);
}

bool Transaction::execute(const std::vector<std::string>& sql, std::string* info)
{
	try
	{
		std::vector<std::string>::const_iterator it = sql.begin();
		std::vector<std::string>::const_iterator end = sql.end();
		for (; it != end; ++it)	execute(*it, it + 1 == end ? true : false);
		return true;
	}
	catch (Exception& ex)
	{
		if (_pLogger) _pLogger->log(ex);
		if(info) *info = ex.displayText();
	}

	rollback();
	return false;
}


void Transaction::commit()
{
	if (_pLogger)
		_pLogger->debug("Committing transaction.");

	_rSession.commit();
	if (_autoCommit)
		_rSession.setFeature("autoCommit", true);
}


void Transaction::rollback()
{
	if (_pLogger)
		_pLogger->debug("Rolling back transaction.");

	_rSession.rollback();
	if (_autoCommit)
		_rSession.setFeature("autoCommit", true);
}


} } // namespace Poco::Data
