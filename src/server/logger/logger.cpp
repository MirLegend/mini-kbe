
#include "logger.h"
#include "logger_interface.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "network/bundle_broadcast.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include <sstream>

#include "proto/coms.pb.h"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(Logger);

static uint64 g_totalNumlogs = 0;
static uint32 g_secsNumlogs = 0;
static uint64 g_lastCalcsecsNumlogsTime = 0;

uint64 totalNumlogs()
{
	return g_totalNumlogs;
}

uint64 secsNumlogs()
{
	return g_secsNumlogs;
}

//-------------------------------------------------------------------------------------
Logger::Logger(Network::EventDispatcher& dispatcher, 
				 Network::NetworkInterface& ninterface, 
				 COMPONENT_TYPE componentType,
				 COMPONENT_ID componentID):
	ServerApp(dispatcher, ninterface, componentType, componentID),
timer_()
{
}

//-------------------------------------------------------------------------------------
Logger::~Logger()
{
}

//-------------------------------------------------------------------------------------		
bool Logger::initializeWatcher()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Logger::run()
{
	dispatcher_.processUntilBreak();
	return true;
}

//-------------------------------------------------------------------------------------
void Logger::handleTimeout(TimerHandle handle, void * arg)
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
void Logger::handleTick()
{
	if(timestamp() - g_lastCalcsecsNumlogsTime > uint64( stampsPerSecond() ))
	{
		g_lastCalcsecsNumlogsTime = timestamp();
		g_secsNumlogs = 0;
	}

	threadPool_.onMainThreadTick();
	networkInterface().processChannels(&LogerInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
bool Logger::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Logger::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Logger::initializeEnd()
{
	timer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));
	return true;
}

//-------------------------------------------------------------------------------------
void Logger::finalise()
{
	timer_.cancel();
	ServerApp::finalise();
}

//-------------------------------------------------------------------------------------	
bool Logger::canShutdown()
{
	if(Components::getSingleton().getGameSrvComponentsSize() > 0)
	{
		INFO_MSG(fmt::format("Logger::canShutdown(): Waiting for components({}) destruction!\n", 
			Components::getSingleton().getGameSrvComponentsSize()));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void Logger::writeLog(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s)
{
	++g_secsNumlogs;
	++g_totalNumlogs;
}

//-------------------------------------------------------------------------------------
void Logger::registerLogWatcher(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{

}

//-------------------------------------------------------------------------------------
void Logger::deregisterLogWatcher(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s)
{
}

//-------------------------------------------------------------------------------------
void Logger::updateLogWatcherSetting(Network::Channel* pChannel, /*KBEngine::*/MemoryStream& s)
{
}

//-------------------------------------------------------------------------------------

}
