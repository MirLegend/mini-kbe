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

#ifndef KBE_KBE_TABLES_H
#define KBE_KBE_TABLES_H

#include "entity_table.h"
#include "common/common.h"
#include "common/memorystream.h"
#include "helper/debug_helper.h"

namespace KBEngine { 

class KBETable : public EntityTable
{
public:
	KBETable():
	EntityTable()
	{
	}
	
	virtual ~KBETable()
	{
	}
	
	/**
		ͬ��entity�����ݿ���
	*/
	virtual bool syncToDB(DBInterface* dbi) = 0;
	
	/**
		��ʼ��
	*/
	virtual bool initialize(std::string nameconst, const DBTABLEITEMS& tableItems){ return true; };
	
	virtual EntityTableItem* createItem(std::string typevirtual, const stTableItem* pTableItemDescription) {return NULL;}
	
protected:

};

/*
	kbeϵͳ��
*/
class KBEEntityLogTable : public KBETable
{
public:
	struct EntityLog
	{
		DBID dbid;
		ENTITY_ID entityID;
		char ip[MAX_IP];
		uint16 port;
		COMPONENT_ID componentID;
	};

	KBEEntityLogTable():
	KBETable()
	{
		tableName("kbe_entitylog");
	}
	
	virtual ~KBEEntityLogTable()
	{
	}
	
	virtual bool logEntity(DBInterface * dbi, const char* ip, uint32 port, DBID dbid,
						COMPONENT_ID componentID, ENTITY_ID entityID) = 0;

	virtual bool queryEntity(DBInterface * dbi, DBID dbid, EntityLog& entitylog) = 0;

	virtual bool eraseEntityLog(DBInterface * dbi, DBID dbid) = 0;
protected:
	
};

class KBEAccountTable : public KBETable
{
public:
	KBEAccountTable():
	KBETable(),
	accountDefMemoryStream_()
	{
		tableName("ziyu_dota_accounts");
	}
	
	virtual ~KBEAccountTable()
	{
	}

	virtual bool queryAccount(DBInterface * dbi, const std::string& name, ACCOUNT_INFOS& info) = 0;
	virtual bool logAccount(DBInterface * dbi, ACCOUNT_INFOS& info) = 0;
	virtual bool setFlagsDeadline(DBInterface * dbi, const std::string& name, uint32 flags, uint64 deadline) = 0;
	virtual bool updateCount(DBInterface * dbi, const std::string& name, DBID dbid) = 0;
	virtual bool queryAccountAllInfos(DBInterface * dbi, const std::string& name, ACCOUNT_INFOS& info) = 0;
	virtual bool updatePassword(DBInterface * dbi, const std::string& name, const std::string& password) = 0;

	MemoryStream& accountDefMemoryStream()
	{ 
		return accountDefMemoryStream_; 
	}

	void accountDefMemoryStream(MemoryStream& s)
	{
		accountDefMemoryStream_.clear(false);
		accountDefMemoryStream_.append(s.data() + s.rpos(), s.length()); 
	}
protected:
	MemoryStream accountDefMemoryStream_;
};

}

#endif // KBE_KBE_TABLES_H
