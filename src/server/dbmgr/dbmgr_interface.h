
#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_DBMGRAPP_INTERFACE_H
#endif


#ifndef KBE_DBMGRAPP_INTERFACE_H
#define KBE_DBMGRAPP_INTERFACE_H

#include "dbmgr.h"
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
	LOGINAPP������Ϣ�ӿ��ڴ˶���
*/
NETWORK_INTERFACE_DECLARE_BEGIN(DbmgrInterface)
	SERVER_MESSAGE_DECLARE_STREAM(DBMgrApp, MSGIDMAKE(1, 1), OnRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(DBMgrApp, MSGIDMAKE(1, 2), CBRegisterServer, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(DBMgrApp, MSGIDMAKE(1, 3), onAppActiveTick, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(DBMgrApp, MSGIDMAKE(2, 1), onAccountLogin, NETWORK_VARIABLE_MESSAGE)
	SERVER_MESSAGE_DECLARE_STREAM(DBMgrApp, MSGIDMAKE(6, 3), QueryAccount, NETWORK_VARIABLE_MESSAGE)
NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}
#endif
