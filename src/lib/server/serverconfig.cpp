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


#include "serverconfig.h"
#include "network/common.h"
#include "network/address.h"
#include "resmgr/resmgr.h"
#include "common/kbekey.h"
#include "common/kbeversion.h"

#ifndef CODE_INLINE
#include "serverconfig.inl"
#endif

namespace KBEngine{
KBE_SINGLETON_INIT(ServerConfig);

//-------------------------------------------------------------------------------------
ServerConfig::ServerConfig():
gameUpdateHertz_(10),
tick_max_buffered_logs_(4096),
tick_max_sync_logs_(32),
interfacesAddr_(),
thread_init_create_(1),
thread_pre_create_(2),
thread_max_create_(8)
{
}

//-------------------------------------------------------------------------------------
ServerConfig::~ServerConfig()
{
}

//-------------------------------------------------------------------------------------
bool ServerConfig::loadConfig(std::string fileName)
{
	TiXmlNode* node = NULL, *rootNode = NULL;
	SmartPointer<XML> xml(new XML(Resmgr::getSingleton().matchRes(fileName).c_str()));

	if (!xml->isGood())
	{
		ERROR_MSG(fmt::format("ServerConfig::loadConfig: load {} is failed!\n",
			fileName.c_str()));

		return false;
	}

	if (xml->getRootNode() == NULL)
	{
		// root节点下没有子节点了
		return true;
	}

	rootNode = xml->getRootNode("thread_pool");
	if (rootNode != NULL)
	{
		TiXmlNode* childnode = xml->enterNode(rootNode, "timeout");
		/*	if(childnode)
		{
		thread_timeout_ = float(ZIYU_MAX(1.0, xml->getValFloat(childnode)));
		}*/

		childnode = xml->enterNode(rootNode, "init_create");
		if (childnode)
		{
			thread_init_create_ = KBE_MAX(1, xml->getValInt(childnode));
		}

		childnode = xml->enterNode(rootNode, "pre_create");
		if (childnode)
		{
			thread_pre_create_ = KBE_MAX(1, xml->getValInt(childnode));
		}

		childnode = xml->enterNode(rootNode, "max_create");
		if (childnode)
		{
			thread_max_create_ = KBE_MAX(1, xml->getValInt(childnode));
		}
	}

	rootNode = xml->getRootNode("channelCommon");
	if (rootNode != NULL)
	{
		TiXmlNode* childnode = xml->enterNode(rootNode, "timeout");
		if (childnode)
		{
			TiXmlNode* childnode1 = xml->enterNode(childnode, "internal");
			if (childnode1)
			{
				channelCommon_.channelInternalTimeout = KBE_MAX(0.f, float(xml->getValFloat(childnode1)));
				Network::g_channelInternalTimeout = channelCommon_.channelInternalTimeout;
			}

			childnode1 = xml->enterNode(childnode, "external");
			if (childnode)
			{
				channelCommon_.channelExternalTimeout = KBE_MAX(0.f, float(xml->getValFloat(childnode1)));
				Network::g_channelExternalTimeout = channelCommon_.channelExternalTimeout;
			}
		}

		childnode = xml->enterNode(rootNode, "resend");
		if (childnode)
		{
			TiXmlNode* childnode1 = xml->enterNode(childnode, "internal");
			if (childnode1)
			{
				TiXmlNode* childnode2 = xml->enterNode(childnode1, "interval");
				if (childnode2)
				{
					Network::g_intReSendInterval = uint32(xml->getValInt(childnode2));
				}

				childnode2 = xml->enterNode(childnode1, "retries");
				if (childnode2)
				{
					Network::g_intReSendRetries = uint32(xml->getValInt(childnode2));
				}
			}

			childnode1 = xml->enterNode(childnode, "external");
			if (childnode)
			{
				TiXmlNode* childnode2 = xml->enterNode(childnode1, "interval");
				if (childnode2)
				{
					Network::g_extReSendInterval = uint32(xml->getValInt(childnode2));
				}

				childnode2 = xml->enterNode(childnode1, "retries");
				if (childnode2)
				{
					Network::g_extReSendRetries = uint32(xml->getValInt(childnode2));
				}
			}
		}

		childnode = xml->enterNode(rootNode, "readBufferSize");
		if (childnode)
		{
			TiXmlNode* childnode1 = xml->enterNode(childnode, "internal");
			if (childnode1)
				channelCommon_.intReadBufferSize = KBE_MAX(0, xml->getValInt(childnode1));

			childnode1 = xml->enterNode(childnode, "external");
			if (childnode1)
				channelCommon_.extReadBufferSize = KBE_MAX(0, xml->getValInt(childnode1));
		}

		childnode = xml->enterNode(rootNode, "writeBufferSize");
		if (childnode)
		{
			TiXmlNode* childnode1 = xml->enterNode(childnode, "internal");
			if (childnode1)
				channelCommon_.intWriteBufferSize = KBE_MAX(0, xml->getValInt(childnode1));

			childnode1 = xml->enterNode(childnode, "external");
			if (childnode1)
				channelCommon_.extWriteBufferSize = KBE_MAX(0, xml->getValInt(childnode1));
		}

		childnode = xml->enterNode(rootNode, "windowOverflow");
		if (childnode)
		{
			TiXmlNode* sendNode = xml->enterNode(childnode, "send");
			if (sendNode)
			{
				TiXmlNode* childnode1 = xml->enterNode(sendNode, "messages");
				if (childnode1)
				{
					TiXmlNode* childnode2 = xml->enterNode(childnode1, "internal");
					if (childnode2)
						Network::g_intSendWindowMessagesOverflow = KBE_MAX(0, xml->getValInt(childnode2));

					childnode2 = xml->enterNode(childnode1, "external");
					if (childnode2)
						Network::g_extSendWindowMessagesOverflow = KBE_MAX(0, xml->getValInt(childnode2));

					childnode2 = xml->enterNode(childnode1, "critical");
					if (childnode2)
						Network::g_sendWindowMessagesOverflowCritical = KBE_MAX(0, xml->getValInt(childnode2));
				}

				childnode1 = xml->enterNode(sendNode, "bytes");
				if (childnode1)
				{
					TiXmlNode* childnode2 = xml->enterNode(childnode1, "internal");
					if (childnode2)
						Network::g_intSendWindowBytesOverflow = KBE_MAX(0, xml->getValInt(childnode2));

					childnode2 = xml->enterNode(childnode1, "external");
					if (childnode2)
						Network::g_extSendWindowBytesOverflow = KBE_MAX(0, xml->getValInt(childnode2));
				}
			}

			TiXmlNode* recvNode = xml->enterNode(childnode, "receive");
			if (recvNode)
			{
				TiXmlNode* childnode1 = xml->enterNode(recvNode, "messages");
				if (childnode1)
				{
					TiXmlNode* childnode2 = xml->enterNode(childnode1, "internal");
					if (childnode2)
						Network::g_intReceiveWindowMessagesOverflow = KBE_MAX(0, xml->getValInt(childnode2));

					childnode2 = xml->enterNode(childnode1, "external");
					if (childnode2)
						Network::g_extReceiveWindowMessagesOverflow = KBE_MAX(0, xml->getValInt(childnode2));

					childnode2 = xml->enterNode(childnode1, "critical");
					if (childnode2)
						Network::g_receiveWindowMessagesOverflowCritical = KBE_MAX(0, xml->getValInt(childnode2));
				}

				childnode1 = xml->enterNode(recvNode, "bytes");
				if (childnode1)
				{
					TiXmlNode* childnode2 = xml->enterNode(childnode1, "internal");
					if (childnode2)
						Network::g_intReceiveWindowBytesOverflow = KBE_MAX(0, xml->getValInt(childnode2));

					childnode2 = xml->enterNode(childnode1, "external");
					if (childnode2)
						Network::g_extReceiveWindowBytesOverflow = KBE_MAX(0, xml->getValInt(childnode2));
				}
			}
		};
	}

	rootNode = xml->getRootNode("gameUpdateHertz");
	if (rootNode != NULL) {
		gameUpdateHertz_ = xml->getValInt(rootNode);
	}

	rootNode = xml->getRootNode("cellapp");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ids");
		if (node != NULL)
		{
			TiXmlNode* childnode = xml->enterNode(node, "criticallyLowSize");
			if (childnode)
			{
				_cellAppInfo.criticallyLowSize = xml->getValInt(childnode);
				if (_cellAppInfo.criticallyLowSize < 100)
					_cellAppInfo.criticallyLowSize = 100;
			}
		}
		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_cellAppInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}
		node = xml->enterNode(rootNode, "loadSmoothingBias");
		if (node != NULL)
			_cellAppInfo.loadSmoothingBias = float(xml->getValFloat(node));
	}

	rootNode = xml->getRootNode("baseapp");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ip");
		if (node)
		{
			_baseAppInfo.ip = xml->getValStr(node);
		}
		if (_baseAppInfo.ip.size() == 0)
			_baseAppInfo.ip = "localhost";

		node = xml->enterNode(rootNode, "port");
		if (node)
		{
			_baseAppInfo.port = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "port_max");
		if (node)
		{
			_baseAppInfo.port_max = xml->getValInt(node);
		}

		if (_baseAppInfo.port > _baseAppInfo.port_max)
		{
			_baseAppInfo.port_max = _baseAppInfo.port;
		}

		node = xml->enterNode(rootNode, "loadSmoothingBias");
		if (node != NULL)
			_baseAppInfo.loadSmoothingBias = float(xml->getValFloat(node));

		node = xml->enterNode(rootNode, "ids");
		if (node != NULL)
		{
			TiXmlNode* childnode = xml->enterNode(node, "criticallyLowSize");
			if (childnode)
			{
				_baseAppInfo.criticallyLowSize = xml->getValInt(childnode);
				if (_baseAppInfo.criticallyLowSize < 100)
					_baseAppInfo.criticallyLowSize = 100;
			}
		}

		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_baseAppInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "respool");
		if (node != NULL)
		{
			TiXmlNode* childnode = xml->enterNode(node, "buffer_size");
			if (childnode)
				_baseAppInfo.respool_buffersize = xml->getValInt(childnode);

			childnode = xml->enterNode(node, "timeout");
			if (childnode)
				_baseAppInfo.respool_timeout = uint64(xml->getValInt(childnode));

			childnode = xml->enterNode(node, "checktick");
			if (childnode)
				Resmgr::respool_checktick = xml->getValInt(childnode);

			Resmgr::respool_timeout = _baseAppInfo.respool_timeout;
			Resmgr::respool_buffersize = _baseAppInfo.respool_buffersize;
		}
	}

	rootNode = xml->getRootNode("dbmgr");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ip");
		if (node)
		{
			_dbmgrInfo.ip = xml->getValStr(node);
		}
		if (_dbmgrInfo.ip.size() == 0)
			_dbmgrInfo.ip = "localhost";

		node = xml->enterNode(rootNode, "port");
		if (node)
		{
			_dbmgrInfo.port = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "port_max");
		if (node)
		{
			_dbmgrInfo.port_max = xml->getValInt(node);
		}

		if (_dbmgrInfo.port > _dbmgrInfo.port_max)
		{
			_dbmgrInfo.port_max = _dbmgrInfo.port;
		}

		node = xml->enterNode(rootNode, "host");
		if (node != NULL)
			strncpy((char*)&_dbmgrInfo.db_ip, xml->getValStr(node).c_str(), MAX_IP);

		node = xml->enterNode(rootNode, "dbport");
		if (node != NULL)
			_dbmgrInfo.db_port = xml->getValInt(node);

		node = xml->enterNode(rootNode, "auth");
		if (node != NULL)
		{
			TiXmlNode* childnode = xml->enterNode(node, "password");
			if (childnode)
			{
				strncpy((char*)&_dbmgrInfo.db_password, xml->getValStr(childnode).c_str(), MAX_BUF * 10);
			}

			childnode = xml->enterNode(node, "username");
			if (childnode)
			{
				strncpy((char*)&_dbmgrInfo.db_username, xml->getValStr(childnode).c_str(), MAX_NAME);
			}
		}

		node = xml->enterNode(rootNode, "databaseName");
		if (node != NULL)
			strncpy((char*)&_dbmgrInfo.db_name, xml->getValStr(node).c_str(), MAX_NAME);

		node = xml->enterNode(rootNode, "numConnections");
		if (node != NULL)
			_dbmgrInfo.db_numConnections = xml->getValInt(node);

		node = xml->enterNode(rootNode, "unicodeString");
		if (node != NULL)
		{
			TiXmlNode* childnode = xml->enterNode(node, "characterSet");
			if (childnode)
			{
				_dbmgrInfo.db_unicodeString_characterSet = xml->getValStr(childnode);
			}

			childnode = xml->enterNode(node, "collation");
			if (childnode)
			{
				_dbmgrInfo.db_unicodeString_collation = xml->getValStr(childnode);
			}
		}

		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_dbmgrInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "debug");
		if (node != NULL) {
			_dbmgrInfo.debugDBMgr = (xml->getValStr(node) == "true");
		}
	}

	if (_dbmgrInfo.db_unicodeString_characterSet.size() == 0)
		_dbmgrInfo.db_unicodeString_characterSet = "utf8";

	if (_dbmgrInfo.db_unicodeString_collation.size() == 0)
		_dbmgrInfo.db_unicodeString_collation = "utf8_bin";

	rootNode = xml->getRootNode("loginapp");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ip");
		if (node)
		{
			_loginAppInfo.ip = xml->getValStr(node);
		}
		if (_loginAppInfo.ip.size() == 0)
			_loginAppInfo.ip = "localhost";

		node = xml->enterNode(rootNode, "port");
		if (node)
		{
			_loginAppInfo.port = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "port_max");
		if (node)
		{
			_loginAppInfo.port_max = xml->getValInt(node);
		}

		if (_loginAppInfo.port > _loginAppInfo.port_max)
		{
			_loginAppInfo.port_max = _loginAppInfo.port;
		}

		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_loginAppInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}
	}

	rootNode = xml->getRootNode("cellappmgr");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ip");
		if (node)
		{
			_cellAppMgrInfo.ip = xml->getValStr(node);
		}
		if (_cellAppMgrInfo.ip.size() == 0)
			_cellAppMgrInfo.ip = "localhost";

		node = xml->enterNode(rootNode, "port");
		if (node)
		{
			_cellAppMgrInfo.port = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "port_max");
		if (node)
		{
			_cellAppMgrInfo.port_max = xml->getValInt(node);
		}

		if (_cellAppMgrInfo.port > _cellAppMgrInfo.port_max)
		{
			_cellAppMgrInfo.port_max = _cellAppMgrInfo.port;
		}

		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_cellAppMgrInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}
	}

	rootNode = xml->getRootNode("baseappmgr");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ip");
		if (node)
		{
			_baseAppMgrInfo.ip = xml->getValStr(node);
		}
		if (_baseAppMgrInfo.ip.size() == 0)
			_baseAppMgrInfo.ip = "localhost";

		node = xml->enterNode(rootNode, "port");
		if (node)
		{
			_baseAppMgrInfo.port = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "port_max");
		if (node)
		{
			_baseAppMgrInfo.port_max = xml->getValInt(node);
		}

		if (_baseAppMgrInfo.port > _baseAppMgrInfo.port_max)
		{
			_baseAppMgrInfo.port_max = _baseAppMgrInfo.port;
		}

		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_baseAppMgrInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}
	}

	rootNode = xml->getRootNode("logger");
	if (rootNode != NULL)
	{
		node = xml->enterNode(rootNode, "ip");
		if (node)
		{
			_loggerInfo.ip = xml->getValStr(node);
		}
		if (_loggerInfo.ip.size() == 0)
			_loggerInfo.ip = "localhost";

		node = xml->enterNode(rootNode, "port");
		if (node)
		{
			_loggerInfo.port = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "port_max");
		if (node)
		{
			_loggerInfo.port_max = xml->getValInt(node);
		}

		if (_loggerInfo.port > _loggerInfo.port_max)
		{
			_loggerInfo.port_max = _loggerInfo.port;
		}

		node = xml->enterNode(rootNode, "SOMAXCONN");
		if (node != NULL) {
			_loggerInfo.tcp_SOMAXCONN = xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "tick_max_buffered_logs");
		if (node != NULL) {
			tick_max_buffered_logs_ = (uint32)xml->getValInt(node);
		}

		node = xml->enterNode(rootNode, "tick_sync_logs");
		if (node != NULL) {
			tick_max_sync_logs_ = (uint32)xml->getValInt(node);
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------	
uint32 ServerConfig::tcp_SOMAXCONN(COMPONENT_TYPE componentType)
{
	ENGINE_COMPONENT_INFO& cinfo = getComponent(componentType);
	return cinfo.tcp_SOMAXCONN;
}

void ServerConfig::updateInfos(bool isPrint, COMPONENT_TYPE componentType, COMPONENT_ID componentID, 
							   const Network::Address& internalAddr, const Network::Address& externalAddr)
{
	std::string infostr = "";

	//updateExternalAddress(getBaseApp().externalAddress);
	//updateExternalAddress(getLoginApp().externalAddress);

#if KBE_PLATFORM == PLATFORM_WIN32
	if(infostr.size() > 0)
	{
		infostr += "\n";
		printf("%s", infostr.c_str());
	}
#endif
}

//-------------------------------------------------------------------------------------		
}
