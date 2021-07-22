//#include <iostream>

using namespace std;

//#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#include "INI.hpp"

typedef struct
{
	WORD  wButtons;
	BYTE  bLeftTrigger;
	BYTE  bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT xThumbRX;
	SHORT xThumbRY;
} XINPUT_GAMEPAD;

typedef struct
{
	DWORD          dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE;

#include <chrono>
#include <thread>

void send_inp(int8_t code, int64_t delay, int64_t duration)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = 0; // hardware scan code for key
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;

	std::this_thread::sleep_for(std::chrono::milliseconds(delay));

	// Press the key
	inp.ki.wVk = code; // virtual-key code for the key
	inp.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));

	std::this_thread::sleep_for(std::chrono::milliseconds(duration));

	// Release the key
	inp.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &inp, sizeof(INPUT));
}

void send_inp_down(int8_t code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = 0; // hardware scan code for key
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;

	inp.ki.wVk = code; // virtual-key code for the "a" key
	inp.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));


}

void send_inp_up(int8_t code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = 0; // hardware scan code for key
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;

	inp.ki.wVk = code; // virtual-key code for the "a" key
	inp.ki.dwFlags = KEYEVENTF_KEYUP; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));
}

HWND GetConsoleHwnd(void)
{
   #define MY_BUFSIZE 1024 // Buffer size for console window titles.
   HWND hwndFound;         // This is what is returned to the caller.
   char pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
									   // WindowTitle.
   char pszOldWindowTitle[MY_BUFSIZE]; // Contains original
									   // WindowTitle.

   // Fetch current window title.

   GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

   // Format a "unique" NewWindowTitle.

   wsprintf(pszNewWindowTitle,"%d/%d",
			   GetTickCount(),
			   GetCurrentProcessId());

   // Change current window title.

   SetConsoleTitle(pszNewWindowTitle);

   // Ensure window title has been updated.

   Sleep(40);

   // Look for NewWindowTitle.

   hwndFound=FindWindow(NULL, pszNewWindowTitle);

   // Restore original window title.

   SetConsoleTitle(pszOldWindowTitle);

   return(hwndFound);
}

int main()
{
	TCHAR dll_path[MAX_PATH];
	GetSystemDirectory(dll_path, sizeof(dll_path));
	strcat(dll_path, "\\xinput1_3.dll");
	HINSTANCE xinputDll = LoadLibrary(dll_path);

	typedef DWORD (__stdcall *XInputGetStateEx_t) (DWORD, XINPUT_STATE*);

	XInputGetStateEx_t XInputGetStateEx = (XInputGetStateEx_t) GetProcAddress(xinputDll, (LPCSTR)100);

	XINPUT_STATE playerStates[4];
	for (auto& state : playerStates)
		ZeroMemory(&state, sizeof(XINPUT_STATE));

//
	bool wasPressedPrev[4] = {false,false,false,false};




	printf("Escape on 360 guide button v5\nby pinumbernumber\nNow listening for player 1 guide button.\nThis window will now minimise.\n\n");

	uint8_t whichKey[4];
	int64_t delays[4];
	int64_t durations[4];
	bool    shouldHold[4];

	int64_t minimiseDelay = 0;

	{
		handy::io::INIFile ini;
		ini.loadFile("button.ini");

		whichKey[0] = ini.getInteger("config", "key_player1", 27);
		whichKey[1] = ini.getInteger("config", "key_player2", 27);
		whichKey[2] = ini.getInteger("config", "key_player3", 27);
		whichKey[3] = ini.getInteger("config", "key_player4", 27);

		delays[0] = ini.getInteger("config", "delay_player1", 0);
		delays[1] = ini.getInteger("config", "delay_player2", 0);
		delays[2] = ini.getInteger("config", "delay_player3", 0);
		delays[3] = ini.getInteger("config", "delay_player4", 0);

		durations[0] = ini.getInteger("config", "duration_player1", 0);
		durations[1] = ini.getInteger("config", "duration_player2", 0);
		durations[2] = ini.getInteger("config", "duration_player3", 0);
		durations[3] = ini.getInteger("config", "duration_player4", 0);

		shouldHold[0] = (ini.getInteger("config", "hold_player1", 0) == 1);
		shouldHold[1] = (ini.getInteger("config", "hold_player2", 0) == 1);
		shouldHold[2] = (ini.getInteger("config", "hold_player3", 0) == 1);
		shouldHold[3] = (ini.getInteger("config", "hold_player4", 0) == 1);
	}

	Sleep(minimiseDelay);
	auto hwnd = GetConsoleHwnd();
	ShowWindow(hwnd, SW_MINIMIZE);





	for (auto& key : whichKey)
		if (key > 255)
			key = 27;

	for (;;)
	{
		for (int i = 0; i < 4; i++)
		{
			XInputGetStateEx(i, &(playerStates[i]));
			if (playerStates[i].Gamepad.wButtons & 0x0400)
			{
				if (!wasPressedPrev[i])
				{

					printf("Player %i pressed!\n", i+1);

					if(shouldHold[i])
					{
						send_inp_down(whichKey[i]);
					}
					else
					{
						send_inp(whichKey[i], delays[i], durations[i]);
					}


				}

				wasPressedPrev[i] = true;
			}
			else
			{
				if ((shouldHold[i]) && (wasPressedPrev[i]))
				{
					send_inp_up(whichKey[i]);
				}
				wasPressedPrev[i] = false;
			}
		}

		// 33
		Sleep(50);

	}
}
