
#ifndef KBE_BASEAPP_H
#define KBE_BASEAPP_H
	
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
#include "server/entity_app.h"
#include "base.h"
#include "proxy.h"
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
class BaseApp: public EntityApp<Base>,
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

	virtual Base* onCreateEntity(const char* entityType, ENTITY_ID eid);
	

	/**
	通知客户端创建一个proxy对应的实体
	*/
	bool createClientProxies(Proxy* base, bool reload = false);

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

	/**
	增加proxices计数
	*/
	void incProxicesCount() { ++numProxices_; }

	/**
	减少proxices计数
	*/
	void decProxicesCount() { --numProxices_; }

	/**
	获得proxices计数
	*/
	int32 numProxices() const { return numProxices_; }

	/**
	获得numClients计数
	*/
	int32 numClients() { return this->networkInterface().numExtChannels(); }
protected:
	TimerHandle												loopCheckTimerHandle_;
	// 记录登录到服务器但还未处理完毕的账号
	PendingLoginMgr											pendingLoginMgr_;
	EntityIDClient											idClient_;

	InitProgressHandler*									pInitProgressHandler_;
	int32													numProxices_;
};

}

#endif // KBE_BASEAPP_H
