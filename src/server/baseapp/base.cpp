/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2017 KBEngine.

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

#include "baseapp.h"
#include "base.h"
#include "base_messages_forward_handler.h"
#include "network/channel.h"	

#ifndef CODE_INLINE
#include "base.inl"
#endif

#include "proto/cb.pb.h"
#include "proto/bmb.pb.h"
#include "../../server/basemgr/basemgr_interface.h"
#include "proto/basedb.pb.h"
#include "../../server/dbmgr/dbmgr_interface.h"

namespace KBEngine{


//-------------------------------------------------------------------------------------
Base::Base(ENTITY_ID id, bool isInitialised):
id_(id),
isDestroyed_(false),
flags_(ENTITY_FLAGS_INITING),
hasDB_(false),
DBID_(0),
isGetingCellData_(false),
isArchiveing_(false),
shouldAutoArchive_(1),
shouldAutoBackup_(1),
creatingCell_(false),
createdSpace_(false),
inRestore_(false),
pBufferedSendToClientMessages_(NULL),
isDirty_(true),
dbInterfaceIndex_(0)
{
}

//-------------------------------------------------------------------------------------
Base::~Base()
{
	isDestroyed_ = true;
	removeFlags(ENTITY_FLAGS_INITING);
	SAFE_RELEASE(pBufferedSendToClientMessages_);

	if(BaseApp::getSingleton().pEntities())
		BaseApp::getSingleton().pEntities()->pGetbages()->erase(id());
}	

//-------------------------------------------------------------------------------------
void Base::onDestroy(bool callScript)
{
	setDirty();
	
	if(callScript)
	{
	}

	if(this->hasDB())
	{
	}
	
	eraseEntityLog();
}

//-------------------------------------------------------------------------------------
void Base::eraseEntityLog()
{
	// 这里没有使用hasDB()来进行判断
	// 用户可能destroy( writeToDB = False ), 这个操作会导致hasDB为false， 因此这里
	// 需要判断dbid是否大于0， 如果大于0则应该要去擦除在线等记录情况.
	if(this->dbid() > 0)
	{
		/*Network::Bundle* pBundle = Network::Bundle::createPoolObject();
		(*pBundle).newMessage(DbmgrInterface::onEntityOffline);
		(*pBundle) << this->dbid();
		(*pBundle) << this->pScriptModule()->getUType();
		(*pBundle) << dbInterfaceIndex();

		Components::COMPONENTS& cts = Components::getSingleton().getComponents(DBMGR_TYPE);
		Components::ComponentInfos* dbmgrinfos = NULL;

		if(cts.size() > 0)
			dbmgrinfos = &(*cts.begin());

		if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
		{
			ERROR_MSG("Base::onDestroy: not found dbmgr!\n");
			Network::Bundle::reclaimPoolObject(pBundle);
			return;
		}

		dbmgrinfos->pChannel->send(pBundle);*/
	}
}

//-------------------------------------------------------------------------------------
void Base::addPersistentsDataToStream(uint32 flags, MemoryStream* s)
{
	
}

void Base::destroyEntity()
{
	BaseApp::getSingleton().destroyEntity(id_, true);
}

//-------------------------------------------------------------------------------------
void Base::onDestroyEntity(bool deleteFromDB, bool writeToDB)
{
	if(deleteFromDB && hasDB())
	{
		Components::COMPONENTS& cts = Components::getSingleton().getComponents(DBMGR_TYPE);
		Components::ComponentInfos* dbmgrinfos = NULL;

		if (cts.size() > 0)
			dbmgrinfos = &(*cts.begin());

		if (dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
		{
			ERROR_MSG(fmt::format("onDestroyEntity({}): writeToDB not found dbmgr!\n", id()));
			return;
		}
		Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
		(*pBundle).newMessage(DbmgrInterface::removeEntity);

		base_dbmgr::RemoveEntity removeCmd;
		removeCmd.set_componentid(g_componentID);
		removeCmd.set_entityid(id());
		removeCmd.set_entitydbid(dbid());

		ADDTOBUNDLE((*pBundle), removeCmd)
			dbmgrinfos->pChannel->send(pBundle);

		this->hasDB(false);
		return;
	}

	if(writeToDB)
	{
		// 这个行为默认会处理
		// this->writeToDB(NULL);
	}
	else
	{
		this->hasDB(false);
	}

	shouldAutoArchive_ = 0;
	shouldAutoBackup_ = 0;
}

//-------------------------------------------------------------------------------------
void Base::onClientDeath()
{
	
}

//-------------------------------------------------------------------------------------
void Base::onRestore()
{
	if(!inRestore_)
		return;

	inRestore_ = false;
	isArchiveing_ = false;
	removeFlags(ENTITY_FLAGS_INITING);
}

//-------------------------------------------------------------------------------------
void Base::writeBackupData(MemoryStream* s)
{
	onBackup();
}

//-------------------------------------------------------------------------------------
void Base::onBackup()
{
	//reqBackupCellData();
}

//-------------------------------------------------------------------------------------
void Base::writeToDB(void* data, void* extra1, void* extra2)
{
	//PyObject* pyCallback = NULL;
	//int8 shouldAutoLoad = dbid() <= 0 ? 0 : -1;

	//// data 是有可能会NULL的， 比如定时存档是不需要回调函数的
	//if(data != NULL)
	//	pyCallback = static_cast<PyObject*>(data);

	//if(extra1 != NULL && (*static_cast<int*>(extra1)) != -1)
	//	shouldAutoLoad = (*static_cast<int*>(extra1)) > 0 ? 1 : 0;

	//if (extra2)
	//{
	//	if (strlen(static_cast<char*>(extra2)) > 0)
	//	{
	//		DBInterfaceInfo* pDBInterfaceInfo = g_kbeSrvConfig.dbInterface(static_cast<char*>(extra2));
	//		if (pDBInterfaceInfo->isPure)
	//		{
	//			ERROR_MSG(fmt::format("Base::writeToDB: dbInterface({}) is a pure database does not support Entity! "
	//				"kbengine[_defs].xml->dbmgr->databaseInterfaces->*->pure\n",
	//				static_cast<char*>(extra2)));

	//			return;
	//		}

	//		int dbInterfaceIndex = pDBInterfaceInfo->index;
	//		if (dbInterfaceIndex >= 0)
	//		{
	//			dbInterfaceIndex_ = dbInterfaceIndex;
	//		}
	//		else
	//		{
	//			ERROR_MSG(fmt::format("Base::writeToDB: not found dbInterface({})!\n",
	//				static_cast<char*>(extra2)));

	//			return;
	//		}
	//	}
	//}

	//if(isArchiveing_)
	//{
	//	// __py_pyWriteToDB没有增加引用
	//	//if(pyCallback != NULL)
	//	//	Py_DECREF(pyCallback);

	//	WARNING_MSG(fmt::format("{}::writeToDB(): is archiveing! entityid={}, dbid={}.\n", 
	//		this->scriptName(), this->id(), this->dbid()));

	//	return;
	//}

	//isArchiveing_ = true;

	//if(isDestroyed())
	//{	
	//	// __py_pyWriteToDB没有增加引用
	//	//if(pyCallback != NULL)
	//	//	Py_DECREF(pyCallback);

	//	ERROR_MSG(fmt::format("{}::writeToDB(): is destroyed! entityid={}, dbid={}.\n", 
	//		this->scriptName(), this->id(), this->dbid()));

	//	return;
	//}

	//CALLBACK_ID callbackID = 0;
	//if(pyCallback != NULL)
	//{
	//	callbackID = callbackMgr().save(pyCallback);
	//}

	//// creatingCell_ 此时可能正在创建cell
	//// 不过我们在此假设在cell未创建完成的时候base这个接口被调用
	//// 写入数据库的是该entity的初始值， 并不影响
	//if(this->cellMailbox() == NULL) 
	//{
	//	onCellWriteToDBCompleted(callbackID, shouldAutoLoad, -1);
	//}
	//else
	//{
	//	Network::Bundle* pBundle = Network::Bundle::createPoolObject();
	//	(*pBundle).newMessage(CellappInterface::reqWriteToDBFromBaseapp);
	//	(*pBundle) << this->id();
	//	(*pBundle) << callbackID;
	//	(*pBundle) << shouldAutoLoad;
	//	sendToCellapp(pBundle);
	//}
}

//-------------------------------------------------------------------------------------
void Base::onWriteToDBCallback(ENTITY_ID eid, 
								DBID entityDBID, 
								CALLBACK_ID callbackID, 
								int8 shouldAutoLoad,
								bool success)
{
	isArchiveing_ = false;


	if(dbid() <= 0)
	{
		dbid(entityDBID);
	}
	
	if (dbid() <= 0)
	{
		KBE_ASSERT(!success);
		hasDB(false);
	}

	if(callbackID > 0)
	{
		
	}
}

//-------------------------------------------------------------------------------------
void Base::onWriteToDB()
{
	
}

//-------------------------------------------------------------------------------------
void Base::onBufferedForwardToCellappMessagesOver()
{
}

//-------------------------------------------------------------------------------------
void Base::onBufferedForwardToClientMessagesOver()
{
	//onMigrationCellappOver(pBufferedSendToClientMessages_->cellappID());
	SAFE_RELEASE(pBufferedSendToClientMessages_);
}

//-------------------------------------------------------------------------------------
void Base::onGetDBID(Network::Channel* pChannel, DBID dbid)
{
	if(pChannel->isExternal())
		return;
}

////-------------------------------------------------------------------------------------
//void Base::onTimer(ScriptID timerID, int useraAgs)
//{
//}
//
////-------------------------------------------------------------------------------------
//bool Base::_reload(bool fullReload)
//{
//	return true;
//}

//-------------------------------------------------------------------------------------
}
