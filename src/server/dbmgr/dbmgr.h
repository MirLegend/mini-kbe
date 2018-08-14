#ifndef KBE_DBMGR_H
#define KBE_DBMGR_H
// common include	
#include "server/kbemain.h"
#include "buffered_dbtasks.h"
#include "server/serverapp.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
#include "common/timer.h"
#include "network/endpoint.h"
#include "network/udp_packet_receiver.h"
#include "network/common.h"
#include "network/address.h"

#include "db_interface/entity_table.h"
#include "db_interface/db_threadpool.h"

//#define NDEBUG
#include <map>	
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

class InterfacesHandler;
class SyncAppDatasHandler;
class DBMgrApp:	public ServerApp, 
	public Singleton<DBMgrApp>
{
public:
	enum TimeOutType
	{
		TIMEOUT_TICK = TIMEOUT_SERVERAPP_MAX + 1,
		TIMEOUT_CHECK_STATUS
	};

	DBMgrApp(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~DBMgrApp();

	/** 获取ID服务器指针 */
	IDServer<ENTITY_ID>& idServer(void) { return idServer_; }
	
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

	void onAccountLogin(Network::Channel* pChannel, MemoryStream& s);
	bool initInterfacesHandler();
	bool initDB();

	SyncAppDatasHandler* pSyncAppDatasHandler() const { return pSyncAppDatasHandler_; }
	void pSyncAppDatasHandler(SyncAppDatasHandler* p) { pSyncAppDatasHandler_ = p; }

	virtual void OnRegisterServer(Network::Channel* pChannel, MemoryStream& s);

	void QueryAccount(Network::Channel* pChannel, MemoryStream& s);

	/** 网络接口
	删除某个entity的存档数据
	*/
	void removeEntity(Network::Channel* pChannel, KBEngine::MemoryStream& s);
	
protected:
	TimerHandle											loopCheckTimerHandle_;
	TimerHandle											mainProcessTimer_;

	// entityID分配服务端
	IDServer<ENTITY_ID>									idServer_;

	InterfacesHandler*									pInterfacesAccountHandler_;

	//分配ids的
	SyncAppDatasHandler*								pSyncAppDatasHandler_;

	Buffered_DBTasks									bufferedDBTasks_;

	uint32												numQueryEntity_;
};

}

#endif // KBE_DBMGR_H
