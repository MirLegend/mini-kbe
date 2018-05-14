
#include "basemgr.h"
#include "basemgr_interface.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include <sstream>

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(BaseMgrApp);

//-------------------------------------------------------------------------------------
BaseMgrApp::BaseMgrApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
timer_()
{
}

//-------------------------------------------------------------------------------------
BaseMgrApp::~BaseMgrApp()
{
}

//-------------------------------------------------------------------------------------		
bool BaseMgrApp::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseMgrApp::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void BaseMgrApp::handleTimeout(TimerHandle handle, void * arg)
{
	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_TICK:
			this->handleTick();
			break;
		default:
			break;
	}

	ServerApp::handleTimeout(handle, arg);
}

//-------------------------------------------------------------------------------------
void BaseMgrApp::handleTick()
{

	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&BaseappmgrInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
bool BaseMgrApp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseMgrApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseMgrApp::initializeEnd()
{

	timer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));
	return true;
}

//-------------------------------------------------------------------------------------
void BaseMgrApp::finalise()
{

	timer_.cancel();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool BaseMgrApp::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("BaseMgrApp::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------

}
