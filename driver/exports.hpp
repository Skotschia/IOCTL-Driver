#pragma once
#include <ntifs.h>

#ifdef max
#undef max
#endif

extern "C" __declspec( dllimport ) NTSTATUS NTAPI MmCopyVirtualMemory( PEPROCESS FromProcess , PVOID FromAddress , PEPROCESS ToProcess , PVOID ToAddress , SIZE_T BufferSize , KPROCESSOR_MODE PreviousMode , PSIZE_T NumberOfBytesCopied );
extern "C" __declspec( dllimport ) PVOID NTAPI PsGetProcessSectionBaseAddress( PEPROCESS Process );