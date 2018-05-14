
#include "cellapp.h"
#include "cellapp_interface.h"
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
KBE_SINGLETON_INIT(CellApp);

//-------------------------------------------------------------------------------------
CellApp::CellApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
timer_()
{
}

//-------------------------------------------------------------------------------------
CellApp::~CellApp()
{
}

//-------------------------------------------------------------------------------------		
bool CellApp::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool CellApp::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void CellApp::handleTimeout(TimerHandle handle, void * arg)
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
void CellApp::handleTick()
{

	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&CellappInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
bool CellApp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool CellApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool CellApp::initializeEnd()
{
	timer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));
	return true;
}

//-------------------------------------------------------------------------------------
void CellApp::finalise()
{

	timer_.cancel();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool CellApp::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("CellApp::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------

}
