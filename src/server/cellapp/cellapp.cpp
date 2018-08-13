
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

#include "../../server/basemgr/basemgr_interface.h"
#include "proto/celldb.pb.h"

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

void CellApp::onDbmgrInitCompleted(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	DEBUG_MSG(fmt::format("CellApp::onDbmgrInitCompleted\n"));
	s.done();
	//cell_dbmgr::DbmgrInitCompleted dicCmd;
	//PARSEBUNDLE(s, dicCmd);

	//idClient_.onAddRange(dicCmd.startentityid(), dicCmd.endentityid());
	//g_kbetime = dicCmd.g_kbetime();

	////PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
	////	const_cast<char*>("onInit"),
	////	const_cast<char*>("i"),
	////	0);

	//pInitProgressHandler_ = new InitProgressHandler(this->networkInterface());
}

void CellApp::onGetEntityAppFromDbmgr(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	cell_dbmgr::GetEntityAppFromDbmgr geafCmd;
	PARSEBUNDLE(s, geafCmd);

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent((
		KBEngine::COMPONENT_TYPE)geafCmd.componenttype(), geafCmd.componentid());

	DEBUG_MSG(fmt::format("CellApp::onGetEntityAppFromDbmgr: app(uid:{0}, username:{1}, componentType:{2}, "
		"componentID:{3}\n",
		geafCmd.uid(),
		geafCmd.username(),
		COMPONENT_NAME_EX((COMPONENT_TYPE)geafCmd.componenttype()),
		geafCmd.componentid()
		));
	if (cinfos)
	{
		if (cinfos->pIntAddr->ip != geafCmd.intaddr() || cinfos->pIntAddr->port != geafCmd.intport())
		{
			ERROR_MSG(fmt::format("CellApp::onGetEntityAppFromDbmgr: Illegal app(uid:{0}, username:{1}, componentType:{2}, "
				"componentID:{3}\n",
				geafCmd.uid(),
				geafCmd.username(),
				COMPONENT_NAME_EX((COMPONENT_TYPE)geafCmd.componenttype()),
				geafCmd.componentid()
				));
		}
	}
	Components::getSingleton().connectComponent((COMPONENT_TYPE)geafCmd.componenttype(),
		geafCmd.componentid(), geafCmd.intaddr(), geafCmd.intport());

	KBE_ASSERT(Components::getSingleton().getDbmgr() != NULL);
}

//-------------------------------------------------------------------------------------

}
