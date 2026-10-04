#include "../../idlib/precompiled.h"

#include <cpuid.h>
#include <float.h>
#include <mmsystem.h>
#include <xmmintrin.h>
#include <x86intrin.h>
#include <stdio.h>

struct cpuInfo_t {
	int processorPackageCount;
	int processorCoreCount;
	int logicalProcessorCount;
	int numaNodeCount;
	struct cacheInfo_t {
		int count;
		int associativity;
		int lineSize;
		int size;
	} cacheLevel[3];
};

static bool GetCPUInfo( cpuInfo_t &cpuInfo ) {
	memset( &cpuInfo, 0, sizeof( cpuInfo ) );

	DWORD byteCount = 0;
	GetLogicalProcessorInformation( NULL, &byteCount );
	if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER || byteCount == 0 ) {
		SYSTEM_INFO systemInfo;
		GetSystemInfo( &systemInfo );
		cpuInfo.processorPackageCount = 1;
		cpuInfo.processorCoreCount = systemInfo.dwNumberOfProcessors;
		cpuInfo.logicalProcessorCount = systemInfo.dwNumberOfProcessors;
		return false;
	}

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION information =
		(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc( byteCount );
	if ( information == NULL ) {
		return false;
	}

	if ( !GetLogicalProcessorInformation( information, &byteCount ) ) {
		free( information );
		return false;
	}

	const DWORD entryCount = byteCount / sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
	for ( DWORD i = 0; i < entryCount; ++i ) {
		const SYSTEM_LOGICAL_PROCESSOR_INFORMATION &entry = information[i];
		switch ( entry.Relationship ) {
			case RelationProcessorCore:
				++cpuInfo.processorCoreCount;
				cpuInfo.logicalProcessorCount += __builtin_popcountll(
					(unsigned long long)entry.ProcessorMask );
				break;
			case RelationProcessorPackage:
				++cpuInfo.processorPackageCount;
				break;
			case RelationCache:
				if ( entry.Cache.Level >= 1 && entry.Cache.Level <= 3 ) {
					cpuInfo_t::cacheInfo_t &cache = cpuInfo.cacheLevel[entry.Cache.Level - 1];
					if ( cache.count == 0 ) {
						cache.associativity = entry.Cache.Associativity;
						cache.lineSize = entry.Cache.LineSize;
						cache.size = entry.Cache.Size;
					}
					++cache.count;
				}
				break;
			case RelationNumaNode:
				++cpuInfo.numaNodeCount;
				break;
			default:
				break;
		}
	}

	free( information );
	return cpuInfo.logicalProcessorCount > 0;
}

double Sys_GetClockTicks() {
	return (double)__rdtsc();
}

double Sys_ClockTicksPerSecond() {
	static double ticksPerSecond = 0.0;
	if ( ticksPerSecond == 0.0 ) {
		LARGE_INTEGER qpcFrequency;
		LARGE_INTEGER qpcStart;
		LARGE_INTEGER qpcEnd;
		if ( QueryPerformanceFrequency( &qpcFrequency )
				&& QueryPerformanceCounter( &qpcStart ) ) {
			const unsigned __int64 tscStart = __rdtsc();
			Sleep( 50 );
			const unsigned __int64 tscEnd = __rdtsc();
			if ( QueryPerformanceCounter( &qpcEnd ) && qpcEnd.QuadPart > qpcStart.QuadPart ) {
				ticksPerSecond = (double)( tscEnd - tscStart ) * qpcFrequency.QuadPart
					/ (double)( qpcEnd.QuadPart - qpcStart.QuadPart );
			}
		}
		if ( ticksPerSecond == 0.0 ) {
			ticksPerSecond = 1000000000.0;
		}
	}
	return ticksPerSecond;
}

void Sys_GetCPUCacheSize( int level, int &count, int &size, int &lineSize ) {
	assert( level >= 1 && level <= 3 );
	cpuInfo_t cpuInfo;
	GetCPUInfo( cpuInfo );
	count = cpuInfo.cacheLevel[level - 1].count;
	size = cpuInfo.cacheLevel[level - 1].size;
	lineSize = cpuInfo.cacheLevel[level - 1].lineSize;
}

void Sys_CPUCount( int &logicalProcessorCount, int &physicalCoreCount, int &packageCount ) {
	cpuInfo_t cpuInfo;
	if ( !GetCPUInfo( cpuInfo ) ) {
		SYSTEM_INFO systemInfo;
		GetSystemInfo( &systemInfo );
		logicalProcessorCount = systemInfo.dwNumberOfProcessors;
		physicalCoreCount = logicalProcessorCount;
		packageCount = 1;
		return;
	}
	logicalProcessorCount = cpuInfo.logicalProcessorCount;
	physicalCoreCount = cpuInfo.processorCoreCount;
	packageCount = cpuInfo.processorPackageCount;
}

cpuid_t Sys_GetCPUId() {
	unsigned int eax = 0;
	unsigned int ebx = 0;
	unsigned int ecx = 0;
	unsigned int edx = 0;
	const unsigned int maxLeaf = __get_cpuid_max( 0, NULL );
	if ( maxLeaf == 0 || !__get_cpuid( 1, &eax, &ebx, &ecx, &edx ) ) {
		return CPUID_UNSUPPORTED;
	}

	char vendor[13];
	__get_cpuid( 0, &eax, &ebx, &ecx, &edx );
	memcpy( vendor, &ebx, 4 );
	memcpy( vendor + 4, &edx, 4 );
	memcpy( vendor + 8, &ecx, 4 );
	vendor[12] = '\0';
	__get_cpuid( 1, &eax, &ebx, &ecx, &edx );

	cpuid_t flags = CPUID_GENERIC;
	if ( strcmp( vendor, "AuthenticAMD" ) == 0 ) {
		flags = (cpuid_t)( flags | CPUID_AMD );
	} else if ( strcmp( vendor, "GenuineIntel" ) == 0 ) {
		flags = (cpuid_t)( flags | CPUID_INTEL );
	}

	if ( edx & bit_MMX ) {
		flags = (cpuid_t)( flags | CPUID_MMX );
	}
	if ( edx & bit_SSE ) {
		flags = (cpuid_t)( flags | CPUID_SSE | CPUID_FTZ );
	}
	if ( edx & bit_SSE2 ) {
		flags = (cpuid_t)( flags | CPUID_SSE2 );
	}
	if ( ecx & bit_SSE3 ) {
		flags = (cpuid_t)( flags | CPUID_SSE3 );
	}
	if ( edx & bit_CMOV ) {
		flags = (cpuid_t)( flags | CPUID_CMOV );
	}
	if ( edx & ( 1u << 28 ) ) {
		flags = (cpuid_t)( flags | CPUID_HTT );
	}

	if ( __get_cpuid_max( 0x80000000, NULL ) >= 0x80000001 ) {
		__get_cpuid( 0x80000001, &eax, &ebx, &ecx, &edx );
		if ( edx & (1u << 31) ) {
			flags = (cpuid_t)( flags | CPUID_3DNOW );
		}
	}

	flags = (cpuid_t)( flags | CPUID_DAZ );
	return flags;
}

int Sys_FPU_PrintStateFlags( char *buffer, int control, int status, int tag, int instructionOffset,
		int instructionSelector, int operandOffset, int operandSelector ) {
	return sprintf( buffer,
		"FPU Control Word: 0x%04x\nFPU Status Word: 0x%04x\nFPU Tag Word: 0x%04x\n"
		"Instruction: %04x:%08x\nOperand: %04x:%08x\n",
		control & 0xffff, status & 0xffff, tag & 0xffff,
		instructionSelector & 0xffff, instructionOffset, operandSelector & 0xffff, operandOffset );
}

bool Sys_FPU_StackIsEmpty() {
	struct __attribute__((packed)) {
		unsigned int control;
		unsigned int status;
		unsigned int tag;
		unsigned int instructionOffset;
		unsigned int selectorAndOpcode;
		unsigned int operandOffset;
		unsigned int operandSelector;
	} environment;

	__asm__ __volatile__( "fnstenv %0" : "=m"( environment ) );
	__asm__ __volatile__( "fldenv %0" : : "m"( environment ) );
	return ( environment.tag & 0xffff ) == 0xffff;
}

void Sys_FPU_ClearStack() {
	__asm__ __volatile__( "fninit" );
}

const char *Sys_FPU_GetState() {
	static char state[128];
	unsigned int control;
	unsigned int status;
	unsigned int tag;
	struct __attribute__((packed)) {
		unsigned int control;
		unsigned int status;
		unsigned int tag;
		unsigned int instructionOffset;
		unsigned int selectorAndOpcode;
		unsigned int operandOffset;
		unsigned int operandSelector;
	} environment;
	__asm__ __volatile__( "fnstenv %0" : "=m"( environment ) );
	__asm__ __volatile__( "fldenv %0" : : "m"( environment ) );
	control = environment.control;
	status = environment.status;
	tag = environment.tag;
	Sys_FPU_PrintStateFlags( state, control, status, tag, 0, 0, 0, 0 );
	return state;
}

void Sys_FPU_EnableExceptions( int exceptions ) {
	(void)exceptions;
}

void Sys_FPU_SetPrecision( int precision ) {
	(void)precision;
}

void Sys_FPU_SetRounding( int rounding ) {
	(void)rounding;
}

void Sys_FPU_SetDAZ( bool enable ) {
	unsigned int control = _mm_getcsr();
	control = enable ? ( control | (1u << 6) ) : ( control & ~(1u << 6) );
	_mm_setcsr( control );
}

void Sys_FPU_SetFTZ( bool enable ) {
	unsigned int control = _mm_getcsr();
	control = enable ? ( control | (1u << 15) ) : ( control & ~(1u << 15) );
	_mm_setcsr( control );
}
