
#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_LOGGER_INTERFACE_H
#endif


#ifndef KBE_LOGGER_INTERFACE_H
#define KBE_LOGGER_INTERFACE_H

// common include	
//#if defined(KBE_LOGGERAPP)
#include "logger.h"
//#endif
#include "network/server_interface_macros.h"
#include "network/interface_defs.h"
#include "server/server_errors.h"
//#define NDEBUG
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

/**
	LOGINAPP所有消息接口在此定义
*/
NETWORK_INTERFACE_DECLARE_BEGIN(LogerInterface)
	
	// hello握手。
	//NETWORK_MESSAGE_EXPOSED(Logger, OnRegisterServer)
	SERVER_MESSAGE_DECLARE_STREAM(Logger, MSGIDMAKE(1, 1), OnRegisterServer,	NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(Logger, MSGIDMAKE(1, 2), CBRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(Logger, MSGIDMAKE(1, 3), onAppActiveTick, NETWORK_VARIABLE_MESSAGE)
NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}
#else
#endif
