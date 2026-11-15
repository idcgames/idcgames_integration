
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <tchar.h>

#include "md5.h"
#include "CGlobal.h"
#include "Notifier.h"
#include "CLogger.h"

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			if (CGlobal::g_hInstance == NULL) {
				CGlobal::g_hInstance = hModule;
			}
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			// Critical: Cleanup resources during DLL unload
			// Prevent cleanup during abnormal termination to avoid Unity crashes
			if (lpReserved != NULL) {
				// Process is terminating - don't try to cleanup
				//CGlobal::Log("Process terminating - skipping cleanup to prevent crash");
				return TRUE;
			}

			if (CGlobal::Instance() != NULL) {
				//CGlobal::Log("DLL_PROCESS_DETACH: Starting cleanup");
				CGlobal* pGlobal = CGlobal::Instance();
				if (pGlobal->m_notifier != NULL && pGlobal->m_notifier->IsRunning()) {
					//CGlobal::Log("Stopping notifier during DLL detach");
					pGlobal->m_notifier->Stop();
					if (pGlobal->m_logger != NULL) {
						pGlobal->m_logger->Stop();
					}
					// Wait longer for threads to complete properly
					Sleep(1000);
				}
				//CGlobal::Log("Global instance cleanup completed (singleton will auto-destroy)");
				//CGlobal::Log("DLL_PROCESS_DETACH: Cleanup completed");
			}
			break;
	}
	return TRUE;
}
_declspec(dllexport) int _stdcall _InitIDCLib(const wchar_t* gameId, const wchar_t* token, unsigned long ProcessId, notification::NotifyCallBack notifyCallBack, LogCallback logCallback, bool useQueue) {
    //CGlobal::Log("[INIT] ============= INIT STARTED =============");
    //CGlobal::Log("[INIT] Time: %lld", GetTickCount64());
    //CGlobal::Log("[INIT] Thread ID: %d", GetCurrentThreadId());

	// Prevent multiple simultaneous initializations
	static volatile bool initializing = false;

	//CGlobal::Log("[INIT] Current initializing state: %s", initializing ? "TRUE" : "FALSE");

	if (initializing) {
		//CGlobal::Log("[INIT] ERROR: Already initializing from another call");
		return -10;  // Custom error code for recursive init
	}

	initializing = true;
	//CGlobal::Log("[INIT] Set initializing flag to TRUE");

	try {
	    //CGlobal::Log("[INIT] Getting CGlobal instance...");
		CGlobal* pGlobal = CGlobal::Instance();
		if (pGlobal == NULL) {
			//CGlobal::Log("[INIT] ERROR: Failed to get global instance");
			initializing = false;
			return -11;  // Failed to get global instance
		}
		//CGlobal::Log("[INIT] CGlobal instance obtained successfully");

		//CGlobal::Log("[INIT] Checking if notifier is running...");
		if (pGlobal->m_notifier->IsRunning()) {
			//CGlobal::Log("[INIT] Notifier is running, stopping services...");
			pGlobal->m_notifier->Stop();
			if (pGlobal->m_logger != NULL) {
				//CGlobal::Log("[INIT] Stopping logger...");
				pGlobal->m_logger->Stop();
			}
			//CGlobal::Log("[INIT] Waiting for services to stop...");
			Sleep(10);
			//CGlobal::Log("[INIT] Services stopped");
		} else {
			//CGlobal::Log("[INIT] Notifier was not running");
		}

		//CGlobal::Log("[INIT] Starting logger...");
		if (pGlobal->m_logger != NULL) {
			pGlobal->m_logger->Start(logCallback, useQueue);
			//CGlobal::Log("[INIT] Logger started successfully");
		} else {
			//CGlobal::Log("[INIT] WARNING: Logger is NULL");
		}

		//CGlobal::Log("[INIT] === IDCLib2 Initialization Parameters ===");
		//CGlobal::Log("[INIT] GameId: %S", gameId);
		//CGlobal::Log("[INIT] ProcessId: %d", ProcessId);
		//CGlobal::Log("[INIT] UseQueue: %s", useQueue ? "true" : "false");
		//CGlobal::Log("[INIT] Callback pointer: %p", notifyCallBack);
		//CGlobal::Log("[INIT] Log callback pointer: %p", logCallback);

		//CGlobal::Log("[INIT] Starting notifier...");
		int ret = pGlobal->m_notifier->Start(gameId, token, ProcessId, notifyCallBack, useQueue);
		//CGlobal::Log("[INIT] Notifier Start() returned: %d", ret);

		/*if(ret==0) {
			//CGlobal::Log("[INIT] Notifier started successfully");
		} else {
			//CGlobal::Log("[INIT] ERROR: Notifier failed to start with error: %d", ret);
		}*/

		initializing = false;
		//CGlobal::Log("[INIT] Reset initializing flag to FALSE");
		//CGlobal::Log("[INIT] Time at end: %lld", GetTickCount64());
		//CGlobal::Log("[INIT] ============= INIT COMPLETED =============");
		return ret;
	}
	catch (...) {
		//CGlobal::Log("Exception during initialization");
		initializing = false;
		return -12;  // Exception during init
	}
}

_declspec(dllexport) int _stdcall _EndIDCLib() {
	//CGlobal::Log("=== IDCLib2 Shutdown Started ===");
	CGlobal* pGlobal = CGlobal::Instance();
	if (pGlobal->m_notifier->IsRunning()) {
		//CGlobal::Log("Stopping notifier and logger...");
		pGlobal->m_notifier->Stop();
		pGlobal->m_logger->Stop();
		// Wait longer for threads to properly terminate
		Sleep(200);
		//CGlobal::Log("Notifier and logger stopped");
	}
	//CGlobal::Log("=== IDCLib2 Shutdown Completed ===");
	return 0;
}

_declspec(dllexport) int _stdcall _PullMessage() {
	CGlobal* pGlobal = CGlobal::Instance();
	if (pGlobal == NULL) return 0;
	if(pGlobal->m_logger!=NULL) pGlobal->m_logger->PullMessage();
	if (pGlobal->m_notifier != NULL) return pGlobal->m_notifier->PullMessage();
	return 0;
}

static bool ShopJsonString(const wchar_t* value, int maxBytes, std::string& json) {
	if (!value) value = L"";
	char utf8[256];
	if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, utf8, maxBytes, NULL, NULL))
		return false;
	json += '"';
	for (const char* p = utf8; *p; ++p) {
		if (static_cast<unsigned char>(*p) < 0x20) return false;
		if (*p == '"' || *p == '\\') json += '\\';
		json += *p;
	}
	json += '"';
	return true;
}

static int OpenShop(const wchar_t* transactionId, const wchar_t* extra, int userLevel,
	bool sandbox, const wchar_t* article, const wchar_t* language) {
	CGlobal* pGlobal = CGlobal::Instance();
	if (!pGlobal || !pGlobal->m_notifier || !pGlobal->m_notifier->IsRunning()) return FALSE;
	std::string json = "{\"transactionID\":";
	if (!ShopJsonString(transactionId, 128, json)) return -1;
	json += ",\"extra\":";
	if (!ShopJsonString(extra, 256, json)) return -1;
	json += ",\"userLevel\":" + std::to_string(userLevel);
	json += sandbox ? ",\"sandbox\":true" : ",\"sandbox\":false";
	json += ",\"article\":";
	if (!ShopJsonString(article, 256, json)) return -1;
	if (language && *language) {
		json += ",\"language\":";
		if (!ShopJsonString(language, 33, json)) return -1;
	}
	json += '}';
	return pGlobal->m_notifier->SendSecure(Command::PurchaseInit, json.c_str()) ? 0 : -1;
}

_declspec(dllexport) int __stdcall _OpenShop(const wchar_t* transactionId, const wchar_t* extra,
	int userLevel, bool sandbox, const wchar_t* article) {
	return OpenShop(transactionId, extra, userLevel, sandbox, article, nullptr);
}

_declspec(dllexport) int __stdcall _OpenShopWithLanguage(const wchar_t* transactionId, const wchar_t* extra,
	int userLevel, bool sandbox, const wchar_t* article, const wchar_t* language) {
	return OpenShop(transactionId, extra, userLevel, sandbox, article, language);
}

_declspec(dllexport) int _stdcall _ClosePurchase(const wchar_t* transactionId, const wchar_t* idcPaymentId) {
	CGlobal* pGlobal = CGlobal::Instance();
	if (!pGlobal->m_notifier->IsRunning())
		return FALSE;
	char _transactionid[128];
	WideCharToMultiByte(CP_UTF8, 0, transactionId, -1, _transactionid, 128, NULL, NULL);
	char _idcPaymentId[128];
	WideCharToMultiByte(CP_UTF8, 0, idcPaymentId, -1, _idcPaymentId, 128, NULL, NULL);
	char  buff[512];
	sprintf_s(buff, 512, "{\"transactionID\":\"%s\", \"idcPaymentId\":\"%s\"}", _transactionid, _idcPaymentId);
	if (pGlobal->m_notifier->SendSecure(Command::PurchaseEnd, buff))
		return 0;
	return -1;
}

_declspec(dllexport) void _stdcall _SetUnitySafeMode(bool enabled) {
	//CGlobal::Log("Setting Unity safe mode: %s", enabled ? "enabled" : "disabled");
	CGlobal* pGlobal = CGlobal::Instance();
	if (pGlobal && pGlobal->m_notifier) {
		pGlobal->m_notifier->SetUnitySafeMode(enabled);
	}
}

_declspec(dllexport) void _stdcall _DiagnoseCommunication() {
	//CGlobal::Log("=== DIAGNOSTIC REQUESTED FROM SHARPTESTER ===");
	CGlobal* pGlobal = CGlobal::Instance();
	if (pGlobal && pGlobal->m_notifier) {
		pGlobal->m_notifier->DiagnoseCommunication();
	} /*else {
		//CGlobal::Log("ERROR: Global instance or notifier is NULL");
	}*/
}
