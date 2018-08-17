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

#ifndef KBE_ENTITIES_H
#define KBE_ENTITIES_H
	
// common include	
#include "helper/debug_helper.h"
#include "common/common.h"
#include "common/smartpointer.h"
#include "entity_garbages.h"
//#include "entitydef/entity_garbages.h"
	
namespace KBEngine{

template<typename T>
class Entities/* : public script::ScriptObject*/
{
public:
	typedef KBEUnordered_map<ENTITY_ID, T*> ENTITYS_MAP;

	Entities():
	_pGarbages(new EntityGarbages<T>())
	{			
	}

	~Entities()
	{
		finalise();
		/*S_RELEASE*/SAFE_RELEASE(_pGarbages);
	}	

	void finalise()
	{
		clear(false);
	}
	
	ENTITYS_MAP& getEntities(void) { return _entities; }

	void add(ENTITY_ID id, T* entity);
	void clear(bool callScript);
	void clear(bool callScript, std::vector<ENTITY_ID> excludes);

	T* erase(ENTITY_ID id);
	T* find(ENTITY_ID id);

	size_t size() const { return _entities.size(); }

	EntityGarbages<T>* pGetbages() { return _pGarbages; }

private:
	ENTITYS_MAP _entities;

	// �Ѿ����ù�destroy��δ������entity�����洢����� ��ʱ��δ������˵��
	// �ű����п��ܴ���ѭ�����õ���������ڴ�й¶��
	EntityGarbages<T>* _pGarbages;
};

//-------------------------------------------------------------------------------------
template<typename T>
void Entities<T>::add(ENTITY_ID id, T* entity)
{ 
	ENTITYS_MAP::const_iterator iter = _entities.find(id);
	if(iter != _entities.end())
	{
		ERROR_MSG(fmt::format("Entities::add: entityID:{} has exist\n.", id));
		return;
	}

	_entities[id] = entity; 
}

//-------------------------------------------------------------------------------------
template<typename T>
T* Entities<T>::erase(ENTITY_ID id)
{
	ENTITYS_MAP::iterator iter = _entities.find(id);
	if (iter != _entities.end())
	{
		T* entity = static_cast<T*>(iter->second/*.get()*/);
		_pGarbages->add(id, entity);
		_entities.erase(iter);
		return entity;
	}

	return NULL;
}

//-------------------------------------------------------------------------------------
template<typename T>
void Entities<T>::clear(bool callScript)
{
	ENTITYS_MAP::const_iterator iter = _entities.begin();
	while (iter != _entities.end())
	{
		T* entity = (T*)iter->second/*.get()*/;
		_pGarbages->add(entity->id(), entity);
		entity->destroy(callScript);
		iter++;
	}

	_entities.clear();
}

//-------------------------------------------------------------------------------------
template<typename T>
void Entities<T>::clear(bool callScript, std::vector<ENTITY_ID> excludes)
{
	ENTITYS_MAP::const_iterator iter = _entities.begin();
	for (;iter != _entities.end();)
	{
		if(std::find(excludes.begin(), excludes.end(), iter->first) != excludes.end())
		{
			++iter;
			continue;
		}

		T* entity = (T*)iter->second/*.get()*/;
		_pGarbages->add(entity->id(), entity);
		entity->destroy(callScript);
		_entities.erase(iter++);
	}
	
	// ���ڴ���excludes�������
	// _entities.clear();
}

//-------------------------------------------------------------------------------------
template<typename T>
T* Entities<T>::find(ENTITY_ID id)
{
	ENTITYS_MAP::const_iterator iter = _entities.find(id);
	if(iter != _entities.end())
	{
		return static_cast<T*>(iter->second/*.get()*/);
	}
	
	return NULL;
}

}
#endif // KBE_ENTITIES_H

