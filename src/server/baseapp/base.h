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
		�Ƿ�洢���ݿ� 
	*/
	INLINE bool hasDB() const;
	INLINE void hasDB(bool has);

	/** 
		���ݿ����ID
	*/
	INLINE DBID dbid() const;
	INLINE void dbid(DBID id);

	void addPersistentsDataToStream(uint32 flags, MemoryStream* s);

	/** 
		д������Ϣ����
	*/
	void writeBackupData(MemoryStream* s);
	void onBackup();

	/** 
		д�浵��Ϣ����
	*/
	void writeArchiveData(MemoryStream* s);

	/** 
		��Ҫ���浽���ݿ�֮ǰ��֪ͨ 
	*/
	void onWriteToDB();

	void onWriteToDBCallback(ENTITY_ID eid, DBID entityDBID,
		CALLBACK_ID callbackID, int8 shouldAutoLoad, bool success);

	/** ����ӿ�
		entity��һ��д���ݿ���dbmgr���ص�dbid
	*/
	void onGetDBID(Network::Channel* pChannel, DBID dbid);

	/** 
		��cellapp������ֹ�� baseapp������ҵ����ʵ�cellapp����ָ���
		����ô˷���
	*/
	void onRestore();

	void writeToDB(void* data, void* extra1 = NULL, void* extra2 = NULL);

	/** 
		�ͻ��˶�ʧ 
	*/
	void onClientDeath();

	///** ����ӿ�
	//	Զ�̺��б�entity�ķ��� 
	//*/
	//void onRemoteMethodCall(Network::Channel* pChannel, MemoryStream& s);

	/** 
		�������entity 
	*/
	void onDestroy(bool callScript);

	virtual void InitDatas(const std::string datas) {};

	/**
		����base�ڲ�֪ͨ
	*/
	void onDestroyEntity(bool deleteFromDB, bool writeToDB);
	
	INLINE bool inRestore();
	
	/**
		���û�ȡ�Ƿ��Զ��浵
	*/
	INLINE int8 shouldAutoArchive() const;
	INLINE void shouldAutoArchive(int8 v);

	/**
		���û�ȡ�Ƿ��Զ�����
	*/
	INLINE int8 shouldAutoBackup() const;
	INLINE void shouldAutoBackup(int8 v);

	/** 
		ת����Ϣ��� 
	*/
	void onBufferedForwardToCellappMessagesOver();
	void onBufferedForwardToClientMessagesOver();
	
	INLINE BaseMessagesForwardClientHandler* pBufferedSendToClientMessages();
	
	/** 
		����ʵ��־û������Ƿ����࣬���˻��Զ��浵 
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
		��db��������log
	*/
	void eraseEntityLog();

protected:
	// ���entity�Ŀͻ���mailbox cellapp mailbox
	/*EntityMailbox*							clientMailbox_;
	EntityMailbox*							cellMailbox_;*/

	// �Ƿ��Ǵ洢�����ݿ��е�entity
	bool									hasDB_;
	DBID									DBID_;

	// �Ƿ����ڻ�ȡcelldata��
	bool									isGetingCellData_;

	// �Ƿ����ڴ浵��
	bool									isArchiveing_;

	// �Ƿ�����Զ��浵 <= 0Ϊfalse, 1Ϊtrue, KBE_NEXT_ONLYΪִ��һ�κ��Զ�Ϊfalse
	int8									shouldAutoArchive_;
	
	// �Ƿ�����Զ����� <= 0Ϊfalse, 1Ϊtrue, KBE_NEXT_ONLYΪִ��һ�κ��Զ�Ϊfalse
	int8									shouldAutoBackup_;

	// �Ƿ����ڴ���cell��
	bool									creatingCell_;

	// �Ƿ��Ѿ�������һ��space
	bool									createdSpace_;

	// �Ƿ����ڻָ�
	bool									inRestore_;
	
	// �����ʱʵ�廹û�б�����ΪENTITY_FLAGS_TELEPORT_START,  ˵��onMigrationCellappArrived��������
	// onMigrationCellappStart����(ĳЩѹ�����µ�����»ᵼ��ʵ��������תʱ����cell1��ת��cell2����
	// ��תǰ�������İ����cell2��enterSpace��������)����˷����������ʱ��Ҫ��cell2�İ��Ȼ���
	// ��cell1�İ������ִ�������ִ��cell2�İ�
	BaseMessagesForwardClientHandler*		pBufferedSendToClientMessages_;
	
	// ��Ҫ�־û��������Ƿ���࣬���û�б��಻��Ҫ�־û�
	bool									isDirty_;

	// ������ʵ���Ѿ�д�����ݿ⣬��ô������Ծ��Ƕ�Ӧ�����ݿ�ӿڵ�����
	uint16									dbInterfaceIndex_;
};

}


#ifdef CODE_INLINE
#include "base.inl"
#endif

#endif // KBE_BASE_H
