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


#include "serverapp.h"
#include "server/component_active_report_handler.h"
#include "server/shutdowner.h"
#include "server/serverconfig.h"
#include "server/components.h"
#include "network/channel.h"
#include "network/bundle.h"
#include "network/common.h"
#include "common/memorystream.h"
#include "helper/console_helper.h"
#include "helper/sys_info.h"
#include "resmgr/resmgr.h"

#include "proto/coms.pb.h"

namespace KBEngine{
COMPONENT_TYPE g_componentType = UNKNOWN_COMPONENT_TYPE;
COMPONENT_ID g_componentID = 0;
COMPONENT_ORDER g_componentGlobalOrder = -1;
COMPONENT_ORDER g_componentGroupOrder = -1;
int32 g_genuuid_sections = -1;

GAME_TIME g_kbetime = 0;

//-------------------------------------------------------------------------------------
ServerApp::ServerApp(Network::EventDispatcher& dispatcher, 
					 Network::NetworkInterface& ninterface, 
					 COMPONENT_TYPE componentType,
					 COMPONENT_ID componentID):
SignalHandler(),
TimerHandler(),
ShutdownHandler(),
Network::ChannelTimeOutHandler(),
Components::ComponentsNotificationHandler(),
componentType_(componentType),
componentID_(componentID),
dispatcher_(dispatcher),
networkInterface_(ninterface),
timers_(),
startGlobalOrder_(-1),
startGroupOrder_(-1),
pShutdowner_(NULL),
pActiveTimerHandle_(NULL),
threadPool_()
{
	networkInterface_.pExtensionData(this);
	networkInterface_.pChannelTimeOutHandler(this);
	networkInterface_.pChannelDeregisterHandler(this);

	// 广播自己的地址给网上上的所有kbemachine
	// 并且从kbemachine获取basappmgr和cellappmgr以及dbmgr地址
	Components::getSingleton().pHandler(this);
	this->dispatcher().addTask(&Components::getSingleton());
	
	pActiveTimerHandle_ = new ComponentActiveReportHandler(this);
	pActiveTimerHandle_->startActiveTick(KBE_MAX(1.f, Network::g_channelInternalTimeout / 2.0f));
}

//-------------------------------------------------------------------------------------
ServerApp::~ServerApp()
{
	SAFE_RELEASE(pActiveTimerHandle_);
	SAFE_RELEASE(pShutdowner_);
}

//-------------------------------------------------------------------------------------	
void ServerApp::shutDown(float shutdowntime)
{
	if(pShutdowner_ == NULL)
		pShutdowner_ = new Shutdowner(this);

	pShutdowner_->shutdown(shutdowntime < 0.f ? 1/*g_kbeSrvConfig.shutdowntime()*/ : shutdowntime, 
		/*g_kbeSrvConfig.shutdownWaitTickTime()*/1, dispatcher_);
}

//-------------------------------------------------------------------------------------
void ServerApp::onShutdownBegin()
{
#if KBE_PLATFORM == PLATFORM_WIN32
	printf("[INFO]: shutdown begin.\n");
#endif

	dispatcher_.setWaitBreakProcessing();
}

//-------------------------------------------------------------------------------------
void ServerApp::onShutdown(bool first)
{
}

//-------------------------------------------------------------------------------------
void ServerApp::onShutdownEnd()
{
	dispatcher_.breakProcessing();
}

//-------------------------------------------------------------------------------------
bool ServerApp::loadConfig()
{
	return true;
}

//-------------------------------------------------------------------------------------		
bool ServerApp::installSingnals()
{
	g_kbeSignalHandlers.attachApp(this);
	g_kbeSignalHandlers.addSignal(SIGINT, this);
	g_kbeSignalHandlers.addSignal(SIGPIPE, this);
	g_kbeSignalHandlers.addSignal(SIGHUP, this);
	return true;
}

//-------------------------------------------------------------------------------------		
bool ServerApp::initialize()
{
	if(!initThreadPool())
		return false;

	if(!installSingnals())
		return false;
	
	if(!loadConfig())
		return false;
	
	if(!initializeBegin())
		return false;
	
	if(!inInitialize())
		return false;

	bool ret = initializeEnd();
	return ret;
}

//-------------------------------------------------------------------------------------		
bool ServerApp::initThreadPool()
{
	if(!threadPool_.isInitialize())
	{
		thread::ThreadPool::timeout = int(1/*g_kbeSrvConfig.thread_timeout_*/);
		threadPool_.createThreadPool(g_kbeSrvConfig.thread_init_create_, 
			g_kbeSrvConfig.thread_pre_create_, g_kbeSrvConfig.thread_max_create_);

		return true;
	}

	return false;
}

//-------------------------------------------------------------------------------------		
void ServerApp::finalise(void)
{
	threadPool_.finalise();
	Network::finalise();
}

//-------------------------------------------------------------------------------------		
double ServerApp::gameTimeInSeconds() const
{
	return double(g_kbetime) / g_kbeSrvConfig.gameUpdateHertz();
}

//-------------------------------------------------------------------------------------
void ServerApp::handleTimeout(TimerHandle, void * arg)
{
}

//-------------------------------------------------------------------------------------
void ServerApp::handleTimers()
{
	timers().process(g_kbetime);
}

//-------------------------------------------------------------------------------------		
bool ServerApp::run(void)
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------	
void ServerApp::onSignalled(int sigNum)
{
	switch (sigNum)
	{
	case SIGINT:
	case SIGHUP:
		this->shutDown(1.f);
	default:
		break;
	}
}

//-------------------------------------------------------------------------------------	
void ServerApp::onChannelDeregister(Network::Channel * pChannel)
{
	if(pChannel->isInternal())
	{
		Components::getSingleton().onChannelDeregister(pChannel, this->isShuttingdown());
	}
}

//-------------------------------------------------------------------------------------	
void ServerApp::onChannelTimeOut(Network::Channel * pChannel)
{
	INFO_MSG(fmt::format("ServerApp::onChannelTimeOut: "
		"Channel {0} timed out.\n", pChannel->c_str()));

	networkInterface_.deregisterChannel(pChannel);
	pChannel->destroy();
	Network::Channel::ObjPool().reclaimObject(pChannel);
}

//-------------------------------------------------------------------------------------
void ServerApp::onAddComponent(const Components::ComponentInfos* pInfos)
{
	if(pInfos->componentType == LOGGER_TYPE)
	{
		//DebugHelper::getSingleton().registerLogger(LoggerInterface::writeLog.msgID, pInfos->pIntAddr.get());
	}
}

//-------------------------------------------------------------------------------------
void ServerApp::onIdentityillegal(COMPONENT_TYPE componentType, COMPONENT_ID componentID, uint32 pid, const char* pAddr)
{
	ERROR_MSG(fmt::format("ServerApp::onIdentityillegal: The current process and {}(componentID={} ->conflicted???, pid={}, addr={}) conflict, the process will exit!\n"
			"Can modify the components-CID and UID to avoid conflict.\n",
		COMPONENT_NAME_EX((COMPONENT_TYPE)componentType), componentID, pid, pAddr));

	this->shutDown(1.f);
}

//-------------------------------------------------------------------------------------
void ServerApp::onRemoveComponent(const Components::ComponentInfos* pInfos)
{
	if(pInfos->componentType == LOGGER_TYPE)
	{
		//DebugHelper::getSingleton().unregisterLogger(LoggerInterface::writeLog.msgID, pInfos->pIntAddr.get());
	}
	else if(pInfos->componentType == DBMGR_TYPE)
	{
		if(g_componentType != MACHINE_TYPE && 
			g_componentType != LOGGER_TYPE && 
			g_componentType != INTERFACES_TYPE &&
			g_componentType != BOTS_TYPE &&
			g_componentType != WATCHER_TYPE)
			this->shutDown(0.f);
	}
}

//-------------------------------------------------------------------------------------
void ServerApp::onRegisterNewApp(Network::Channel* pChannel, int32 uid, std::string& username, 
						COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
						uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx)
{
	if (pChannel->isExternal())
		return;

	INFO_MSG(fmt::format("ServerApp::onRegisterNewApp: uid:{0}, username:{1}, componentType:{2}, "
		"componentID:{3}, intaddr:{4}, intport:{5}, extaddr:{6}, extport:{7},  from {8}.\n",
		uid,
		username.c_str(),
		COMPONENT_NAME_EX((COMPONENT_TYPE)componentType),
		componentID,
		inet_ntoa((struct in_addr&)intaddr),
		ntohs(intport),
		(extaddr != 0 ? inet_ntoa((struct in_addr&)extaddr) : "nonsupport"),
		ntohs(extport),
		pChannel->c_str()));

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent((
		KBEngine::COMPONENT_TYPE)componentType, uid, componentID);

	pChannel->componentID(componentID);
	if (cinfos == NULL)
	{
		Components::getSingleton().addComponent(uid, username.c_str(),
			(KBEngine::COMPONENT_TYPE)componentType, componentID, intaddr, intport, extaddr, extport,/* 0,
																										 0.f, 0.f, 0, 0, 0, 0, 0,*/ pChannel);
	}
	else
	{
		ERROR_MSG(fmt::format("ServerApp::onRegisterNewApp ERROR : componentName:{0}, "
			"componentID:{1}, intaddr:{2}, intport:{3} is exist.\n",
			COMPONENT_NAME_EX((COMPONENT_TYPE)componentType), componentID, inet_ntoa((struct in_addr&)intaddr),
			ntohs(intport)));
	}
}


//-------------------------------------------------------------------------------------
void ServerApp::onAppActiveTick(Network::Channel* pChannel, MemoryStream& s)
{
	COMPONENT_TYPE componentType;
	COMPONENT_ID componentID;
	servercommon::ActiveTick activeCmd;
	PARSEBUNDLE(s, activeCmd)

		componentType = (COMPONENT_TYPE)activeCmd.componenttype();
	componentID = (COMPONENT_ID)activeCmd.componentid();
	if (componentType != CLIENT_TYPE)
		if (pChannel->isExternal())
			return;

	Network::Channel* pTargetChannel = NULL;
	if (componentType != CONSOLE_TYPE && componentType != CLIENT_TYPE)
	{
		Components::ComponentInfos* cinfos =
			Components::getSingleton().findComponent(componentType, /*KBEngine::*/getUserUID(), componentID);

		if (cinfos == NULL || cinfos->pChannel == NULL)
		{
			ERROR_MSG(fmt::format("ServerApp::onAppActiveTick[{:p}]: {}:{} not found.\n",
				(void*)pChannel, COMPONENT_NAME_EX(componentType), componentID));

			return;
		}
		pTargetChannel = cinfos->pChannel;
		pTargetChannel->updateLastReceivedTime();
	}
	else
	{
		pChannel->updateLastReceivedTime();
		pTargetChannel = pChannel;
	}

}

//-------------------------------------------------------------------------------------
void ServerApp::hello(Network::Channel* pChannel, MemoryStream& s)
{
	
}

//服务器组件注册
void ServerApp::OnRegisterServer(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	Components::getSingleton().OnRegisterServer(pChannel, s);
}

//服务器组件注册返回
void ServerApp::CBRegisterServer(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	Components::getSingleton().CBRegisterServer(pChannel, s);
}


//-------------------------------------------------------------------------------------		
}
