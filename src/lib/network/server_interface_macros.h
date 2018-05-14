
#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_SERVER_INTERFACE_MACRO_H
#endif


#ifndef KBE_SERVER_INTERFACE_MACRO_H
#define KBE_SERVER_INTERFACE_MACRO_H

// common include	
#include "network/interface_defs.h"
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

/**
	消息宏，  参数为流， 需要自己解开
*/
#if defined(NETWORK_INTERFACE_DECLARE_BEGIN)
#undef SERVER_MESSAGE_HANDLER_STREAM
#endif

#if defined(DEFINE_IN_INTERFACE)

#if defined(MYSERVER)
#define SERVER_MESSAGE_HANDLER_STREAM(DOMAIN, NAME)									\
	void NAME##DOMAIN##Messagehandler_stream::handle(Network::Channel* pChannel,\
													KBEngine::MemoryStream& s)	\
	{																			\
			KBEngine::DOMAIN::getSingleton().NAME(pChannel, s);				\
	}																			\

#else
#define SERVER_MESSAGE_HANDLER_STREAM(DOMAIN, NAME)									\
	void NAME##DOMAIN##Messagehandler_stream::handle(Network::Channel* pChannel,\
													KBEngine::MemoryStream& s)	\
	{																			\
	}																			\

#endif
#else
#define SERVER_MESSAGE_HANDLER_STREAM(DOMAIN, NAME)									\
	class NAME##DOMAIN##Messagehandler_stream : public Network::MessageHandler	\
	{																			\
	public:																		\
		virtual void handle(Network::Channel* pChannel,							\
							KBEngine::MemoryStream& s);							\
	};																			\

#endif

#define SERVER_MESSAGE_DECLARE_STREAM(DOMAIN, MSGID, NAME, MSG_LENGTH)						\
	SERVER_MESSAGE_HANDLER_STREAM(DOMAIN, NAME)										\
	NETWORK_MESSAGE_DECLARE_STREAM(DOMAIN, MSGID, NAME,								\
				NAME##DOMAIN##Messagehandler_stream, MSG_LENGTH)				\
																				\


}
#endif
