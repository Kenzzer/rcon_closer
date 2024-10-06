#pragma once

#include <cstdint>

#ifdef _WINDOWS
#include <winsock.h>
#else
#include <unistd.h>
#define closesocket close
#endif

typedef std::int32_t SocketHandle_t;

class CSocketCreator {
public:
	SocketHandle_t* GetHandle() const { return (SocketHandle_t*)((std::uint8_t*)this + m_hListenSocket); };
	void Close() {
		auto handle = GetHandle();
		if (*handle != -1) {
			g_pSM->LogMessage(myself, "Closed RCON Socket %i", *handle);
			closesocket(*handle);
		}
		*handle = -1;
		// Drop all rcon connections, always
		(this->*fn_CloseAllAcceptedSockets)();
	}

	void* m_pListener;
	static int m_hListenSocket;
	static void (CSocketCreator::*fn_CloseAllAcceptedSockets)();
};

class CRCONServer {
public:
	void** vtable;
	CSocketCreator m_Socket;
};