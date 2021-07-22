//#include <iostream>

using namespace std;

#include <stdio.h>
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

void send_inp(int8_t code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = 0; // hardware scan code for key
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;

	// Press the "A" key
	inp.ki.wVk = code; // virtual-key code for the "a" key
	inp.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));

	// Release the "A" key
	inp.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &inp, sizeof(INPUT));
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




	printf("Escape on 360 guide button v2\nby pinumbernumber\nNow listening for player 1 guide button.\nYou can minimise this console window now.\n\n");

	uint8_t whichKey[4];

	{
		handy::io::INIFile ini;
		ini.loadFile("button.ini");
		whichKey[0] = ini.getInteger("config", "key_player1", 27);
		whichKey[1] = ini.getInteger("config", "key_player2", 27);
		whichKey[2] = ini.getInteger("config", "key_player3", 27);
		whichKey[3] = ini.getInteger("config", "key_player4", 27);
	}

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

					printf("player %i pressed!\n", i+1);
					send_inp(whichKey[i]);

				}

				wasPressedPrev[i] = true;
			}
			else
			{
				wasPressedPrev[i] = false;
			}
		}


		Sleep(33);

	}
}
