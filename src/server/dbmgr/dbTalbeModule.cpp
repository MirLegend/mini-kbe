#include "dbTalbeModule.h"


namespace KBEngine{
namespace DBTalbeModule {
//-------------------------------------------------------------------------------------
	DBTABLEDEFS tabelDefs;

	bool LoadDBTableDefs()
	{
		tabelDefs.clear();
		std::string sysResPath = Resmgr::getSingleton().getPySysResPath();
		std::string tablesFile = sysResPath + "dbtables.xml";
		std::string defTablePath = sysResPath + "dbtable/";
		//SmartPointer<XML> xml(new XML(Resmgr::getSingleton().matchRes("dbtables.xml").c_str()));
		// �����dbtables.xml�ļ�
		SmartPointer<XML> xml(new XML());
		if (!xml->openSection(tablesFile.c_str()))
			return false;

		if (!xml->isGood())
		{
			ERROR_MSG(fmt::format("ServerConfig::loadConfig: load {} is failed!\n", tablesFile.c_str()));
			return false;
		}

		// ���entities.xml���ڵ�, ���û�ж���һ��entity��ôֱ�ӷ���true
		TiXmlNode* node = xml->getRootNode();
		if (node == NULL)
			return false;

		// ��ʼ�������е�table�ڵ�
		XML_FOR_BEGIN(node)
		{
			std::string tableName = xml.get()->getKey(node); //��ȡ������
			SmartPointer<XML> defxml(new XML());
			std::string deftablefile = defTablePath + tableName + ".xml";
			if (!defxml->openSection(deftablefile.c_str()))
				return false;

			TiXmlNode* defNode = defxml->getRootNode();
			if (defNode == NULL)
			{
				// root�ڵ���û���ӽڵ���
				return false;
			}

			DBTABLEITEMS& tableItems = tabelDefs[tableName];
			// ����def�ļ��еĶ���
			if (!loadTableDefInfo(defTablePath, tableName, defxml.get(), defNode, tableItems))
			{
				ERROR_MSG(fmt::format("DBMgrApp::LoadDBTableDefs: failed to load table:{}, filepath:{}.\n",
					tableName.c_str(), deftablefile.c_str()));

				return false;
			}
		}
		XML_FOR_END(node);
		return true;
	}

	bool loadTableDefInfo(std::string defTablePath, std::string tableName, XML* defxml, TiXmlNode* node, DBTABLEITEMS& tableItems)
	{
		int32 utype = 1;
		XML_FOR_BEGIN(node)
		{
			stTableItem sti;
			sti.utype = utype++;
			std::string columnName = defxml->getKey(node); //��ȡ������
														   //DEBUG_MSG(fmt::format("DBMgrApp::loadTableDefInfo columnName:{} \n", columnName.c_str()));
			strncpy(sti.tblItemName, columnName.c_str(), sizeof(sti.tblItemName));
			TiXmlNode* pColumnNode = defxml->enterNode(node, columnName.c_str());
			if (pColumnNode == NULL)
				return false;

			TiXmlNode* pTypeNode = defxml->enterNode(pColumnNode, "Type");
			if (pTypeNode == NULL)
				return false;
			strncpy(sti.tblItemType, defxml->getValStr(pTypeNode).c_str(), sizeof(sti.tblItemType));

			sti.length = 0;
			TiXmlNode* pLengthNode = defxml->enterNode(pColumnNode, "DBLength");
			if (pLengthNode != NULL)
				sti.length = defxml->getValInt(pLengthNode);
			sti.flag = 0;
			TiXmlNode* pFlagNode = defxml->enterNode(pColumnNode, "Flags");
			if (pFlagNode != NULL)
				sti.flag = defxml->getValInt(pFlagNode);
			sti.indexType[0] = 0;
			TiXmlNode* pIndexNode = defxml->enterNode(pColumnNode, "IndexType");
			if (pIndexNode != NULL)
				strncpy(sti.indexType, defxml->getValStr(pIndexNode).c_str(), sizeof(sti.indexType));
			sti.defaultValue[0] = 0;
			TiXmlNode* pDefaultNode = defxml->enterNode(pColumnNode, "Default");
			if (pDefaultNode != NULL)
				kbe_snprintf(sti.defaultValue, sizeof(sti.defaultValue), "DEFAULT '%s' ", defxml->getValStr(pDefaultNode).c_str());
			//strncpy(sti.defaultValue, defxml->getValStr(pDefaultNode).c_str(), sizeof(sti.defaultValue));
			DEBUG_MSG(fmt::format("DBMgrApp::loadTableDefInfo: tablename:{} itemname:{} flag:{} type:{} length:{} indextp:{} default:{}.\n",
				tableName.c_str(), sti.tblItemName, sti.flag, sti.tblItemType, sti.length, sti.indexType, sti.defaultValue));
			tableItems[/*std::string(*/sti.tblItemName/*)*/] = sti;
		}
		XML_FOR_END(node);
		return true;
	}
}
}
