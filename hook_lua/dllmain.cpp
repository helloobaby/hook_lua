#include "pch.h"
#include "Sig.hpp"
#include "util.hpp"
#include <string>

#include <MinHook.h>
#include <lua.h>
#include <thread>

using namespace std;

int DetourluaL_loadfile (lua_State *L, const char *filename);
int DetourluaL_loadbuffer (lua_State *L,const char *buff,size_t sz,const char *name);
decltype(&DetourluaL_loadfile) OriluaL_loadfile;
decltype(&DetourluaL_loadbuffer) OriluaL_loadbuffer;
wchar_t *Curl_convert_UTF8_to_wchar(const char *str_utf8);


__declspec(dllexport) void ijtdummy() {

}


bool wrap(HMODULE hModule);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
      return wrap(hModule);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

bool wrap(HMODULE hModule) {
  GetModuleFileNameW(NULL, exepath, 512);
  wstring wsPath = exepath;
  // 不是目标进程直接退出,并且卸载DLL
  if (wsPath.find(L"dontstarve_steam_x64.exe") == wstring::npos) {
    std::thread unloaddll([&]() { FreeLibraryAndExitThread(hModule,0); });
    unloaddll.detach();
    return true;
  }

  OutputDebug(L"[64]dll loaded into %ws\n", exepath);

  PVOID ExeBase = (PVOID)GetModuleHandleW(NULL);
  OutputDebug(L"[64]Exe Base %p\n", ExeBase);
  const void* found = Sig::find(ExeBase, 7500800, "40 53 56 57 48 81 EC 40");

  const void* atluaL_loadbuffer =
      Sig::find(ExeBase, 7500800, "40 57 48 83 EC 30 4C 8B D1 48 83 C9 FF 48");

  if (!found) {
    OutputDebug(L"[64]Cant find luaL_loadfile\n");
    return true;
  } else {
    OutputDebug(L"[64]luaL_loadfile %p\n", found);
  }

  if (!atluaL_loadbuffer) {
    OutputDebug(L"[64]Cant find luaL_loadbuffer\n");
    return true;
  } else {
    OutputDebug(L"[64]luaL_loadbuffer %p\n", atluaL_loadbuffer);
  }

  MH_STATUS Status = MH_Initialize();
  if (Status != MH_OK) {
    OutputDebug(L"[64]Cant Init MinHook\n");
    return true;
  }

  MH_CreateHook((LPVOID)found, DetourluaL_loadfile, (LPVOID*)&OriluaL_loadfile);
  MH_CreateHook((LPVOID)atluaL_loadbuffer, DetourluaL_loadbuffer,
                (LPVOID*)&OriluaL_loadbuffer);

  MH_EnableHook(MH_ALL_HOOKS);

  return true;
}
int DetourluaL_loadfile(lua_State* L, const char* filename) {
  // OutputDebug(L"[64]%s", filename);
  return OriluaL_loadfile(L, filename);
}

int DetourluaL_loadbuffer(lua_State* L, const char* buff, size_t sz,
                          const char* name) {
  wchar_t* wp = Curl_convert_UTF8_to_wchar(buff);
  OutputDebug(L"[64]lua buff %ws\n",wp);

  free(wp);
  return OriluaL_loadbuffer(L, buff, sz, name);
}


wchar_t *Curl_convert_UTF8_to_wchar(const char *str_utf8) {
  wchar_t *str_w = NULL;

  if (str_utf8) {
    int str_w_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str_utf8,
                                        -1, NULL, 0);
    if (str_w_len > 0) {
      str_w = (wchar_t*)malloc(str_w_len * sizeof(wchar_t));
      if (str_w) {
        if (MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, str_w, str_w_len) ==
            0) {
          free(str_w);
          return NULL;
        }
      }
    }
  }

  return str_w;
}