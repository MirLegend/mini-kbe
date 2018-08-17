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



#ifndef KBE_BASE_H
#define KBE_BASE_H
	
#include "common/common.h"
#include "helper/debug_helper.h"
#include "server/entity_app.h"
/*#include "server/script_timers.h"	*/	
	
namespace KBEngine{

//class EntityMailbox;
class BaseMessagesForwardCellappHandler;
class BaseMessagesForwardClientHandler;

namespace Network
{
class Channel;
}


class Base
{
protected:
	ENTITY_ID										id_;
	bool											isDestroyed_;
	uint32											flags_;

public:
	Base(ENTITY_ID id, bool isInitialised = true);
	~Base();

	/** 
		是否存储数据库 
	*/
	INLINE bool hasDB() const;
	INLINE void hasDB(bool has);

	/** 
		数据库关联ID
	*/
	INLINE DBID dbid() const;
	INLINE void dbid(DBID id);

	void addPersistentsDataToStream(uint32 flags, MemoryStream* s);

	/** 
		写备份信息到流
	*/
	void writeBackupData(MemoryStream* s);
	void onBackup();

	/** 
		写存档信息到流
	*/
	void writeArchiveData(MemoryStream* s);

	/** 
		将要保存到数据库之前的通知 
	*/
	void onWriteToDB();

	void onWriteToDBCallback(ENTITY_ID eid, DBID entityDBID,
		CALLBACK_ID callbackID, int8 shouldAutoLoad, bool success);

	/** 网络接口
		entity第一次写数据库由dbmgr返回的dbid
	*/
	void onGetDBID(Network::Channel* pChannel, DBID dbid);

	/** 
		当cellapp意外终止后， baseapp如果能找到合适的cellapp则将其恢复后
		会调用此方法
	*/
	void onRestore();

	void writeToDB(void* data, void* extra1 = NULL, void* extra2 = NULL);

	/** 
		客户端丢失 
	*/
	void onClientDeath();

	///** 网络接口
	//	远程呼叫本entity的方法 
	//*/
	//void onRemoteMethodCall(Network::Channel* pChannel, MemoryStream& s);

	/** 
		销毁这个entity 
	*/
	void onDestroy(bool callScript);

	virtual void InitDatas(const std::string datas) {};

	/**
		销毁base内部通知
	*/
	void onDestroyEntity(bool deleteFromDB, bool writeToDB);
	
	INLINE bool inRestore();
	
	/**
		设置获取是否自动存档
	*/
	INLINE int8 shouldAutoArchive() const;
	INLINE void shouldAutoArchive(int8 v);

	/**
		设置获取是否自动备份
	*/
	INLINE int8 shouldAutoBackup() const;
	INLINE void shouldAutoBackup(int8 v);

	/** 
		转发消息完成 
	*/
	void onBufferedForwardToCellappMessagesOver();
	void onBufferedForwardToClientMessagesOver();
	
	INLINE BaseMessagesForwardClientHandler* pBufferedSendToClientMessages();
	
	/** 
		设置实体持久化数据是否已脏，脏了会自动存档 
	*/
	INLINE void setDirty(bool dirty = true);
	INLINE bool isDirty() const;

	INLINE ENTITY_ID id() const	
	{
		return id_;		
	}
		
		INLINE void id(ENTITY_ID v)		
	{					
		id_ = v; 
	}
		
		INLINE bool hasFlags(uint32 v) const
	{
		return (flags_ & v) > 0;
	}	
		
		INLINE uint32 flags() const	
	{
		return flags_;	
	}

		INLINE void flags(uint32 v)	
	{
		flags_ = v; 
	}
		
		INLINE uint32 addFlags(uint32 v)
	{
		flags_ |= v;	
		return flags_;
	}
		INLINE uint32 removeFlags(uint32 v)	
	{	
		flags_ &= ~v; 
		return flags_;
	}

	void destroy(bool callScript = true)
	{
		if (hasFlags(ENTITY_FLAGS_DESTROYING))
			return;
		if (!isDestroyed_)
		{
			isDestroyed_ = true;
			addFlags(ENTITY_FLAGS_DESTROYING);
			onDestroy(callScript);
			removeFlags(ENTITY_FLAGS_DESTROYING);
		}
	}

	INLINE bool isDestroyed() const
	{
		return isDestroyed_;
	}

	void destroyEntity();
	
protected:
	/**
		从db擦除在线log
	*/
	void eraseEntityLog();

protected:
	// 这个entity的客户端mailbox cellapp mailbox
	/*EntityMailbox*							clientMailbox_;
	EntityMailbox*							cellMailbox_;*/

	// 是否是存储到数据库中的entity
	bool									hasDB_;
	DBID									DBID_;

	// 是否正在获取celldata中
	bool									isGetingCellData_;

	// 是否正在存档中
	bool									isArchiveing_;

	// 是否进行自动存档 <= 0为false, 1为true, KBE_NEXT_ONLY为执行一次后自动为false
	int8									shouldAutoArchive_;
	
	// 是否进行自动备份 <= 0为false, 1为true, KBE_NEXT_ONLY为执行一次后自动为false
	int8									shouldAutoBackup_;

	// 是否正在创建cell中
	bool									creatingCell_;

	// 是否已经创建了一个space
	bool									createdSpace_;

	// 是否正在恢复
	bool									inRestore_;
	
	// 如果此时实体还没有被设置为ENTITY_FLAGS_TELEPORT_START,  说明onMigrationCellappArrived包优先于
	// onMigrationCellappStart到达(某些压力所致的情况下会导致实体跨进程跳转时（由cell1跳转到cell2），
	// 跳转前所产生的包会比cell2的enterSpace包慢到达)，因此发生这种情况时需要将cell2的包先缓存
	// 等cell1的包到达后执行完毕再执行cell2的包
	BaseMessagesForwardClientHandler*		pBufferedSendToClientMessages_;
	
	// 需要持久化的数据是否变脏，如果没有变脏不需要持久化
	bool									isDirty_;

	// 如果这个实体已经写到数据库，那么这个属性就是对应的数据库接口的索引
	uint16									dbInterfaceIndex_;
};

}


#ifdef CODE_INLINE
#include "base.inl"
#endif

#endif // KBE_BASE_H
