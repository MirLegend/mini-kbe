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

#ifndef KBE_ENTITYGARBAGES_H
#define KBE_ENTITYGARBAGES_H
	
// common include	
#include "helper/debug_helper.h"
#include "common/common.h"
#include "common/smartpointer.h"
	
namespace KBEngine{

template<typename T>
class EntityGarbages
{
public:
	typedef KBEUnordered_map<ENTITY_ID, /*PyObject*/void*> ENTITYS_MAP;

	EntityGarbages():
	_entities(),
	_lastTime(0)
	{			
	}

	~EntityGarbages()
	{
		if(size() > 0)
		{
			ERROR_MSG(fmt::format("EntityGarbages::~EntityGarbages(): leaked, size={}.\n", 
				size()));

			int i = 0;

			ENTITYS_MAP::iterator iter = _entities.begin();
			for(; iter != _entities.end(); ++iter)
			{
				if(i++ >= 256)
					break;

				/*	ERROR_MSG(fmt::format("\t--> leaked: {}({}).\n",
						iter->second->ob_type->tp_name, iter->first));*/
			}
		}

		finalise();
	}	

	void finalise()
	{
		clear();
	}

	ENTITYS_MAP& getEntities(void){ return _entities; }

	void add(ENTITY_ID id, T* entity);
	void clear();
	void erase(ENTITY_ID id);

	T* find(ENTITY_ID id);

	size_t size() const { return _entities.size(); }
		
private:
	ENTITYS_MAP _entities;
	uint64 _lastTime;
};

//-------------------------------------------------------------------------------------
template<typename T>
void EntityGarbages<T>::add(ENTITY_ID id, T* entity)
{ 
	ENTITYS_MAP::const_iterator iter = _entities.find(id);
	if(iter != _entities.end())
	{
		ERROR_MSG(fmt::format("EntityGarbages::add: entityID:{} has exist\n.", id));
		return;
	}

	if(_entities.size() == 0)
	{
		_lastTime = timestamp();
	}
	else
	{
		// X秒内没有清空过garbages则错误警告
		if(_lastTime > 0 && timestamp() - _lastTime > uint64(stampsPerSecond()) * 3600)
		{
			// 再未清空情况下，下次不提示了
			_lastTime = 0;
			
			ERROR_MSG(fmt::format("For a long time(3600s) not to empty the garbages, there may be a leak of the entitys(size:{}), "
				"please use the \"KBEngine.entities.garbages.items()\" command query!\n", 
				size()));
		}
	}
	
	_entities[id] = entity; 
}

//-------------------------------------------------------------------------------------
template<typename T>
void EntityGarbages<T>::clear()
{
	_entities.clear();
	_lastTime = 0;
}

//-------------------------------------------------------------------------------------
template<typename T>
T* EntityGarbages<T>::find(ENTITY_ID id)
{
	ENTITYS_MAP::const_iterator iter = _entities.find(id);
	if(iter != _entities.end())
	{
		return static_cast<T*>(iter->second);
	}
	
	return NULL;
}

//-------------------------------------------------------------------------------------
template<typename T>
void EntityGarbages<T>::erase(ENTITY_ID id)
{
	_entities.erase(id);
	
	if(_entities.size() == 0)
		_lastTime = 0;
}

}
#endif // KBE_ENTITYGARBAGES_H

