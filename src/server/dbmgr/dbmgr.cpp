#include "dbmgr.h"
#include "dbmgr_interface.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include <sstream>

#include "proto/ldb.pb.h"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(DBMgrApp);

//-------------------------------------------------------------------------------------
DBMgrApp::DBMgrApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	mainProcessTimer_()
{
}

//-------------------------------------------------------------------------------------
DBMgrApp::~DBMgrApp()
{
	loopCheckTimerHandle_.cancel();
	mainProcessTimer_.cancel();
	sleep(300);
}

//-------------------------------------------------------------------------------------		
bool DBMgrApp::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool DBMgrApp::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void DBMgrApp::handleTimeout(TimerHandle handle, void * arg)
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
void DBMgrApp::handleTick()
{

	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&DbmgrInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
bool DBMgrApp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool DBMgrApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool DBMgrApp::initializeEnd()
{
	// 添加一个timer， 每秒检查一些状态
	loopCheckTimerHandle_ = this->dispatcher().addTimer(1000000, this,
		reinterpret_cast<void *>(TIMEOUT_CHECK_STATUS));

	mainProcessTimer_ = this->dispatcher().addTimer(1000000 / 50, this,
		reinterpret_cast<void *>(TIMEOUT_TICK));

	return initInterfacesHandler() && initDB();
}

//-------------------------------------------------------------------------------------		
bool DBMgrApp::initDB()
{

	return true;
}

//-------------------------------------------------------------------------------------		
bool DBMgrApp::initInterfacesHandler()
{
	return true;
}

//-------------------------------------------------------------------------------------
void DBMgrApp::finalise()
{
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool DBMgrApp::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("DBMgrApp::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void DBMgrApp::onAccountLogin(Network::Channel* pChannel, MemoryStream& s)
{
	std::string loginName, password, datas;
	login_dbmgr::AccountLogin loginCmd;
	PARSEBUNDLE(s, loginCmd)

	loginName = loginCmd.accountname();
	password = loginCmd.password();
	datas = loginCmd.extradata();

	if (loginName.size() == 0)
	{
		ERROR_MSG("Dbmgr::onAccountLogin: loginName is empty.\n");
		return;
	}
}

}
