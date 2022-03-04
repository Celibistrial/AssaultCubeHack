#include "pch.h"
#include <stdio.h>
#include <vector>





bool callHook(LPVOID addrToHook, LPVOID redirectFunc, int len) {
    if (len < 5) {
        return false;
    }
    DWORD oldProtection;
    VirtualProtect(addrToHook, len, PAGE_EXECUTE_READWRITE, &oldProtection);
    memset(addrToHook, 0x90, len);     
    uintptr_t relativeAddress = ((uintptr_t)redirectFunc - (uintptr_t)addrToHook) - 5;
    *(BYTE*)addrToHook = 0xE9;
    *(uintptr_t*)((uintptr_t)addrToHook + 1) = relativeAddress;
    VirtualProtect(addrToHook, len, oldProtection,NULL);
    return true;
}
DWORD jumpBackAddrAmmo;
DWORD jumpBackAddrHealth;
uintptr_t* ammopointer;
uintptr_t* healthpointer;
bool isAmmoPointerInitialized = false;
bool isHealthPointerInitialized = false;

char* patternScan(char* pattern, char* mask, char* begin, unsigned int size)
{
    unsigned int patternLength = strlen(mask);

    for (unsigned int i = 0; i < size - patternLength; i++)
    {
        bool found = true;
        for (unsigned int j = 0; j < patternLength; j++)
        {
            if (mask[j] != '?' && pattern[j] != *(begin + i + j))
            {
                found = false;
                break;
            }
        }
        if (found)
        {
            return (begin + i);
        }
    }
    return nullptr;
}
void writeToMemory(uintptr_t address, char* Value, int bytes) {
    DWORD oldProtection;
    VirtualProtect((LPVOID)address, bytes, PAGE_EXECUTE_READWRITE, &oldProtection);
    memcpy((LPVOID)address, Value, bytes);
    VirtualProtect((LPVOID)address, bytes, oldProtection, NULL);
}
void __declspec(naked) ammoHookFunc() {
    isAmmoPointerInitialized = true;
    __asm {
        mov ammopointer,eax
        dec [eax]
        lea eax, [esp + 0x1C]
        jmp [jumpBackAddrAmmo]
    }
}
void __declspec(naked) healthHookFunc() {
    isHealthPointerInitialized = true;
    __asm {
       
        push eax
        lea eax,[edx +0x000000EC]
        mov healthpointer,eax
        pop eax
        jmp[jumpBackAddrHealth]
    }
}

 


DWORD WINAPI MainThread(HMODULE hModule) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONOUT$", "w", stdout);
   
    int hookLengthAmmo = 6;
    int hookLengthHealth = 6;
    uintptr_t ac_addr = (uintptr_t)GetModuleHandle(L"ac_client.exe");
    uintptr_t ammo_addr = (uintptr_t)patternScan((char*)"\x8b\x46\x00\x89\x08\x8b\x46\x00\xff\x08", (char*)"xx?xxxx?xx", (char*)ac_addr, 2);
    uintptr_t health_addr = (uintptr_t)patternScan((char*)"\x89\x82\x00\x00\x00\x00\x0f\x94", (char*)"xx????xx", (char*)ac_addr, 6);
    ammo_addr += 8;

    jumpBackAddrAmmo = ammo_addr + hookLengthAmmo;
    jumpBackAddrHealth = health_addr + hookLengthHealth;
    callHook((LPVOID)ammo_addr, ammoHookFunc, hookLengthAmmo);
    callHook((LPVOID)health_addr, healthHookFunc, hookLengthHealth);
    while (!GetAsyncKeyState(VK_END)) {
        if(isAmmoPointerInitialized)
            *ammopointer = 1300;
        if (isHealthPointerInitialized)
            *healthpointer = 1300;
       
        Sleep(500);
    }
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
    
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr,0,(LPTHREAD_START_ROUTINE)MainThread,hModule,0,nullptr));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}




