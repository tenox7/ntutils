//======================================================================
//
// Native.h
//
// Mark Russinovich
// http://www.ntinternals.com
//
// This file includes the definitions required by the Native.exe sample
// NT native program to do what it does. 
//
//======================================================================

//
// Environment information, which includes command line and
// image file name
//
typedef struct {
       ULONG            Unknown[21];     
       UNICODE_STRING   CommandLine;
       UNICODE_STRING   ImageFile;
} ENVIRONMENT_INFORMATION, *PENVIRONMENT_INFORMATION;

//
// This structure is passed as NtProcessStartup's parameter
//
typedef struct {
       ULONG                     Unknown[3];
       PENVIRONMENT_INFORMATION  Environment;
} STARTUP_ARGUMENT, *PSTARTUP_ARGUMENT;

//
// Data structure for heap definition. This includes various
// sizing parameters and callback routines, which, if left NULL,
// result in default behavior
//
typedef struct {
	ULONG    	Length;
	ULONG    	Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

//
// Native NT api function to write something to the boot-time
// blue screen
//
NTSTATUS 
NTAPI 
NtDisplayString(
		PUNICODE_STRING String 
		);

//
// Native applications must kill themselves when done - the job
// of this native API
//
NTSTATUS 
NTAPI 
NtTerminateProcess(
		   HANDLE ProcessHandle, 
		   LONG ExitStatus 
		   );

//
// Definition to represent current process
//
#define NtCurrentProcess() ( (HANDLE) -1 )

//
// Heap creation routine
//
HANDLE 
NTAPI 
RtlCreateHeap(
	      ULONG Flags, 
	      PVOID BaseAddress, 
	      ULONG SizeToReserve, 
	      ULONG SizeToCommit, 
	      PVOID Unknown,
	      PRTL_HEAP_DEFINITION Definition
	      );

//
// Heap allocation function (ala "malloc")
//
PVOID 
NTAPI 
RtlAllocateHeap(
		HANDLE Heap, 
		ULONG Flags, 
		ULONG Size 
		);

//
// Heap free function (ala "free")
//
BOOLEAN 
NTAPI 
RtlFreeHeap(
	    HANDLE Heap, 
	    ULONG Flags, 
	    PVOID Address 
	    );
