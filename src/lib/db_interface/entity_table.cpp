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

#include "db_tasks.h"
#include "db_threadpool.h"
#include "entity_table.h"
#include "db_interface.h"
//#include "entitydef/entitydef.h"
//#include "entitydef/scriptdef_module.h"
#include "thread/threadguard.h"

namespace KBEngine { 
KBE_SINGLETON_INIT(EntityTables);

EntityTables g_EntityTables;

//-------------------------------------------------------------------------------------
void EntityTable::addItem(EntityTableItem* pItem)
{
	tableItems_[pItem->utype()].reset(pItem);
	tableFixedOrderItems_.push_back(pItem);
}

//-------------------------------------------------------------------------------------
EntityTableItem* EntityTable::findItem(int32/*ENTITY_PROPERTY_UID*/ utype)
{
	TABLEITEM_MAP::iterator iter = tableItems_.find(utype);
	if(iter != tableItems_.end())
	{
		return iter->second.get();
	}

	return NULL;
}

//-------------------------------------------------------------------------------------
int32 EntityTable::findItemUtype(const char* itemName)
{
	//std::string dbName(itemName);
	DBTABLEITEMS::const_iterator iter = mpTableItemDef->find(/*"sm_" + */itemName);
	if (iter != mpTableItemDef->end())
	{
		return iter->second.utype;
	}

	return 0;
}

//-------------------------------------------------------------------------------------
DBID EntityTable::writeTable(DBInterface* dbi, DBID dbid, bool binsert, MemoryStream* s)
{
	//while(s->length() > 0)
	//{
	//	//ENTITY_PROPERTY_UID pid;
	//	//(*s) >> pid;
	//	
	//	EntityTableItem* pTableItem = this->findItem(pid);
	//	if(pTableItem == NULL)
	//	{
	//		ERROR_MSG(fmt::format("EntityTable::writeTable: not found item[{}].\n", pid));
	//		return dbid;
	//	}

	//	if(!pTableItem->writeItem(dbi, dbid, s))
	//	{
	//		return dbid;
	//	}
	//};

	return dbid;
}

//-------------------------------------------------------------------------------------
bool EntityTable::removeEntity(DBInterface* dbi, DBID dbid)
{
	return true;
}

//-------------------------------------------------------------------------------------
bool EntityTable::queryTable(DBInterface* dbi, DBID dbid, MemoryStream* s)
{
	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	for(; iter != tableFixedOrderItems_.end(); ++iter)
	{
		if(!(*iter)->queryTable(dbi, dbid, s))
			return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
EntityTables::EntityTables():
tables_(),
kbe_tables_(),
numSyncTables_(0),
syncTablesError_(false)
{
}

//-------------------------------------------------------------------------------------
EntityTables::~EntityTables()
{
	tables_.clear();
	kbe_tables_.clear();
}

//-------------------------------------------------------------------------------------
bool EntityTables::load(DBInterface* dbi, const DBTABLEDEFS& tabelDefs)
{
	DBTABLEDEFS::const_iterator iter = tabelDefs.begin();
	for (; iter != tabelDefs.end(); ++iter)
	{
		const std::string& tableName = iter->first;
		const DBTABLEITEMS& tableItems = iter->second;

		EntityTable* pEtable = dbi->createEntityTable();

		if (!pEtable)
			continue;

		if (!pEtable->initialize(tableName, tableItems))
		{
			delete pEtable;
			return false;
		}

		tables_[pEtable->tableName()].reset(pEtable);
	}

	return true;
}

//-------------------------------------------------------------------------------------
void EntityTables::onTableSyncSuccessfully(KBEShared_ptr<EntityTable> pEntityTable, bool error)
{
	if(error)
	{
		syncTablesError_ = true;
		return;
	}

	numSyncTables_++;
}

//-------------------------------------------------------------------------------------
bool EntityTables::syncToDB(DBInterface* dbi)
{
	DBThreadPool* pDBThreadPool = static_cast<DBThreadPool*>(DBUtil::pThreadPool());
	KBE_ASSERT(pDBThreadPool != NULL);
	
	int num = 0;
	try
	{
		// 开始同步所有表
		EntityTables::TABLES_MAP::iterator kiter = kbe_tables_.begin();
		for(; kiter != kbe_tables_.end(); ++kiter)
		{
			num++;
			pDBThreadPool->addTask(new DBTaskSyncTable(kiter->second));
		}

		EntityTables::TABLES_MAP::iterator iter = tables_.begin();
		for(; iter != tables_.end(); ++iter)
		{
			if(!iter->second->hasSync())
			{
				num++;
				pDBThreadPool->addTask(new DBTaskSyncTable(iter->second));
			}
		}

		while(true)
		{
			if(syncTablesError_)
				return false;

			if(numSyncTables_ == num)
				break;

			pDBThreadPool->onMainThreadTick();
			sleep(10);
		};


		std::vector<std::string> dbTableNames;
		dbi->getTableNames(dbTableNames, "");

		// 检查是否有需要删除的表
		std::vector<std::string>::iterator iter0 = dbTableNames.begin();
		for(; iter0 != dbTableNames.end(); ++iter0)
		{
			std::string tname = (*iter0);
			if(std::string::npos == tname.find(ENTITY_TABLE_PERFIX"_"))
				continue;

			KBEngine::strutil::kbe_replace(tname, ENTITY_TABLE_PERFIX"_", "");
			EntityTables::TABLES_MAP::iterator iter = tables_.find(tname);
			if(iter == tables_.end())
			{
				if(!dbi->dropEntityTableFromDB((std::string(ENTITY_TABLE_PERFIX"_") + tname).c_str()))
					return false;
			}
		}
	}
	catch (std::exception& e)
	{
		ERROR_MSG(fmt::format("EntityTables::syncToDB: {}\n", e.what()));
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void EntityTables::addTable(EntityTable* pTable)
{
	TABLES_MAP::iterator iter = tables_.begin();

	for(; iter != tables_.end(); ++iter)
	{
		if(iter->first == pTable->tableName())
		{
			KBE_ASSERT(false && "table exist!\n");
			return;
		}
	}
	tables_[pTable->tableName()].reset(pTable);
}

//-------------------------------------------------------------------------------------
EntityTable* EntityTables::findTable(std::string name)
{
	TABLES_MAP::iterator iter = tables_.find(name);
	if(iter != tables_.end())
	{
		return iter->second.get();
	}

	return NULL;
};

//-------------------------------------------------------------------------------------
void EntityTables::addKBETable(EntityTable* pTable)
{
	TABLES_MAP::iterator iter = kbe_tables_.begin();

	for(; iter != kbe_tables_.end(); ++iter)
	{
		if(iter->first == pTable->tableName())
		{
			KBE_ASSERT(false && "table exist!\n");
			return;
		}
	}

	kbe_tables_[pTable->tableName()].reset(pTable);
}

//-------------------------------------------------------------------------------------
EntityTable* EntityTables::findKBETable(std::string name)
{
	TABLES_MAP::iterator iter = kbe_tables_.find(name);
	if(iter != kbe_tables_.end())
	{
		return iter->second.get();
	}

	return NULL;
};

//-------------------------------------------------------------------------------------
DBID EntityTables::writeEntity(DBInterface* dbi, DBID dbid, bool binsert, MemoryStream* s, const std::string tableName)
{
	EntityTable* pTable = this->findTable(tableName);
	KBE_ASSERT(pTable != NULL);

	return pTable->writeTable(dbi, dbid, binsert, s);
}

//-------------------------------------------------------------------------------------
bool EntityTables::removeEntity(DBInterface* dbi, DBID dbid, const std::string& tableName)
{
	EntityTable* pTable = this->findTable(tableName);
	KBE_ASSERT(pTable != NULL);

	return pTable->removeEntity(dbi, dbid);
}

//-------------------------------------------------------------------------------------
bool EntityTables::queryEntity(DBInterface* dbi, DBID dbid, MemoryStream* s, const std::string& tableName)
{
	EntityTable* pTable = this->findTable(tableName);
	KBE_ASSERT(pTable != NULL);

	return pTable->queryTable(dbi, dbid, s);
}

//-------------------------------------------------------------------------------------
}
