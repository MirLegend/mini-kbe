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

#ifndef KBE_ENTITY_APP_H
#define KBE_ENTITY_APP_H

// common include
#include "common/common.h"
#include "math/math.h"
#include "common/timer.h"
#include "common/smartpointer.h"
#include "helper/debug_helper.h"
#include "helper/script_loglevel.h"
#include "helper/profile.h"
#include "server/kbemain.h"	
#include "entities.h"
//#include "server/script_timers.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
//#include "server/globaldata_client.h"
#include "server/globaldata_server.h"
//#include "server/callbackmgr.h"	
#include "network/message_handler.h"
#include "resmgr/resmgr.h"
//#include "helper/console_helper.h"
#include "server/serverapp.h"

#if KBE_PLATFORM == PLATFORM_WIN32
#pragma warning (disable : 4996)
#endif

	
namespace KBEngine{

// ʵ��ı�־
#define ENTITY_FLAGS_UNKNOWN			0x00000000
#define ENTITY_FLAGS_DESTROYING			0x00000001
#define ENTITY_FLAGS_INITING			0x00000002
#define ENTITY_FLAGS_TELEPORT_START		0x00000004
#define ENTITY_FLAGS_TELEPORT_END		0x00000008

template<class E>
class EntityApp : public ServerApp
{
public:
	enum TimeOutType
	{
		TIMEOUT_GAME_TICK = TIMEOUT_SERVERAPP_MAX + 1,
		TIMEOUT_ENTITYAPP_MAX
	};

public:
	EntityApp(Network::EventDispatcher& dispatcher, 
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~EntityApp();
	
	/** 
		��ش���ӿ� 
	*/
	virtual void handleTimeout(TimerHandle handle, void * arg);
	virtual void handleGameTick();

	/**
		ͨ��entityIDѰ�ҵ���Ӧ��ʵ�� 
	*/
	E* findEntity(ENTITY_ID entityID);

	/** 
		ͨ��entityID����һ��entity 
	*/
	virtual bool destroyEntity(ENTITY_ID entityID, bool callScript);
	
	virtual bool initializeWatcher();

	virtual void finalise();
	virtual bool inInitialize();
	virtual bool initialize();

	virtual void onSignalled(int sigNum);
	
	Entities<E>* pEntities() const{ return pEntities_; }
	ArraySize entitiesSize() const { return (ArraySize)pEntities_->size(); }

	EntityIDClient& idClient(){ return idClient_; }

	/**
		����һ��entity 
	*/
	E* createEntity(const char* entityType, ENTITY_ID eid = 0, bool initProperty = true);

	virtual E* onCreateEntity(const char* entityType, ENTITY_ID eid);

	/** ����ӿ�
		�������һ��ENTITY_ID�εĻص�
	*/
	void onReqAllocEntityID(Network::Channel* pChannel, ENTITY_ID startID, ENTITY_ID endID);

	/** ����ӿ�
		dbmgr���ͳ�ʼ��Ϣ
		startID: ��ʼ����ENTITY_ID ����ʼλ��
		endID: ��ʼ����ENTITY_ID �ν���λ��
		startGlobalOrder: ȫ������˳�� �������ֲ�ͬ���
		startGroupOrder: ��������˳�� ����������baseapp�еڼ���������
	*/
	void onDbmgrInitCompleted(Network::Channel* pChannel, 
		GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, COMPONENT_ORDER startGlobalOrder, 
		COMPONENT_ORDER startGroupOrder, const std::string& digest);

	/** ����ӿ�
		dbmgr�㲥global���ݵĸı�
	*/
	void onBroadcastGlobalDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/**
		���¸������
	*/
	int tickPassedPercent(uint64 curr = timestamp());
	float getLoad() const { return load_; }
	void updateLoad();
	virtual void onUpdateLoad(){}
	virtual void calcLoad(float spareTime);
	uint64 checkTickPeriod();

protected:

	EntityIDClient											idClient_;

	// �洢���е�entity������
	Entities<E>*											pEntities_;

	TimerHandle												gameTimer_;

	// globalData
	//GlobalDataClient*										pGlobalData_;

	//PY_CALLBACKMGR											pyCallbackMgr_;

	uint64													lastTimestamp_;

	// ���̵�ǰ����
	float													load_;
};


template<class E>
EntityApp<E>::EntityApp(Network::EventDispatcher& dispatcher, 
					 Network::NetworkInterface& ninterface, 
					 COMPONENT_TYPE componentType,
					 COMPONENT_ID componentID):
ServerApp(dispatcher, ninterface, componentType, componentID),
idClient_(),
pEntities_(NULL),
gameTimer_(),
//pGlobalData_(NULL),
lastTimestamp_(timestamp()),
load_(0.f)
{
	idClient_.pApp(this);
	pEntities_ = new Entities<E>();
}

template<class E>
EntityApp<E>::~EntityApp()
{
	SAFE_RELEASE(pEntities_);
}

template<class E>
bool EntityApp<E>::inInitialize()
{
	return true;
}

template<class E>
bool EntityApp<E>::initialize()
{
	bool ret = ServerApp::initialize();
	if(ret)
	{
		gameTimer_ = this->dispatcher().addTimer(1000000 / g_kbeSrvConfig.gameUpdateHertz(), this,
								reinterpret_cast<void *>(TIMEOUT_GAME_TICK));
	}

	lastTimestamp_ = timestamp();
	return ret;
}

template<class E>
bool EntityApp<E>::initializeWatcher()
{
	//WATCH_OBJECT("entitiesSize", this, &EntityApp<E>::entitiesSize);
	return true;// ServerApp::initializeWatcher();
}

template<class E>
void EntityApp<E>::finalise(void)
{
	gameTimer_.cancel();

	//WATCH_FINALIZE;

	if(pEntities_)
		pEntities_->finalise();

	ServerApp::finalise();
}

template<class E>
E* EntityApp<E>::createEntity(const char* entityType, ENTITY_ID eid, bool initProperty)
{
	// ���ID�Ƿ��㹻, ���㷵��NULL
	if(eid <= 0 && idClient_.size() == 0)
	{
		return NULL;
	}
	
	/*ScriptDefModule* sm = EntityDef::findScriptModule(entityType);
	if(sm == NULL)
	{
		PyErr_Format(PyExc_TypeError, "EntityApp::createEntity: entityType [%s] not found! Please register in entities.xml and implement a %s.def and %s.py\n",
			entityType, entityType, entityType);

		PyErr_PrintEx(0);
		return NULL;
	}
	else if(componentType_ == CELLAPP_TYPE ? !sm->hasCell() : !sm->hasBase())
	{
		PyErr_Format(PyExc_TypeError, "EntityApp::createEntity: cannot create %s(%s=false)! Please check the setting of the entities.xml and the implementation of %s.py\n",
			entityType,
			(componentType_ == CELLAPP_TYPE ? "hasCell()" : "hasBase()"),
			entityType);

		PyErr_PrintEx(0);
		return NULL;
	}

	PyObject* obj = sm->createObject();*/

	// �ж��Ƿ�Ҫ����һ���µ�id
	ENTITY_ID id = eid;
	if(id <= 0)
		id = idClient_.alloc();
	
	E* entity = onCreateEntity(entityType, id);

	//if(initProperty)
	//	entity->initProperty();

	// ��entity����entities
	pEntities_->add(id, entity); 

	// ��ʼ���ű�
	/*if(isInitializeScript)
		entity->initializeEntity(params);

	SCRIPT_ERROR_CHECK();*/

	INFO_MSG(fmt::format("EntityApp::createEntity: new {0} {1}\n", entityType, id));

	return entity;
}

template<class E>
E* EntityApp<E>::onCreateEntity(const char* entityType, ENTITY_ID eid)
{
	// ִ��Entity�Ĺ��캯��
	//return new(pyEntity) E(eid, sm);
	return new E(eid);
}

template<class E>
void EntityApp<E>::onSignalled(int sigNum)
{
	this->ServerApp::onSignalled(sigNum);
	
	switch (sigNum)
	{
	case SIGQUIT:
		CRITICAL_MSG(fmt::format("Received QUIT signal. This is likely caused by the "
				"{}Mgr killing this {} because it has been "
					"unresponsive for too long. Look at the callstack from "
					"the core dump to find the likely cause.\n",
				COMPONENT_NAME_EX(componentType_), 
				COMPONENT_NAME_EX(componentType_)));
		
		break;
	default: 
		break;
	}
}

template<class E>
void EntityApp<E>::handleTimeout(TimerHandle handle, void * arg)
{
	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_GAME_TICK:
			this->handleGameTick();
			break;
		default:
			break;
	}

	ServerApp::handleTimeout(handle, arg);
}

template<class E>
void EntityApp<E>::handleGameTick()
{
	// time_t t = ::time(NULL);
	// DEBUG_MSG("EntityApp::handleGameTick[%"PRTime"]:%u\n", t, time_);

	++g_kbetime;
	threadPool_.onMainThreadTick();
	handleTimers();
	
	{
		//AUTO_SCOPED_PROFILE("processRecvMessages");
		networkInterface().processChannels(KBEngine::Network::MessageHandlers::pMainMessageHandlers);
	}
}

template<class E>
bool EntityApp<E>::destroyEntity(ENTITY_ID entityID, bool callScript)
{
	E* entity = pEntities_->erase(entityID);
	if(entity != NULL)
	{
		static_cast<E*>(entity)->destroy(callScript);
		return true;
	}

	ERROR_MSG(fmt::format("EntityApp::destroyEntity: not found {}!\n", entityID));
	return false;
}

template<class E>
E* EntityApp<E>::findEntity(ENTITY_ID entityID)
{
	//AUTO_SCOPED_PROFILE("findEntity");
	return pEntities_->find(entityID);
}

template<class E>
void EntityApp<E>::onReqAllocEntityID(Network::Channel* pChannel, ENTITY_ID startID, ENTITY_ID endID)
{
	if(pChannel->isExternal())
		return;
	
	// INFO_MSG("EntityApp::onReqAllocEntityID: entityID alloc(%d-%d).\n", startID, endID);
	idClient_.onAddRange(startID, endID);
}

template<class E>
void EntityApp<E>::onDbmgrInitCompleted(Network::Channel* pChannel, 
						GAME_TIME gametime, ENTITY_ID startID, ENTITY_ID endID, 
						COMPONENT_ORDER startGlobalOrder, COMPONENT_ORDER startGroupOrder, 
						const std::string& digest)
{
	if(pChannel->isExternal())
		return;
	
	INFO_MSG(fmt::format("EntityApp::onDbmgrInitCompleted: entityID alloc({}-{}), startGlobalOrder={}, startGroupOrder={}, digest={}.\n",
		startID, endID, startGlobalOrder, startGroupOrder, digest));

	startGlobalOrder_ = startGlobalOrder;
	startGroupOrder_ = startGroupOrder;
	g_componentGlobalOrder = startGlobalOrder;
	g_componentGroupOrder = startGroupOrder;

	idClient_.onAddRange(startID, endID);
	g_kbetime = gametime;

	/*setEvns();*/

}

template<class E>
void EntityApp<E>::onBroadcastGlobalDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	std::string key, value;
	bool isDelete;
	
	s >> isDelete;
	s.readBlob(key);

	if(!isDelete)
	{
		s.readBlob(value);
	}

	//if(isDelete)
	//{
	//	if(pGlobalData_->del(pyKey))
	//	{
	//		// ֪ͨ�ű�
	//	}
	//}
	//else
	//{
	//	
	//}
}

template<class E>
int EntityApp<E>::tickPassedPercent(uint64 curr)
{
	// �õ���һ��tick�����������ŵ�ʱ��
	uint64 pass_stamps = (curr - lastTimestamp_) * uint64(1000) / stampsPerSecond();

	// �õ�ÿHertz�ĺ�����
	static int expected = (1000 / g_kbeSrvConfig.gameUpdateHertz());

	// �õ���ǰ���ŵ�ʱ��ռһ��ʱ�����ڵĵİٷֱ�
	return int(pass_stamps) * 100 / expected;
}

template<class E>
uint64 EntityApp<E>::checkTickPeriod()
{
	uint64 curr = timestamp();
	int percent = tickPassedPercent(curr);

	if (percent > 200)
	{
		WARNING_MSG(fmt::format("EntityApp::checkTickPeriod: tick took {0}% ({1:.2f} seconds)!\n",
					percent, (float(percent)/1000.f)));
	}

	uint64 elapsed = curr - lastTimestamp_;
	lastTimestamp_ = curr;
	return elapsed;
}


template<class E>
void EntityApp<E>::updateLoad()
{
	uint64 lastTickInStamps = checkTickPeriod();

	// ��ÿ���ʱ�����
	double spareTime = 1.0;
	if (lastTickInStamps != 0)
	{
		spareTime = double(dispatcher_.getSpareTime()) / double(lastTickInStamps);
	}

	dispatcher_.clearSpareTime();

	// �������ʱ�����С��0 ���ߴ���1�������ʱ��׼ȷ
	if ((spareTime < 0.f) || (1.f < spareTime))
	{
		if (g_timingMethod == RDTSC_TIMING_METHOD)
		{
			CRITICAL_MSG(fmt::format("EntityApp::handleGameTick: "
						"Invalid timing result {:.3f}.\n"
						"Please change the environment variable KBE_TIMING_METHOD to [rdtsc|gettimeofday|gettime](curr = {1})!",
						spareTime, getTimingMethodName()));
		}
		else
		{
			CRITICAL_MSG(fmt::format("EntityApp::handleGameTick: Invalid timing result {:.3f}.\n",
					spareTime));
		}
	}

	calcLoad((float)spareTime);
	onUpdateLoad();
}

template<class E>
void EntityApp<E>::calcLoad(float spareTime)
{
	// ���ص�ֵΪ1.0 - ����ʱ�����, ������0-1.f֮��
	float load = KBEClamp(1.f - spareTime, 0.f, 1.f);

	// �˴��㷨��server_operations_guide.pdf����loadSmoothingBias��
	// loadSmoothingBias �������θ���ȡ���һ�θ��ص�loadSmoothingBiasʣ����� + ��ǰ���ص�loadSmoothingBias����
	static float loadSmoothingBias = 0.01f;// g_kbeSrvConfig.loadSmoothingBias;
	load_ = (1 - loadSmoothingBias) * load_ + loadSmoothingBias * load;
}
}

#endif // KBE_ENTITY_APP_H
