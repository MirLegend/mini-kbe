#include "dbmgr.h"
#include "interfaces_handler.h"
#include "db_interface/db_interface.h"
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
	mainProcessTimer_(),
	pInterfacesAccountHandler_(NULL)
{
}

//-------------------------------------------------------------------------------------
DBMgrApp::~DBMgrApp()
{
	loopCheckTimerHandle_.cancel();
	mainProcessTimer_.cancel();
	sleep(300);

	SAFE_RELEASE(pInterfacesAccountHandler_);
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
	DBUtil::pThreadPool()->onMainThreadTick();
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

	// 添加globalData, baseAppData, cellAppData支持
	/*pGlobalData_ = new GlobalDataServer(GlobalDataServer::GLOBAL_DATA);
	pBaseAppData_ = new GlobalDataServer(GlobalDataServer::BASEAPP_DATA);
	pCellAppData_ = new GlobalDataServer(GlobalDataServer::CELLAPP_DATA);
	pGlobalData_->addConcernComponentType(CELLAPP_TYPE);
	pGlobalData_->addConcernComponentType(BASEAPP_TYPE);
	pBaseAppData_->addConcernComponentType(BASEAPP_TYPE);
	pCellAppData_->addConcernComponentType(CELLAPP_TYPE);*/

	return initInterfacesHandler() && initDB();
}

//-------------------------------------------------------------------------------------		
bool DBMgrApp::initDB()
{
	if (!DBUtil::initialize())
	{
		ERROR_MSG("Dbmgr::initDB(): can't initialize dbinterface!\n");
		return false;
	}

	DBInterface* pDBInterface = DBUtil::createInterface();
	if (pDBInterface == NULL)
	{
		ERROR_MSG("Dbmgr::initDB(): can't create dbinterface!\n");
		return false;
	}

	bool ret = DBUtil::initInterface(pDBInterface);
	pDBInterface->detach();
	SAFE_RELEASE(pDBInterface);

	if (!ret)
		return false;

	return ret;
}

//-------------------------------------------------------------------------------------		
bool DBMgrApp::initInterfacesHandler()
{
	pInterfacesAccountHandler_ = InterfacesHandlerFactory::create("normal", *static_cast<KBEngine::DBThreadPool*>(DBUtil::pThreadPool()));

	return pInterfacesAccountHandler_->initialize();
}

//-------------------------------------------------------------------------------------
void DBMgrApp::finalise()
{
	DBUtil::finalise();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool DBMgrApp::canShutdown()
{
	Components::COMPONENTS& cellapp_components = Components::getSingleton().getComponents(CELLAPP_TYPE);
	if (cellapp_components.size() > 0)
	{
		std::string s;
		for (size_t i = 0; i < cellapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", cellapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Dbmgr::canShutdown(): Waiting for cellapp[{}] destruction!\n",
			s));

		return false;
	}

	Components::COMPONENTS& baseapp_components = Components::getSingleton().getComponents(BASEAPP_TYPE);
	if (baseapp_components.size() > 0)
	{
		std::string s;
		for (size_t i = 0; i < baseapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", baseapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Dbmgr::canShutdown(): Waiting for baseapp[{}] destruction!\n",
			s));

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

	pInterfacesAccountHandler_->loginAccount(pChannel, loginName, password, datas);
}

}
