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


#include "message_handler.h"
#include "common/md5.h"
#include "network/channel.h"
#include "network/network_interface.h"
#include "network/packet_receiver.h"
#include "network/fixed_messages.h"
#include "helper/watcher.h"
#include "xml/xml.h"
#include "resmgr/resmgr.h"	

namespace KBEngine { 
namespace Network
{
Network::MessageHandlers* MessageHandlers::pMainMessageHandlers = 0;
std::vector<MessageHandlers*>* g_pMessageHandlers;

//-------------------------------------------------------------------------------------
MessageHandlers::MessageHandlers():
msgHandlers_(),
msgID_(1),
exposedMessages_()
{
	messageHandlers().push_back(this);
}

//-------------------------------------------------------------------------------------
MessageHandlers::~MessageHandlers()
{
	MessageHandlerMap::iterator iter = msgHandlers_.begin();
	for(; iter != msgHandlers_.end(); ++iter)
	{
		if(iter->second)
			delete iter->second;
	};
}

//-------------------------------------------------------------------------------------
MessageHandler::MessageHandler():
pArgs(NULL),
pMessageHandlers(NULL),
send_size(0),
send_count(0),
recv_size(0),
recv_count(0)
{
}

//-------------------------------------------------------------------------------------
MessageHandler::~MessageHandler()
{
	SAFE_RELEASE(pArgs);
}

//-------------------------------------------------------------------------------------
const char* MessageHandler::c_str()
{
	static char buf[MAX_BUF];
	kbe_snprintf(buf, MAX_BUF, "id:%u, len:%d", msgID, msgLen);
	return buf;
}

//-------------------------------------------------------------------------------------
bool MessageHandlers::initializeWatcher()
{
	std::vector< std::string >::iterator siter = exposedMessages_.begin();
	for(; siter != exposedMessages_.end(); ++siter)
	{
		MessageHandlerMap::iterator iter = msgHandlers_.begin();
		for(; iter != msgHandlers_.end(); ++iter)
		{
			if((*siter) == iter->second->name)
			{
				iter->second->exposed = true;
			}
		}
	}

	MessageHandlerMap::iterator iter = msgHandlers_.begin();
	for(; iter != msgHandlers_.end(); ++iter)
	{
		char buf[MAX_BUF];
		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/id", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second->msgID);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/len", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second->msgLen);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/sentSize", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second, &MessageHandler::sendsize);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/sentCount", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second, &MessageHandler::sendcount);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/sentAvgSize", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second, &MessageHandler::sendavgsize);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/recvSize", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second, &MessageHandler::recvsize);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/recvCount", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second, &MessageHandler::recvsize);

		kbe_snprintf(buf, MAX_BUF, "network/messages/%s/recvAvgSize", iter->second->name.c_str());
		WATCH_OBJECT(buf, iter->second, &MessageHandler::recvavgsize);
	}

	return true;
}

//-------------------------------------------------------------------------------------
MessageHandler* MessageHandlers::add(MessageID msgID, std::string ihName, MessageArgs* args,
	int32 msgLen, MessageHandler* msgHandler)
{
	msgHandler->msgID = msgID;
	msgHandler->name = ihName;
	msgHandler->pArgs = args;
	msgHandler->msgLen = msgLen;
	msgHandler->exposed = false;
	msgHandler->pMessageHandlers = this;
	msgHandler->onInstall();

	msgHandlers_[msgHandler->msgID] = msgHandler;

	return msgHandler;
}


//-------------------------------------------------------------------------------------
MessageHandler* MessageHandlers::find(MessageID msgID)
{
	MessageHandlerMap::iterator iter = msgHandlers_.find(msgID);
	if(iter != msgHandlers_.end())
	{
		return iter->second;
	};
	
	return NULL;
}

//-------------------------------------------------------------------------------------
std::vector<MessageHandlers*>& MessageHandlers::messageHandlers()
{
	if(g_pMessageHandlers == NULL)
		g_pMessageHandlers = new std::vector<MessageHandlers*>;

	return *g_pMessageHandlers;
}

//-------------------------------------------------------------------------------------
void MessageHandlers::finalise(void)
{
	SAFE_RELEASE(g_pMessageHandlers);
}

//-------------------------------------------------------------------------------------
bool MessageHandlers::pushExposedMessage(std::string msgname)
{
	exposedMessages_.push_back(msgname);
	return true;
}

//-------------------------------------------------------------------------------------
} 
}
