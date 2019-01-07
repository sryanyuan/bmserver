#ifndef _INC_SSERVERCONN_
#define _INC_SSERVERCONN_
//////////////////////////////////////////////////////////////////////////
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <string>
#include "SServerBuffer.h"
#include "Def.h"
//////////////////////////////////////////////////////////////////////////
class SServerEngine;
//////////////////////////////////////////////////////////////////////////
enum SServerConnStateType
{
	kSServerConnState_None,
	kSServerConnState_Connecting,
	kSServerConnState_Connected,
	kSServerConnState_ConnectFailed,
	kSServerConnState_Disconnected
};
//////////////////////////////////////////////////////////////////////////
struct sockaddr_in;

struct AddressInfo {
	std::string strIP;
	unsigned short uPort;

	AddressInfo() {
		uPort = 0;
	}

	void SetAddress(const sockaddr_in *_pAddr);
};
//////////////////////////////////////////////////////////////////////////
#define SSERVERCONN_FLAG	BEV_OPT_CLOSE_ON_FREE
//////////////////////////////////////////////////////////////////////////
class SServerConn
{
	friend class SServerEngine;

public:
	SServerConn();
	~SServerConn();

public:
	void SetAddress(const sockaddr_in* _pAddr);
	const AddressInfo* GetAddress();
	bool GetAddress(char* _pBuffer, unsigned short* _pPort);

	void Callback_OnConnectSuccess();
	void Callback_OnConnectFailed();

private:
	void readHead();
	void readBody();

private:
	SServerEngine* pEng;
	bufferevent* pEv;
	unsigned int uConnIndex;
	evutil_socket_t fd;
	bool bServerConn;
	SServerConnStateType eConnState;

	unsigned int m_uPacketHeadLength;

	SServerBuffer m_xReadBuffer;

	AddressInfo m_stAddress;

	//	for connect to server
	FUNC_ONCONNECTFAILED m_fnOnConnectFailed;
	FUNC_ONCONNECTSUCCESS m_fnOnConnectSuccess;
	void* m_pConnectResultArg;
};
//////////////////////////////////////////////////////////////////////////
#endif