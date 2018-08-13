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


#include "components.h"
#include "helper/debug_helper.h"
#include "helper/sys_info.h"
#include "network/channel.h"	
#include "network/address.h"	
#include "network/bundle.h"	
#include "network/udp_packet.h"
#include "network/tcp_packet.h"
#include "network/bundle_broadcast.h"
#include "network/network_interface.h"
#include "server/serverconfig.h"

#include "proto/coms.pb.h"
#include "../../server/logger/logger_interface.h"
#include "../../server/login/login_interface.h"
#include "../../server/dbmgr/dbmgr_interface.h"
#include "../../server/basemgr/basemgr_interface.h"
#include "../../server/cellmgr/cellmgr_interface.h"
#include "../../server/baseapp/baseapp_interface.h"
#include "../../server/cellapp/cellapp_interface.h"

namespace KBEngine
{
int32 Components::ANY_UID = -1;

KBE_SINGLETON_INIT(Components);
Components _g_components;

//-------------------------------------------------------------------------------------
Components::Components():
Task(),
_baseapps(),
_cellapps(),
_dbmgrs(),
_loginapps(),
_cellappmgrs(),
_baseappmgrs(),
_loggers(),
_pNetworkInterface(NULL),
_pHandler(NULL),
componentType_(UNKNOWN_COMPONENT_TYPE),
componentID_(0),
state_(0),
findIdx_(0)
{
}

//-------------------------------------------------------------------------------------
Components::~Components()
{
}

//-------------------------------------------------------------------------------------
void Components::initialize(Network::NetworkInterface * pNetworkInterface, COMPONENT_TYPE componentType, COMPONENT_ID componentID)
{ 
	KBE_ASSERT(pNetworkInterface != NULL); 
	_pNetworkInterface = pNetworkInterface; 

	componentType_ = componentType;
	componentID_ = componentID;

	for(uint8 i=0; i<8; ++i)
		findComponentTypes_[i] = UNKNOWN_COMPONENT_TYPE;
	needCompnentNum = 0;

	switch(componentType_)
	{
	case CELLAPP_TYPE:
		findComponentTypes_[0] = LOGGER_TYPE;
		findComponentTypes_[1] = DBMGR_TYPE;
		findComponentTypes_[2] = CELLAPPMGR_TYPE;
		findComponentTypes_[3] = BASEAPPMGR_TYPE;
		needCompnentNum = 4;
		break;
	case BASEAPP_TYPE:
		findComponentTypes_[0] = LOGGER_TYPE;
		findComponentTypes_[1] = DBMGR_TYPE;
		findComponentTypes_[2] = BASEAPPMGR_TYPE;
		findComponentTypes_[3] = CELLAPPMGR_TYPE;
		needCompnentNum = 4;
		break;
	case BASEAPPMGR_TYPE:
		findComponentTypes_[0] = LOGGER_TYPE;
		findComponentTypes_[1] = DBMGR_TYPE;
		findComponentTypes_[2] = CELLAPPMGR_TYPE;
		needCompnentNum = 3;
		break;
	case CELLAPPMGR_TYPE:
		findComponentTypes_[0] = LOGGER_TYPE;
		findComponentTypes_[1] = DBMGR_TYPE;
		//findComponentTypes_[2] = BASEAPPMGR_TYPE;
		needCompnentNum = 2;
		break;
	case LOGINAPP_TYPE:
		findComponentTypes_[0] = LOGGER_TYPE;
		findComponentTypes_[1] = DBMGR_TYPE;
		findComponentTypes_[2] = BASEAPPMGR_TYPE;
		needCompnentNum = 3;
		break;
	case DBMGR_TYPE:
		findComponentTypes_[0] = LOGGER_TYPE;
		needCompnentNum = 1;
		break;
	default:
		if (componentType_ != LOGGER_TYPE &&
			componentType_ != MACHINE_TYPE &&
			componentType_ != INTERFACES_TYPE)
		{
			findComponentTypes_[0] = LOGGER_TYPE;
			needCompnentNum = 1;
		}
		break;
	};
}

//-------------------------------------------------------------------------------------
void Components::finalise()
{
	clear(0, false);
}

//-------------------------------------------------------------------------------------
bool Components::checkComponents(int32 uid, COMPONENT_ID componentID, COMPONENT_TYPE componentType)
{
	if(componentID <= 0)
		return false;

	COMPONENTS& components = getComponents(componentType);
	if (!COMPONENT_DUPS[componentType] && components.size()>0)
	{
		ERROR_MSG(fmt::format("checkComponents: size:{} error 1111.\n",
			components.size()));
		return false;
	}

	int idx = 0;
	while(true)
	{
		COMPONENT_TYPE ct = ALL_COMPONENT_TYPES[idx++];
		if(ct == UNKNOWN_COMPONENT_TYPE)
			break;

		ComponentInfos* cinfos = findComponent(ct, uid, componentID);
		if(cinfos != NULL)
		{
			ERROR_MSG(fmt::format("checkComponents: componentID:{} error 2222.\n",
				componentID));
			return false;
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------		
void Components::addComponent(int32 uid, const char* username, 
			COMPONENT_TYPE componentType, COMPONENT_ID componentID,
			uint32 intaddr, uint16 intport, 
			uint32 extaddr, uint16 extport,
			Network::Channel* pChannel)
{
	COMPONENTS& components = getComponents(componentType);

	ComponentInfos* cinfos = findComponent(componentType, uid, componentID);
	if(cinfos != NULL)
	{
		WARNING_MSG(fmt::format("Components::addComponent[{}]: uid:{}, username:{}, "
			"componentType:{}, componentID:{} is exist!\n",
			COMPONENT_NAME_EX(componentType), uid, username, (int32)componentType, componentID));
		return;
	}
	
	ComponentInfos componentInfos;

	componentInfos.pIntAddr.reset(new Network::Address(intaddr, intport));
	componentInfos.pExtAddr.reset(new Network::Address(extaddr, extport));

	componentInfos.uid = uid;
	componentInfos.cid = componentID;
	componentInfos.pChannel = pChannel;
	componentInfos.componentType = componentType;
	

	if(pChannel)
		pChannel->componentID(componentID);

	strncpy(componentInfos.username, username, MAX_NAME);

	if (cinfos == NULL)
		components.push_back(componentInfos);
	else
		*cinfos = componentInfos;

	INFO_MSG(fmt::format("Components::addComponent[{}], uid={}, "
		"componentID={}, totalcount={}, port={}\n",
		COMPONENT_NAME_EX(componentType),
		uid,
		componentID,
		/*((int32)componentInfos.globalOrderid),
		((int32)componentInfos.groupOrderid),*/
		components.size(),
		pChannel->addr().port));

	if (_pHandler)
		_pHandler->onAddComponent(&componentInfos);
}

//-------------------------------------------------------------------------------------		
void Components::delComponent(int32 uid, COMPONENT_TYPE componentType, 
							  COMPONENT_ID componentID, bool ignoreComponentID, bool shouldShowLog)
{
	COMPONENTS& components = getComponents(componentType);
	COMPONENTS::iterator iter = components.begin();
	for(; iter != components.end();)
	{
		if((uid < 0 || (*iter).uid == uid) && (ignoreComponentID == true || (*iter).cid == componentID))
		{
			INFO_MSG(fmt::format("Components::delComponent[{}] componentID={}, component:totalcount={}.\n", 
				COMPONENT_NAME_EX(componentType), componentID, components.size()));

			ComponentInfos* componentInfos = &(*iter);

			//SAFE_RELEASE((*iter).pIntAddr);
			//SAFE_RELEASE((*iter).pExtAddr);
			//(*iter).pChannel->decRef();

			if(_pHandler)
				_pHandler->onRemoveComponent(componentInfos);

			iter = components.erase(iter);
			if(!ignoreComponentID)
				return;
		}
		else
			iter++;
	}

	if(shouldShowLog)
	{
		ERROR_MSG(fmt::format("Components::delComponent::not found [{}] component:totalcount:{}\n", 
			COMPONENT_NAME_EX(componentType), components.size()));
	}
}

//-------------------------------------------------------------------------------------		
void Components::removeComponentByChannel(Network::Channel * pChannel, bool isShutingdown)
{
	int ifind = 0;
	while(ALL_COMPONENT_TYPES[ifind] != UNKNOWN_COMPONENT_TYPE)
	{
		COMPONENT_TYPE componentType = ALL_COMPONENT_TYPES[ifind++];
		COMPONENTS& components = getComponents(componentType);
		COMPONENTS::iterator iter = components.begin();

		for(; iter != components.end();)
		{
			if((*iter).pChannel == pChannel)
			{
				//SAFE_RELEASE((*iter).pIntAddr);
				//SAFE_RELEASE((*iter).pExtAddr);
				// (*iter).pChannel->decRef();

				if(!isShutingdown)
				{
					ERROR_MSG(fmt::format("Components::removeComponentByChannel: {} : {}, Abnormal exit.\n",
						COMPONENT_NAME_EX(componentType), (*iter).cid));

#if KBE_PLATFORM == PLATFORM_WIN32
					printf("[ERROR]: %s.\n", (fmt::format("Components::removeComponentByChannel: {} : {}, Abnormal exit!\n",
						COMPONENT_NAME_EX(componentType), (*iter).cid)).c_str());
#endif
				}
				else
				{
					INFO_MSG(fmt::format("Components::removeComponentByChannel: {} : {}, Normal exit!\n",
						COMPONENT_NAME_EX(componentType), (*iter).cid));
				}

				ComponentInfos* componentInfos = &(*iter);

				if(_pHandler)
					_pHandler->onRemoveComponent(componentInfos);

				iter = components.erase(iter);
				return;
			}
			else
				iter++;
		}
	}

	// KBE_ASSERT(false && "channel is not found!\n");
}

//-------------------------------------------------------------------------------------		
void Components::clear(int32 uid, bool shouldShowLog)
{
	delComponent(uid, DBMGR_TYPE, uid, true, shouldShowLog);
	delComponent(uid, BASEAPPMGR_TYPE, uid, true, shouldShowLog);
	delComponent(uid, CELLAPPMGR_TYPE, uid, true, shouldShowLog);
	delComponent(uid, CELLAPP_TYPE, uid, true, shouldShowLog);
	delComponent(uid, BASEAPP_TYPE, uid, true, shouldShowLog);
	delComponent(uid, LOGINAPP_TYPE, uid, true, shouldShowLog);
	//delComponent(uid, LOGGER_TYPE, uid, true, shouldShowLog);
}

//-------------------------------------------------------------------------------------		
Components::COMPONENTS& Components::getComponents(COMPONENT_TYPE componentType)
{
	switch(componentType)
	{
	case DBMGR_TYPE:
		return _dbmgrs;
	case LOGINAPP_TYPE:
		return _loginapps;
	case BASEAPPMGR_TYPE:
		return _baseappmgrs;
	case CELLAPPMGR_TYPE:
		return _cellappmgrs;
	case CELLAPP_TYPE:
		return _cellapps;
	case BASEAPP_TYPE:
		return _baseapps;
	case LOGGER_TYPE:
		return _loggers;			
	default:
		break;
	};

	return _loggers;
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::findComponent(COMPONENT_TYPE componentType, int32 uid,
																			COMPONENT_ID componentID)
{
	/*COMPONENTS& components = getComponents(componentType);
	COMPONENTS::iterator iter = components.begin();
	for(; iter != components.end(); ++iter)
	{
		if((*iter).uid == uid && (componentID == 0 || (*iter).cid == componentID))
			return &(*iter);
	}

	return NULL;*/
	return findComponent(componentType, componentID);
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::findComponent(COMPONENT_TYPE componentType, COMPONENT_ID componentID)
{
	COMPONENTS& components = getComponents(componentType);
	COMPONENTS::iterator iter = components.begin();
	for(; iter != components.end(); ++iter)
	{
		if(componentID == 0 || (*iter).cid == componentID)
			return &(*iter);
	}

	return NULL;
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::findComponent(COMPONENT_ID componentID)
{
	int idx = 0;
	int32 uid = getUserUID();

	while(true)
	{
		COMPONENT_TYPE ct = ALL_COMPONENT_TYPES[idx++];
		if(ct == UNKNOWN_COMPONENT_TYPE)
			break;

		ComponentInfos* cinfos = findComponent(ct, uid, componentID);
		if(cinfos != NULL)
		{
			return cinfos;
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::findComponent(Network::Channel * pChannel)
{
	int ifind = 0;

	while(ALL_COMPONENT_TYPES[ifind] != UNKNOWN_COMPONENT_TYPE)
	{
		COMPONENT_TYPE componentType = ALL_COMPONENT_TYPES[ifind++];
		COMPONENTS& components = getComponents(componentType);
		COMPONENTS::iterator iter = components.begin();

		for(; iter != components.end(); ++iter)
		{
			if((*iter).pChannel == pChannel)
			{
				return &(*iter);
			}
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::findComponent(Network::Address* pAddress)
{
	int ifind = 0;

	while(ALL_COMPONENT_TYPES[ifind] != UNKNOWN_COMPONENT_TYPE)
	{
		COMPONENT_TYPE componentType = ALL_COMPONENT_TYPES[ifind++];
		COMPONENTS& components = getComponents(componentType);
		COMPONENTS::iterator iter = components.begin();

		for(; iter != components.end(); ++iter)
		{
			if((*iter).pChannel && (*iter).pChannel->addr() == *pAddress)
			{
				return &(*iter);
			}
		}
	}

	return NULL;
}


//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::getBaseappmgr()
{
	return findComponent(BASEAPPMGR_TYPE, getUserUID(), 0);
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::getCellappmgr()
{
	return findComponent(CELLAPPMGR_TYPE, getUserUID(), 0);
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::getDbmgr()
{
	return findComponent(DBMGR_TYPE, getUserUID(), 0);
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::getLogger()
{
	return findComponent(LOGGER_TYPE, getUserUID(), 0);
}

//-------------------------------------------------------------------------------------		
Components::ComponentInfos* Components::getInterfaceses()
{
	return findComponent(INTERFACES_TYPE, getUserUID(), 0);
}

//-------------------------------------------------------------------------------------		
Network::Channel* Components::getBaseappmgrChannel()
{
	Components::ComponentInfos* cinfo = getBaseappmgr();
	if(cinfo == NULL)
		 return NULL;

	return cinfo->pChannel;
}

//-------------------------------------------------------------------------------------		
Network::Channel* Components::getCellappmgrChannel()
{
	Components::ComponentInfos* cinfo = getCellappmgr();
	if(cinfo == NULL)
		 return NULL;

	return cinfo->pChannel;
}

//-------------------------------------------------------------------------------------		
Network::Channel* Components::getDbmgrChannel()
{
	Components::ComponentInfos* cinfo = getDbmgr();
	if(cinfo == NULL)
		 return NULL;

	return cinfo->pChannel;
}

//-------------------------------------------------------------------------------------		
Network::Channel* Components::getLoggerChannel()
{
	Components::ComponentInfos* cinfo = getLogger();
	if(cinfo == NULL)
		 return NULL;

	return cinfo->pChannel;
}

//-------------------------------------------------------------------------------------	
size_t Components::getGameSrvComponentsSize()
{
	COMPONENTS	_baseapps;
	COMPONENTS	_cellapps;
	COMPONENTS	_dbmgrs;
	COMPONENTS	_loginapps;
	COMPONENTS	_cellappmgrs;
	COMPONENTS	_baseappmgrs;

	return _baseapps.size() + _cellapps.size() + _dbmgrs.size() + 
		_loginapps.size() + _cellappmgrs.size() + _baseappmgrs.size();
}

//-------------------------------------------------------------------------------------
Network::EventDispatcher & Components::dispatcher()
{
	return pNetworkInterface()->dispatcher();
}

//-------------------------------------------------------------------------------------
void Components::onChannelDeregister(Network::Channel * pChannel, bool isShutingdown)
{
	removeComponentByChannel(pChannel, isShutingdown);
}

//-------------------------------------------------------------------------------------
bool Components::findLogger()
{
	if(g_componentType == LOGGER_TYPE || g_componentType == MACHINE_TYPE)
	{
		return true;
	}
	
	int i = 0;
	

	return false;
}

//-------------------------------------------------------------------------------------
bool Components::findComponents()
{
	while (findComponentTypes_[findIdx_] != UNKNOWN_COMPONENT_TYPE)
	{
		if (dispatcher().hasBreakProcessing() || dispatcher().waitingBreakProcessing())
			return false;

		COMPONENT_TYPE findComponentType = (COMPONENT_TYPE)findComponentTypes_[findIdx_];

		INFO_MSG(fmt::format("Components::findComponents: register self to {}...\n",
			COMPONENT_NAME_EX((COMPONENT_TYPE)findComponentType)));

		if (connectComponent(static_cast<COMPONENT_TYPE>(findComponentType), getUserUID(), 0) != 0)
		{
			ERROR_MSG(fmt::format("Components::findComponents: register self to {} is error!\n",
				COMPONENT_NAME_EX((COMPONENT_TYPE)findComponentType)));
			//dispatcher().breakProcessing();
			return false;
		}

		findIdx_++;
		return false;
	}
	state_ = 2;
	return true;
}

//-------------------------------------------------------------------------------------		
int Components::connectComponent(COMPONENT_TYPE rcomponentType, COMPONENT_ID rcomponentID, uint32 intaddr, uint16 intport)
{
	Network::EndPoint * pEndpoint = Network::EndPoint::ObjPool().createObject();
	pEndpoint->socket(SOCK_STREAM);
	if (!pEndpoint->good())
	{
		ERROR_MSG("Components::connectComponent: couldn't create a socket\n");
		Network::EndPoint::ObjPool().reclaimObject(pEndpoint);
		return -1;
	}
	Network::Address addr(intaddr, intport);
	pEndpoint->addr(addr);
	int ret = pEndpoint->connect(addr.port, addr.ip);

	if (ret == 0)
	{
		Network::Channel* pChannel = Network::Channel::ObjPool().createObject();
		bool ret = pChannel->initialize(*_pNetworkInterface, pEndpoint, Network::Channel::INTERNAL);
		if (!ret)
		{
			ERROR_MSG(fmt::format("Components::connectComponent: initialize({}) is failed!\n",
				pChannel->c_str()));

			pChannel->destroy();
			Network::Channel::ObjPool().reclaimObject(pChannel);
			return -1;
		}


		if (!_pNetworkInterface->registerChannel(pChannel))
		{
			ERROR_MSG(fmt::format("Components::connectComponent: registerChannel({}) is failed!\n",
				pChannel->c_str()));
			pChannel->destroy();
			Network::Channel::ObjPool().reclaimObject(pChannel);

			// 此时不可强制释放内存，reclaimObject中已经对其减引用
			// SAFE_RELEASE(pComponentInfos->pChannel);
			pChannel = NULL;
			return -1;
		}
		else
		{
			Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
			{
				DEBUG_MSG(fmt::format("[NetProtoStreamHandlerHandler]: register, compentType:{0}, name:{1}, \
				compentId:{2} \n", rcomponentType, COMPONENT_NAME_EX(rcomponentType), rcomponentID));

				COMMON_NETWORK_MESSAGE(rcomponentType, (*pBundle), OnRegisterServer);

				servercommon::RegisterSelf regCmd;
				regCmd.set_componentid(this->componentID());
				regCmd.set_componenttype(this->componentType());
				regCmd.set_uid(getUserUID());
				regCmd.set_username(COMPONENT_NAME_EX(this->componentType()));
				regCmd.set_intaddr(_pNetworkInterface->intaddr().ip);
				regCmd.set_intport(_pNetworkInterface->intaddr().port);
				regCmd.set_extaddr(_pNetworkInterface->extaddr().ip);
				regCmd.set_extport(_pNetworkInterface->extaddr().port);

				ADDTOBUNDLE((*pBundle), regCmd)

					WARNING_MSG(fmt::format("Bundle::staticAddToBundle: cmd({},{}) bundle length:{}!\n",
						pBundle->messageID() >> 8, (uint32)((uint8)pBundle->messageID()), pBundle->currMsgLength()));
			}

			pChannel->send(pBundle);
		}
	}
	else
	{
		ERROR_MSG(fmt::format("Components::connectComponent: connect({}) is failed! {}.\n",
			addr.c_str(), kbe_strerror()));

		Network::EndPoint::ObjPool().reclaimObject(pEndpoint);
		return -1;
	}

	return ret;
}

//-------------------------------------------------------------------------------------		
int Components::connectComponent(COMPONENT_TYPE rcomponentType, int32 ruid, COMPONENT_ID rcomponentID)
{
	ENGINE_COMPONENT_INFO& serverConfig = g_kbeSrvConfig.getComponent(rcomponentType);
	Network::Address addr(serverConfig.ip, serverConfig.port);
	return connectComponent(rcomponentType, rcomponentID, addr.ip, addr.port);
}


//-------------------------------------------------------------------------------------
void Components::onFoundAllComponents()
{
	INFO_MSG("Components::process(): Found all the components!\n");

#if KBE_PLATFORM == PLATFORM_WIN32
		DebugHelper::getSingleton().set_normalcolor();
		printf("[INFO]: Found all the components!\n");
		DebugHelper::getSingleton().set_normalcolor();
#endif
}

void Components::OnRegisterServer(Network::Channel* pChannel, MemoryStream& s)
{
	servercommon::RegisterSelf regCmd;
	PARSEBUNDLE(s, regCmd)
		COMPONENT_TYPE componentType = (COMPONENT_TYPE)regCmd.componenttype();
	uint32 uid = regCmd.uid();
	COMPONENT_ID componentID = regCmd.componentid();
	uint32 extaddr = 0;

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	COMMON_NETWORK_MESSAGE(componentType, (*pBundle), CBRegisterServer);

	servercommon::CBRegisterSelf cbregCmd;

	if (!this->checkComponents(uid, componentID, componentType))
	{
		ERROR_MSG(fmt::format("checkComponents: The current process and {}(componentID={} uid={} conflict, the process will exit!\n"
			"Can modify the components-CID and UID to avoid conflict 22.\n",
			COMPONENT_NAME_EX((COMPONENT_TYPE)componentType), componentID, uid));
		cbregCmd.set_result(2);
	}
	else
	{
		ComponentInfos* cinfos = this->findComponent((
			KBEngine::COMPONENT_TYPE)componentType, uid, componentID);
		pChannel->componentID(componentID);
		if (cinfos == NULL)
		{
			Components::getSingleton().addComponent(uid, regCmd.username().c_str(),
				(KBEngine::COMPONENT_TYPE)componentType, componentID, regCmd.intaddr(), regCmd.intport(),
				regCmd.has_extaddr() ? regCmd.extaddr() : 0, regCmd.has_extport() ? regCmd.extport() : 0, pChannel);
			cbregCmd.set_result(0);
			cbregCmd.set_uid(getUserUID());
			cbregCmd.set_componentid(this->componentID());
			cbregCmd.set_componenttype(this->componentType());
			cbregCmd.set_username(COMPONENT_NAME_EX(this->componentType()));
			cbregCmd.set_intaddr(pChannel->addr().ip);
			cbregCmd.set_intport(pChannel->addr().port);
		}
		else //如果已经存在了
		{
			ERROR_MSG(fmt::format("ServerApp::onRegisterNewApp ERROR : componentName:{0}, "
				"componentID:{1}, intaddr:{2}, intport:{3} is exist.\n",
				COMPONENT_NAME_EX((COMPONENT_TYPE)componentType), componentID, inet_ntoa((struct in_addr&)pChannel->addr().ip),
				ntohs(pChannel->addr().port)));

			cbregCmd.set_result(1);
		}
	}

	ADDTOBUNDLE((*pBundle), cbregCmd)
		pChannel->send(pBundle);
}

void Components::CBRegisterServer(Network::Channel* pChannel, MemoryStream& s)
{
	servercommon::CBRegisterSelf cbregCmd;
	PARSEBUNDLE(s, cbregCmd)
		uint32 result = cbregCmd.result();
	if (result > 0) // 大于0为失败码
	{
		//失败了
		// 存在相同身份， 程序该退出了
		if (_pHandler)
			_pHandler->onIdentityillegal((COMPONENT_TYPE)this->componentType(), this->componentID(), getUserUID(), inet_ntoa((struct in_addr&)pChannel->addr().ip));
	}
	else //ok
	{
		uint32 componentType = cbregCmd.componenttype();
		uint32 uid = cbregCmd.uid();
		COMPONENT_ID componentID = cbregCmd.componentid();
		ComponentInfos* cinfos = this->findComponent((
			KBEngine::COMPONENT_TYPE)componentType, uid, componentID);
		pChannel->componentID(componentID);

		if (NULL == cinfos)
		{
			Components::getSingleton().addComponent(uid, cbregCmd.username().c_str(),
				(KBEngine::COMPONENT_TYPE)componentType, componentID, pChannel->addr().ip, pChannel->addr().port,
				0, 0, pChannel);
			needCompnentNum -= 1;
		}
	}
}


//-------------------------------------------------------------------------------------
bool Components::process()
{
	if (componentType_ == LOGGER_TYPE)
	{
		onFoundAllComponents();
		return false;
	}
	state_ = 1; //直接进入连接依赖服务器组
				
	if (1 == state_) //连接阶段
	{
		static uint64 lastTime = timestamp();

		if (timestamp() - lastTime > uint64(stampsPerSecond()))
		{
			if (!findComponents())
			{
				if (state_ != 2)
					lastTime = timestamp();

				return true;
			}
		}
		else
			return true;
	}
	else if (2 == state_)
	{
		if (needCompnentNum > 0)
		{
			return true;
		}
	}

	onFoundAllComponents();
	return false;
}

//-------------------------------------------------------------------------------------		
	
}
