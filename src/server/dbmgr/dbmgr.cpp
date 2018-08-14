#include "dbmgr.h"
#include "dbTalbeModule.h"
#include "interfaces_handler.h"
#include "db_interface/db_interface.h"
#include "sync_app_datas_handler.h"
#include "dbmgr_interface.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include "resmgr/resmgr.h"
#include <sstream>

#include "proto/coms.pb.h"
#include "proto/ldb.pb.h"
#include "proto/basedb.pb.h"
#include "../../server/baseapp/baseapp_interface.h"
#include "proto/celldb.pb.h"
#include "../../server/cellapp/cellapp_interface.h"

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
	idServer_(1, 1024),
	pInterfacesAccountHandler_(NULL),
	pSyncAppDatasHandler_(NULL),
	bufferedDBTasks_(),
	numQueryEntity_(0)
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
	
	DBTalbeModule::LoadDBTableDefs();
	bool ret = DBUtil::initInterface(pDBInterface, DBTalbeModule::tabelDefs);
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

//服务器组件注册
void DBMgrApp::OnRegisterServer(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	MemoryStream tempS(s);
	ServerApp::OnRegisterServer(pChannel, s);
	servercommon::RegisterSelf regCmd;
	PARSEBUNDLE(tempS, regCmd);
	COMPONENT_TYPE componentType = (COMPONENT_TYPE)regCmd.componenttype();
	uint32 uid = regCmd.uid();
	COMPONENT_ID componentID = regCmd.componentid();
	uint32 extaddr = 0;
	KBEngine::COMPONENT_TYPE tcomponentType = (KBEngine::COMPONENT_TYPE)componentType;

	if (pSyncAppDatasHandler_ == NULL)
		pSyncAppDatasHandler_ = new SyncAppDatasHandler(this->networkInterface());

	// 下一步:
	// 如果是连接到dbmgr则需要等待接收app初始信息
	// 例如：初始会分配entityID段以及这个app启动的顺序信息（是否第一个baseapp启动）
	if (tcomponentType == BASEAPP_TYPE ||
		tcomponentType == CELLAPP_TYPE ||
		tcomponentType == LOGINAPP_TYPE)
	{
	
	}

	pSyncAppDatasHandler_->pushApp(componentID);
	DEBUG_MSG(fmt::format("dbmgr OnRegisterServer  type:{0}, name:{1}\n ",
		componentType, COMPONENT_NAME_EX(componentType)));
	// 如果是baseapp或者cellapp则将自己注册到所有其他baseapp和cellapp
	if (tcomponentType == BASEAPP_TYPE ||
		tcomponentType == CELLAPP_TYPE)
	{
		KBEngine::COMPONENT_TYPE broadcastCpTypes[2] = { BASEAPP_TYPE, CELLAPP_TYPE };
		for (int idx = 0; idx < 2; ++idx)
		{
			Components::COMPONENTS& cts = Components::getSingleton().getComponents(broadcastCpTypes[idx]);
			Components::COMPONENTS::iterator fiter = cts.begin();
			for (; fiter != cts.end(); ++fiter)
			{
				if ((*fiter).cid == componentID)
					continue;

				Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
				ENTITTAPP_COMMON_NETWORK_MESSAGE(broadcastCpTypes[idx], (*pBundle), onGetEntityAppFromDbmgr);

				if (broadcastCpTypes[idx] == BASEAPP_TYPE)
				{
					//BaseappInterface::onGetEntityAppFromDbmgrArgs11::staticAddToBundle((*pBundle),
					//	uid, username, componentType, componentID, startGlobalOrder, startGroupOrder,
					//	intaddr, intport, extaddr, extport, g_kbeSrvConfig.getConfig().externalAddress);
					base_dbmgr::GetEntityAppFromDbmgr geafCmd;
					geafCmd.set_componentid(regCmd.componentid());
					geafCmd.set_componenttype(regCmd.componenttype());
					geafCmd.set_uid(regCmd.uid());
					geafCmd.set_username(regCmd.username());
					geafCmd.set_intaddr(regCmd.intaddr());
					geafCmd.set_intport(regCmd.intport());
					geafCmd.set_extaddr(regCmd.extaddr());
					geafCmd.set_extport(regCmd.extport());
					DEBUG_MSG(fmt::format("onGetEntityAppFromDbmgr: baseapp  ip:{0}, "
						"port:{1} cmd:{2}/{3} \n",
						regCmd.intaddr(), regCmd.intport(), pBundle->messageID()>>8, (uint8)pBundle->messageID()));
					ADDTOBUNDLE((*pBundle), geafCmd);
				}
				else
				{
					cell_dbmgr::GetEntityAppFromDbmgr geafCmd;
					geafCmd.set_componentid(regCmd.componentid());
					geafCmd.set_componenttype(regCmd.componenttype());
					geafCmd.set_uid(regCmd.uid());
					geafCmd.set_username(regCmd.username());
					geafCmd.set_intaddr(regCmd.intaddr());
					geafCmd.set_intport(regCmd.intport());
					geafCmd.set_extaddr(regCmd.extaddr());
					geafCmd.set_extport(regCmd.extport());
					DEBUG_MSG(fmt::format("onGetEntityAppFromDbmgr: celleapp  ip:{0}, "
						"port:{1}  cmd:{2}/{3} \n",
						regCmd.intaddr(), regCmd.intport(), pBundle->messageID() >> 8, (uint8)pBundle->messageID()));
					ADDTOBUNDLE((*pBundle), geafCmd);
					/*CellappInterface::onGetEntityAppFromDbmgrArgs11::staticAddToBundle((*pBundle),
						uid, username, componentType, componentID, startGlobalOrder, startGroupOrder,
						intaddr, intport, extaddr, extport, g_kbeSrvConfig.getConfig().externalAddress);*/
				}

				KBE_ASSERT((*fiter).pChannel != NULL);
				(*fiter).pChannel->send(pBundle);
			}
		}
	}
}

void DBMgrApp::QueryAccount(Network::Channel* pChannel, MemoryStream& s)
{
	base_dbmgr::QueryAccount queryCmd;
	PARSEBUNDLE(s, queryCmd);
	std::string accountName = queryCmd.account();
	std::string password = queryCmd.password();
	COMPONENT_ID componentID = queryCmd.componentid();
	ENTITY_ID entityID = queryCmd.entityid();
	DBID entityDBID = queryCmd.entitydbid();
	uint32 ip = queryCmd.addrip();
	uint16 port = queryCmd.addrport();

	if (accountName.size() == 0)
	{
		ERROR_MSG("Dbmgr::queryAccount: accountName is empty.\n");
		return;
	}

	bufferedDBTasks_.addTask(new DBTaskQueryAccount(pChannel->addr(),
		accountName, password, componentID, entityID, entityDBID, ip, port));

	numQueryEntity_++;
}


//-------------------------------------------------------------------------------------
void DBMgrApp::removeEntity(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	base_dbmgr::RemoveEntity removeCmd;
	PARSEBUNDLE(s, removeCmd);
	COMPONENT_ID componentID = removeCmd.componentid();
	ENTITY_ID entityID = removeCmd.entityid();
	DBID entityDBID = removeCmd.entitydbid();

	bufferedDBTasks_.addTask(new DBTaskRemoveEntity(pChannel->addr(),
		componentID, entityID, entityDBID));
}


}
