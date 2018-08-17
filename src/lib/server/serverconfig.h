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

/*
		ServerConfig::getSingleton().loadConfig("../../res/server/KBEngine.xml");
		ENGINE_COMPONENT_INFO& ecinfo = ServerConfig::getSingleton().getCellApp();													
*/
#ifndef KBE_SERVER_CONFIG_H
#define KBE_SERVER_CONFIG_H

#define __LIB_DLLAPI__	

#include "common/common.h"
#if KBE_PLATFORM == PLATFORM_WIN32
#pragma warning (disable : 4996)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>	
#include <stdarg.h> 
#include "common/singleton.h"
#include "thread/threadmutex.h"
#include "thread/threadguard.h"
#include "xml/xml.h"	

	
namespace KBEngine{
namespace Network
{
class Address;
}

struct Profiles_Config
{
	Profiles_Config():
		open_pyprofile(false),
		open_cprofile(false),
		open_eventprofile(false),
		open_networkprofile(false)
	{
	}

	bool open_pyprofile;
	bool open_cprofile;
	bool open_eventprofile;
	bool open_networkprofile;
};

struct ChannelCommon
{
	float channelInternalTimeout;
	float channelExternalTimeout;
	uint32 extReadBufferSize;
	uint32 extWriteBufferSize;
	uint32 intReadBufferSize;
	uint32 intWriteBufferSize;
	uint32 intReSendInterval;
	uint32 intReSendRetries;
	uint32 extReSendInterval;
	uint32 extReSendRetries;
};

struct EmailServerInfo
{
	std::string smtp_server;
	uint32 smtp_port;
	std::string username;
	std::string password;
	uint8 smtp_auth;
};

struct EmailSendInfo
{
	std::string subject;
	std::string message;
	std::string backlink_success_message, backlink_fail_message, backlink_hello_message;

	uint32 deadline;
};

// 引擎组件信息结构体
typedef struct EngineComponentInfo
{
	EngineComponentInfo()
	{
		port = 0;
		port_max = 0;
		tcp_SOMAXCONN = 5;
		notFoundAccountAutoCreate = false;
		debugDBMgr = false;
	}

	~EngineComponentInfo()
	{
	}

	uint32 port;											// 组件的运行后监听的端口
	uint32 port_max;										// 组件的监听的端口最大值, login 、base用到了
															//char ip[MAX_BUF];										// 组件的运行期ip地址
	std::string ip; 
	COMPONENT_ID componentID;
	uint32 db_port;											// 数据库的端口
	char db_ip[MAX_BUF];									// 数据库的ip地址
	char db_username[MAX_NAME];								// 数据库的用户名
	char db_password[MAX_BUF * 10];							// 数据库的密码
	char db_name[MAX_NAME];									// 数据库名
	uint16 db_numConnections;								// 数据库最大连接
	std::string db_unicodeString_characterSet;				// 设置数据库字符集
	std::string db_unicodeString_collation;
	bool notFoundAccountAutoCreate;							// 登录合法时游戏数据库找不到游戏账号则自动创建
	float loadSmoothingBias;								// baseapp负载滤平衡调整值，
	ENTITY_ID criticallyLowSize;							// id剩余这么多个时向dbmgr申请新的id资源
	Profiles_Config profiles;
	uint32 tcp_SOMAXCONN;									// listen监听队列最大值
	uint64 respool_timeout;
	uint32 respool_buffersize;

	bool debugDBMgr;										// debug模式下可输出读写操作信息

}ENGINE_COMPONENT_INFO;

class ServerConfig : public Singleton<ServerConfig>
{
public:
	ServerConfig();
	~ServerConfig();
	
	bool loadConfig(std::string fileName);
	
	INLINE ENGINE_COMPONENT_INFO& getCellApp(void);
	INLINE ENGINE_COMPONENT_INFO& getBaseApp(void);
	INLINE ENGINE_COMPONENT_INFO& getDBMgr(void);
	INLINE ENGINE_COMPONENT_INFO& getLoginApp(void);
	INLINE ENGINE_COMPONENT_INFO& getCellAppMgr(void);
	INLINE ENGINE_COMPONENT_INFO& getBaseAppMgr(void);
	INLINE ENGINE_COMPONENT_INFO& getLogger(void);

	INLINE ENGINE_COMPONENT_INFO& getComponent(COMPONENT_TYPE componentType);

	INLINE ENGINE_COMPONENT_INFO& getConfig();

 	void updateInfos(bool isPrint, COMPONENT_TYPE componentType, COMPONENT_ID componentID, 
 				const Network::Address& internalAddr, const Network::Address& externalAddr);

	INLINE int16 gameUpdateHertz(void) const;
	INLINE Network::Address interfacesAddr(void) const;
	
	const ChannelCommon& channelCommon(){ return channelCommon_; }

	uint32 tcp_SOMAXCONN(COMPONENT_TYPE componentType);

	uint32 tickMaxBufferedLogs() const { return tick_max_buffered_logs_; }
	uint32 tickMaxSyncLogs() const { return tick_max_sync_logs_; }

private:
	ENGINE_COMPONENT_INFO _cellAppInfo;
	ENGINE_COMPONENT_INFO _baseAppInfo;
	ENGINE_COMPONENT_INFO _dbmgrInfo;
	ENGINE_COMPONENT_INFO _loginAppInfo;
	ENGINE_COMPONENT_INFO _cellAppMgrInfo;
	ENGINE_COMPONENT_INFO _baseAppMgrInfo;
	ENGINE_COMPONENT_INFO _loggerInfo;

public:
	int16 gameUpdateHertz_;
	uint32 tick_max_buffered_logs_;
	uint32 tick_max_sync_logs_;

	ChannelCommon channelCommon_;	

	Network::Address interfacesAddr_;

	uint32 thread_init_create_, thread_pre_create_, thread_max_create_;

};

#define g_kbeSrvConfig ServerConfig::getSingleton()
}


#ifdef CODE_INLINE
#include "serverconfig.inl"
#endif
#endif // KBE_SERVER_CONFIG_H
