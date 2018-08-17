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

// ���������Ϣ�ṹ��
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

	uint32 port;											// ��������к�����Ķ˿�
	uint32 port_max;										// ����ļ����Ķ˿����ֵ, login ��base�õ���
															//char ip[MAX_BUF];										// �����������ip��ַ
	std::string ip; 
	COMPONENT_ID componentID;
	uint32 db_port;											// ���ݿ�Ķ˿�
	char db_ip[MAX_BUF];									// ���ݿ��ip��ַ
	char db_username[MAX_NAME];								// ���ݿ���û���
	char db_password[MAX_BUF * 10];							// ���ݿ������
	char db_name[MAX_NAME];									// ���ݿ���
	uint16 db_numConnections;								// ���ݿ��������
	std::string db_unicodeString_characterSet;				// �������ݿ��ַ���
	std::string db_unicodeString_collation;
	bool notFoundAccountAutoCreate;							// ��¼�Ϸ�ʱ��Ϸ���ݿ��Ҳ�����Ϸ�˺����Զ�����
	float loadSmoothingBias;								// baseapp������ƽ�����ֵ��
	ENTITY_ID criticallyLowSize;							// idʣ����ô���ʱ��dbmgr�����µ�id��Դ
	Profiles_Config profiles;
	uint32 tcp_SOMAXCONN;									// listen�����������ֵ
	uint64 respool_timeout;
	uint32 respool_buffersize;

	bool debugDBMgr;										// debugģʽ�¿������д������Ϣ

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
