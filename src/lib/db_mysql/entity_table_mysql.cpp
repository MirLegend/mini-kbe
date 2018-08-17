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

// 同步成功时回调
typedef void(*onSyncItemToDBSuccessPtr)(DBInterface*, const char*, const char*);

bool sync_item_to_db(DBInterface* dbi,
	const char* datatype,
	const char* tableName,
	const char* itemName,
	uint32 length,
	enum_field_types
	sqlitemtype,
	unsigned int itemflags,
	void* pData, onSyncItemToDBSuccessPtr callback = NULL)
{
	if (pData)
	{
		DBInterfaceMysql::TABLE_FIELDS* pTFData = static_cast<DBInterfaceMysql::TABLE_FIELDS*>(pData);
		DBInterfaceMysql::TABLE_FIELDS::iterator iter = pTFData->find(itemName);
		if (iter != pTFData->end())
		{
			TABLE_FIELD& tf = iter->second;
			if (tf.type == sqlitemtype && ((tf.flags & itemflags) == itemflags))
			{
				if (sqlitemtype == MYSQL_TYPE_VAR_STRING)
					length *= 3; //这里是varchar 字节数 而不是字符数 utf-8 要乘以3
				if ((length == 0) || (int32)length == tf.length)
				{
					return true;
				}
				else
				{
					DEBUG_MSG(fmt::format("syncToDB========(): {}->{}({} {}) length:({} {}).\n", tf.type, sqlitemtype,
						tf.flags & itemflags, itemflags, tf.length, length));
				}
					
			}
			else {
				DEBUG_MSG(fmt::format("syncToDB2===========: {}->{}({} {}) length:({} {}).\n", tf.type, sqlitemtype, 
					tf.flags & itemflags, itemflags, tf.length, length));
			}
		}
	}

	DEBUG_MSG(fmt::format("syncToDB(): {}->{}({}).\n", tableName, itemName, datatype));

	char __sql_str__[MAX_BUF];

	kbe_snprintf(__sql_str__, MAX_BUF, "ALTER TABLE `" ENTITY_TABLE_PERFIX "_%s` ADD `%s` %s;",
		tableName, itemName, datatype);

	try
	{
		dbi->query(__sql_str__, strlen(__sql_str__), false);
	}
	catch (...)
	{
	}

	if (dbi->getlasterror() == 1060)
	{
		kbe_snprintf(__sql_str__, MAX_BUF, "ALTER TABLE `" ENTITY_TABLE_PERFIX "_%s` MODIFY COLUMN `%s` %s;",
			tableName, itemName, datatype);

		try
		{
			if (dbi->query(__sql_str__, strlen(__sql_str__), false))
			{
				if (callback)
					(*callback)(dbi, tableName, itemName);

				return true;
			}
		}
		catch (...)
		{
			ERROR_MSG(fmt::format("syncToDB(): {}->{}({}) is error({}: {})\n lastQuery: {}.\n",
				tableName, itemName, datatype, dbi->getlasterror(), dbi->getstrerror(), static_cast<DBInterfaceMysql*>(dbi)->lastquery()));

			return false;
		}
	}

	if (callback)
		(*callback)(dbi, tableName, itemName);

	return true;
}

//-------------------------------------------------------------------------------------
EntityTableMysql::EntityTableMysql()
{
}

//-------------------------------------------------------------------------------------
EntityTableMysql::~EntityTableMysql()
{
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::initialize(std::string name, const DBTABLEITEMS& tableItems)
{
	// 获取表名
	tableName(name);
	mpTableItemDef = &tableItems;
	// 找到所有存储属性并且创建出所有的字段
	DBTABLEITEMS::const_iterator iter = tableItems.begin();
	//int i = 0;
	prikeyIndex = 1;
	for(; iter != tableItems.end(); ++iter)
	{
		const stTableItem* pTblItem = &(iter->second);
		EntityTableItem* pETItem = this->createItem(pTblItem->tblItemType, pTblItem);
		pETItem->flags(pETItem->flags() | pTblItem->flag);
		
		pETItem->pParentTable(this);
		pETItem->utype(pTblItem->utype);
		if ((pTblItem->flag&PRI_KEY_FLAG) > 0)
		{
			prikeyIndex = pETItem->utype();
		}
		pETItem->tableName(this->tableName());
		pETItem->itemName(pTblItem->tblItemName);
		tableItems_[pETItem->utype()].reset(pETItem);
		tableFixedOrderItems_.push_back(pETItem);
	}
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
		static_cast<EntityTableItemMysqlBase*>(iter->second.get())->init_db_item_name(exstrFlag.c_str());
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
			if((*iiter)->pTableItemDescription()->length == 0)
			{
				ERROR_MSG(fmt::format("EntityTableMysql::syncIndexToDB(): INDEX({}) without a key length, *.def-><{}>-><DatabaseLength> ? </DatabaseLength>\n",
					(*iiter)->itemName(), (*iiter)->itemName()));
			}
			else
			{
				lengthinfos = fmt::format("({})", (*iiter)->pTableItemDescription()->length);
			}
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

	char sql_str[MAX_BUF];
	char priKeySql[MAX_BUF];
	//EntityTableItem* pETItem = tableItems_[prikeyIndex];
	char keyStr[64];
	bool autoincrement = (tableItems_[prikeyIndex]->flags() & AUTO_INCREMENT_FLAG) > 0;
	kbe_snprintf(keyStr, 64, TABLE_ITEM_PERFIX"_%s", tableItems_[prikeyIndex]->itemName());
	kbe_snprintf(priKeySql, MAX_BUF, "%s %s %s, PRIMARY KEY idKey (%s)",
		keyStr, tableItems_[prikeyIndex]->itemDBType(), autoincrement?"AUTO_INCREMENT":"", keyStr);

	kbe_snprintf(sql_str, MAX_BUF, "CREATE TABLE IF NOT EXISTS " ENTITY_TABLE_PERFIX "_%s "
			"(%s)"
		"ENGINE=" MYSQL_ENGINE_TYPE, 
		tableName(), priKeySql);

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
		
		/*if(tname == TABLE_ID_CONST_STR || 
			tname == TABLE_ITEM_PERFIX"_" TABLE_AUTOLOAD_CONST_STR || 
			tname == TABLE_PARENTID_CONST_STR)
		{
			continue;
		}*/

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
EntityTableItem* EntityTableMysql::createItem(std::string type, const stTableItem* pTableItemDescription)
{
	char itemDBType[MAX_BUF];
	std::string indexType = pTableItemDescription->indexType;
	bool bunique = (indexType == "UNIQUE")|| ((pTableItemDescription->flag & PRI_KEY_FLAG) > 0);
	if(type == "INT8")
	{
		char dataLength[64]="";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}
		
		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "tinyint%s not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else 
		{
			kbe_snprintf(itemDBType, MAX_BUF, "tinyint%s not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 4, NOT_NULL_FLAG, FIELD_TYPE_TINY);
	}
	else if(type == "INT16")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "smallint%s not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "smallint%s not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 6, NOT_NULL_FLAG, FIELD_TYPE_SHORT);
	}
	else if(type == "INT32")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "int%s not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "int%s not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 11, NOT_NULL_FLAG, FIELD_TYPE_LONG);
	}
	else if(type == "INT64")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "bigint%s not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "bigint%s not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 20, NOT_NULL_FLAG, FIELD_TYPE_LONGLONG);
	}
	else if(type == "UINT8")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "tinyint%s unsigned not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "tinyint%s unsigned not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 3, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_TINY);
	}
	else if(type == "UINT16")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "smallint%s unsigned not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "smallint%s unsigned not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 5, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_SHORT);
	}
	else if(type == "UINT32")
	{

		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "int%s unsigned not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "int%s unsigned not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 10, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_LONG);
	}
	else if(type == "UINT64")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "bigint%s unsigned not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "bigint%s unsigned not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 20, NOT_NULL_FLAG|UNSIGNED_FLAG, FIELD_TYPE_LONGLONG);
	}
	else if(type == "FLOAT")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "float%s not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "float%s not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 0, NOT_NULL_FLAG, FIELD_TYPE_FLOAT);
	}
	else if(type == "DOUBLE")
	{
		char dataLength[64] = "";
		if (pTableItemDescription->length > 0)
		{
			kbe_snprintf(dataLength, MAX_BUF, "(%d)", pTableItemDescription->length);
		}

		if (!bunique)
		{
			kbe_snprintf(itemDBType, MAX_BUF, "double%s not null %s", dataLength, pTableItemDescription->defaultValue);
		}
		else
		{
			kbe_snprintf(itemDBType, MAX_BUF, "double%s not null ", dataLength);
		}
		return new EntityTableItemMysql_DIGIT(type, pTableItemDescription, itemDBType, 0, NOT_NULL_FLAG, FIELD_TYPE_DOUBLE);
	}
	else if(type == "STRING")
	{
		return new EntityTableItemMysql_STRING("varchar", pTableItemDescription, 0, 0, FIELD_TYPE_STRING);
	}
	else if(type == "UNICODE")
	{
		return new EntityTableItemMysql_UNICODE("varchar", pTableItemDescription, 0, 0, FIELD_TYPE_VAR_STRING);
	}
	else if(type == "BLOB")
	{
		return new EntityTableItemMysql_BLOB("blob", pTableItemDescription, 0, 0, FIELD_TYPE_BLOB);
	}
	//else if(type == "ARRAY")
	//{
	//	return new EntityTableItemMysql_ARRAY("", 0, 0, FIELD_TYPE_BLOB);
	//}
	//else if(type == "FIXED_DICT")
	//{
	//	return new EntityTableItemMysql_FIXED_DICT("", 0, 0, FIELD_TYPE_BLOB);
	//}

	KBE_ASSERT(false && "not found type.\n");
	return new EntityTableItemMysql_STRING("", pTableItemDescription, 0, 0, FIELD_TYPE_STRING);
}

//-------------------------------------------------------------------------------------
DBID EntityTableMysql::writeTable(DBInterface* dbi, DBID dbid, bool binsert, MemoryStream* s)
{
	DBContext context;
	context.dbid = dbid;
	context.tableName = tableName();
	context.isEmpty = false;
	context.readresultIdx = 0;

	while(s->length() > 0)
	{
		int32 pid;
		(*s) >> pid;
		
		EntityTableItem* pTableItem = this->findItem(pid);
		if(pTableItem == NULL)
		{
			ERROR_MSG(fmt::format("EntityTable::writeTable: not found item[{}].\n", pid));
			return dbid;
		}
		
		static_cast<EntityTableItemMysqlBase*>(pTableItem)->getWriteSqlItem(dbi, s, context);
	};

	if(!WriteEntityHelper::writeDB(context.dbid > 0 ? TABLE_OP_UPDATE : TABLE_OP_INSERT, 
		dbi, context))
		return 0;

	dbid = context.dbid;

	// 如果dbid为0则存储失败返回
	if(dbid <= 0)
		return dbid;

	return dbid;
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::removeEntity(DBInterface* dbi, DBID dbid)
{
	KBE_ASSERT(dbid > 0);

	DBContext context;
	context.dbid = dbid;
	//context.tableName = pModule->getName();
	context.isEmpty = false;
	context.readresultIdx = 0;

	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	for (; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->getReadSqlItem(context);
	}

	bool ret = RemoveEntityHelper::removeDB(dbi, context);
	KBE_ASSERT(ret);

	return true;
}

//-------------------------------------------------------------------------------------
bool EntityTableMysql::queryTable(DBInterface* dbi, DBID dbid, MemoryStream* s)
{
	KBE_ASSERT( s && dbid > 0);

	DBContext context;
	//context.parentTableName = "";
	//context.parentTableDBID = 0;
	context.dbid = dbid;
	std::string tableName = this->tableName();
	context.tableName = /*ENTITY_TABLE_PERFIX"_" +*/ tableName;
	context.isEmpty = false;
	context.readresultIdx = 0;

	/*std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	for (; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->getReadSqlItem(context);
	}*/
	//改用 utype 的 顺序
	TABLEITEM_MAP::iterator iter = tableItems_.begin();
	for (; iter != tableItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>(iter->second.get())->getReadSqlItem(context);
	}

	if (!ReadEntityHelper::queryDB(dbi, context))
		return false;

	/*iter = tableFixedOrderItems_.begin();
	for (; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->addToStream(s, context, dbid);
	}*/
	iter = tableItems_.begin();
	for (; iter != tableItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>(iter->second.get())->addToStream(s, context, dbid);
	}

	return  context.dbid == dbid;
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::addToStream(MemoryStream* s, DBContext& context, DBID resultDBID)
{
	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();
	for (; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->addToStream(s, context, resultDBID);
	}
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::getWriteSqlItem(DBInterface* dbi, MemoryStream* s, DBContext& context)
{
	if(tableFixedOrderItems_.size() == 0)
		return;
	
	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();

	DBContext* context1 = new DBContext();
	context1->tableName = (*iter)->tableName();
	context1->dbid = 0;
	context1->isEmpty = (s == NULL);
	context1->readresultIdx = 0;

	for(; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->getWriteSqlItem(dbi, s, *context1);
	}
}

//-------------------------------------------------------------------------------------
void EntityTableMysql::getReadSqlItem(DBContext& context)
{
	if(tableFixedOrderItems_.size() == 0)
		return;
	
	std::vector<EntityTableItem*>::iterator iter = tableFixedOrderItems_.begin();

	DBContext* context1 = new DBContext();
	//context1->parentTableName = (*iter)->pParentTable()->tableName();
	context1->tableName = (*iter)->tableName();
	//context1->parentTableDBID = 0;
	context1->dbid = 0;
	context1->isEmpty = true;
	context1->readresultIdx = 0;

	KBEShared_ptr< DBContext > opTableValBox1Ptr(context1);
	//context.optable.push_back( std::pair<std::string/*tableName*/, KBEShared_ptr< DBContext > >
	//	((*iter)->tableName(), opTableValBox1Ptr));

	for(; iter != tableFixedOrderItems_.end(); ++iter)
	{
		static_cast<EntityTableItemMysqlBase*>((*iter))->getReadSqlItem(*context1);
	}
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysqlBase::init_db_item_name(const char* exstrFlag)
{
	kbe_snprintf(db_item_name_, MAX_BUF, TABLE_ITEM_PERFIX"_%s", /*exstrFlag, */itemName());
}

//-------------------------------------------------------------------------------------
bool EntityTableItemMysqlBase::initialize(std::string name, const stTableItem* pTableItemDescription)
{
	itemName(name);
	pTableItemDescription_ = pTableItemDescription;
	//pDataType_ = pDataType;
	indexType_ = pTableItemDescription_->indexType;
	return true;
}

//-------------------------------------------------------------------------------------
bool EntityTableItemMysql_DIGIT::syncToDB(DBInterface* dbi, void* pData)
{
	if (datalength_ == 0)
	{
		return sync_item_to_db(dbi, itemDBType_.c_str(), tableName_.c_str(),
			db_item_name(), datalength_, this->mysqlItemtype_, this->flags(), pData);
	}

	uint32 length = pTableItemDescription_->length;
	char sql_str[MAX_BUF];
	kbe_snprintf(sql_str, MAX_BUF, "%s", itemDBType_.c_str());

	//if (length <= 0)
	//	kbe_snprintf(sql_str, MAX_BUF, "%s", itemDBType_.c_str());
	//else
	//	kbe_snprintf(sql_str, MAX_BUF, "%s(%u)", itemDBType_.c_str(), length);

	return sync_item_to_db(dbi, sql_str, tableName_.c_str(), db_item_name(), length, this->mysqlItemtype_, this->flags(), pData);
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_DIGIT::addToStream(MemoryStream* s, DBContext& context, DBID resultDBID)
{
	if (kbe_stricmp(db_item_name(), "sm_id") == 0)
	{
		return;
	}
	std::stringstream stream;
	stream << context.results[context.readresultIdx++];

	if (dataSType_ == "INT8")
	{
		int32 v;
		stream >> v;
		int8 vv = static_cast<int8>(v);
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)vv));
		(*s) << vv;
	}
	else if (dataSType_ == "INT16")
	{
		int16 v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "INT32")
	{
		int32 v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "INT64")
	{
		int64 v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "UINT8")
	{
		int32 v;
		stream >> v;
		uint8 vv = static_cast<uint8>(v);
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)vv));
		(*s) << vv;
	}
	else if (dataSType_ == "UINT16")
	{
		uint16 v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "UINT32")
	{
		uint32 v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "UINT64")
	{
		uint64 v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "FLOAT")
	{
		float v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
	else if (dataSType_ == "DOUBLE")
	{
		double v;
		stream >> v;
		ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, dataSType_.c_str(), (uint32)v));
		(*s) << v;
	}
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_DIGIT::getWriteSqlItem(DBInterface* dbi, MemoryStream* s, DBContext& context)
{
	if (s == NULL)
		return;

	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();

	if (dataSType_ == "INT8")
	{
		int8 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%d", v);
	}
	else if (dataSType_ == "INT16")
	{
		int16 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%d", v);
	}
	else if (dataSType_ == "INT32")
	{
		int32 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%d", v);
	}
	else if (dataSType_ == "INT64")
	{
		int64 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%" PRI64, v);
	}
	else if (dataSType_ == "UINT8")
	{
		uint8 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%u", v);
	}
	else if (dataSType_ == "UINT16")
	{
		uint16 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%u", v);
	}
	else if (dataSType_ == "UINT32")
	{
		uint32 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%u", v);
	}
	else if (dataSType_ == "UINT64")
	{
		uint64 v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%" PRIu64, v);
	}
	else if (dataSType_ == "FLOAT")
	{
		float v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%f", v);
	}
	else if (dataSType_ == "DOUBLE")
	{
		double v;
		(*s) >> v;
		kbe_snprintf(pSotvs->sqlval, MAX_BUF, "%lf", v);
	}

	pSotvs->sqlkey = db_item_name();
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_DIGIT::getReadSqlItem(DBContext& context)
{
	if (kbe_stricmp(db_item_name(), "sm_id") == 0)
	{
		return;
	}
	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();
	pSotvs->sqlkey = db_item_name();
	memset(pSotvs->sqlval, 0, MAX_BUF);
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}


//-------------------------------------------------------------------------------------
bool EntityTableItemMysql_STRING::syncToDB(DBInterface* dbi, void* pData)
{
	uint32 length = pTableItemDescription_->length;
	char sql_str[MAX_BUF];

	if (length > 0)
	{
		kbe_snprintf(sql_str, MAX_BUF, "%s(%u)", itemDBType_.c_str(), length);
	}
	else
	{
		kbe_snprintf(sql_str, MAX_BUF, "%s", itemDBType_.c_str());
	}

	return sync_item_to_db(dbi, sql_str, tableName_.c_str(), db_item_name(), length,
		this->mysqlItemtype_, this->flags(), pData);
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_STRING::addToStream(MemoryStream* s,
	DBContext& context, DBID resultDBID)
{
	if (kbe_stricmp(db_item_name(), "sm_id") == 0)
	{
		return;
	}
	(*s) << context.results[context.readresultIdx++];
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_STRING::getWriteSqlItem(DBInterface* dbi,
	MemoryStream* s, DBContext& context)
{
	if (s == NULL)
		return;

	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();

	std::string val;
	(*s) >> val;

	pSotvs->extraDatas = "\"";

	char* tbuf = new char[val.size() * 2 + 1];
	memset(tbuf, 0, val.size() * 2 + 1);

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(),
		tbuf, val.c_str(), val.size());

	pSotvs->extraDatas += tbuf;
	pSotvs->extraDatas += "\"";

	SAFE_RELEASE_ARRAY(tbuf);

	memset(pSotvs, 0, sizeof(pSotvs->sqlval));
	pSotvs->sqlkey = db_item_name();
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_STRING::getReadSqlItem(DBContext& context)
{
	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();
	pSotvs->sqlkey = db_item_name();
	memset(pSotvs->sqlval, 0, MAX_BUF);
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}

//-------------------------------------------------------------------------------------
bool EntityTableItemMysql_UNICODE::syncToDB(DBInterface* dbi, void* pData)
{
	uint32 length = pTableItemDescription_->length;
	//ERROR_MSG(fmt::format("EntityTableItemMysql_UNICODE::syncToDB::{} {} .\n", pTableItemDescription_->tblItemName, pTableItemDescription_->length));
	char sql_str[MAX_BUF];

	if (length > 0)
	{
		kbe_snprintf(sql_str, MAX_BUF, "%s(%u)", itemDBType_.c_str(), length);
	}
	else
	{
		kbe_snprintf(sql_str, MAX_BUF, "%s", itemDBType_.c_str());
	}

	return sync_item_to_db(dbi, sql_str, tableName_.c_str(), db_item_name(), length,
		this->mysqlItemtype_, this->flags(), pData);
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_UNICODE::addToStream(MemoryStream* s,
	DBContext& context, DBID resultDBID)
{
	if (kbe_stricmp(db_item_name(), "sm_id") == 0)
	{
		return;
	}
	ERROR_MSG(fmt::format("addToStream: name:[{}], type[{}], value[{}]\n", pTableItemDescription_->tblItemName, pTableItemDescription_->tblItemType, context.results[context.readresultIdx]));
	s->appendBlob(context.results[context.readresultIdx++]);
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_UNICODE::getWriteSqlItem(DBInterface* dbi, MemoryStream* s,
	DBContext& context)
{
	if (s == NULL)
		return;

	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();

	std::string val;
	s->readBlob(val);

	char* tbuf = new char[val.size() * 2 + 1];
	memset(tbuf, 0, val.size() * 2 + 1);

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(),
		tbuf, val.c_str(), val.size());

	pSotvs->extraDatas = "\"";
	pSotvs->extraDatas += tbuf;
	pSotvs->extraDatas += "\"";
	SAFE_RELEASE_ARRAY(tbuf);

	memset(pSotvs, 0, sizeof(pSotvs->sqlval));
	pSotvs->sqlkey = db_item_name();
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_UNICODE::getReadSqlItem(DBContext& context)
{
	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();
	pSotvs->sqlkey = db_item_name();
	memset(pSotvs->sqlval, 0, MAX_BUF);
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}

//-------------------------------------------------------------------------------------
bool EntityTableItemMysql_BLOB::syncToDB(DBInterface* dbi, void* pData)
{
	return sync_item_to_db(dbi, itemDBType_.c_str(), tableName_.c_str(), db_item_name(), 0,
		this->mysqlItemtype_, this->flags(), pData);
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_BLOB::addToStream(MemoryStream* s, DBContext& context, DBID resultDBID)
{
	std::string& datas = context.results[context.readresultIdx++];
	s->appendBlob(datas.data(), datas.size());
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_BLOB::getWriteSqlItem(DBInterface* dbi, MemoryStream* s, DBContext& context)
{
	if (s == NULL)
		return;

	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();

	std::string val;
	s->readBlob(val);

	char* tbuf = new char[val.size() * 2 + 1];
	memset(tbuf, 0, val.size() * 2 + 1);

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(),
		tbuf, val.data(), val.size());

	pSotvs->extraDatas = "\"";
	pSotvs->extraDatas += tbuf;
	pSotvs->extraDatas += "\"";
	SAFE_RELEASE_ARRAY(tbuf);

	memset(pSotvs, 0, sizeof(pSotvs->sqlval));
	pSotvs->sqlkey = db_item_name();
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}

//-------------------------------------------------------------------------------------
void EntityTableItemMysql_BLOB::getReadSqlItem(DBContext& context)
{
	DBContext::DB_ITEM_DATA* pSotvs = new DBContext::DB_ITEM_DATA();
	pSotvs->sqlkey = db_item_name();
	memset(pSotvs->sqlval, 0, MAX_BUF);
	context.items.push_back(KBEShared_ptr<DBContext::DB_ITEM_DATA>(pSotvs));
}


//-------------------------------------------------------------------------------------
}
