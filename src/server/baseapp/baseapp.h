
#ifndef KBE_BASE_H
#define KBE_BASE_H
	
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
class BaseApp:	public ServerApp, 
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
protected:
	TimerHandle												loopCheckTimerHandle_;
	// ��¼��¼������������δ������ϵ��˺�
	PendingLoginMgr											pendingLoginMgr_;
	EntityIDClient											idClient_;

	InitProgressHandler*									pInitProgressHandler_;
};

}

#endif // KBE_BASE_H
