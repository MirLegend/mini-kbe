#include "dbmgr.h"
#include "db_interface/entity_table.h"
//#define NDEBUG
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{
	namespace DBTalbeModule {
		extern DBTABLEDEFS tabelDefs;

		bool LoadDBTableDefs();
		bool loadTableDefInfo(std::string defTablePath, std::string tableName, XML* defxml, TiXmlNode* node, DBTABLEITEMS& tableItems);
	}
}
