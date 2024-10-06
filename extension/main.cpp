#include "main.hpp"
#include "rconserver.hpp"
#include <CDetour/detours.h>

RCONCloser gExt;
SMEXT_LINK(&gExt);

CRCONServer* (*RCONServer)() = nullptr;
void (CSocketCreator::*CSocketCreator::fn_CloseAllAcceptedSockets)() = nullptr;
int CSocketCreator::m_hListenSocket = 0;

CDetour* gDetour2 = nullptr;
DETOUR_DECL_MEMBER0(CSocketCreator_ProcessAccept, void) {
	g_pSM->LogMessage(myself, "Socket creator: 0x%p", this);
	DETOUR_MEMBER_CALL(CSocketCreator_ProcessAccept)();
}

CDetour* gDetour = nullptr;
// Technically a struct is passed by reference but we can assume its a ptr
DETOUR_DECL_MEMBER1(CSocketCreator_CreateListenSocket, bool, void*, ptr) {
	auto socketCreator = (CSocketCreator*)this;

	if (((CRCONServer*)socketCreator->m_pListener) == (*RCONServer)()) {
		// Never allow the opening of the socket
		return false;
	}
	return DETOUR_MEMBER_CALL(CSocketCreator_CreateListenSocket)(ptr);
}

void Frame_Hook(bool simulating) {
	// Keep the RCON socket closed
	(*RCONServer)()->m_Socket.Close();
}

bool RCONCloser::SDK_OnLoad(char* error, size_t maxlength, bool late) {
	IGameConfig* conf;
	if (!gameconfs->LoadGameConfigFile("rcon_closer", &conf, error, maxlength)) {
		return false;
	}

	if (!conf->GetMemSig("RCONServer", reinterpret_cast<void **>(&RCONServer))) {
		g_pSM->LogMessage(myself, "Couldn't locate function RCONServer!");
		return false;
	}

	if (!conf->GetMemSig("CSocketCreator::CloseAllAcceptedSockets", reinterpret_cast<void **>(&CSocketCreator::fn_CloseAllAcceptedSockets))) {
		g_pSM->LogMessage(myself, "Couldn't locate function CSocketCreator::CloseAllAcceptedSockets!");
		return false;
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), conf);

	int offset = -1;
	if (conf->GetOffset("RCONServer", &offset) && offset != -1) {
		// We didn't actually retrieve RCONServer but a function calling it
		auto adjust = (std::uint8_t*)RCONServer;
		adjust += offset;

		std::int32_t relativeOffset = *(std::int32_t*)adjust;
		// Offset to the subroutine
		adjust += sizeof(std::int32_t) + relativeOffset;
		RCONServer = (decltype(RCONServer))adjust;
	}

	if (!conf->GetOffset("CSocketCreator::m_hListenSocket", &CSocketCreator::m_hListenSocket) || CSocketCreator::m_hListenSocket == 0) {
		g_pSM->LogMessage(myself, "Couldn't locate find CSocketCreator::m_hListenSocket offset!");
		return false;
	}

	g_pSM->LogMessage(myself, "RCON Server: 0x%p", (*RCONServer)());
	g_pSM->LogMessage(myself, "RCON Server(Socket): 0x%p", &(*RCONServer)()->m_Socket);
	g_pSM->LogMessage(myself, "RCON Server(Socket->Listener): 0x%p", (*RCONServer)()->m_Socket.m_pListener);
	g_pSM->LogMessage(myself, "RCON Server(Socket->Handle): %i", *((*RCONServer)()->m_Socket.GetHandle()));

	// Setup detour
	gDetour = DETOUR_CREATE_MEMBER(CSocketCreator_CreateListenSocket, "CSocketCreator::CreateListenSocket");
	if (gDetour) {
		gDetour->EnableDetour();
	}

	/*gDetour2 = DETOUR_CREATE_MEMBER(CSocketCreator_ProcessAccept, "CSocketCreator::ProcessAccept");
	if (gDetour2) {
		g_pSM->LogMessage(myself, "CSocketCreator::ProcessAccept");
		gDetour2->EnableDetour();
	}*/

	// Close the RCON socket immediately
	(*RCONServer)()->m_Socket.Close();
	// In the event the detour fails (missing signature), we also register a frame hook
	g_pSM->AddGameFrameHook(&Frame_Hook);

	// Release the config file, we don't need it anymore
	gameconfs->CloseGameConfigFile(conf);
	return true;
}

void RCONCloser::SDK_OnUnload() {
	if (gDetour) {
		gDetour->DisableDetour();
	}
	if (gDetour2) {
		gDetour2->DisableDetour();
	}
	g_pSM->RemoveGameFrameHook(&Frame_Hook);
}