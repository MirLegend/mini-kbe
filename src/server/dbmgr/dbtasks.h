/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2016 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KBE_DBTASKS_H
#define KBE_DBTASKS_H

// common include	
// #define NDEBUG
#include "common/common.h"
#include "common/memorystream.h"
#include "common/timestamp.h"
#include "thread/threadtask.h"
#include "helper/debug_helper.h"
#include "network/address.h"
#include "db_interface/db_tasks.h"
#include "server/server_errors.h"

namespace KBEngine{ 

class DBInterface;
class Buffered_DBTasks;
struct ACCOUNT_INFOS;

/*
	数据库线程任务基础类
*/

class DBTask : public DBTaskBase
{
public:
	DBTask(const Network::Address& addr, MemoryStream& datas);

	DBTask(const Network::Address& addr):
	DBTaskBase(),
	pDatas_(0),
	addr_(addr)
	{
	}

	virtual ~DBTask();

	bool send(Network::Bundle* pBundle);

protected:
	MemoryStream* pDatas_;
	Network::Address addr_;
};

/**
	执行一条sql语句
*/
class DBTaskExecuteRawDatabaseCommand : public DBTask
{
public:
	DBTaskExecuteRawDatabaseCommand(const Network::Address& addr, MemoryStream& datas);
	virtual ~DBTaskExecuteRawDatabaseCommand();
	virtual bool db_thread_process();
	virtual thread::TPTask::TPTaskState presentMainThread();

protected:
	COMPONENT_ID componentID_;
	COMPONENT_TYPE componentType_;
	std::string sdatas_;
	CALLBACK_ID callbackID_;
	std::string error_;
	MemoryStream execret_;
};

/**
一个新用户登录， 需要检查合法性
*/
class DBTaskAccountLogin : public DBTask
{
public:
	DBTaskAccountLogin(const Network::Address& addr, std::string& loginName,
		std::string& accountName, std::string& password, SERVER_ERROR_CODE retcode, std::string& postdatas, std::string& getdatas);

	virtual ~DBTaskAccountLogin();
	virtual bool db_thread_process();
	virtual thread::TPTask::TPTaskState presentMainThread();

protected:
	std::string loginName_;
	std::string accountName_;
	std::string password_;
	std::string postdatas_, getdatas_;
	SERVER_ERROR_CODE retcode_;
	COMPONENT_ID componentID_;
	ENTITY_ID entityID_;
	DBID dbid_;
	uint32 flags_;
	uint64 deadline_;
};

/**
创建一个账号到数据库
*/
class DBTaskCreateAccount : public DBTask
{
public:
	DBTaskCreateAccount(const Network::Address& addr, std::string& registerName, std::string& accountName,
		std::string& password, std::string& postdatas, std::string& getdatas);
	virtual ~DBTaskCreateAccount();
	virtual bool db_thread_process();
	virtual thread::TPTask::TPTaskState presentMainThread();

	static bool writeAccount(DBInterface* pdbi, const std::string& accountName,
		const std::string& passwd, const std::string& datas, ACCOUNT_INFOS& info);

protected:
	std::string registerName_;
	std::string accountName_;
	std::string password_;
	std::string postdatas_, getdatas_;
	bool success_;

};


/*
EntityDBTask
*/
class EntityDBTask : public DBTask
{
public:
	EntityDBTask(const Network::Address& addr, MemoryStream& datas, ENTITY_ID entityID, DBID entityDBID) :
		DBTask(addr, datas),
		_entityID(entityID),
		_entityDBID(entityDBID),
		_pBuffered_DBTasks(NULL)
	{
	}

	EntityDBTask(const Network::Address& addr, ENTITY_ID entityID, DBID entityDBID) :
		DBTask(addr),
		_entityID(entityID),
		_entityDBID(entityDBID),
		_pBuffered_DBTasks(NULL)
	{
	}

	virtual ~EntityDBTask() {}

	ENTITY_ID EntityDBTask_entityID() const { return _entityID; }
	DBID EntityDBTask_entityDBID() const { return _entityDBID; }

	void pBuffered_DBTasks(Buffered_DBTasks* v) { _pBuffered_DBTasks = v; }
	virtual thread::TPTask::TPTaskState presentMainThread();

	DBTask* tryGetNextTask();

private:
	ENTITY_ID _entityID;
	DBID _entityDBID;
	Buffered_DBTasks* _pBuffered_DBTasks;
};

/**
baseapp请求查询account信息
*/
class DBTaskQueryAccount : public EntityDBTask
{
public:
	DBTaskQueryAccount(const Network::Address& addr, std::string& accountName, std::string& password,
		COMPONENT_ID componentID, ENTITY_ID entityID, DBID entityDBID, uint32 ip, uint16 port);
	virtual ~DBTaskQueryAccount();
	virtual bool db_thread_process();
	virtual thread::TPTask::TPTaskState presentMainThread();

protected:
	std::string accountName_;
	std::string password_;
	bool success_;
	MemoryStream s_;
	DBID dbid_;
	COMPONENT_ID componentID_;
	ENTITY_ID entityID_;
	std::string error_;
	uint32 ip_;
	uint16 port_;

	uint32 flags_;
	uint64 deadline_;
};

/**
从数据库中删除entity
*/
class DBTaskRemoveEntity : public EntityDBTask
{
public:
	DBTaskRemoveEntity(const Network::Address& addr, COMPONENT_ID componentID,
		ENTITY_ID eid, DBID entityDBID/*, MemoryStream& datas*/);

	virtual ~DBTaskRemoveEntity();
	virtual bool db_thread_process();
	virtual thread::TPTask::TPTaskState presentMainThread();

protected:
	COMPONENT_ID componentID_;
	ENTITY_ID eid_;
	DBID entityDBID_;
};


}

#endif // KBE_DBTASKS_H
