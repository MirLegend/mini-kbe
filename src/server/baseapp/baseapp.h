
#ifndef KBE_BASEAPP_H
#define KBE_BASEAPP_H
	
// common include	
#include "server/kbemain.h"
#include "server/serverapp.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
#include "server/pendingLoginmgr.h"
#include "common/timer.h"
#include "network/endpoint.h"
#include "network/udp_packet_receiver.h"
#include "network/common.h"
#include "network/address.h"
#include "server/entity_app.h"
#include "base.h"
#include "proxy.h"
//#include "logwatcher.h"

//#define NDEBUG
#include <map>	
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

class InitProgressHandler;
class BaseApp: public EntityApp<Base>,
	public Singleton<BaseApp>
{
public:
	enum TimeOutType
	{
		TIMEOUT_TICK = TIMEOUT_SERVERAPP_MAX + 1
	};

	BaseApp(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~BaseApp();
	
	bool run();
	
	virtual bool initializeWatcher();

	void handleTimeout(TimerHandle handle, void * arg);
	void handleTick();

	/* ��ʼ����ؽӿ� */
	bool initializeBegin();
	bool inInitialize();
	bool initializeEnd();
	void finalise();

	virtual bool canShutdown();

	virtual Base* onCreateEntity(const char* entityType, ENTITY_ID eid);
	

	/**
	֪ͨ�ͻ��˴���һ��proxy��Ӧ��ʵ��
	*/
	bool createClientProxies(Proxy* base, bool reload = false);

	EntityIDClient& idClient() { return idClient_; }
	void registerPendingLogin(Network::Channel* pChannel, MemoryStream& s);

	/** ����ӿ�
	dbmgr���ͳ�ʼ��Ϣ
	startID: ��ʼ����ENTITY_ID ����ʼλ��
	endID: ��ʼ����ENTITY_ID �ν���λ��
	startGlobalOrder: ȫ������˳�� �������ֲ�ͬ���
	startGroupOrder: ��������˳�� ����������baseapp�еڼ���������
	machineGroupOrder: ��machine����ʵ����˳��, �ṩ�ײ���ĳЩʱ���ж��Ƿ�Ϊ��һ��baseappʱʹ��
	*/
	void onDbmgrInitCompleted(Network::Channel* pChannel, MemoryStream& s
		/*GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID*/);
	void onGetEntityAppFromDbmgr(Network::Channel* pChannel, MemoryStream& s);
	void onQueryPlayerCBFromDbmgr(Network::Channel* pChannel, MemoryStream& s);

	void onClientHello(Network::Channel* pChannel, MemoryStream& s); //�ͻ�������
	void loginBaseapp(Network::Channel* pChannel, MemoryStream& s);
	/**
	��¼ʧ��
	@failedcode: ʧ�ܷ����� NETWORK_ERR_SRV_NO_READY:������û��׼����,
	NETWORK_ERR_ILLEGAL_LOGIN:�Ƿ���¼,
	NETWORK_ERR_NAME_PASSWORD:�û����������벻��ȷ
	*/
	void loginBaseappFailed(Network::Channel* pChannel, std::string& accountName,
		SERVER_ERROR_CODE failedcode, bool relogin = false);

	virtual void onChannelDeregister(Network::Channel * pChannel);

	/**
	����proxices����
	*/
	void incProxicesCount() { ++numProxices_; }

	/**
	����proxices����
	*/
	void decProxicesCount() { --numProxices_; }

	/**
	���proxices����
	*/
	int32 numProxices() const { return numProxices_; }

	/**
	���numClients����
	*/
	int32 numClients() { return this->networkInterface().numExtChannels(); }
protected:
	TimerHandle												loopCheckTimerHandle_;
	// ��¼��¼������������δ������ϵ��˺�
	PendingLoginMgr											pendingLoginMgr_;
	EntityIDClient											idClient_;

	InitProgressHandler*									pInitProgressHandler_;
	int32													numProxices_;
};

}

#endif // KBE_BASEAPP_H
