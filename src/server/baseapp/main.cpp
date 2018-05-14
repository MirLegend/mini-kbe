#include "server/kbemain.h"
#include "BaseApp.h"

#undef DEFINE_IN_INTERFACE
#undef KBE_SERVER_INTERFACE_MACRO_H //很重要这个操作
#include "login/login_interface.h"
#define DEFINE_IN_INTERFACE
#include "login/login_interface.h"

#undef DEFINE_IN_INTERFACE
#undef KBE_SERVER_INTERFACE_MACRO_H //很重要这个操作
#include "logger/logger_interface.h"
#define DEFINE_IN_INTERFACE
#include "logger/logger_interface.h"

#undef DEFINE_IN_INTERFACE
#undef KBE_SERVER_INTERFACE_MACRO_H //很重要这个操作
#include "cellmgr/cellmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "cellmgr/cellmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#undef KBE_SERVER_INTERFACE_MACRO_H //很重要这个操作
#include "dbmgr/dbmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "dbmgr/dbmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#undef KBE_SERVER_INTERFACE_MACRO_H //很重要这个操作
#include "basemgr/basemgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "basemgr/basemgr_interface.h"

#undef DEFINE_IN_INTERFACE
#undef KBE_SERVER_INTERFACE_MACRO_H //很重要这个操作
#include "cellapp/cellapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "cellapp/cellapp_interface.h"

using namespace KBEngine;
int KBENGINE_MAIN(int argc, char* argv[])
{
	ENGINE_COMPONENT_INFO& info = g_kbeSrvConfig.getBaseApp();
	return kbeMainT<BaseApp>(argc, argv, BASEAPP_TYPE, info.port, info.port_max, info.ip.c_str());
}
