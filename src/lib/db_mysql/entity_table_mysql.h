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

#ifndef KBE_ENTITY_TABLE_MYSQL_H
#define KBE_ENTITY_TABLE_MYSQL_H
#include "db_interface_mysql.h"
#include "common.h"
#include "common/common.h"
#include "common/singleton.h"
#include "helper/debug_helper.h"
#include "db_interface/entity_table.h"

namespace KBEngine { 

class EntityTableMysql;

#define MYSQL_ENGINE_TYPE "InnoDB"

/*
	维护entity在数据库中的表
*/
class EntityTableMysql : public EntityTable
{
public:
	EntityTableMysql();
	virtual ~EntityTableMysql();
	
	/**
		初始化
	*/
	virtual bool initialize(std::string name);

	/**
		同步entity表到数据库中
	*/
	virtual bool syncToDB(DBInterface* dbi);

	/**
		同步表索引
	*/
	virtual bool syncIndexToDB(DBInterface* dbi);

	/** 
		创建一个表item
	*/
	virtual EntityTableItem* createItem(std::string type);

	DBID writeTable(DBInterface* dbi, DBID dbid, int8 shouldAutoLoad, MemoryStream* s);

	/**
		从数据库删除entity
	*/
	bool removeEntity(DBInterface* dbi, DBID dbid);

	/**
		获取所有的数据放到流中
	*/
	virtual bool queryTable(DBInterface* dbi, DBID dbid, MemoryStream* s);

	/**
		设置是否自动加载
	*/
	virtual void entityShouldAutoLoad(DBInterface* dbi, DBID dbid, bool shouldAutoLoad) {};

	/**
		查询自动加载的实体
	*/
	virtual void queryAutoLoadEntities(DBInterface* dbi,
		ENTITY_ID start, ENTITY_ID end, std::vector<DBID>& outs) {};

	/**
		获取某个表所有的数据放到流中
	*/
	void addToStream(MemoryStream* s, DBContext& context, DBID resultDBID);

	/**
		获取需要存储的表名， 字段名和转换为sql存储时的字符串值
	*/
	virtual void getWriteSqlItem(DBInterface* dbi, MemoryStream* s, DBContext& context);
	virtual void getReadSqlItem(DBContext& context);

	void init_db_item_name();

protected:
	
};


}

#ifdef CODE_INLINE
#include "entity_table_mysql.inl"
#endif
#endif // KBE_ENTITY_TABLE_MYSQL_H
