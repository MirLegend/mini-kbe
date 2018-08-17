/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2017 KBEngine.

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

#include "baseapp.h"
#include "proxy.h"
#include "proxy_forwarder.h"
//#include "data_download.h"
#include "network/channel.h"

#include "proto/cb.pb.h"
#include "proto/bmb.pb.h"
#include "../../server/basemgr/basemgr_interface.h"
#include "proto/basedb.pb.h"
#include "../../server/dbmgr/dbmgr_interface.h"

#ifndef CODE_INLINE
#include "proxy.inl"
#endif

namespace KBEngine{
	
//-------------------------------------------------------------------------------------
Proxy::Proxy(ENTITY_ID id):
Base(id, true),
rndUUID_(KBEngine::genUUID64()),
addr_(Network::Address::NONE),
entitiesEnabled_(false),
encryptionKey(),
pProxyForwarder_(NULL),
clientComponentType_(UNKNOWN_CLIENT_COMPONENT_TYPE),
loginDatas_(),
createDatas_()
{
	BaseApp::getSingleton().incProxicesCount();

	pProxyForwarder_ = new ProxyForwarder(this);
}

//-------------------------------------------------------------------------------------
Proxy::~Proxy()
{
	BaseApp::getSingleton().decProxicesCount();
	kick();
	SAFE_RELEASE(pProxyForwarder_);
}

//-------------------------------------------------------------------------------------
void Proxy::kick()
{
	// 如果被销毁频道仍然存活则将其关闭
	Network::Channel* pChannel = BaseApp::getSingleton().networkInterface().findChannel(addr_);
	//if(pChannel && !pChannel->isDestroyed())
	//{
	//	Network::Bundle* pBundle = Network::Bundle::createPoolObject();
	//	(*pBundle).newMessage(ClientInterface::onKicked);
	//	ClientInterface::onKickedArgs1::staticAddToBundle(*pBundle, SERVER_ERR_PROXY_DESTROYED);
	//	//pBundle->send(BaseApp::getSingleton().networkInterface(), pChannel);
	//	this->sendToClient(ClientInterface::onKicked, pBundle);
	//	this->sendToClient();
	//	pChannel->condemn();
	//}
}

//-------------------------------------------------------------------------------------
void Proxy::onEntitiesEnabled(void)
{
	entitiesEnabled_ = true;
}

//-------------------------------------------------------------------------------------
int32 Proxy::onLogOnAttempt(const char* addr, uint32 port, const char* password)
{
	int32 ret = LOG_ON_REJECT;

	return ret;
}

//-------------------------------------------------------------------------------------
void Proxy::onClientDeath(void)
{
	DEBUG_MSG(fmt::format("Proxy::onClientDeath: {}.\n", 
		 this->id()));

	addr(Network::Address::NONE);

	entitiesEnabled_ = false;

	this->onDestroyEntity(true, true);
	this->destroyEntity();

}

//-------------------------------------------------------------------------------------
void Proxy::onGetWitness()
{

}

//-------------------------------------------------------------------------------------
double Proxy::getRoundTripTime() const
{
	Network::Channel* pChannel = Components::getSingleton().pNetworkInterface()->findChannel(addr_);
	if (!pChannel)
	{
		return 0.0;
	}
	return double(pChannel->pEndPoint()->getRTT()) / 1000000.0;
}

void Proxy::InitDatas(const std::string datas)
{
	MemoryStream playerData;
	playerData.append(datas);
	int32 serverid;
	std::string nickName;
	int32 lastsetnametime;
	int32 avatar;
	int32 level;
	int64 exp;
	int64 money;
	int64 gem;
	int32 arena_point;
	int32 crusade_point;
	int32 guild_point;
	int32 last_midas_time;
	int32 today_midas_times;
	int32 total_online_time;
	int16 tutorialstep;
	int64 rechargegem;
	int8 facebook_follow;

	playerData >> serverid;
	playerData.readBlob(nickName);
	//>> nickName
	playerData >> lastsetnametime
		>> avatar
		>> level
		>> exp
		>> money
		>> gem
		>> arena_point
		>> crusade_point
		>> guild_point
		>> last_midas_time
		>> today_midas_times
		>> total_online_time
		>> tutorialstep
		>> rechargegem
		>> facebook_follow;

	INFO_MSG(fmt::format("Proxy::InitDatas: user={}, dbid={}, entityID={},serverid={}, nickName={}, level={}, exp={}, money={}, rechargegem={}\n",
		accountName(), dbid(), id(), serverid, nickName, level, exp, money, rechargegem));
}

//-------------------------------------------------------------------------------------
double Proxy::getTimeSinceHeardFromClient() const
{
	const Network::Channel* pChanel = Components::getSingleton().pNetworkInterface()->findChannel(addr_);
	return double(timestamp() - pChanel->lastReceivedTime()) / stampsPerSecondD();
}


//-------------------------------------------------------------------------------------
bool Proxy::hasClient() const
{
	return Components::getSingleton().pNetworkInterface()->findChannel(addr_) !=NULL;
}

//-------------------------------------------------------------------------------------
Network::Channel* Proxy::pChannel()
{
	return Components::getSingleton().pNetworkInterface()->findChannel(addr_);
}

//-------------------------------------------------------------------------------------
bool Proxy::pushBundle(Network::Bundle* pBundle)
{
	Network::Channel* pChannel = this->pChannel();
	if(!pChannel)
		return false;

	//pChannel->send(pBundle);

	pBundle->pChannel(pChannel);
	pBundle->finiMessage(true);
	pChannel->pushBundle(pBundle);

	{
		// 如果数据大量阻塞发不出去将会报警
		//AUTO_SCOPED_PROFILE("pushBundleAndSendToClient");
		//pChannel->send(pBundle);
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Proxy::sendToClient(Network::Bundle* pBundle)
{
	if(pushBundle(pBundle))
		return true;

	ERROR_MSG(fmt::format("Proxy::sendToClient: {} pBundles is NULL, not found channel.\n", id()));
	Network::Bundle::reclaimPoolObject(pBundle);
	return false;
}

//-------------------------------------------------------------------------------------
bool Proxy::sendToClient(bool expectData)
{
	Network::Channel* pChannel = this->pChannel();
	if(!pChannel)
		return false;

	if(expectData)
	{
		if(pChannel->bundles().size() == 0)
		{
			WARNING_MSG("Proxy::sendToClient: no data!\n");
			return false;
		}
	}

	{
		// 如果数据大量阻塞发不出去将会报警

		pChannel->send();
	}

	return true;
}

//-------------------------------------------------------------------------------------
}
