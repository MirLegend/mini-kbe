
#ifndef KBE_BASE_H
#define KBE_BASE_H
	
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

class InitProgressHandler;
class BaseApp:	public ServerApp, 
	public Singleton<BaseApp>
{
public:
	enum TimeOutType
	{
		TIMEOUT_TICK = TIMEOUT_SERVERAPP_MAX + 1
	};

	BaseApp(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~BaseApp();
	
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

	EntityIDClient& idClient() { return idClient_; }
	void registerPendingLogin(Network::Channel* pChannel, MemoryStream& s);

	/** 网络接口
	dbmgr发送初始信息
	startID: 初始分配ENTITY_ID 段起始位置
	endID: 初始分配ENTITY_ID 段结束位置
	startGlobalOrder: 全局启动顺序 包括各种不同组件
	startGroupOrder: 组内启动顺序， 比如在所有baseapp中第几个启动。
	machineGroupOrder: 在machine中真实的组顺序, 提供底层在某些时候判断是否为第一个baseapp时使用
	*/
	void onDbmgrInitCompleted(Network::Channel* pChannel, MemoryStream& s
		/*GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID*/);
	void onGetEntityAppFromDbmgr(Network::Channel* pChannel, MemoryStream& s);
	void onQueryPlayerCBFromDbmgr(Network::Channel* pChannel, MemoryStream& s);

	void onClientHello(Network::Channel* pChannel, MemoryStream& s); //客户端握手
	void loginBaseapp(Network::Channel* pChannel, MemoryStream& s);
	/**
	登录失败
	@failedcode: 失败返回码 NETWORK_ERR_SRV_NO_READY:服务器没有准备好,
	NETWORK_ERR_ILLEGAL_LOGIN:非法登录,
	NETWORK_ERR_NAME_PASSWORD:用户名或者密码不正确
	*/
	void loginBaseappFailed(Network::Channel* pChannel, std::string& accountName,
		SERVER_ERROR_CODE failedcode, bool relogin = false);

	virtual void onChannelDeregister(Network::Channel * pChannel);
protected:
	TimerHandle												loopCheckTimerHandle_;
	// 记录登录到服务器但还未处理完毕的账号
	PendingLoginMgr											pendingLoginMgr_;
	EntityIDClient											idClient_;

	InitProgressHandler*									pInitProgressHandler_;
};

}

#endif // KBE_BASE_H
