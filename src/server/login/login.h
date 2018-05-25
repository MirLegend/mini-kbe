
#ifndef KBE_LOGINE_H
#define KBE_LOGINE_H
	
// common include	
#include "server/kbemain.h"
#include "server/serverapp.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
#include "server/pendingLoginmgr.h"
#include "common/timer.h"
#include "network/endpoint.h"
#include "network/udp_packet_receiver.h"
#include "network/common.h"
#include "network/address.h"
//#include "logwatcher.h"

//#define NDEBUG
#include <map>	
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{


class LoginApp:	public ServerApp, 
	public Singleton<LoginApp>
{
public:
	//typedef std::map<Network::Address, LogWatcher> LOG_WATCHERS;
public:
	enum TimeOutType
	{
		TIMEOUT_TICK = TIMEOUT_SERVERAPP_MAX + 1
	};

	LoginApp(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~LoginApp();
	
	bool run();
	
	virtual bool initializeWatcher();

	void handleTimeout(TimerHandle handle, void * arg);
	void handleTick();

	/* 初始化相关接口 */
	bool initializeBegin();
	bool inInitialize();
	bool initializeEnd();
	void finalise();

	virtual bool canShutdown();

	void onClientHello(Network::Channel* pChannel, MemoryStream& s); //客户端握手

	void Login(Network::Channel* pChannel, MemoryStream& s); //客户端登陆
															 /*
															 登录失败
															 failedcode: 失败返回码 NETWORK_ERR_SRV_NO_READY:服务器没有准备好,
															 NETWORK_ERR_SRV_OVERLOAD:服务器负载过重,
															 NETWORK_ERR_NAME_PASSWORD:用户名或者密码不正确
															 */
	void _loginFailed(Network::Channel* pChannel, std::string& loginName, SERVER_ERROR_CODE failedcode, std::string& datas, bool force = false);

	void onLoginAccountQueryResultFromDbmgr(Network::Channel* pChannel, MemoryStream& s);
	void onLoginAccountQueryBaseappAddrFromBaseappmgr(Network::Channel* pChannel, MemoryStream& s);
	void onBaseappInitProgress(Network::Channel* pChannel, MemoryStream& s);
protected:
	TimerHandle	timer_;
	// 记录登录到服务器但还未处理完毕的账号
	PendingLoginMgr						pendingLoginMgr_;
	float								initProgress_;
};

}

#endif // KBE_LOGINE_H
