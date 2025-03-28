/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////				
//							Silverhook HostTool                                                                                        //
//																																	   //
// Current Supported Version: 1.16.*																								   //
//																																	   // 
// Created to be used by hosts, best used with Hosttool mods.															     		   //
// Can tag to any nation, disable/enable ai to fix bugged nations																	   //
// If you are tagging, you have to use the number ID, so GER = 1, ENG = 2, SOV = 3 and so forth, unless its a mod that changes IDs.	   //
// If you need to get IDs in-game, you can just use cheat engine to enable debug to see country IDs.								   //
// 																																	   //
// 																																	   //
// 																																	   //
// 																																	   //
// 																																	   //
// 																																	   //
// TODOS:																															   //
// 																																	   //
//																																	   //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "includes.h"
#include "sdk.h"
#include "kiero/minhook/include/MinHook.h"
#include <vector>
#include <Windows.h>
#include <iostream>
#include <string>
#include <Psapi.h>





typedef bool(__fastcall* CSessionPost)(void* pThis, CCommand* pCommand, bool ForceSend);
CSessionPost CSessionPostHook;
CSessionPost CSessionPostTramp;

typedef CCrash* (__fastcall* GetCCrash)(void* pThis, unsigned int a1);
GetCCrash CCrashFixFunc;
GetCCrash CCrashFixTramp;


typedef CAiEnableCommand* (__fastcall* GetEnableAI)(void* pThis, int* tag, int toggled);
GetEnableAI SetAIFunc;

typedef CMultiplayerConfig* (__fastcall* GetMultiplayerConfig)(void* pThis, void* Unknown, DWORD* PlayerName, __int64 Configuration, bool isGameOwner);
GetMultiplayerConfig MultiConfigFunc;
GetMultiplayerConfig MultiConfigTramp;

typedef LPVOID(__fastcall* GetCCommand)(__int64 a1);
GetCCommand GetCCommandFunc;


typedef __int64(__fastcall* CGameStateSetPlayer)(void* pThis, int* Tag);
CGameStateSetPlayer CGameStateSetPlayerHook;
CGameStateSetPlayer CGameStateSetPlayerTramp;






bool isHost = false;

bool AllocatedConsole = false;
bool Debug = false;



//SessionPost
void* pCSession = nullptr;

//GameSetState
void* pCGameState = nullptr;


char TagBuffer[16];

//ImGui
bool bMenuOpen = true;





//Hook
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
uintptr_t GameBase = (uintptr_t)GetModuleHandleA("hoi4.exe");




uintptr_t FindPattern(char* pattern, char* mask)
{
	uintptr_t base = GameBase;
	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)GameBase, &modInfo, sizeof(MODULEINFO));
	uintptr_t size = modInfo.SizeOfImage;

	uintptr_t patternLength = (uintptr_t)strlen(mask);

	for (uintptr_t i = 0; i < size - patternLength; i++)
	{
		bool found = true;

		for (uintptr_t j = 0; j < patternLength; j++)
		{
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		if (found)
			return base + i;
	}

	return 0xDEADBEEF;
}




//Console, just for me
void printm(std::string str)
{
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);

	std::cout << "[SilverHook] " << str << std::endl;
}








	


void InitImGui()
{
	ImGui::CreateContext();
	ImGui::StyleColorsLight();
	
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesCyrillic()); // For Russian (UNUSED NOW)
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
	


}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			InitImGui();
			ImGui::SetNextWindowSize(ImVec2(350, 1050));
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	if (GetAsyncKeyState(VK_INSERT) & 1)
		bMenuOpen = !bMenuOpen;




	if (bMenuOpen)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureMouse = true;
		io.WantCaptureKeyboard = true;

		if (GetAsyncKeyState(VK_LBUTTON))
		{
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
		}
		else
		{
			io.MouseReleased[0] = true;
			io.MouseDown[0] = false;
			io.MouseClicked[0] = false;
		}





		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Host Tool: 1.16.*", &bMenuOpen);

		if (isHost)
			ImGui::Text("You are host");
		else
			ImGui::Text("You are NOT Host");


		ImGui::Text("Function Calls");
		ImGui::Text("");
		ImGui::Columns(2);
		if (ImGui::Button("Disable", ImVec2(140, 28)) && pCSession != nullptr) {
			if (isHost) {
				CCrash* Disable = (CCrash*)GetCCommandFunc(48);
				Disable = CCrashFixFunc(Disable, 0);
				CSessionPostTramp(pCSession, Disable, true);
			}
		}
		ImGui::NextColumn();
		if (ImGui::Button("Enable", ImVec2(140, 28)) && pCSession != nullptr) {
			if (isHost) {
				CCrash* Enable = (CCrash*)GetCCommandFunc(48);
				Enable = CCrashFixFunc(Enable, 28671);
				CSessionPostTramp(pCSession, Enable, true);
			}
				
		}
		ImGui::Columns(1);
		//ImGui::SameLine();
		//ImGui::Text("Mem Addresses");
		/*ImGui::Text("");
		ImGui::Columns(2);

		if (ImGui::Button("FOW", ImVec2(140, 28)) && pCSession != nullptr)
		{
			if (isHost)
				ChangeByteAddressValue(FOWAddr);
		}
		ImGui::NextColumn();
		if (ImGui::Button("AllowTraits", ImVec2(140, 28)) && pCSession != nullptr)
		{
			if (isHost)
				ChangeByteAddressValue(ALWTRAddr);
		}
		ImGui::NextColumn();
		if (ImGui::Button("Debug", ImVec2(140, 28)) && pCSession != nullptr)
		{
			if (isHost)
				ChangeByteAddressValue(DBGAddr);
		}

		ImGui::Columns(1);

		#############################################################################################################################################################################################
		######################################																												   ######################################
		######################################                               THIS SHIZZ DESYNCS HOST ! DONT USE ! ! !										   ######################################
		###################################### Also, if you want to use this, youll need to make your own function to change the address values, I removed it. ######################################
		######################################																												   ######################################
		#############################################################################################################################################################################################

		*/
		ImGui::Text("Country Tag:");
		ImGui::Text("");
		ImGui::SetNextItemWidth(70.f);
		ImGui::InputText("", TagBuffer, IM_ARRAYSIZE(TagBuffer));
		ImGui::SameLine();

		if (ImGui::Button("Enable AI") && pCSession != nullptr)
		{

			std::string sTagBuffer = TagBuffer;
			char* endptr;
			int a1 = std::strtol(TagBuffer, &endptr, 10);
			
			if (isHost)	
				if (sTagBuffer.length() > 0)
				{
					int* TagPtr = &a1;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = SetAIFunc(EnableAI, TagPtr, 2);
					CSessionPostTramp(pCSession, EnableAI, 1);
				}

		}

		ImGui::SameLine();
		if (ImGui::Button("Disable AI") && pCSession != nullptr)
		{

			std::string sTagBuffer = TagBuffer;
			char* endptr;
			int a1 = std::strtol(TagBuffer, &endptr, 10);

			if (isHost)
				if (sTagBuffer.length() > 0)
				{

					int* TagPtr = &a1;
					CAiEnableCommand* DisableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					DisableAI = SetAIFunc(DisableAI, TagPtr, 0);
					CSessionPostTramp(pCSession, DisableAI, 1);
				}

		}

		if (ImGui::Button("Tagswitch") && pCSession != nullptr)
		{
			std::string sTagBuffer = TagBuffer;

			if (isHost)
				if (sTagBuffer.length() > 0)
				{
					int Tag = std::stoi(sTagBuffer);
					int* pTag = &Tag;
					if (pCGameState != nullptr)
						CGameStateSetPlayerTramp(pCGameState, pTag);

				}
		}	

		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			memset(TagBuffer, 0, sizeof(TagBuffer));
		}

		ImGui::Text("");
		ImGui::Text("Credits: Silver");
	

		if (Debug) {
			if (!AllocatedConsole) {
				AllocConsole();
				AllocatedConsole = true;
			}
		}
		


		ImGui::End();
		ImGui::Render();

		pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	
	return oPresent(pSwapChain, SyncInterval, Flags);
}





bool __fastcall hkCSessionPost(void* pThis, CCommand* pCommand, bool ForceSend)
{
	
	pCSession = pThis;
	if (Debug) {

		printm("CSP Called");
	}
	return CSessionPostTramp(pThis, pCommand, ForceSend);
}



__int64 __fastcall hkCGameStateSetPlayer(void* pThis, int* Tag)
{

	pCGameState = pThis;
	if (Debug) {
		printm("CGSSP Called");
	}
	return CGameStateSetPlayerTramp(pThis, Tag);
	
}

CCrash* __fastcall hkCrash(void* pThis, unsigned int a1) {

	if (Debug) {
		printm("Crash Called");
		std::string x = std::to_string(a1);
		printm(x);
	}


	return CCrashFixTramp(pThis, a1);
}

bool calculateHostStatus(int value) {
	return ((value * 13 + 7) % 2) == 1;
}

CMultiplayerConfig* __fastcall hkMultiConfig(void* pThis, void* Unknown, DWORD* PlayerName, __int64 Configuration, bool isGameOwner) {

	if (Debug) {
		printm("Config Called");

	}

	if (isGameOwner) {
		isHost = calculateHostStatus(isGameOwner ? 42 : 99);
	}
	else {
		isHost = calculateHostStatus(isGameOwner ? 42 : 99);
	}

	return MultiConfigTramp(pThis, Unknown, PlayerName, Configuration, isGameOwner);
}



void HookFunctions() {

	

	CSessionPostHook = CSessionPost(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xD9\x45\x84\xC0"), const_cast <char*>("xxxx?xxxx????xxxxxxxxx")));
	MH_CreateHook(CSessionPostHook, &hkCSessionPost, (LPVOID*)&CSessionPostTramp);
	MH_EnableHook(CSessionPostHook);

	CGameStateSetPlayerHook = CGameStateSetPlayer(FindPattern(const_cast <char*>("\x40\x56\x57\x41\x57\x48\x83\xEC\x00\x8B\x02"), const_cast <char*>("xxxxxxxx?xx")));

	MH_CreateHook(CGameStateSetPlayerHook, &hkCGameStateSetPlayer, (LPVOID*)&CGameStateSetPlayerTramp);
	MH_EnableHook(CGameStateSetPlayerHook);
	
	CCrashFixFunc = GetCCrash(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x8B\xDA\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xC9\x89\x5F\x00\x48\x8B\x5C\x24\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x07\x48\x8B\xC7\x66\xC7\x47\x00\x00\x00\xC7\x47\x00\x00\x00\x00\x00\x89\x4F\x00\xC7\x47\x00\x00\x00\x00\x00\x66\x89\x4F\x00\x48\x89\x4F\x00\x48\x83\xC4\x00\x5F\xC3\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x48\x8D\x05"), const_cast <char*>("xxxx?xxxx?xxxxxx????xxxx?xxxx?xxx????xxxxxxxxx???xx?????xx?xx?????xxx?xxx?xxx?xxxxxx?xxxx?xxx")));
	MH_CreateHook(CCrashFixFunc, &hkCrash, (LPVOID*)&CCrashFixTramp);
	MH_EnableHook(CCrashFixFunc);
	
	MultiConfigFunc = GetMultiplayerConfig(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x4C\x24\x00\x56\x57\x41\x56\x48\x83\xEC\x00\x49\x8B\xF1\x49\x8B\xF8\x48\x8B\xDA\x48\x8B\xE9"), const_cast <char*>("xxxx?xxxx?xxxx?xxxxxxx?xxxxxxxxxxxx")));
	MH_CreateHook(MultiConfigFunc, &hkMultiConfig, (LPVOID*)&MultiConfigTramp);
	MH_EnableHook(MultiConfigFunc);

	GetCCommandFunc = GetCCommand(FindPattern(const_cast <char*>("\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53\x48\x83\xEC\x00\x33\xC0"), const_cast <char*>("x????xxxxxxxxxxxx????xxxxxxxxxxxxxxxx?xx")));
	
	SetAIFunc = GetEnableAI(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x00\x41\x8B\xF0\x48\x8B\xDA\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xC9\x66\xC7\x47\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x89\x4F\x00\x48\x89\x07\x66\x89\x4F\x00\x48\x89\x4F\x00\xC7\x47\x00\x00\x00\x00\x00\xC7\x47\x00\x00\x00\x00\x00\x8B\x03\x48\x8B\x5C\x24\x00\x89\x47\x00\x48\x8B\xC7\x89\x77\x00\x48\x8B\x74\x24\x00\x48\x83\xC4\x00\x5F\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x48\x8B\xF9"), const_cast <char*>("xxxx?xxxx?xxxx?xxxxxxxxxx????xxxxx???xxx????xx?xxxxxx?xxx?xx?????xx?????xxxxxx?xx?xxxxx?xxxx?xxx?xxxxxxxxxxxxxxxxxxx?xxxx?xxx")));

}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)& oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);

	HookFunctions();
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);


		break;
	}
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}