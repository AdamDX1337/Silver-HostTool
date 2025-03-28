#include <windows.h>
#include <iostream>

struct CPersistentVtbl
{
	/*void* (__fastcall* __vecDelDtor)(unsigned int);
	void(__fastcall* Write)(CWriter*);
	void(__fastcall* WriteMembers)(CWriter*);
	void(__fastcall* Read)(CReader*);
	void(__fastcall* ReadMember)(CReader*, int);
	void(__fastcall* InitPostRead)(CString*, int, int);
	void(__fastcall* InitPostRead)();*/
};

struct CPersistent
{
	CPersistentVtbl* vfptr;
	int _TypeToken; //357
};

struct CCommand : CPersistent
{
	bool _bLogged; //0
	int _nCommandSender; //-1
	unsigned int _nRecipient; //0
	unsigned __int16 _nRecipientPort; //-65536
	unsigned __int16 _nTickStamp;
	bool _bTargetedAsynchronous; //0
	bool _bMayBeRemoved;
	unsigned int _nIdentity; //0
};


struct CCrash : CCommand
{
	unsigned int _nCrash;
};



struct CAiEnableCommand : CCommand
{

};


struct CMultiplayerConfig {
	void* _Name;
	DWORD* _a2;
	void* _SessionConfig;
	bool _IsGameOwner;

};

