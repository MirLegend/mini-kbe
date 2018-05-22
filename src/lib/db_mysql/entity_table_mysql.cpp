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

#include "entity_table_mysql.h"
#include "kbe_table_mysql.h"
#include "read_entity_helper.h"
#include "write_entity_helper.h"
#include "remove_entity_helper.h"
//#include "entitydef/scriptdef_module.h"
//#include "entitydef/property.h"
#include "db_interface/db_interface.h"
#include "db_interface/entity_table.h"
#include "network/fixed_messages.h"

#ifndef CODE_INLINE
#include "entity_table_mysql.inl"
#endif

namespace KBEngine { 

//-------------------------------------------------------------------------------------
EntityTableMysql::EntityTableMysql()
{
}

//-------------------------------------------------------------------------------------
EntityTableMysql::~EntityTableMysql()
{
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::initialize(std::string name)
{
	// 获取表名
	tableName(name);

	// 找到所有存储属性并且创建出所有的字段
	std::string hasUnique = "";

	/*for(; iter != pdescrsMap.end(); ++iter)
	{
		PropertyDescription* pdescrs = iter->second;

		EntityTableItem* pETItem = this->createItem(pdescrs->getDataType()->getName());

		pETItem->pParentTable(this);
		pETItem->utype(pdescrs->getUType());
		pETItem->tableName(this->tableName());

		bool ret = pETItem->initialize(pdescrs, pdescrs->getDataType(), pdescrs->getName());

		if(!ret)
		{
			delete pETItem;
			return false;
		}

		tableItems_[pETItem->utype()].reset(pETItem);
		tableFixedOrderItems_.push_back(pETItem);
	}*/
	init_db_item_name();
	return true;
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::init_db_item_name()
{
	EntityTable::TABLEITEM_MAP::iterator iter = tableItems_.begin();
	for(; iter != tableItems_.end(); ++iter)
	{
		// 处理fixedDict字段名称的特例情况
		std::string exstrFlag = "";
		if(iter->second->type() == TABLE_ITEM_TYPE_FIXEDDICT)
		{
			exstrFlag = iter->second->itemName();
			if(exstrFlag.size() > 0)
				exstrFlag += "_";
		}

		//static_cast<EntityTableItemMysqlBase*>(iter->second.get())->init_db_item_name(exstrFlag.c_str());
	}
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::syncIndexToDB(DBInterface* dbi)
{
	std::vector<EntityTableItem*> indexs;

	EntityTable::TABLEITEM_MAP::iterator iter = tableItems_.begin();
	for(; iter != tableItems_.end(); ++iter)
	{
		if(strlen(iter->second->indexType()) == 0)
			continue;

		indexs.push_back(iter->second.get());
	}

	char sql_str[MAX_BUF];

	kbe_snprintf(sql_str, MAX_BUF, "show index from " ENTITY_TABLE_PERFIX "_%s", 
		tableName());

	try
	{
		bool ret = dbi->query(sql_str, strlen(sql_str), false);
		if(!ret)
		{
			return false;
		}
	}
	catch(...)
	{
		return false;
	}

	KBEUnordered_map<std::string, std::string> currDBKeys;

	MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
	if(pResult)
	{
		MYSQL_ROW arow;
		while((arow = mysql_fetch_row(pResult)) != NULL)
		{
			std::string keytype = "UNIQUE";

			if(std::string("1") == arow[1])
				keytype = "INDEX";

			std::string keyname = arow[2];
			std::string colname = arow[4];

			if(keyname == "PRIMARY" || colname != keyname || 
				keyname == TABLE_PARENTID_CONST_STR ||
				keyname == TABLE_ID_CONST_STR ||
				keyname == TABLE_ITEM_PERFIX"_" TABLE_AUTOLOAD_CONST_STR)
				continue;

			currDBKeys[colname] = keytype;
		}

		mysql_free_result(pResult);
	}

	bool done = false;
	std::string sql = fmt::format("ALTER TABLE " ENTITY_TABLE_PERFIX "_{} ", tableName());
	std::vector<EntityTableItem*>::iterator iiter = indexs.begin();
	for(; iiter != indexs.end(); )
	{
		std::string itemName = fmt::format(TABLE_ITEM_PERFIX"_{}", (*iiter)->itemName());
		KBEUnordered_map<std::string, std::string>::iterator fiter = currDBKeys.find(itemName);
		if(fiter != currDBKeys.end())
		{
			bool deleteIndex = fiter->second != (*iiter)->indexType();
			
			// 删除已经处理的，剩下的就是要从数据库删除的index
			currDBKeys.erase(fiter);
			
			if(deleteIndex)
			{
				sql += fmt::format("DROP INDEX `{}`,", itemName);
				done = true;
			}
			else
			{
				iiter = indexs.erase(iiter);
				continue;
			}
		}

		std::string lengthinfos = "";
		if((*iiter)->type() == TABLE_ITEM_TYPE_BLOB || 
			(*iiter)->type() == TABLE_ITEM_TYPE_STRING ||
			 (*iiter)->type() == TABLE_ITEM_TYPE_UNICODE )
		{
			/*if((*iiter)->pPropertyDescription()->getDatabaseLength() == 0)
			{
				ERROR_MSG(fmt::format("EntityTableMysql::syncIndexToDB(): INDEX({}) without a key length, *.def-><{}>-><DatabaseLength> ? </DatabaseLength>\n",
					(*iiter)->itemName(), (*iiter)->itemName()));
			}
			else
			{
				lengthinfos = fmt::format("({})", (*iiter)->pPropertyDescription()->getDatabaseLength());
			}*/
		}

		sql += fmt::format("ADD {} {}({}{}),", (*iiter)->indexType(), itemName, itemName, lengthinfos);
		++iiter;
		done = true;
	}

	// 剩下的就是要从数据库删除的index
	KBEUnordered_map<std::string, std::string>::iterator dbkey_iter = currDBKeys.begin();
	for(; dbkey_iter != currDBKeys.end(); ++dbkey_iter)
	{
		sql += fmt::format("DROP INDEX `{}`,", dbkey_iter->first);
		done = true;		
	}
	
	// 没有需要修改或者添加的
	if(!done)
		return true;
	
	sql.erase(sql.end() - 1);
	
	try
	{
		bool ret = dbi->query(sql.c_str(), sql.size(), true);
		if(!ret)
		{
			return false;
		}
	}
	catch(...)
	{
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::syncToDB(DBInterface* dbi)
{
	if(hasSync())
		return true;

	// DEBUG_MSG(fmt::format("EntityTableMysql::syncToDB(): {}.\n", tableName()));

	char sql_str[MAX_BUF];
	std::string exItems = "";

	if(this->isChild())
		exItems = ", " TABLE_PARENTID_CONST_STR " bigint(20) unsigned NOT NULL, INDEX(" TABLE_PARENTID_CONST_STR ")";

	kbe_snprintf(sql_str, MAX_BUF, "CREATE TABLE IF NOT EXISTS " ENTITY_TABLE_PERFIX "_%s "
			"(id bigint(20) unsigned AUTO_INCREMENT, PRIMARY KEY idKey (id)%s)"
		"ENGINE=" MYSQL_ENGINE_TYPE, 
		tableName(), exItems.c_str());

	try
	{
		bool ret = dbi->query(sql_str, strlen(sql_str), false);
		if(!ret)
		{
			return false;
		}
	}
	catch(...)
	{
		ERROR_MSG(fmt::format("EntityTableMysql::syncToDB(): is error({}: {})\n lastQuery: {}.\n", 
			dbi->getlasterror(), dbi->getstrerror(), static_cast<DBInterfaceMysql*>(dbi)->lastquery()));

		return false;
	}

	DBInterfaceMysql::TABLE_FIELDS outs;
	static_cast<DBInterfaceMysql*>(dbi)->getFields(outs, this->tableName());

	/*sync_item_to_db(dbi, "tinyint not null DEFAULT 0", this->tableName(), TABLE_ITEM_PERFIX"_" TABLE_AUTOLOAD_CONST_STR, 0, 
			FIELD_TYPE_TINY, NOT_NULL_FLAG, (void*)&outs, &sync_autoload_item_index);*/

	EntityTable::TABLEITEM_MAP::iterator iter = tableItems_.begin();
	for(; iter != tableItems_.end(); ++iter)
	{
		if(!iter->second->syncToDB(dbi, (void*)&outs))
			return false;
	}

	std::vector<std::string> dbTableItemNames;

	std::string ttablename = ENTITY_TABLE_PERFIX"_";
	ttablename += tableName();

	dbi->getTableItemNames(ttablename.c_str(), dbTableItemNames);

	// 检查是否有需要删除的表字段
	std::vector<std::string>::iterator iter0 = dbTableItemNames.begin();
	for(; iter0 != dbTableItemNames.end(); ++iter0)
	{
		std::string tname = (*iter0);
		
		if(tname == TABLE_ID_CONST_STR || 
			tname == TABLE_ITEM_PERFIX"_" TABLE_AUTOLOAD_CONST_STR || 
			tname == TABLE_PARENTID_CONST_STR)
		{
			continue;
		}

		EntityTable::TABLEITEM_MAP::iterator iter = tableItems_.begin();
		bool found = false;

		for(; iter != tableItems_.end(); ++iter)
		{
			if(iter->second->isSameKey(tname))
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			if(!dbi->dropEntityTableItemFromDB(ttablename.c_str(), tname.c_str()))
				return false;
		}
	}

	// 同步表索引
	if(!syncIndexToDB(dbi))
		return false;

	sync_ = true;
	return true;
}

//-------------------------------------------------------------------------------------
EntityTableItem* EntityTableMysql::createItem(std::string type)
{
	/*if(type == "INT8")
	{
		return new EntityTableItemMysql_DIGIT(type, "tinyint not null DEFAULT 0", 4, NOT_NULL_FLAG, FIELD_TYPE_TINY);
	}
	else if(type == "INT16")
	{
		return new EntityTableItemMysql_DIGIT(type, "smallint not null DEFAULT 0", 6, NOT_NULL_FLAG, FIELD_TYPE_SHORT);
	}
	else if(type == "INT32")
	{
		return new EntityTableItemMysql_DIGIT(type, "int not null DEFAULT 0", 11, NOT_NULL_FLAG, FIELD_TYPE_LONG);
	}
	else if(type == "INT64")
	{
		return new EntityTableItemMysql_DIGIT(type, "bigint not null DEFAULT 0", 20, NOT_NULL_FLAG, FIELD_TYPE_LONGLONG);
	}
	else if(type == "UINT8")
	{
		return new EntityTableItemMysql_DIGIT(type, "tinyint unsigned not null DEFAULT 0", 3, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_TINY);
	}
	else if(type == "UINT16")
	{
		return new EntityTableItemMysql_DIGIT(type, "smallint unsigned not null DEFAULT 0", 5, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_SHORT);
	}
	else if(type == "UINT32")
	{
		return new EntityTableItemMysql_DIGIT(type, "int unsigned not null DEFAULT 0", 10, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_LONG);
	}
	else if(type == "UINT64")
	{
		return new EntityTableItemMysql_DIGIT(type, "bigint unsigned not null DEFAULT 0", 20, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_LONGLONG);
	}
	else if(type == "FLOAT")
	{
		return new EntityTableItemMysql_DIGIT(type, "float not null DEFAULT 0", 0, NOT_NULL_FLAG, FIELD_TYPE_FLOAT);
	}
	else if(type == "DOUBLE")
	{
		return new EntityTableItemMysql_DIGIT(type, "double not null DEFAULT 0", 0, NOT_NULL_FLAG, FIELD_TYPE_DOUBLE);
	}
	else if(type == "STRING")
	{
		return new EntityTableItemMysql_STRING("text", 0, 0, FIELD_TYPE_BLOB);
	}
	else if(type == "UNICODE")
	{
		return new EntityTableItemMysql_UNICODE("text", 0, 0, FIELD_TYPE_BLOB);
	}
	else if(type == "BLOB")
	{
		return new EntityTableItemMysql_BLOB("blob", 0, 0, FIELD_TYPE_BLOB);
	}
	else if(type == "ARRAY")
	{
		return new EntityTableItemMysql_ARRAY("", 0, 0, FIELD_TYPE_BLOB);
	}
	else if(type == "FIXED_DICT")
	{
		return new EntityTableItemMysql_FIXED_DICT("", 0, 0, FIELD_TYPE_BLOB);
	}

	KBE_ASSERT(false && "not found type.\n");
	return new EntityTableItemMysql_STRING("", 0, 0, FIELD_TYPE_STRING);*/
	return NULL;
}

//-------------------------------------------------------------------------------------
DBID EntityTableMysql::writeTable(DBInterface* dbi, DBID dbid, int8 shouldAutoLoad, MemoryStream* s)
{
	//DBContext context;
	//context.parentTableName = "";
	//context.parentTableDBID = 0;
	//context.dbid = dbid;
	//context.tableName = pModule->getName();
	//context.isEmpty = false;
	//context.readresultIdx = 0;

	//while(s->length() > 0)
	//{
	//	ENTITY_PROPERTY_UID pid;
	//	(*s) >> pid;
	//	
	//	EntityTableItem* pTableItem = this->findItem(pid);
	//	if(pTableItem == NULL)
	//	{
	//		ERROR_MSG(fmt::format("EntityTable::writeTable: not found item[{}].\n", pid));
	//		return dbid;
	//	}
	//	
	//	static_cast<EntityTableItemMysqlBase*>(pTableItem)->getWriteSqlItem(dbi, s, context);
	//};

	//if(!WriteEntityHelper::writeDB(context.dbid > 0 ? TABLE_OP_UPDATE : TABLE_OP_INSERT, 
	//	dbi, context))
	//	return 0;

	//dbid = context.dbid;

	//// 如果dbid为0则存储失败返回
	//if(dbid <= 0)
	//	return dbid;

	//// 设置实体是否自动加载
	//if(shouldAutoLoad > -1)
	//	entityShouldAutoLoad(dbi, dbid, shouldAutoLoad > 0);

	return dbid;
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::removeEntity(DBInterface* dbi, DBID dbid)
{
	/*KBE_ASSERT(pModule && dbid > 0);

	DBContext context;
	context.parentTableName = "";
	context.parentTableDBID = 0;
	context.dbid = dbid;
	context.tableName = pModule->getName();
	context.isEmpty = false;
	context.readresultIdx = 0;

	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	for(; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->getReadSqlItem(context);
	}

	bool ret = RemoveEntityHelper::removeDB(dbi, context);
	KBE_ASSERT(ret);
*/
	return true;
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::queryTable(DBInterface* dbi, DBID dbid, MemoryStream* s)
{
	/*KBE_ASSERT(pModule && s && dbid > 0);

	DBContext context;
	context.parentTableName = "";
	context.parentTableDBID = 0;
	context.dbid = dbid;
	context.tableName = pModule->getName();
	context.isEmpty = false;
	context.readresultIdx = 0;

	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	for(; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->getReadSqlItem(context);
	}

	if(!ReadEntityHelper::queryDB(dbi, context))
		return false;

	if(context.dbids[dbid].size() == 0)
		return false;

	iter = tableFixedOrderItems_.begin();
	for(; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->addToStream(s, context, dbid);
	}
*/
	return true;// context.dbid == dbid;
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::addToStream(MemoryStream* s, DBContext& context, DBID resultDBID)
{
	//std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	//for(; iter != tableFixedOrderItems_.end(); ++iter)
	//{
	//	static_cast<EntityTableItemMysqlBase*>((*iter))->addToStream(s, context, resultDBID);
	//}
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::getWriteSqlItem(DBInterface* dbi, MemoryStream* s, DBContext& context)
{
	//if(tableFixedOrderItems_.size() == 0)
	//	return;
	//
	//std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();

	//DBContext* context1 = new DBContext();
	//context1->parentTableName = (*iter)->pParentTable()->tableName();
	//context1->tableName = (*iter)->tableName();
	//context1->parentTableDBID = 0;
	//context1->dbid = 0;
	//context1->isEmpty = (s == NULL);
	//context1->readresultIdx = 0;

	//KBEShared_ptr< DBContext > opTableValBox1Ptr(context1);
	//context.optable.push_back( std::pair<std::string/*tableName*/, KBEShared_ptr< DBContext > >
	//	((*iter)->tableName(), opTableValBox1Ptr));

	//for(; iter != tableFixedOrderItems_.end(); ++iter)
	//{
	//	static_cast<EntityTableItemMysqlBase*>((*iter))->getWriteSqlItem(dbi, s, *context1);
	//}
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::getReadSqlItem(DBContext& context)
{
	//if(tableFixedOrderItems_.size() == 0)
	//	return;
	//
	//std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();

	//DBContext* context1 = new DBContext();
	//context1->parentTableName = (*iter)->pParentTable()->tableName();
	//context1->tableName = (*iter)->tableName();
	//context1->parentTableDBID = 0;
	//context1->dbid = 0;
	//context1->isEmpty = true;
	//context1->readresultIdx = 0;

	//KBEShared_ptr< DBContext > opTableValBox1Ptr(context1);
	//context.optable.push_back( std::pair<std::string/*tableName*/, KBEShared_ptr< DBContext > >
	//	((*iter)->tableName(), opTableValBox1Ptr));

	//for(; iter != tableFixedOrderItems_.end(); ++iter)
	//{
	//	static_cast<EntityTableItemMysqlBase*>((*iter))->getReadSqlItem(*context1);
	//}
}

//-------------------------------------------------------------------------------------
}
