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
#include "interfaces_handler.h"
#include "db_interface/db_threadpool.h"
#include "thread/threadpool.h"
#include "thread/threadguard.h"
#include "server/serverconfig.h"
#include "network/endpoint.h"
#include "network/channel.h"
#include "network/bundle.h"

#include "baseapp/baseapp_interface.h"

namespace KBEngine{

//-------------------------------------------------------------------------------------
InterfacesHandler* InterfacesHandlerFactory::create(std::string type,  
											  DBThreadPool& dbThreadPool)
{
	if(type.size() == 0 || type == "normal")
	{
		return new InterfacesHandler_Normal(dbThreadPool);
	}
	else if(type == "thirdparty")
	{
		return new InterfacesHandler_ThirdParty(dbThreadPool);
	}

	return NULL;
}

//-------------------------------------------------------------------------------------
InterfacesHandler::InterfacesHandler(DBThreadPool& dbThreadPool):
dbThreadPool_(dbThreadPool)
{
}

//-------------------------------------------------------------------------------------
InterfacesHandler::~InterfacesHandler()
{
}

//-------------------------------------------------------------------------------------
InterfacesHandler_Normal::InterfacesHandler_Normal(DBThreadPool& dbThreadPool):
InterfacesHandler(dbThreadPool)
{
}

//-------------------------------------------------------------------------------------
InterfacesHandler_Normal::~InterfacesHandler_Normal()
{
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_Normal::createAccount(Network::Channel* pChannel, std::string& registerName, 
										  std::string& password, std::string& datas)
{
	dbThreadPool_.addTask(new DBTaskCreateAccount(pChannel->addr(), 
		registerName, registerName, password, datas, datas));

	return true;
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::onCreateAccountCB(KBEngine::MemoryStream& s)
{
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_Normal::loginAccount(Network::Channel* pChannel, std::string& loginName, 
										 std::string& password, std::string& datas)
{
	dbThreadPool_.addTask(new DBTaskAccountLogin(pChannel->addr(), 
		loginName, loginName, password, SERVER_SUCCESS, datas, datas));

	return true;
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::onLoginAccountCB(KBEngine::MemoryStream& s)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::charge(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	INFO_MSG("InterfacesHandler_Normal::charge: no implement!\n");
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::onChargeCB(KBEngine::MemoryStream& s)
{
	INFO_MSG("InterfacesHandler_Normal::onChargeCB: no implement!\n");
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::eraseClientReq(Network::Channel* pChannel, std::string& logkey)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::accountActivate(Network::Channel* pChannel, std::string& scode)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::accountReqResetPassword(Network::Channel* pChannel, std::string& accountName)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::accountResetPassword(Network::Channel* pChannel, std::string& accountName, std::string& newpassword, std::string& scode)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::accountReqBindMail(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
											   std::string& password, std::string& email)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::accountBindMail(Network::Channel* pChannel, std::string& username, std::string& scode)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_Normal::accountNewPassword(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
											   std::string& password, std::string& newpassword)
{
}

//-------------------------------------------------------------------------------------
InterfacesHandler_ThirdParty::InterfacesHandler_ThirdParty(DBThreadPool& dbThreadPool):
InterfacesHandler_Normal(dbThreadPool)
{
}

//-------------------------------------------------------------------------------------
InterfacesHandler_ThirdParty::~InterfacesHandler_ThirdParty()
{
	//pInterfacesChannel = NULL;
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_ThirdParty::createAccount(Network::Channel* pChannel, std::string& registerName, 
											  std::string& password, std::string& datas)
{
	return true;
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::onCreateAccountCB(KBEngine::MemoryStream& s)
{
	
	//dbThreadPool_.addTask(new DBTaskCreateAccount(cinfos->pChannel->addr(), 
	//	registerName, accountName, password, postdatas, getdatas));
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_ThirdParty::loginAccount(Network::Channel* pChannel, std::string& loginName, 
											 std::string& password, std::string& datas)
{
	
	return true;
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::onLoginAccountCB(KBEngine::MemoryStream& s)
{
	//dbThreadPool_.addTask(new DBTaskAccountLogin(cinfos->pChannel->addr(), 
	//	loginName, accountName, password, success, postdatas, getdatas));
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_ThirdParty::initialize()
{

	return reconnect();
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_ThirdParty::reconnect()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool InterfacesHandler_ThirdParty::process()
{
	return true;
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::charge(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::onChargeCB(KBEngine::MemoryStream& s)
{

}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::eraseClientReq(Network::Channel* pChannel, std::string& logkey)
{
	
}


//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::accountActivate(Network::Channel* pChannel, std::string& scode)
{
}


//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::accountReqResetPassword(Network::Channel* pChannel, std::string& accountName)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::accountResetPassword(Network::Channel* pChannel, std::string& accountName, std::string& newpassword, std::string& scode)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::accountReqBindMail(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
												   std::string& password, std::string& email)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::accountBindMail(Network::Channel* pChannel, std::string& username, std::string& scode)
{
}

//-------------------------------------------------------------------------------------
void InterfacesHandler_ThirdParty::accountNewPassword(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
												   std::string& password, std::string& newpassword)
{
}

//-------------------------------------------------------------------------------------

}
