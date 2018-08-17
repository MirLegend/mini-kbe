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
#include "db_exception.h"
#include "db_interface_mysql.h"
#include "db_interface/db_interface.h"
#include "db_interface/entity_table.h"
//#include "entitydef/entitydef.h"
//#include "entitydef/scriptdef_module.h"
#include "server/serverconfig.h"

namespace KBEngine { 

//-------------------------------------------------------------------------------------
bool KBEEntityLogTableMysql::syncToDB(DBInterface* dbi)
{
	std::string sqlstr = "DROP TABLE kbe_entitylog;";

	try
	{
		dbi->query(sqlstr.c_str(), sqlstr.size(), false);
	}
	catch(...)
	{
	}
	
	bool ret = false;

	sqlstr = "CREATE TABLE IF NOT EXISTS kbe_entitylog "
			"(entityDBID bigint(20) unsigned not null DEFAULT 0,"
			"entityID int unsigned not null DEFAULT 0,"
			"ip varchar(64),"
			"port int unsigned not null DEFAULT 0,"
			"componentID bigint unsigned not null DEFAULT 0,"
			"PRIMARY KEY (entityDBID))"
		"ENGINE=" MYSQL_ENGINE_TYPE;

	ret = dbi->query(sqlstr.c_str(), sqlstr.size(), true);
	KBE_ASSERT(ret);
	return ret;
}

//-------------------------------------------------------------------------------------
bool KBEEntityLogTableMysql::logEntity(DBInterface * dbi, const char* ip, uint32 port, DBID dbid,
					COMPONENT_ID componentID, ENTITY_ID entityID)
{
	std::string sqlstr = "insert into kbe_entitylog (entityDBID, entityID, ip, port, componentID) values(";

	char* tbuf = new char[MAX_BUF * 3];

	kbe_snprintf(tbuf, MAX_BUF, "%" PRDBID, dbid);
	sqlstr += tbuf;
	sqlstr += ",";
	
	//kbe_snprintf(tbuf, MAX_BUF, "%u", entityType);
	//sqlstr += tbuf;
	//sqlstr += ",";

	kbe_snprintf(tbuf, MAX_BUF, "%d", entityID);
	sqlstr += tbuf;
	sqlstr += ",\"";

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
		tbuf, ip, strlen(ip));

	sqlstr += tbuf;
	sqlstr += "\",";

	kbe_snprintf(tbuf, MAX_BUF, "%u", port);
	sqlstr += tbuf;
	sqlstr += ",";
	
	kbe_snprintf(tbuf, MAX_BUF, "%" PRDBID, componentID);
	sqlstr += tbuf;
	sqlstr += ")";

	SAFE_RELEASE_ARRAY(tbuf);

	try
	{
		if(!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
		{
			// 1062 int err = dbi->getlasterror(); 
			return false;
		}
	}
	catch (std::exception & e)
	{
		DBException& dbe = static_cast<DBException&>(e);
		if(dbe.isLostConnection())
		{
			if(dbi->processException(e))
				return true;
		}

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool KBEEntityLogTableMysql::queryEntity(DBInterface * dbi, DBID dbid, EntityLog& entitylog)
{
	std::string sqlstr = "select entityID, ip, port, componentID from kbe_entitylog where entityDBID=";

	char tbuf[MAX_BUF];
	kbe_snprintf(tbuf, MAX_BUF, "%" PRDBID, dbid);
	sqlstr += tbuf;
	
	//sqlstr += " and entityType=";
	//kbe_snprintf(tbuf, MAX_BUF, "%u", entityType);
	//sqlstr += tbuf;
	sqlstr += " LIMIT 1";

	if(!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
	{
		return true;
	}
	
	entitylog.dbid = dbid;
	entitylog.componentID = 0;
	entitylog.entityID = 0;
	entitylog.ip[0] = '\0';
	entitylog.port = 0;

	MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
	if(pResult)
	{
		MYSQL_ROW arow = mysql_fetch_row(pResult);
		if(arow != NULL)
		{
			StringConv::str2value(entitylog.entityID, arow[0]);
			kbe_snprintf(entitylog.ip, MAX_IP, "%s", arow[1]);
			StringConv::str2value(entitylog.port, arow[2]);
			StringConv::str2value(entitylog.componentID, arow[3]);
		}

		mysql_free_result(pResult);
	}

	return entitylog.componentID > 0;
}

//-------------------------------------------------------------------------------------
bool KBEEntityLogTableMysql::eraseEntityLog(DBInterface * dbi, DBID dbid)
{
	std::string sqlstr = "delete from kbe_entitylog where entityDBID=";

	char tbuf[MAX_BUF];

	kbe_snprintf(tbuf, MAX_BUF, "%" PRDBID, dbid);
	sqlstr += tbuf;

	/*sqlstr += " and entityType=";
	kbe_snprintf(tbuf, MAX_BUF, "%u", entityType);
	sqlstr += tbuf;*/

	if(!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
	{
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
KBEEntityLogTableMysql::KBEEntityLogTableMysql():
	KBEEntityLogTable()
{
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::syncToDB(DBInterface* dbi)
{
	bool ret = false;

	/*std::string sqlstr = "CREATE TABLE IF NOT EXISTS kbe_accountinfos "
			"(`accountName` varchar(255) not null, PRIMARY KEY idKey (`accountName`),"
			"`password` varchar(255),"
			"`bindata` blob,"
			"`email` varchar(255),"
			"`entityDBID` bigint(20) unsigned not null DEFAULT 0, UNIQUE KEY `entityDBID` (`entityDBID`),"
			"`flags` int unsigned not null DEFAULT 0,"
			"`deadline` bigint(20) not null DEFAULT 0,"
			"`regtime` bigint(20) not null DEFAULT 0,"
			"`lasttime` bigint(20) not null DEFAULT 0,"
			"`numlogin` int unsigned not null DEFAULT 0)"
		"ENGINE=" MYSQL_ENGINE_TYPE;*/

	std::string sqlstr = "CREATE TABLE IF NOT EXISTS ziyu_dota_accounts "
		"(`user_dbid` bigint(20) NOT NULL AUTO_INCREMENT,"
		"`accountName` varchar(255) not null,"
		"`password` varchar(255),"
		"`reg_ip` varchar(32) NOT NULL,"
		"`last_login_ip` varchar(32) NOT NULL,"
		"`regtime` bigint(20) not null DEFAULT 0,"
		"`lasttime` bigint(20) not null DEFAULT 0,"
		"`numlogin` int unsigned not null DEFAULT 0,"
		"`server_id` int(11) NOT NULL DEFAULT '0',"
		"PRIMARY KEY (`user_dbid`),"
		"unique (`accountName`)) "
		"ENGINE=" MYSQL_ENGINE_TYPE
		" DEFAULT CHARSET=utf8";
	ret = dbi->query(sqlstr.c_str(), sqlstr.size(), true);
	KBE_ASSERT(ret);
	return ret;
}

//-------------------------------------------------------------------------------------
KBEAccountTableMysql::KBEAccountTableMysql():
	KBEAccountTable()
{
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::setFlagsDeadline(DBInterface * dbi, const std::string& name, uint32 flags, uint64 deadline)
{
	return true;
	//char* tbuf = new char[name.size() * 2 + 1];

	//mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
	//	tbuf, name.c_str(), name.size());

	//std::string sqlstr = fmt::format("update ziyu_dota_accounts set flags={}, deadline={} where accountName=\"{}\"", 
	//	flags, deadline, tbuf);

	//SAFE_RELEASE_ARRAY(tbuf);

	//// 如果查询失败则返回存在， 避免可能产生的错误
	//if(dbi->query(sqlstr.c_str(), sqlstr.size(), false))
	//	return true;

	//return false;
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::queryAccount(DBInterface * dbi, const std::string& name, ACCOUNT_INFOS& info)
{
	std::string sqlstr = "select * from ziyu_dota_accounts where accountName=\"";

	char* tbuf = new char[name.size() * 2 + 1];

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
		tbuf, name.c_str(), name.size());

	sqlstr += tbuf;
	sqlstr += "\" LIMIT 1";
	SAFE_RELEASE_ARRAY(tbuf);

	// 如果查询失败则返回存在， 避免可能产生的错误
	if(!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
		return true;

	info.usrId = 0;
	MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
	if(pResult)
	{
		MYSQL_ROW arow = mysql_fetch_row(pResult);
		if(arow != NULL)
		{
			KBEngine::StringConv::str2value(info.usrId, arow[0]);
			info.name = name;
			info.password = arow[2];
		}

		mysql_free_result(pResult);
	}

	return info.usrId > 0;
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::queryAccountAllInfos(DBInterface * dbi, const std::string& name, ACCOUNT_INFOS& info)
{
	//std::string sqlstr = "select * from ziyu_dota_accounts where accountName=\"";

	//char* tbuf = new char[name.size() * 2 + 1];

	//mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
	//	tbuf, name.c_str(), name.size());

	//sqlstr += tbuf;
	//sqlstr += "\" LIMIT 1";
	//SAFE_RELEASE_ARRAY(tbuf);

	//// 如果查询失败则返回存在， 避免可能产生的错误
	//if(!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
	//	return true;

	//info.dbid = 0;
	//MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
	//if(pResult)
	//{
	//	MYSQL_ROW arow = mysql_fetch_row(pResult);
	//	if(arow != NULL)
	//	{
	//		KBEngine::StringConv::str2value(info.dbid, arow[0]);
	//		info.name = name;
	//		info.password = arow[1];
	//		info.email = arow[2];
	//		KBEngine::StringConv::str2value(info.flags, arow[3]);
	//		KBEngine::StringConv::str2value(info.deadline, arow[4]);
	//	}

	//	mysql_free_result(pResult);
	//}

	//return info.dbid > 0;
	return true;
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::updateCount(DBInterface * dbi, const std::string& name, DBID dbid)
{
	// 如果查询失败则返回存在， 避免可能产生的错误
	if(!dbi->query(fmt::format("update ziyu_dota_accounts set lasttime={}, numlogin=numlogin+1 where user_dbid={}",
		time(NULL), dbid), false))
		return false;

	return true;
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::updatePassword(DBInterface * dbi, const std::string& name, const std::string& password)
{
	char* tbuf = new char[MAX_BUF * 3];
	char* tbuf1 = new char[MAX_BUF * 3];

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
		tbuf, password.c_str(), password.size());

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
		tbuf1, name.c_str(), name.size());

	// 如果查询失败则返回存在， 避免可能产生的错误
	if(!dbi->query(fmt::format("update ziyu_dota_accounts set password=\"{}\" where accountName like \"{}\"", 
		password, tbuf1), false))
	{
		SAFE_RELEASE_ARRAY(tbuf);
		SAFE_RELEASE_ARRAY(tbuf1);
		return false;
	}
	
	SAFE_RELEASE_ARRAY(tbuf);
	SAFE_RELEASE_ARRAY(tbuf1);
	return true;
}

//-------------------------------------------------------------------------------------
bool KBEAccountTableMysql::logAccount(DBInterface * dbi, ACCOUNT_INFOS& info)
{
	std::string sqlstr = "insert into ziyu_dota_accounts (accountName, password, regtime, lasttime) values(";

	char* tbuf = new char[MAX_BUF * 3];

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
		tbuf, info.name.c_str(), info.name.size());

	sqlstr += "\"";
	sqlstr += tbuf;
	sqlstr += "\",";

	mysql_real_escape_string(static_cast<DBInterfaceMysql*>(dbi)->mysql(), 
		tbuf, info.password.c_str(), info.password.size());

	sqlstr += "md5(\"";
	sqlstr += tbuf;
	sqlstr += "\"),";
	
	kbe_snprintf(tbuf, MAX_BUF, "%" PRTime, time(NULL));
	sqlstr += tbuf;
	sqlstr += ",";

	kbe_snprintf(tbuf, MAX_BUF, "%" PRTime, time(NULL));
	sqlstr += tbuf;
	sqlstr += ")";

	SAFE_RELEASE_ARRAY(tbuf);

	// 如果查询失败则返回存在， 避免可能产生的错误
	if(!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
	{
		ERROR_MSG(fmt::format("KBEAccountTableMysql::logAccount({}): sql({}) is failed({})!\n", 
				info.name, sqlstr, dbi->getstrerror()));

		return false;
	}
	info.usrId = static_cast<DBInterfaceMysql*>(dbi)->insertID();
	return true;
}


//KBEPlayerTableMysql::KBEPlayerTableMysql(){}

//const stTableItem s_tableItems[] = {
//	//{ "user_id", "INT64",0, PRI_KEY_FLAG, "", "" },
//	//{ "server_id", "INT32", 0/*UNIQUE_KEY_FLAG*/, 0, "INDEX", "" },
//	//{ "nickname", "UNICODE", 32, 0, "UNIQUE", "" },
//	//{ "last_set_name_time", "INT32", 0, 0, "" , "" },
//	//{ "avatar", "INT32", 0, 0, "", "" },
//	//{ "level", "INT32", 0 , 0, "", "" },
//	//{ "exp", "INT64", 0 , 0, "" , "" },
//	//{ "money", "INT64", 0 , 0, "", "" },
//	//{ "gem", "INT64", 0 , 0, "" , "" },
//	//{ "arena_point", "INT32", 0 , 0, "", "" },
//	//{ "crusade_point", "INT32", 0 , 0, "" , "" },
//	//{ "guild_point", "INT32", 0 , 0, "", "" },
//	//{ "last_midas_time", "INT32", 0 , 0, "" , "" },
//	//{ "today_midas_times", "INT32", 0 , 0, "", "" },
//	//{ "total_online_time", "INT32", 0 , 0, "" , "" },
//	//{ "tutorialstep", "INT16", 0 , 0, "", "" },
//	//{ "rechargegem", "INT64", 0, 0, "" , "" },
//	{1, "facebook_follow", "INT8", 0 , 0, "" , "" },
//};
//bool KBEPlayerTableMysql::initialize(std::string name, const DBTABLEITEMS& tableItems)
//{
//	// 获取表名
//	//tableName(name);
//	tableName("ziyu_dota_players");
//	
//	for (int i = 0; i < (sizeof(s_tableItems) / sizeof(s_tableItems[0])); i++)
//	{
//		EntityTableItem* pETItem = this->createItem(s_tableItems[i].tblItemType, &s_tableItems[i]);
//		//ERROR_MSG(fmt::format("EntityTableItem::{} {} \n", s_tableItems[i].tblItemName, s_tableItems[i].length));
//		pETItem->flags(pETItem->flags() | s_tableItems[i].flag);
//		pETItem->pParentTable(this);
//		pETItem->utype(i+1);
//		pETItem->tableName(this->tableName());
//		pETItem->itemName(s_tableItems[i].tblItemName);
//		tableItems_[pETItem->utype()].reset(pETItem);
//		tableFixedOrderItems_.push_back(pETItem);
//	}
//	//ERROR_MSG(fmt::format("KBEPlayerTableMysql::initialize \n"));
//	prikeyIndex = 1;
//	init_db_item_name();
//	return true;
//}

/**
同步表到数据库中
*/
//bool KBEPlayerTableMysql::syncToDB(DBInterface* dbi)
//{
//	bool ret = false;
//
//	std::string sqlstr = "CREATE TABLE IF NOT EXISTS ziyu_dota_players "
//		"(`user_id` bigint(20) NOT NULL,"
//		"`server_id` int(11) NOT NULL DEFAULT '0',"
//		"`nickname` varchar(250) NOT NULL DEFAULT '',"
//		"`last_set_name_time` int(11) NOT NULL DEFAULT '0',"
//		"`avatar` int(11) NOT NULL DEFAULT '0',"
//		"`level` int(11) NOT NULL DEFAULT '1',"
//		"`exp` bigint(20) NOT NULL DEFAULT '0',"
//		"`money` bigint(20) NOT NULL DEFAULT '0',"
//		"`gem` bigint(20) NOT NULL DEFAULT '0',"
//		"`arena_point` int(11) NOT NULL DEFAULT '0',"
//		"`crusade_point` int(11) NOT NULL DEFAULT '0',"
//		"`guild_point` int(11) NOT NULL,"
//		"`last_midas_time` int(11) NOT NULL DEFAULT '0',"
//		"`today_midas_times` int(11) NOT NULL DEFAULT '0',"
//		"`total_online_time` int(11) NOT NULL,"
//		"`tutorialstep` int(5) DEFAULT NULL,"
//		"`rechargegem` bigint(20) NOT NULL,"
//		"`facebook_follow` tinyint(1) NOT NULL DEFAULT '0',"
//		"PRIMARY KEY (`user_id`),"
//		"KEY `name` (`nickname`)) "
//		"ENGINE=" MYSQL_ENGINE_TYPE
//		" DEFAULT CHARSET=utf8";
//	ret = dbi->query(sqlstr.c_str(), sqlstr.size(), true);
//	KBE_ASSERT(ret);
//	return ret;
//}

/**
获取所有的数据放到流中
*/
//bool KBEPlayerTableMysql::queryTable(DBInterface* dbi, DBID dbid, MemoryStream* s)
//{
//	struct  
//	{
//		DBID usrId;
//		uint32 serverId;
//		std::string nickName;
//		uint32 last_set_name_time;
//		uint32 avatar;
//		uint32 level;
//		int64 exp;
//		int64 money;
//		int64 gem;
//		uint32 arena_point;
//		uint32 crusade_point;
//		uint32 guild_point;
//		uint32 last_midas_time;
//		uint32 today_midas_times;
//		uint32 total_online_time;
//		uint32 tutorialstep;
//		uint64 rechargegem;
//		uint8 facebook_follow;
//	} _table;
//	_table.usrId = 0;
//
//	std::string sqlstr = "select * from ziyu_dota_players where user_id=\"";
//	sqlstr += dbid;
//	sqlstr += "\" LIMIT 1";
//
//	// 如果查询失败则返回存在， 避免可能产生的错误
//	if (!dbi->query(sqlstr.c_str(), sqlstr.size(), false))
//		return true;
//
//	MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
//	if (pResult)
//	{
//		MYSQL_ROW arow = mysql_fetch_row(pResult);
//		if (arow != NULL)
//		{
//			KBEngine::StringConv::str2value(_table.usrId, arow[0]);
//			KBEngine::StringConv::str2value(_table.serverId, arow[1]);
//			_table.nickName = arow[2];
//			KBEngine::StringConv::str2value(_table.last_set_name_time, arow[3]);
//			KBEngine::StringConv::str2value(_table.avatar, arow[4]);
//			KBEngine::StringConv::str2value(_table.level, arow[5]);
//			KBEngine::StringConv::str2value(_table.exp, arow[6]);
//			KBEngine::StringConv::str2value(_table.money, arow[7]);
//			KBEngine::StringConv::str2value(_table.gem, arow[8]);
//			KBEngine::StringConv::str2value(_table.arena_point, arow[9]);
//			KBEngine::StringConv::str2value(_table.crusade_point, arow[10]);
//			KBEngine::StringConv::str2value(_table.guild_point, arow[11]);
//			KBEngine::StringConv::str2value(_table.last_midas_time, arow[12]);
//			KBEngine::StringConv::str2value(_table.today_midas_times, arow[13]);
//			KBEngine::StringConv::str2value(_table.total_online_time, arow[14]);
//			KBEngine::StringConv::str2value(_table.tutorialstep, arow[15]);
//			KBEngine::StringConv::str2value(_table.rechargegem, arow[16]);
//			KBEngine::StringConv::str2value(_table.facebook_follow, arow[17]);
//		}
//
//		mysql_free_result(pResult);
//	}
//
//	return _table.usrId > 0;
//}

//-------------------------------------------------------------------------------------
}
