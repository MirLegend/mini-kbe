
#ifndef KBE_LOGGER_H
#define KBE_LOGGER_H
	
// common include	
#include "server/kbemain.h"
#include "server/serverapp.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
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


class Logger:	public ServerApp, 
				public Singleton<Logger>
{
public:
	//typedef std::map<Network::Address, LogWatcher> LOG_WATCHERS;
public:
	enum TimeOutType
	{
		TIMEOUT_TICK = TIMEOUT_SERVERAPP_MAX + 1
	};

	Logger(Network::EventDispatcher& dispatcher, 
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Logger();
	
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

	uint32 bufferedLogsSize(){
		return 0;// buffered_logs_.size();
	}

	/** 网络接口
		写日志
	*/
	void writeLog(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s);

	/** 网络接口
		注册log监听者
	*/
	void registerLogWatcher(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s);

	/** 网络接口
		注销log监听者
	*/
	void deregisterLogWatcher(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s);

	/** 网络接口
		log监听者更新自己的设置
	*/
	void updateLogWatcherSetting(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s);

	//LOG_WATCHERS& logWatchers(){ return logWatchers_; }

	//void sendInitLogs(LogWatcher& logWatcher);

	//virtual void OnRegisterServer(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s);

protected:
	//LOG_WATCHERS logWatchers_;
	//std::deque<LOG_ITEM*> buffered_logs_;
	TimerHandle	timer_;
};

}

#endif // KBE_LOGGER_H
