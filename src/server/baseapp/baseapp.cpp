
#include "baseapp.h"
#include "baseapp_interface.h"
#include "initprogress_handler.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include <sstream>

#include "proto/bmb.pb.h"
#include "../../server/basemgr/basemgr_interface.h"
#include "proto/basedb.pb.h"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(BaseApp);

//-------------------------------------------------------------------------------------
BaseApp::BaseApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	pendingLoginMgr_(),
	idClient_(),
	pInitProgressHandler_(NULL)
{
	idClient_.pApp(this);
}

//-------------------------------------------------------------------------------------
BaseApp::~BaseApp()
{
	// 不需要主动释放
	pInitProgressHandler_ = NULL;
}

//-------------------------------------------------------------------------------------		
bool BaseApp::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseApp::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void BaseApp::handleTimeout(TimerHandle handle, void * arg)
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
void BaseApp::handleTick()
{

	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&BaseappInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
bool BaseApp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool BaseApp::initializeEnd()
{
	loopCheckTimerHandle_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));
	return true;
}

//-------------------------------------------------------------------------------------
void BaseApp::finalise()
{

	loopCheckTimerHandle_.cancel();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool BaseApp::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("BaseApp::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

////-------------------------------------------------------------------------------------
//void BaseApp::handleCheckStatusTick()
//{
//	pendingLoginMgr_.process();
//}

void BaseApp::registerPendingLogin(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
	{
		s.done();
		return;
	}

	std::string									loginName;
	std::string									accountName;
	std::string									password;
	std::string									datas;
	ENTITY_ID									entityID;
	DBID										entityDBID;
	uint32										flags;

	basemgr_base::registerPendingLogin rplCmd;
	PARSEBUNDLE(s, rplCmd)
	loginName = rplCmd.loginname();
	accountName = rplCmd.accountname();
	password = rplCmd.password();
	datas = rplCmd.extradata();
	entityID = rplCmd.eid();
	entityDBID = rplCmd.dbid();
	flags = rplCmd.flags();

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(BaseappmgrInterface::onPendingAccountGetBaseappAddr);

	basemgr_base::PendingAccountGetBaseappAddr pagbaCmd;
	pagbaCmd.set_loginname(loginName);
	pagbaCmd.set_accountname(accountName);
	pagbaCmd.set_ip(inet_ntoa((struct in_addr&)networkInterface().extaddr().ip));
	pagbaCmd.set_port(this->networkInterface().extaddr().port);

	ADDTOBUNDLE((*pBundle), pagbaCmd)
	pChannel->send(pBundle);

	PendingLoginMgr::PLInfos* ptinfos = new PendingLoginMgr::PLInfos;
	ptinfos->accountName = accountName;
	ptinfos->password = password;
	ptinfos->entityID = entityID;
	ptinfos->entityDBID = entityDBID;
	ptinfos->flags = flags;
	ptinfos->datas = datas;
	pendingLoginMgr_.add(ptinfos);
}


void BaseApp::onDbmgrInitCompleted(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	ERROR_MSG(fmt::format("CellApp::onDbmgrInitCompleted\n"));
	base_dbmgr::DbmgrInitCompleted dicCmd;
	PARSEBUNDLE(s, dicCmd);

	idClient_.onAddRange(dicCmd.startentityid(), dicCmd.endentityid());
	g_kbetime = dicCmd.g_kbetime();

	//PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
	//	const_cast<char*>("onInit"),
	//	const_cast<char*>("i"),
	//	0);

	pInitProgressHandler_ = new InitProgressHandler(this->networkInterface());
}

void BaseApp::onGetEntityAppFromDbmgr(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	base_dbmgr::GetEntityAppFromDbmgr geafCmd;
	PARSEBUNDLE(s, geafCmd);

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent((
		KBEngine::COMPONENT_TYPE)geafCmd.componenttype(), geafCmd.componentid());

	ERROR_MSG(fmt::format("BaseApp::onGetEntityAppFromDbmgr: app(uid:{0}, username:{1}, componentType:{2}, "
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
			ERROR_MSG(fmt::format("Baseapp::onGetEntityAppFromDbmgr: Illegal app(uid:{0}, username:{1}, componentType:{2}, "
				"componentID:{3}\n",
				geafCmd.uid(),
				geafCmd.username(),
				COMPONENT_NAME_EX((COMPONENT_TYPE)geafCmd.componenttype()),
				geafCmd.componentid()
				 ));

			/*Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
			(*pBundle).newMessage(DbmgrInterface::reqKillServer);
			(*pBundle) << g_componentID << g_componentType << KBEngine::getUsername() << KBEngine::getUserUID() << "Duplicate app-id.";
			pChannel->send(pBundle);*/
		}
	}

	//ServerApp::onRegisterNewApp(pChannel, uid, username, componentType, componentID, globalorderID, grouporderID,
	//	intaddr, intport, extaddr, extport, extaddrEx);
	Components::getSingleton().connectComponent((COMPONENT_TYPE)geafCmd.componenttype(),
		geafCmd.componentid(), geafCmd.intaddr(), geafCmd.intport());

	KBE_ASSERT(Components::getSingleton().getDbmgr() != NULL);
}
//-------------------------------------------------------------------------------------

}
