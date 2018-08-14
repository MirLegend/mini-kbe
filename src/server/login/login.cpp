#include "login.h"
#include "login_interface.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include <sstream>

#include "proto/cl.pb.h"
#include "proto/ldb.pb.h"
#include "proto/lbm.pb.h"
#include "../../server/logger/logger_interface.h"
#include "../../server/dbmgr/dbmgr_interface.h"
#include "../../server/basemgr/basemgr_interface.h"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(LoginApp);

//-------------------------------------------------------------------------------------
LoginApp::LoginApp(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
	timer_(),
	pendingLoginMgr_(),
	initProgress_(0.f)
{
}

//-------------------------------------------------------------------------------------
LoginApp::~LoginApp()
{
}

//-------------------------------------------------------------------------------------		
bool LoginApp::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool LoginApp::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void LoginApp::handleTimeout(TimerHandle handle, void * arg)
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
void LoginApp::handleTick()
{
	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&LoginappInterface::messageHandlers);
	pendingLoginMgr_.process();
}

//-------------------------------------------------------------------------------------
bool LoginApp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool LoginApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool LoginApp::initializeEnd()
{
	timer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));
	return true;
}

//-------------------------------------------------------------------------------------
void LoginApp::finalise()
{
	timer_.cancel();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool LoginApp::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("LoginApp::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void LoginApp::onClientHello(Network::Channel* pChannel, MemoryStream& s)
{
	client_loginserver::Hello helloCmd;
	PARSEBUNDLE(s, helloCmd)
	uint32 clientVersion = helloCmd.version();
	const std::string& extradata = helloCmd.extradata();
	
	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	pBundle->newMessage(90, 2);
	
	client_loginserver::HelloCB helloCbCmd;
	helloCbCmd.set_result(1);
	helloCbCmd.set_version(68);
	helloCbCmd.set_extradata(extradata);

	ADDTOBUNDLE((*pBundle), helloCbCmd)
	pChannel->send(pBundle);

	//Network::Bundle* pBundle1 = Network::Bundle::ObjPool().createObject();
	//(*pBundle1).newMessage(90, 6);
	//client_loginserver::LoginFailed loginfailCmd;
	//loginfailCmd.set_failedcode(1);
	//loginfailCmd.set_extradata(extradata);

	//ADDTOBUNDLE((*pBundle1), loginfailCmd)
	//pChannel->send(pBundle1);
}

//-------------------------------------------------------------------------------------
void LoginApp::Login(Network::Channel* pChannel, MemoryStream& s)
{
	//AUTO_SCOPED_PROFILE("login");
	client_loginserver::Login loginCmd;
	PARSEBUNDLE(s, loginCmd)

	CLIENT_CTYPE ctype = loginCmd.has_ctype()?loginCmd.ctype(): UNKNOWN_CLIENT_COMPONENT_TYPE;
	std::string accountName = loginCmd.account();
	std::string password = loginCmd.password();
	std::string datas = loginCmd.extradata();
	accountName = strutil::kbe_trim(accountName);
	if (accountName.size() == 0)
	{
		INFO_MSG("Loginapp::login: accountName is NULL.\n");
		_loginFailed(pChannel, accountName, SERVER_ERR_NAME, datas, true);
		return;
	}

	if (accountName.size() > ACCOUNT_NAME_MAX_LENGTH)
	{
		INFO_MSG(fmt::format("Loginapp::login: accountName is too long, size={}, limit={}.\n",
			accountName.size(), ACCOUNT_NAME_MAX_LENGTH));

		_loginFailed(pChannel, accountName, SERVER_ERR_NAME, datas, true);
		return;
	}

	if (password.size() > ACCOUNT_PASSWD_MAX_LENGTH)
	{
		INFO_MSG(fmt::format("Loginapp::login: password is too long, size={}, limit={}.\n",
			password.size(), ACCOUNT_PASSWD_MAX_LENGTH));

		_loginFailed(pChannel, accountName, SERVER_ERR_PASSWORD, datas, true);
		return;
	}

	if (datas.size() > ACCOUNT_DATA_MAX_LENGTH)
	{
		INFO_MSG(fmt::format("Loginapp::login: bindatas is too long, size={}, limit={}.\n",
			datas.size(), ACCOUNT_DATA_MAX_LENGTH));

		_loginFailed(pChannel, accountName, SERVER_ERR_OP_FAILED, datas, true);
		return;
	}

	// 首先必须baseappmgr和dbmgr都已经准备完毕了。
	Components::ComponentInfos* baseappmgrinfos = Components::getSingleton().getBaseappmgr();
	if (baseappmgrinfos == NULL || baseappmgrinfos->pChannel == NULL || baseappmgrinfos->cid == 0)
	{
		datas = "";
		_loginFailed(pChannel, accountName, SERVER_ERR_SRV_NO_READY, datas, true);
		return;
	}

	Components::ComponentInfos* dbmgrinfos = Components::getSingleton().getDbmgr();
	if (dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		datas = "";
		_loginFailed(pChannel, accountName, SERVER_ERR_SRV_NO_READY, datas, true);
		return;
	}

	PendingLoginMgr::PLInfos* ptinfos = pendingLoginMgr_.find(accountName);
	if (ptinfos != NULL)
	{
		datas = "";
		_loginFailed(pChannel, accountName, SERVER_ERR_BUSY, datas, true);
		return;
	}

	ptinfos = new PendingLoginMgr::PLInfos;
	ptinfos->ctype = (COMPONENT_CLIENT_TYPE)ctype;
	ptinfos->datas = datas;
	ptinfos->accountName = accountName;
	ptinfos->password = password;
	ptinfos->addr = pChannel->addr();
	pendingLoginMgr_.add(ptinfos);

	if (ctype < UNKNOWN_CLIENT_COMPONENT_TYPE || ctype >= CLIENT_TYPE_END)
		ctype = UNKNOWN_CLIENT_COMPONENT_TYPE;

	if (shuttingdown_ != SHUTDOWN_STATE_STOP)
	{
		INFO_MSG(fmt::format("Loginapp::login: shutting down, {} login failed!\n", accountName));

		datas = "";
		_loginFailed(pChannel, accountName, SERVER_ERR_IN_SHUTTINGDOWN, datas);
		return;
	}

	if (initProgress_ < 1.f)
	{
		datas = fmt::format("initProgress: {}", initProgress_);
		_loginFailed(pChannel, accountName, SERVER_ERR_SRV_STARTING, datas);
		return;
	}

	INFO_MSG(fmt::format("Loginapp::login: new client[{0}], accountName={1}, datas={2}.\n",
		COMPONENT_CLIENT_NAME[ctype], accountName, datas));

	pChannel->extra(accountName);

	// 向dbmgr查询用户合法性
	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	pBundle->newMessage(DbmgrInterface::onAccountLogin);
	login_dbmgr::AccountLogin alCmd;
	alCmd.set_accountname(accountName);
	alCmd.set_password(password);
	alCmd.set_extradata(datas);
	ADDTOBUNDLE((*pBundle), alCmd)

	dbmgrinfos->pChannel->send(pBundle);
}

//-------------------------------------------------------------------------------------
void LoginApp::_loginFailed(Network::Channel* pChannel, std::string& loginName, SERVER_ERROR_CODE failedcode, std::string& datas, bool force)
{
	INFO_MSG(fmt::format("Loginapp::loginFailed: loginName={0} login is failed. failedcode={1}, datas={2}.\n",
		loginName, SERVER_ERR_STR[failedcode], datas));

	PendingLoginMgr::PLInfos* infos = NULL;

	if (!force)
	{
		infos = pendingLoginMgr_.remove(loginName);
		if (infos == NULL)
			return;
	}

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(90, 6);
	client_loginserver::LoginFailed loginfailCmd;
	loginfailCmd.set_failedcode(failedcode);
	loginfailCmd.set_extradata(datas);

	ADDTOBUNDLE((*pBundle), loginfailCmd)

	if (pChannel)
	{
		pChannel->send(pBundle);
	}
	else
	{
		if (infos)
		{
			Network::Channel* pClientChannel = this->networkInterface().findChannel(infos->addr);
			if (pClientChannel)
				pClientChannel->send(pBundle);
			else
				Network::Bundle::ObjPool().reclaimObject(pBundle);
		}
		else
		{
			ERROR_MSG(fmt::format("Loginapp::_loginFailed: infos({}) is NULL!\n",
				loginName));

			Network::Bundle::ObjPool().reclaimObject(pBundle);
		}
	}

	SAFE_RELEASE(infos);
}

//-------------------------------------------------------------------------------------
void LoginApp::onLoginAccountQueryResultFromDbmgr(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;

	std::string loginName, accountName, password, datas;
	SERVER_ERROR_CODE retcode = SERVER_SUCCESS;
	COMPONENT_ID componentID;
	ENTITY_ID entityID;
	DBID dbid;
	uint32 flags;

	// 登录名既登录时客户端输入的名称， 账号名则是dbmgr查询得到的名称
	// 这个机制用于一个账号多名称系统或者多个第三方账号系统登入服务器
	// accountName为本游戏服务器账号所绑定的终身名称
	// 客户端得到baseapp地址的同时也会返回这个账号名称
	// 客户端登陆baseapp应该使用这个账号名称登陆
	login_dbmgr::AccountLoginQueryResult loginQueryResultCmd;
	PARSEBUNDLE(s, loginQueryResultCmd)
	loginName = loginQueryResultCmd.loginname();
	accountName = loginQueryResultCmd.accountname();
	password = loginQueryResultCmd.password();
	datas = loginQueryResultCmd.getdatas();
	retcode = loginQueryResultCmd.retcode();
	componentID = loginQueryResultCmd.componentid();
	entityID = loginQueryResultCmd.entityid();
	dbid = loginQueryResultCmd.dbid();
	flags = loginQueryResultCmd.flags();

	PendingLoginMgr::PLInfos* infos = pendingLoginMgr_.find(loginName);
	if (infos == NULL)
	{
		_loginFailed(NULL, loginName, SERVER_ERR_SRV_OVERLOAD, datas);
		return;
	}

	infos->datas = datas;

	Network::Channel* pClientChannel = this->networkInterface().findChannel(infos->addr);
	if (pClientChannel)
		pClientChannel->extra("");

	if (retcode != SERVER_SUCCESS && entityID == 0 && componentID == 0)
	{
		_loginFailed(NULL, loginName, retcode, datas);
		return;
	}

	// 获得baseappmgr地址。
	Components::COMPONENTS& cts = Components::getSingleton().getComponents(BASEAPPMGR_TYPE);
	Components::ComponentInfos* baseappmgrinfos = NULL;
	if (cts.size() > 0)
		baseappmgrinfos = &(*cts.begin());

	if (baseappmgrinfos == NULL || baseappmgrinfos->pChannel == NULL || baseappmgrinfos->cid == 0)
	{
		_loginFailed(NULL, loginName, SERVER_ERR_SRV_NO_READY, datas);
		return;
	}

	// 如果大于0则说明当前账号仍然存活于某个baseapp上
	if (componentID > 0)
	{
		Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
		pBundle->newMessage(BaseappmgrInterface::onRegisterPendingAccountEx);
		login_basemgr::RegisterPendingAccountEx rpaeCmd;
		rpaeCmd.set_loginname(loginName);
		rpaeCmd.set_accountname(accountName);
		rpaeCmd.set_password(password);
		rpaeCmd.set_componentid(componentID);
		rpaeCmd.set_entityid(entityID);
		rpaeCmd.set_dbid(dbid);
		rpaeCmd.set_flags(flags);
		rpaeCmd.set_extradata(infos->datas);
		ADDTOBUNDLE((*pBundle), rpaeCmd)
		baseappmgrinfos->pChannel->send(pBundle);
		return;
	}
	else
	{
		// 注册到baseapp并且获取baseapp的地址
		Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
		pBundle->newMessage(BaseappmgrInterface::onRegisterPendingAccount);
		login_basemgr::RegisterPendingAccount rpaCmd;
		rpaCmd.set_loginname(loginName);
		rpaCmd.set_accountname(accountName);
		rpaCmd.set_password(password);
		rpaCmd.set_dbid(dbid);
		rpaCmd.set_flags(flags);
		rpaCmd.set_extradata(infos->datas);
		ADDTOBUNDLE((*pBundle), rpaCmd)
		baseappmgrinfos->pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void LoginApp::onLoginAccountQueryBaseappAddrFromBaseappmgr(Network::Channel* pChannel, MemoryStream& s)
{
	if (pChannel->isExternal())
		return;
	std::string loginName;
	std::string accountName;
	std::string addr;
	uint16 port;
	login_basemgr::LoginAccountQueryBaseappAddrFromBaseappmgr laqbCmd;
	PARSEBUNDLE(s, laqbCmd);
	loginName = laqbCmd.loginname();
	accountName = laqbCmd.accountname();
	addr = laqbCmd.ip();
	port = laqbCmd.port();

	if (addr.size() == 0)
	{
		ERROR_MSG(fmt::format("Loginapp::onLoginAccountQueryBaseappAddrFromBaseappmgr:accountName={}, not found baseapp.\n",
			loginName));

		std::string datas;
		_loginFailed(NULL, loginName, SERVER_ERR_SRV_NO_READY, datas);
	}

	Network::Address address(addr, ntohs(port));

	DEBUG_MSG(fmt::format("Loginapp::onLoginAccountQueryBaseappAddrFromBaseappmgr:accountName={0}, addr={1}.\n",
		loginName, address.c_str()));

	// 这里可以不做删除， 仍然使其保留一段时间避免同一时刻同时登录造成意外影响
	PendingLoginMgr::PLInfos* infos = pendingLoginMgr_.remove(loginName);
	if (infos == NULL)
		return;

	infos->lastProcessTime = timestamp();
	Network::Channel* pClientChannel = this->networkInterface().findChannel(infos->addr);

	if (pClientChannel == NULL)
	{
		SAFE_RELEASE(infos);
		return;
	}

	uint16 fport = ntohs(port);
	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	pBundle->newMessage(90, 7);

	client_loginserver::LoginSuccessfully loginCBCmd;
	loginCBCmd.set_accountname(accountName);
	loginCBCmd.set_baseip(addr);
	loginCBCmd.set_baseport(fport);
	loginCBCmd.set_extradata(infos->datas);

	ADDTOBUNDLE((*pBundle), loginCBCmd)
	pClientChannel->send(pBundle);

	SAFE_RELEASE(infos);
}

//-------------------------------------------------------------------------------------
void LoginApp::onBaseappInitProgress(Network::Channel* pChannel, MemoryStream& s)
{
	float progress;
	login_basemgr::BaseappInitProgress bipCmd;
	PARSEBUNDLE(s, bipCmd);
	progress = (float)bipCmd.baseappsinitprogress()/100.0f;
	if (progress > 1.f)
	{
		INFO_MSG(fmt::format("Loginapp::onBaseappInitProgress: progress={}.\n",
			(progress > 1.f ? 1.f : progress)));
	}

	initProgress_ = progress;
}

}
