/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2016 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dbmgr.h"
#include "sync_app_datas_handler.h"
#include "math/math.h"
#include "network/bundle.h"
#include "network/channel.h"
#include "server/components.h"

//#include "baseapp/baseapp_interface.h"
//#include "cellapp/cellapp_interface.h"
#include "proto/basedb.pb.h"
#include "../../server/baseapp/baseapp_interface.h"
#include "proto/celldb.pb.h"
#include "../../server/cellapp/cellapp_interface.h"

namespace KBEngine{	

//-------------------------------------------------------------------------------------
SyncAppDatasHandler::SyncAppDatasHandler(Network::NetworkInterface & networkInterface):
Task(),
//networkInterface_(networkInterface),
lastRegAppTime_(0),
apps_()
{
	networkInterface.dispatcher().addTask(this);
}

//-------------------------------------------------------------------------------------
SyncAppDatasHandler::~SyncAppDatasHandler()
{
	// networkInterface_.dispatcher().cancelTask(this);
	DEBUG_MSG("SyncAppDatasHandler::~SyncAppDatasHandler()\n");

	DBMgrApp::getSingleton().pSyncAppDatasHandler(NULL);
}

//-------------------------------------------------------------------------------------
void SyncAppDatasHandler::pushApp(COMPONENT_ID cid/*, COMPONENT_ORDER startGroupOrder, COMPONENT_ORDER startGlobalOrder*/)
{
	lastRegAppTime_ = timestamp();
	std::vector<ComponentInitInfo>::iterator iter = apps_.begin();
	for(; iter != apps_.end(); ++iter)
	{
		if((*iter).cid == cid)
		{
			ERROR_MSG(fmt::format("SyncAppDatasHandler::pushApp: cid({}) is exist!\n", cid));
			return;
		}
	}

	ComponentInitInfo cinfo;
	cinfo.cid = cid;
	//cinfo.startGroupOrder = startGroupOrder;
	//cinfo.startGlobalOrder = startGlobalOrder;
	apps_.push_back(cinfo);
}

//-------------------------------------------------------------------------------------
bool SyncAppDatasHandler::process()
{
	if(lastRegAppTime_ == 0)
		return true;

	bool hasApp = false;

	std::vector<ComponentInitInfo>::iterator iter = apps_.begin();
	for(; iter != apps_.end(); ++iter)
	{
		ComponentInitInfo cInitInfo = (*iter);
		Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(cInitInfo.cid);

		if(cinfos == NULL)
			continue;

		COMPONENT_TYPE tcomponentType = cinfos->componentType;
		if(tcomponentType == BASEAPP_TYPE || 
			tcomponentType == CELLAPP_TYPE)
		{
			hasApp = true;
			break;
		}
	}
	
	if(!hasApp)
	{
		lastRegAppTime_ = timestamp();
		return true;
	}

	if(timestamp() - lastRegAppTime_ < uint64( 3 * stampsPerSecond() ) )
		return true;

	// 如果是连接到dbmgr则需要等待接收app初始信息
	// 例如：初始会分配entityID段以及这个app启动的顺序信息（是否第一个baseapp启动）
	iter = apps_.begin();
	for(; iter != apps_.end(); ++iter)
	{
		ComponentInitInfo cInitInfo = (*iter);
		Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(cInitInfo.cid);

		if(cinfos == NULL)
			continue;

		COMPONENT_TYPE tcomponentType = cinfos->componentType;

		if(tcomponentType == BASEAPP_TYPE || 
			tcomponentType == CELLAPP_TYPE || 
			tcomponentType == LOGINAPP_TYPE)
		{
			Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
			
			switch(tcomponentType)
			{
			case BASEAPP_TYPE:
				{
					//DBMgrApp::getSingleton().onGlobalDataClientLogon(cinfos->pChannel, BASEAPP_TYPE);

					std::pair<ENTITY_ID, ENTITY_ID> idRange = DBMgrApp::getSingleton().idServer().allocRange();

					(*pBundle).newMessage(BaseappInterface::onDbmgrInitCompleted);
					base_dbmgr::DbmgrInitCompleted dicCmd;
					dicCmd.set_g_kbetime(g_kbetime);
					dicCmd.set_startentityid(idRange.first);
					dicCmd.set_endentityid(idRange.second);
					ADDTOBUNDLE((*pBundle), dicCmd);
				}
				break;
			case CELLAPP_TYPE:
				{
					//DBMgrApp::getSingleton().onGlobalDataClientLogon(cinfos->pChannel, CELLAPP_TYPE);

					std::pair<ENTITY_ID, ENTITY_ID> idRange = DBMgrApp::getSingleton().idServer().allocRange();
					(*pBundle).newMessage(CellappInterface::onDbmgrInitCompleted);
					cell_dbmgr::DbmgrInitCompleted dicCmd;
					dicCmd.set_g_kbetime(g_kbetime);
					dicCmd.set_startentityid(idRange.first);
					dicCmd.set_endentityid(idRange.second);
					ADDTOBUNDLE((*pBundle), dicCmd);
				}
				break;
			case LOGINAPP_TYPE:
				/*(*pBundle).newMessage(LoginappInterface::onDbmgrInitCompleted);
				LoginappInterface::onDbmgrInitCompletedArgs3::staticAddToBundle((*pBundle),
						cInitInfo.startGlobalOrder, cInitInfo.startGroupOrder,
						digest);
*/
				break;
			default:
				break;
			}

			cinfos->pChannel->send(pBundle);
		}
	}

	apps_.clear();

	delete this;
	return false;
}

//-------------------------------------------------------------------------------------

}
