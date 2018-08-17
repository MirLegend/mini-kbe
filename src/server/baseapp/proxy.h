/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2017 KBEngine.

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


#ifndef KBE_PROXY_H
#define KBE_PROXY_H
	
#include "base.h"
//#include "data_downloads.h"
#include "common/common.h"
#include "helper/debug_helper.h"
#include "network/address.h"
#include "network/message_handler.h"
	
namespace KBEngine{


namespace Network
{
class Channel;
}

class ProxyForwarder;

#define LOG_ON_REJECT  0
#define LOG_ON_ACCEPT  1
#define LOG_ON_WAIT_FOR_DESTROY 2

class Proxy : public Base
{

public:
	Proxy(ENTITY_ID id);
	~Proxy();
	
	INLINE void addr(const Network::Address& address);
	INLINE const Network::Address& addr() const;

	typedef std::vector<Network::Bundle*> Bundles;
	bool pushBundle(Network::Bundle* pBundle);

	/**
		向witness客户端推送一条消息
	*/
	//bool sendToClient(const Network::MessageHandler& msgHandler, Network::Bundle* pBundle);
	bool sendToClient(Network::Bundle* pBundle);
	bool sendToClient(bool expectData = true);

	virtual void InitDatas(const std::string datas);

	/** 
		脚本请求获取连接的rtt值
	*/
	double getRoundTripTime() const;

	/** 
		This is the number of seconds since a packet from the client was last received. 
	*/
	double getTimeSinceHeardFromClient() const;

	/** 
		脚本请求获取是否有client绑定到proxy上
	*/
	bool hasClient() const;

	/** 
		实体是否可用
	*/
	INLINE bool entitiesEnabled() const;

	//-------------------------------------------------------------------------------------
	INLINE const std::string& accountName();

	//-------------------------------------------------------------------------------------
	INLINE void accountName(const std::string& account);

	//-------------------------------------------------------------------------------------
	INLINE const std::string& password();

	//-------------------------------------------------------------------------------------
	INLINE void password(const std::string& pwd);

	/**
		这个entity被激活了, 在客户端初始化好对应的entity后， 这个方法被调用
	*/
	void onEntitiesEnabled(void);

	/**
		登陆尝试， 当正常的登陆失败之后， 调用这个接口再进行尝试 
	*/
	int32 onLogOnAttempt(const char* addr, uint32 port, const char* password);


	/** 
		当察觉这个entity对应的客户端socket断开时被调用 
	*/
	void onClientDeath(void);

	/**
		获取前端类别
	*/
	INLINE COMPONENT_CLIENT_TYPE getClientType() const;
	INLINE void setClientType(COMPONENT_CLIENT_TYPE ctype);

	/**
		获取前端附带数据
	*/
	INLINE const std::string& getLoginDatas();
	INLINE void setLoginDatas(const std::string& datas);
	
	INLINE const std::string& getCreateDatas();
	INLINE void setCreateDatas(const std::string& datas);

	/**
		每个proxy创建之后都会由系统产生一个uuid， 提供前端重登陆时用作身份识别
	*/
	INLINE uint64 rndUUID() const;
	INLINE void rndUUID(uint64 uid);

	/**
		绑定了witness
	*/
	void onGetWitness();

	/**
		将客户端从服务器踢出
	*/
	void kick();

	/**
		获得这个proxy的客户端连接对象
	*/
	Network::Channel* pChannel();

protected:
	uint64 rndUUID_;
	Network::Address addr_;

	std::string accountName_;
	std::string password_;

	bool entitiesEnabled_;

	//// 限制客户端每秒所能使用的带宽
	//int32 bandwidthPerSecond_;

	// 通信加密key 默认blowfish
	std::string encryptionKey;

	ProxyForwarder* pProxyForwarder_;

	COMPONENT_CLIENT_TYPE clientComponentType_;

	// 登陆时附带的datas数据（不存档）
	std::string loginDatas_;

	// 注册时附带的datas数据（永久存档）
	std::string createDatas_;
};

}


#ifdef CODE_INLINE
#include "proxy.inl"
#endif

#endif // KBE_PROXY_H
