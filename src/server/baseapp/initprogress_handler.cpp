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

#include "baseapp.h"
#include "initprogress_handler.h"
#include "network/bundle.h"
#include "network/channel.h"

#include "proto/bmb.pb.h"
#include "../../server/basemgr/basemgr_interface.h"

namespace KBEngine{	

//-------------------------------------------------------------------------------------
InitProgressHandler::InitProgressHandler(Network::NetworkInterface & networkInterface):
Task(),
networkInterface_(networkInterface),
delayTicks_(0),
error_(false),
baseappReady_(false)
{
	networkInterface.dispatcher().addTask(this);
}

//-------------------------------------------------------------------------------------
InitProgressHandler::~InitProgressHandler()
{
	// networkInterface_.dispatcher().cancelTask(this);
	DEBUG_MSG("InitProgressHandler::~InitProgressHandler()\n");
}

//-------------------------------------------------------------------------------------
void InitProgressHandler::setAutoLoadState(int8 state)
{ 
}

//-------------------------------------------------------------------------------------
void InitProgressHandler::onEntityAutoLoadCBFromDBMgr(Network::Channel* pChannel, MemoryStream& s)
{
}

//-------------------------------------------------------------------------------------
void InitProgressHandler::setError()
{
	error_ = true;
}

//-------------------------------------------------------------------------------------
bool InitProgressHandler::process()
{
	if(error_)
	{
		BaseApp::getSingleton().dispatcher().breakProcessing();
		return false;
	}

	Network::Channel* pChannel = Components::getSingleton().getBaseappmgrChannel();

	if(pChannel == NULL)
		return true;

	if(BaseApp::getSingleton().idClient().size() == 0)
		return true;

	if(delayTicks_++ < 1)
		return true;

	// 只有第一个baseapp上会创建EntityAutoLoader来自动加载数据库实体
	if(g_componentGroupOrder == 1)
	{
		
	}

	if(!baseappReady_)
	{
		baseappReady_ = true;

		//SCOPED_PROFILE(SCRIPTCALL_PROFILE);

		// 所有脚本都加载完毕
		/*PyObject* pyResult = PyObject_CallMethod(Baseapp::getSingleton().getEntryScript().get(),
											const_cast<char*>("onBaseAppReady"),
											const_cast<char*>("O"),
											PyBool_FromLong((g_componentGroupOrder == 1) ? 1 : 0));
*/
		return true;
	}

	float v = 0.0f;
	bool completed = false;


	{
		v = 100.f;
		completed = true;
	}
	
	if(v >= 0.9999f)
	{
		v = 100.f;
		completed = true;
	}

	Network::Bundle* pBundle = Network::Bundle::ObjPool().createObject();
	DEBUG_MSG(fmt::format("InitProgressHandler::process completed basemgr.port:{}, compoentid:{}\n",
		pChannel->addr().port, pChannel->componentID()));
	(*pBundle).newMessage(BaseappmgrInterface::onBaseappInitProgress);
	basemgr_base::BaseappInitProgress bipCmd;
	bipCmd.set_componentid(g_componentID);
	bipCmd.set_progress((uint32)(v * 100));
	ADDTOBUNDLE((*pBundle), bipCmd);
	pChannel->send(pBundle);
	
	if(completed)
	{
		delete this;
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------

}
