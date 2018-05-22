
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

#include "proto/lbm.pb.h"
#include "../../server/login/login_interface.h"
#include "proto/bmb.pb.h"
#include "../../server/baseapp/baseapp_interface.h"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(BaseMgrApp);

//-------------------------------------------------------------------------------------
BaseMgrApp::BaseMgrApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
gameTimer_(),
forward_baseapp_messagebuffer_(ninterface, BASEAPP_TYPE),
bestBaseappID_(0),
pending_logins_()
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

	gameTimer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));
	return true;
}

//-------------------------------------------------------------------------------------
void BaseMgrApp::finalise()
{

	gameTimer_.cancel();
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
//void BaseMgrApp::updateBestBaseapp()
//{
//	bestBaseappID_ = findFreeBaseapp();
//}

//-------------------------------------------------------------------------------------
void BaseMgrApp::onRegisterPendingAccount(Network::Channel* pChannel, MemoryStream& s)
{
	std::string loginName;
	std::string accountName;
	std::string password;
	std::string datas;
	DBID entityDBID;
	uint32 flags;

	login_basemgr::RegisterPendingAccount rpaCmd;
	PARSEBUNDLE(s, rpaCmd)
	loginName = rpaCmd.loginname();
	accountName = rpaCmd.accountname();
	password = rpaCmd.password();
	datas = rpaCmd.extradata();
	entityDBID = rpaCmd.dbid();
	flags = rpaCmd.flags();

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(pChannel);
	if (cinfos == NULL || cinfos->pChannel == NULL)
	{
		ERROR_MSG("Baseappmgr::registerPendingAccountToBaseapp: not found loginapp!\n");
		return;
	}

	pending_logins_[loginName] = cinfos->cid;

	ENTITY_ID eid = 0;
	cinfos = Components::getSingleton().findComponent(BASEAPP_TYPE, bestBaseappID_);

	if (cinfos == NULL || cinfos->pChannel == NULL || cinfos->state != COMPONENT_STATE_RUN)
	{
		Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
		ForwardItem* pFI = new ForwardItem();
		pFI->pBundle = pBundle;

		(*pBundle).newMessage(BaseappInterface::registerPendingLogin);
		basemgr_base::registerPendingLogin rplCmd;
		rplCmd.set_loginname(loginName);
		rplCmd.set_accountname(accountName);
		rplCmd.set_password(password);
		rplCmd.set_eid(eid);
		rplCmd.set_dbid(entityDBID);
		rplCmd.set_flags(flags);
		rplCmd.set_extradata(datas);
		ADDTOBUNDLE((*pBundle), rplCmd)

		WARNING_MSG("Baseappmgr::registerPendingAccountToBaseapp: not found baseapp, message is buffered.\n");
		pFI->pHandler = NULL;
		forward_baseapp_messagebuffer_.push(pFI);
		return;
	}


	DEBUG_MSG(fmt::format("Baseappmgr::registerPendingAccountToBaseapp:{0}. allocBaseapp=[{1}].\n",
		accountName, bestBaseappID_));

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(BaseappInterface::registerPendingLogin);
	basemgr_base::registerPendingLogin rplCmd;
	rplCmd.set_loginname(loginName);
	rplCmd.set_accountname(accountName);
	rplCmd.set_password(password);
	rplCmd.set_eid(eid);
	rplCmd.set_dbid(entityDBID);
	rplCmd.set_flags(flags);
	rplCmd.set_extradata(datas);
	ADDTOBUNDLE((*pBundle), rplCmd)
	cinfos->pChannel->send(pBundle);
}

void BaseMgrApp::onRegisterPendingAccountEx(Network::Channel* pChannel, MemoryStream& s)
{
	COMPONENT_ID componentID;
	std::string loginName;
	std::string accountName;
	std::string password;
	std::string datas;
	ENTITY_ID entityID;
	DBID entityDBID;
	uint32 flags;
	uint64 deadline;

	login_basemgr::RegisterPendingAccountEx rpaxCmd;
	PARSEBUNDLE(s, rpaxCmd)
	loginName = rpaxCmd.loginname();
	accountName = rpaxCmd.accountname();
	password = rpaxCmd.password();
	datas = rpaxCmd.extradata();
	entityDBID = rpaxCmd.dbid();
	flags = rpaxCmd.flags();
	componentID = rpaxCmd.componentid();
	entityID = rpaxCmd.entityid();

	DEBUG_MSG(fmt::format("Baseappmgr::registerPendingAccountToBaseappAddr:{0}, componentID={1}, entityID={2}.\n",
		accountName, componentID, entityID));

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(pChannel);
	if (cinfos == NULL || cinfos->pChannel == NULL)
	{
		ERROR_MSG("Baseappmgr::registerPendingAccountToBaseapp: not found loginapp!\n");
		return;
	}

	pending_logins_[loginName] = cinfos->cid;

	cinfos = Components::getSingleton().findComponent(componentID);
	if (cinfos == NULL || cinfos->pChannel == NULL)
	{
		ERROR_MSG(fmt::format("Baseappmgr::registerPendingAccountToBaseappAddr: not found baseapp({}).\n", componentID));
		sendAllocatedBaseappAddr(pChannel, loginName, accountName, "", 0);
		return;
	}

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();

	(*pBundle).newMessage(BaseappInterface::registerPendingLogin);
	basemgr_base::registerPendingLogin rplCmd;
	rplCmd.set_loginname(loginName);
	rplCmd.set_accountname(accountName);
	rplCmd.set_password(password);
	rplCmd.set_eid(entityID);
	rplCmd.set_dbid(entityDBID);
	rplCmd.set_flags(flags);
	rplCmd.set_extradata(datas);
	ADDTOBUNDLE((*pBundle), rplCmd)
	cinfos->pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void BaseMgrApp::sendAllocatedBaseappAddr(Network::Channel* pChannel,
	std::string& loginName, std::string& accountName, const std::string& addr, uint16 port)
{
	KBEUnordered_map< std::string, COMPONENT_ID >::iterator iter = pending_logins_.find(loginName);
	if (iter == pending_logins_.end())
	{
		ERROR_MSG("Baseappmgr::sendAllocatedBaseappAddr: not found loginapp, pending_logins is error!\n");
		return;
	}

	Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(iter->second);
	if (cinfos == NULL || cinfos->pChannel == NULL)
	{
		ERROR_MSG("Baseappmgr::sendAllocatedBaseappAddr: not found loginapp!\n");
		return;
	}

	Network::Bundle* pBundleToLoginapp = Network::Bundle::ObjPool().createObject();
	(*pBundleToLoginapp).newMessage(LoginappInterface::onLoginAccountQueryBaseappAddrFromBaseappmgr);

	login_basemgr::LoginAccountQueryBaseappAddrFromBaseappmgr laqbCmd;
	laqbCmd.set_accountname(accountName);
	laqbCmd.set_loginname(loginName);
	laqbCmd.set_ip(addr);
	laqbCmd.set_port(port);
	ADDTOBUNDLE((*pBundleToLoginapp), laqbCmd)
	cinfos->pChannel->send(pBundleToLoginapp);
	pending_logins_.erase(iter);
}

void BaseMgrApp::onPendingAccountGetBaseappAddr(Network::Channel* pChannel, MemoryStream& s)
{
	std::string loginName;
	std::string accountName;
	std::string addr;
	uint16 port;

	basemgr_base::PendingAccountGetBaseappAddr pagbaCmd;
	PARSEBUNDLE(s, pagbaCmd)
	loginName = pagbaCmd.loginname();
	accountName = pagbaCmd.accountname();
	addr = pagbaCmd.ip();
	port = pagbaCmd.port();

	sendAllocatedBaseappAddr(pChannel, loginName, accountName, addr, port);
}

//-------------------------------------------------------------------------------------

}
