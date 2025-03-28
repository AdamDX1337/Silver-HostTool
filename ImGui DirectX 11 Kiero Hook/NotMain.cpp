// Thisll literally never get called or built into the project, im just using this to test.
#include <Windows.h>


__int64 started;
void *pThis;
__int64 *StartGameGetCommand;

__int64 *v7; //ignore

//ignore the "TST"


char __fastcall InGameLobbyTST(__int64 a1) {

	started = 0;

	pThis = (void *)(*(__int64(__fastcall**)(DWORD))(**(DWORD**)(a1 + 80) + 128i64))(*(DWORD*)(a1 + 80));

	StartGameGetCommand = (__int64*)CommandFuncTST(41);

	v7 = StartGameGetCommand; //ignore

	if (StartGameGetCommand) {
		started = StartGameTST((__int64)StartGameGetCommand);
	}

	return SessionPostTST((__int64)pThis, started, false);

}

void* __fastcall CommandFuncTST(__int64 a1) {

}

__int64 __fastcall StartGameTST(__int64 a1) {

}

char __fastcall SessionPostTST(__int64 a1, __int64 a2, char a3) {

}