#ifndef ZMOTION_H
#define ZMOTION_H

#include <stdint.h>

#ifdef _WIN32
    #ifdef ZMOTION_EXPORTS
        #define ZMOTION_API __declspec(dllexport) __stdcall
    #else
        #define ZMOTION_API __declspec(dllimport) __stdcall
    #endif
#else
    #define ZMOTION_API
#endif

typedef void *ZMC_HANDLE;

#define ERR_OK 0

#ifdef __cplusplus
extern "C" {
#endif

int32_t ZMOTION_API ZMC_OpenEth(char *ipaddr, ZMC_HANDLE *phandle);
int32_t ZMOTION_API ZMC_Close(ZMC_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif // ZMOTION_H
