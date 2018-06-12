
#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_BASEMGRAPP_INTERFACE_H
#endif


#ifndef KBE_BASEMGRAPP_INTERFACE_H
#define KBE_BASEMGRAPP_INTERFACE_H

#include "basemgr.h"
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
NETWORK_INTERFACE_DECLARE_BEGIN(BaseappmgrInterface)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(1, 1), OnRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(1, 2), CBRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(1, 3), onAppActiveTick, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(3, 1), onRegisterPendingAccount, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(3, 2), onRegisterPendingAccountEx, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(4, 2), onPendingAccountGetBaseappAddr, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(BaseMgrApp, MSGIDMAKE(4, 3), onBaseappInitProgress, NETWORK_VARIABLE_MESSAGE)
NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}
#endif
