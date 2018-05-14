
#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_CELLMGRAPP_INTERFACE_H
#endif


#ifndef KBE_CELLMGRAPP_INTERFACE_H
#define KBE_CELLMGRAPP_INTERFACE_H

#include "cellmgr.h"
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
NETWORK_INTERFACE_DECLARE_BEGIN(CellappmgrInterface)
	SERVER_MESSAGE_DECLARE_STREAM(MyCellMgrApp, MSGIDMAKE(1, 1), OnRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(MyCellMgrApp, MSGIDMAKE(1, 2), CBRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(MyCellMgrApp, MSGIDMAKE(1, 3), onAppActiveTick, NETWORK_VARIABLE_MESSAGE)
NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}
#endif
