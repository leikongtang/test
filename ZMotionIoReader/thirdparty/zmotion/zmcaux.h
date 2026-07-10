#ifndef ZMCAUX_H
#define ZMCAUX_H

#include "zmotion.h"

#ifdef _WIN32
    #ifdef ZMCAUX_EXPORTS
        #define ZMCAUX_API __declspec(dllexport) __stdcall
    #else
        #define ZMCAUX_API __declspec(dllimport) __stdcall
    #endif
#else
    #define ZMCAUX_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

int32_t ZMCAUX_API ZAux_OpenEth(char *ipaddr, ZMC_HANDLE *phandle);
int32_t ZMCAUX_API ZAux_Close(ZMC_HANDLE handle);

int32_t ZMCAUX_API ZAux_Direct_GetIn(ZMC_HANDLE handle, int ionum, uint32_t *piValue);
int32_t ZMCAUX_API ZAux_Direct_GetInMulti(ZMC_HANDLE handle, int ionumfirst, int ionumend, int32_t *piValue);
int32_t ZMCAUX_API ZAux_Direct_GetOp(ZMC_HANDLE handle, int ionum, uint32_t *piValue);
int32_t ZMCAUX_API ZAux_Direct_GetOutMulti(ZMC_HANDLE handle, int ionumfirst, int ionumend, uint32_t *piValue);
int32_t ZMCAUX_API ZAux_Direct_SetOp(ZMC_HANDLE handle, int ionum, int iValue);
int32_t ZMCAUX_API ZAux_Direct_SetOutMulti(ZMC_HANDLE handle, int ionumfirst, int ionumend, int32_t iValue);
int32_t ZMCAUX_API ZAux_GetModbusIn(ZMC_HANDLE handle, int ionumfirst, int ionumend, uint8_t *pValueList);

#ifdef __cplusplus
}
#endif

#endif // ZMCAUX_H
