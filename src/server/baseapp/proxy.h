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
		��witness�ͻ�������һ����Ϣ
	*/
	//bool sendToClient(const Network::MessageHandler& msgHandler, Network::Bundle* pBundle);
	bool sendToClient(Network::Bundle* pBundle);
	bool sendToClient(bool expectData = true);

	virtual void InitDatas(const std::string datas);

	/** 
		�ű������ȡ���ӵ�rttֵ
	*/
	double getRoundTripTime() const;

	/** 
		This is the number of seconds since a packet from the client was last received. 
	*/
	double getTimeSinceHeardFromClient() const;

	/** 
		�ű������ȡ�Ƿ���client�󶨵�proxy��
	*/
	bool hasClient() const;

	/** 
		ʵ���Ƿ����
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
		���entity��������, �ڿͻ��˳�ʼ���ö�Ӧ��entity�� �������������
	*/
	void onEntitiesEnabled(void);

	/**
		��½���ԣ� �������ĵ�½ʧ��֮�� ��������ӿ��ٽ��г��� 
	*/
	int32 onLogOnAttempt(const char* addr, uint32 port, const char* password);


	/** 
		��������entity��Ӧ�Ŀͻ���socket�Ͽ�ʱ������ 
	*/
	void onClientDeath(void);

	/**
		��ȡǰ�����
	*/
	INLINE COMPONENT_CLIENT_TYPE getClientType() const;
	INLINE void setClientType(COMPONENT_CLIENT_TYPE ctype);

	/**
		��ȡǰ�˸�������
	*/
	INLINE const std::string& getLoginDatas();
	INLINE void setLoginDatas(const std::string& datas);
	
	INLINE const std::string& getCreateDatas();
	INLINE void setCreateDatas(const std::string& datas);

	/**
		ÿ��proxy����֮�󶼻���ϵͳ����һ��uuid�� �ṩǰ���ص�½ʱ�������ʶ��
	*/
	INLINE uint64 rndUUID() const;
	INLINE void rndUUID(uint64 uid);

	/**
		����witness
	*/
	void onGetWitness();

	/**
		���ͻ��˴ӷ������߳�
	*/
	void kick();

	/**
		������proxy�Ŀͻ������Ӷ���
	*/
	Network::Channel* pChannel();

protected:
	uint64 rndUUID_;
	Network::Address addr_;

	std::string accountName_;
	std::string password_;

	bool entitiesEnabled_;

	//// ���ƿͻ���ÿ������ʹ�õĴ���
	//int32 bandwidthPerSecond_;

	// ͨ�ż���key Ĭ��blowfish
	std::string encryptionKey;

	ProxyForwarder* pProxyForwarder_;

	COMPONENT_CLIENT_TYPE clientComponentType_;

	// ��½ʱ������datas���ݣ����浵��
	std::string loginDatas_;

	// ע��ʱ������datas���ݣ����ô浵��
	std::string createDatas_;
};

}


#ifdef CODE_INLINE
#include "proxy.inl"
#endif

#endif // KBE_PROXY_H
