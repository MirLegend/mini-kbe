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

#include "dbtasks.h"
#include "dbmgr.h"
#include "buffered_dbtasks.h"
#include "network/common.h"
#include "network/channel.h"
#include "network/message_handler.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include "server/serverconfig.h"
#include "db_interface/db_interface.h"
#include "db_mysql/db_exception.h"
#include "db_mysql/db_interface_mysql.h"
#include "db_interface/kbe_tables.h"
#include "common/md5.h"

#include "proto/ldb.pb.h"
#include "../../server/login/login_interface.h"
#include "proto/basedb.pb.h"
#include "../../server/baseapp/baseapp_interface.h"

#if KBE_PLATFORM == PLATFORM_WIN32
#ifdef _DEBUG
#pragma comment(lib, "libeay32_d.lib")
#pragma comment(lib, "ssleay32_d.lib")
#else
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#endif
#endif


namespace KBEngine{

//-------------------------------------------------------------------------------------
DBTask::DBTask(const Network::Address& addr, MemoryStream& datas):
DBTaskBase(),
pDatas_(0),
addr_(addr)
{
	pDatas_ = MemoryStream::ObjPool().createObject();
	*pDatas_ = datas;
}

//-------------------------------------------------------------------------------------
DBTask::~DBTask()
{
	if(pDatas_)
		MemoryStream::ObjPool().reclaimObject(pDatas_);
}

//-------------------------------------------------------------------------------------
bool DBTask::send(Network::Bundle* pBundle)
{
	Network::Channel* pChannel = DBMgrApp::getSingleton().networkInterface().findChannel(addr_);
	
	if(pChannel){
		pChannel->send(pBundle);
	}
	else{
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
DBTaskAccountLogin::DBTaskAccountLogin(const Network::Address& addr,
	std::string& loginName,
	std::string& accountName,
	std::string& password,
	SERVER_ERROR_CODE retcode,
	std::string& postdatas,
	std::string& getdatas) :
	DBTask(addr),
	loginName_(loginName),
	accountName_(accountName),
	password_(password),
	postdatas_(postdatas),
	getdatas_(getdatas),
	retcode_(retcode),
	componentID_(0),
	entityID_(0),
	dbid_(0),
	flags_(0),
	deadline_(0)
{
}

//-------------------------------------------------------------------------------------
DBTaskAccountLogin::~DBTaskAccountLogin()
{
}

//-------------------------------------------------------------------------------------
bool DBTaskAccountLogin::db_thread_process()
{
	// 如果Interfaces已经判断不成功就没必要继续下去
	if (retcode_ != SERVER_SUCCESS)
	{
		ERROR_MSG(fmt::format("DBTaskAccountLogin::db_thread_process(): interfaces is failed!\n"));
		return false;
	}

	retcode_ = SERVER_ERR_OP_FAILED;

	if (accountName_.size() == 0)
	{
		ERROR_MSG(fmt::format("DBTaskAccountLogin::db_thread_process(): accountName is NULL!\n"));
		retcode_ = SERVER_ERR_NAME;
		return false;
	}

	KBEEntityLogTable* pELTable = static_cast<KBEEntityLogTable*>
		(EntityTables::getSingleton().findKBETable("kbe_entitylog"));
	KBE_ASSERT(pELTable);

	KBEAccountTable* pTable = static_cast<KBEAccountTable*>(EntityTables::getSingleton().findKBETable("ziyu_dota_accounts"));
	KBE_ASSERT(pTable);

	ACCOUNT_INFOS info;
	info.usrId = 0;
	if (!pTable->queryAccount(pdbi_, accountName_, info))
	{
		if (true || g_kbeSrvConfig.getDBMgr().notFoundAccountAutoCreate)
		{
			if (!DBTaskCreateAccount::writeAccount(pdbi_, accountName_, password_, postdatas_, info) || info.usrId == 0)
			{
				ERROR_MSG(fmt::format("DBTaskAccountLogin::db_thread_process(): writeAccount[{}] is error!\n",
					accountName_));

				retcode_ = SERVER_ERR_DB;
				return false;
			}

			INFO_MSG(fmt::format("DBTaskAccountLogin::db_thread_process(): not found account[{}], autocreate successfully!\n",
				accountName_));

			
			{
				info.password = KBE_MD5::getDigest(password_.data(), password_.length());
			}
		}
		else
		{
			ERROR_MSG(fmt::format("DBTaskAccountLogin::db_thread_process(): not found account[{}], login failed!\n",
				accountName_));

			retcode_ = SERVER_ERR_NOT_FOUND_ACCOUNT;
			return false;
		}
	}

	if (info.usrId == 0)
	{
		ERROR_MSG(fmt::format("DBTaskAccountLogin::db_thread_process(): info.dbid == 0, login failed!\n"));
		return false;
	}

	{
		if (kbe_stricmp(info.password.c_str(), KBE_MD5::getDigest(password_.data(), password_.length()).c_str()) != 0)
		{
			retcode_ = SERVER_ERR_PASSWORD;
			return false;
		}
	}

	pTable->updateCount(pdbi_, accountName_, info.usrId);

	retcode_ = SERVER_ERR_ACCOUNT_IS_ONLINE;
	KBEEntityLogTable::EntityLog entitylog;
	bool success = !pELTable->queryEntity(pdbi_, info.usrId, entitylog);

	// 如果有在线纪录
	if (!success)
	{
		componentID_ = entitylog.componentID;
		entityID_ = entitylog.entityID;
	}
	else
	{
		retcode_ = SERVER_SUCCESS;
	}

	dbid_ = info.usrId;
	return false;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState DBTaskAccountLogin::presentMainThread()
{
	DEBUG_MSG(fmt::format("Dbmgr::onAccountLogin:loginName={0}, accountName={1}, success={2}, componentID={3}, dbid={4}, flags={5}, deadline={6}.\n",
		loginName_,
		accountName_,
		retcode_,
		componentID_,
		dbid_,
		flags_,
		deadline_
		));

	// 一个用户登录， 构造一个数据库查询指令并加入到执行队列， 执行完毕将结果返回给loginapp
	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	pBundle->newMessage(LoginappInterface::onLoginAccountQueryResultFromDbmgr);
	login_dbmgr::AccountLoginQueryResult lqrCmd;
	lqrCmd.set_retcode(retcode_);
	lqrCmd.set_loginname(accountName_);
	lqrCmd.set_accountname(accountName_);
	lqrCmd.set_password(password_);
	lqrCmd.set_componentid(componentID_);
	lqrCmd.set_entityid(entityID_);
	lqrCmd.set_dbid(dbid_);
	lqrCmd.set_flags(flags_);
	lqrCmd.set_getdatas(getdatas_);
	ADDTOBUNDLE((*pBundle), lqrCmd)

	if (!this->send(pBundle))
	{
		ERROR_MSG(fmt::format("DBTaskAccountLogin::presentMainThread: channel({}) not found.\n", addr_.c_str()));
		Network::Bundle::ObjPool().reclaimObject(pBundle);
	}

	return DBTask::presentMainThread();
}

//-------------------------------------------------------------------------------------
DBTaskCreateAccount::DBTaskCreateAccount(const Network::Address& addr,
	std::string& registerName,
	std::string& accountName,
	std::string& password,
	std::string& postdatas,
	std::string& getdatas) :
	DBTask(addr),
	registerName_(registerName),
	accountName_(accountName),
	password_(password),
	postdatas_(postdatas),
	getdatas_(getdatas),
	success_(false)
{
}

//-------------------------------------------------------------------------------------
DBTaskCreateAccount::~DBTaskCreateAccount()
{
}

//-------------------------------------------------------------------------------------
bool DBTaskCreateAccount::db_thread_process()
{
	ACCOUNT_INFOS info;
	success_ = DBTaskCreateAccount::writeAccount(pdbi_, accountName_, password_, postdatas_, info) && info.usrId > 0;
	return false;
}

//-------------------------------------------------------------------------------------
bool DBTaskCreateAccount::writeAccount(DBInterface* pdbi, const std::string& accountName,
	const std::string& passwd, const std::string& datas, ACCOUNT_INFOS& info)
{
	info.usrId = 0;
	if (accountName.size() == 0)
	{
		return false;
	}

	// 寻找dblog是否有此账号， 如果有则创建失败
	// 如果没有则向account表新建一个entity数据同时在accountlog表写入一个log关联dbid
	KBEAccountTable* pTable = static_cast<KBEAccountTable*>(EntityTables::getSingleton().findKBETable("ziyu_dota_accounts"));
	KBE_ASSERT(pTable);

	if (pTable->queryAccount(pdbi, accountName, info))
	{
		if (pdbi->getlasterror() > 0)
		{
			WARNING_MSG(fmt::format("DBTaskCreateAccount::writeAccount(): queryAccount error: {}\n",
				pdbi->getstrerror()));
		}

		return false;
	}

	bool hasset = (info.usrId != 0);
	if (!hasset)
	{
		//info.flags = g_kbeSrvConfig.getDBMgr().accountDefaultFlags;
		//info.deadline = g_kbeSrvConfig.getDBMgr().accountDefaultDeadline;
	}

	info.name = accountName;
	info.password = passwd;
	//info.usrId = entityDBID;

	if (!hasset)
	{
		if (!pTable->logAccount(pdbi, info))
		{
			if (pdbi->getlasterror() > 0)
			{
				WARNING_MSG(fmt::format("DBTaskCreateAccount::writeAccount(): logAccount error:{}\n",
					pdbi->getstrerror()));
			}
			return false;
		}
		DBID entityDBID = info.usrId;
		KBE_ASSERT(entityDBID > 0);
		//if (entityDBID == 0)
		{
			// 防止多线程问题， 这里做一个拷贝。
			MemoryStream copyAccountDefMemoryStream/*(pTable->accountDefMemoryStream())*/;
			EntityTable* pPlayerTable = EntityTables::getSingleton().findTable("dota_players");
			KBE_ASSERT(pPlayerTable != NULL);
			copyAccountDefMemoryStream << pPlayerTable->findItemUtype("id");
			copyAccountDefMemoryStream << entityDBID;
			DEBUG_MSG(fmt::format("-------------Creat Player dbid:{}------------", entityDBID));
			pPlayerTable->writeTable(pdbi, 0, true, &copyAccountDefMemoryStream);
			//entityDBID = EntityTables::getSingleton().writeEntity(pdbi, 0/*entityDBID*/, true,
			//	&copyAccountDefMemoryStream, "dota_players");
		}
	}
	else
	{
		if (!pTable->setFlagsDeadline(pdbi, accountName, 0, 0/*info.flags, info.deadline*/))
		{
			if (pdbi->getlasterror() > 0)
			{
				WARNING_MSG(fmt::format("DBTaskCreateAccount::writeAccount(): logAccount error:{}\n",
					pdbi->getstrerror()));
			}

			return false;
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState DBTaskCreateAccount::presentMainThread()
{
	DEBUG_MSG(fmt::format("Dbmgr::reqCreateAccount:{}.\n", registerName_.c_str()));

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	//TODO::

	if (!this->send(pBundle))
	{
		ERROR_MSG(fmt::format("DBTaskCreateAccount::presentMainThread: channel({}) not found.\n", addr_.c_str()));
		Network::Bundle::ObjPool().reclaimObject(pBundle);
	}

	return thread::TPTask::TPTASK_STATE_COMPLETED;
}


//-------------------------------------------------------------------------------------
DBTaskExecuteRawDatabaseCommand::DBTaskExecuteRawDatabaseCommand(const Network::Address& addr, MemoryStream& datas):
DBTask(addr, datas),
componentID_(0),
componentType_(UNKNOWN_COMPONENT_TYPE),
sdatas_(),
callbackID_(0),
error_(),
execret_()
{
}

//-------------------------------------------------------------------------------------
DBTaskExecuteRawDatabaseCommand::~DBTaskExecuteRawDatabaseCommand()
{
}

//-------------------------------------------------------------------------------------
bool DBTaskExecuteRawDatabaseCommand::db_thread_process()
{
	(*pDatas_) >> componentID_ >> componentType_;
	(*pDatas_) >> callbackID_;
	(*pDatas_).readBlob(sdatas_);

	try
	{
		if(!pdbi_->query(sdatas_.data(), sdatas_.size(), false, &execret_))
		{
			error_ = pdbi_->getstrerror();
		}
	}
	catch (std::exception & e)
	{
		DBException& dbe = static_cast<DBException&>(e);
		if(dbe.isLostConnection())
		{
			static_cast<DBInterfaceMysql*>(pdbi_)->processException(e);
			return true;
		}

		error_ = e.what();
	}

	return false;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState DBTaskExecuteRawDatabaseCommand::presentMainThread()
{
	DEBUG_MSG(fmt::format("DBTask::ExecuteRawDatabaseCommandByEntity::presentMainThread: {}.\n", sdatas_.c_str()));

	// 如果不需要回调则结束
	if (callbackID_ <= 0)
		return thread::TPTask::TPTASK_STATE_COMPLETED;

	/*Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();

	if (componentType_ == BASEAPP_TYPE)
		(*pBundle).newMessage(BaseappInterface::onExecuteRawDatabaseCommandCB);
	else if (componentType_ == CELLAPP_TYPE)
		(*pBundle).newMessage(CellappInterface::onExecuteRawDatabaseCommandCB);
	else
	{
		KBE_ASSERT(false && "no support!\n");
	}

	(*pBundle) << callbackID_;
	(*pBundle) << error_;

	if (error_.size() <= 0)
		(*pBundle).append(execret_);

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(componentType_, componentID_);

	if (cinfos && cinfos->pChannel)
	{
		cinfos->pChannel->send(pBundle);
	}
	else
	{
		ERROR_MSG(fmt::format("DBTask::ExecuteRawDatabaseCommandByEntity::presentMainThread: {} not found!\n",
			COMPONENT_NAME_EX(componentType_)));

		Network::Bundle::ObjPool().reclaimObject(pBundle);
	}*/

	return thread::TPTask::TPTASK_STATE_COMPLETED;
}

//-------------------------------------------------------------------------------------
DBTask* EntityDBTask::tryGetNextTask()
{
	KBE_ASSERT(_pBuffered_DBTasks != NULL);
	return _pBuffered_DBTasks->tryGetNextTask(this);
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState EntityDBTask::presentMainThread()
{
	return DBTask::presentMainThread();
}


//-------------------------------------------------------------------------------------
DBTaskQueryAccount::DBTaskQueryAccount(const Network::Address& addr, std::string& accountName, std::string& password,
	COMPONENT_ID componentID, ENTITY_ID entityID, DBID entityDBID, uint32 ip, uint16 port) :
	EntityDBTask(addr, entityID, entityDBID),
	accountName_(accountName),
	password_(password),
	success_(false),
	s_(),
	dbid_(entityDBID),
	componentID_(componentID),
	entityID_(entityID),
	error_(),
	ip_(ip),
	port_(port),
	flags_(0),
	deadline_(0)
{
}

//-------------------------------------------------------------------------------------
DBTaskQueryAccount::~DBTaskQueryAccount()
{
}

//-------------------------------------------------------------------------------------
bool DBTaskQueryAccount::db_thread_process()
{
	if (accountName_.size() == 0)
	{
		error_ = "accountName_ is NULL";
		return false;
	}

	KBEAccountTable* pTable = static_cast<KBEAccountTable*>(EntityTables::getSingleton().findKBETable("ziyu_dota_accounts"));
	KBE_ASSERT(pTable);

	ACCOUNT_INFOS info;
	info.name = "";
	info.password = "";
	info.usrId = dbid_;

	if (dbid_ == 0)
	{
		if (!pTable->queryAccount(pdbi_, accountName_, info))
		{
			error_ = "pTable->queryAccount() is failed!";

			if (pdbi_->getlasterror() > 0)
			{
				error_ += pdbi_->getstrerror();
			}

			return false;
		}

		if (info.usrId == 0 /*|| info.flags != ACCOUNT_FLAG_NORMAL*/)
		{
			error_ = "dbid is 0 or flags != ACCOUNT_FLAG_NORMAL";
			return false;
		}

		if (kbe_stricmp(info.password.c_str(), KBE_MD5::getDigest(password_.data(), password_.length()).c_str()) != 0)
		{
			error_ = "password is error";
			return false;
		}
	}

	//ScriptDefModule* pModule = EntityDef::findScriptModule(DBUtil::accountScriptName());
	//success_ = EntityTables::getSingleton().queryEntity(pdbi_, info.dbid, &s_, pModule);
	success_ = EntityTables::getSingleton().queryEntity(pdbi_, info.usrId, &s_, "dota_players");

	if (!success_ && pdbi_->getlasterror() > 0)
	{
		error_ += "queryEntity: ";
		error_ += pdbi_->getstrerror();
	}

	dbid_ = info.usrId;

	if (!success_)
		return false;

	success_ = false;

	// 先写log， 如果写失败则可能这个entity已经在线
	KBEEntityLogTable* pELTable = static_cast<KBEEntityLogTable*>
		(EntityTables::getSingleton().findKBETable("kbe_entitylog"));
	KBE_ASSERT(pELTable);

	success_ = pELTable->logEntity(pdbi_, inet_ntoa((struct in_addr&)ip_), port_, dbid_,
		componentID_, entityID_);

	if (!success_ && pdbi_->getlasterror() > 0)
	{
		error_ += "logEntity: ";
		error_ += pdbi_->getstrerror();
	}

	return false;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState DBTaskQueryAccount::presentMainThread()
{
	DEBUG_MSG(fmt::format("Dbmgr::queryAccount:{}, success={}, flags={}, deadline={}.\n",
		accountName_.c_str(), success_, flags_, deadline_));

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(BaseappInterface::onQueryPlayerCBFromDbmgr);
	
	base_dbmgr::QueryPlayerCBFromDbmgr queryCmd;
	queryCmd.set_account(accountName_);
	queryCmd.set_password(password_);
	queryCmd.set_entitydbid(dbid_);
	queryCmd.set_success(success_);
	queryCmd.set_entityid(entityID_);

	if (success_)
	{
		queryCmd.set_datas(s_.data() + s_.rpos(), s_.length());
		//pBundle->append(s_);
	}
	else
	{
		queryCmd.set_datas(error_);
		//(*pBundle) << error_;
	}
	ERROR_MSG(fmt::format("DBTaskQueryAccount::QueryPlayerCBFromDbmgr size:({}) \n", queryCmd.ByteSize()));
	ADDTOBUNDLE((*pBundle), queryCmd);
	if (!this->send(pBundle))
	{
		ERROR_MSG(fmt::format("DBTaskQueryAccount::presentMainThread: channel({}) not found.\n", addr_.c_str()));
		Network::Bundle::ObjPool().reclaimObject(pBundle);
	}

	return EntityDBTask::presentMainThread();
}


//-------------------------------------------------------------------------------------
DBTaskRemoveEntity::DBTaskRemoveEntity(const Network::Address& addr,
	COMPONENT_ID componentID, ENTITY_ID eid,
	DBID entityDBID/*, MemoryStream& datas*/) :
	EntityDBTask(addr, MemoryStream(), eid, entityDBID),
	componentID_(componentID),
	eid_(eid),
	entityDBID_(entityDBID)
{
}

//-------------------------------------------------------------------------------------
DBTaskRemoveEntity::~DBTaskRemoveEntity()
{
}

//-------------------------------------------------------------------------------------
bool DBTaskRemoveEntity::db_thread_process()
{

	KBEEntityLogTable* pELTable = static_cast<KBEEntityLogTable*>
		(EntityTables::getSingleton().findKBETable("kbe_entitylog"));
	KBE_ASSERT(pELTable);
	pELTable->eraseEntityLog(pdbi_, entityDBID_);

	//EntityTables::getSingleton().removeEntity(pdbi_, entityDBID_);
	return false;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState DBTaskRemoveEntity::presentMainThread()
{
	DEBUG_MSG(fmt::format("Dbmgr::removeEntity: ({}).\n",  entityDBID_));
	return EntityDBTask::presentMainThread();
}

//-------------------------------------------------------------------------------------
}
