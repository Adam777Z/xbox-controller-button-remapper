

#include <string>

#include <chrono>
#include <thread>

#include "INI.hpp"

using namespace std;

#include <windows.h>
#include "Signal.hpp"



void fatal_error(const std::string&& text)
{
	MessageBox(NULL, text.c_str(), "Fatal Error", MB_OK);
	std::terminate();
}

void key_down(int8_t code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = 0; // hardware scan code for key
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;

	inp.ki.wVk = code; // virtual-key code
	inp.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));
}

void key_up(int8_t code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = 0; // hardware scan code for key
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;

	inp.ki.wVk = code; // virtual-key code
	inp.ki.dwFlags = KEYEVENTF_KEYUP; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));
}


void key_tap(int8_t code, int64_t delay, int64_t duration)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	key_down(code);
	std::this_thread::sleep_for(std::chrono::milliseconds(duration));
	key_up(code);
}

HWND get_console_hwnd(void)
{
   static const auto titleBufferLen = 1024;

   // Fetch current window title.
   char oldWindowTitle[titleBufferLen];
   GetConsoleTitle(oldWindowTitle, titleBufferLen);

   // Format a "unique" NewWindowTitle.
   char tempWindowTitle[titleBufferLen];
   wsprintf(tempWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

   // Change current window title.
   SetConsoleTitle(tempWindowTitle);

   // Ensure window title has been updated.
   Sleep(40);

   // Look for NewWindowTitle.
   auto hwndFound = FindWindow(NULL, tempWindowTitle);

   // Restore original window title.
   SetConsoleTitle(oldWindowTitle);

   return(hwndFound);
}



struct XINPUT_GAMEPAD
{
	WORD  wButtons;
	BYTE  bLeftTrigger;
	BYTE  bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT xThumbRX;
	SHORT xThumbRY;
};

struct XINPUT_STATE
{
	DWORD          dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
};

struct PlayerSettings
{
	int64_t key;
	int64_t hold_mode;
	int64_t longpress_key;
	int64_t longpress_duration;
	int64_t delay;
};

bool inline is_guide_down(XINPUT_STATE& state)
{
	return state.Gamepad.wButtons & 0x0400;
}

void print(int8_t v)
{
	printf("%i", (int)v);
}

void print(uint8_t v)
{
	printf("%u", (unsigned int)v);
}

void print(int16_t v)
{
	printf("%i", (int)v);
}

void print(uint16_t v)
{
	printf("%u", (unsigned int)v);
}

template <typename T>
void println(T v)
{
	print(v);
	printf("\n");

}

struct GlobalData
{

	PlayerSettings settings[4];
	handy::arch::Signal<void(int which)> guidePressed;
	handy::arch::Signal<void(int which)> guideReleased;


	// Singleton
	static inline GlobalData& get()
	{
		static GlobalData dat;
		return dat;
	}

	void update()
	{
		for (int i = 0; i < 4; ++i)
		{
			XInputGetStateEx(i, &states[i]);

			if ((is_guide_down(states[i])) && (!wasGuideDownPrev[i]))
				guidePressed.call(i);
			else if ((!is_guide_down(states[i])) && (wasGuideDownPrev[i]))
				guideReleased.call(i);

			wasGuideDownPrev[i] = is_guide_down(states[i]);
		}
	}

private:


	XINPUT_STATE   states[4];
	bool           wasGuideDownPrev[4];
	typedef DWORD (__stdcall *XInputGetStateEx_t) (DWORD, XINPUT_STATE*);
	XInputGetStateEx_t XInputGetStateEx;

	HINSTANCE dll;

	GlobalData()
	{
		dll = NULL;

		TCHAR dll_path[MAX_PATH];
		GetSystemDirectory(dll_path, sizeof(dll_path));
		strcat(dll_path, "\\xinput1_3.dll");
		dll = LoadLibrary(dll_path);

		if (!dll)
			fatal_error("Error loading XInput DLL.");

		XInputGetStateEx = (XInputGetStateEx_t)GetProcAddress(dll, (LPCSTR)100);

		handy::io::INIFile ini;
		if (!ini.loadFile("button2.ini"))
			fatal_error("Couldn't load button2.ini");

		settings[0].key                = ini.getInteger("player1", "key", 27);
		settings[0].hold_mode          = ini.getInteger("player1", "hold_mode", 1);
		settings[0].longpress_key      = ini.getInteger("player1", "longpress_key", 27);
		settings[0].longpress_duration = ini.getInteger("player1", "longpress_duration", 1000);
		settings[0].delay              = ini.getInteger("player1", "delay", 0);

		settings[1].key                = ini.getInteger("player2", "key", 27);
		settings[1].hold_mode          = ini.getInteger("player2", "hold_mode", 1);
		settings[1].longpress_key      = ini.getInteger("player2", "longpress_key", 27);
		settings[1].longpress_duration = ini.getInteger("player2", "longpress_duration", 1000);
		settings[1].delay              = ini.getInteger("player2", "delay", 0);

		settings[2].key                = ini.getInteger("player3", "key", 27);
		settings[2].hold_mode          = ini.getInteger("player3", "hold_mode", 1);
		settings[2].longpress_key      = ini.getInteger("player3", "longpress_key", 27);
		settings[2].longpress_duration = ini.getInteger("player3", "longpress_duration", 1000);
		settings[2].delay              = ini.getInteger("player3", "delay", 0);

		settings[3].key                = ini.getInteger("player4", "key", 27);
		settings[3].hold_mode          = ini.getInteger("player4", "hold_mode", 1);
		settings[3].longpress_key      = ini.getInteger("player4", "longpress_key", 27);
		settings[3].longpress_duration = ini.getInteger("player4", "longpress_duration", 1000);
		settings[3].delay              = ini.getInteger("player4", "delay", 0);


	}
};

#include "qsb.hpp"


#include <stdio.h>

int main()
{
	GlobalData& d = GlobalData::get();

	printf("Button on 360 guide v6, by pinumbernumber.\nWill minimise now\n\n");
	Sleep(500);
	ShowWindow(get_console_hwnd(), SW_MINIMIZE);

	typedef std::chrono::high_resolution_clock hrc;

	auto now = hrc::now();
	hrc::time_point guideDepressed[4] = {now,now,now,now};



	d.guidePressed.connect([&](int i)
	{
		if (d.settings[i].hold_mode == 1)
		{
			if (d.settings[i].key != 0)
			{

				printf ("Player %i guide down, will start holding key %lli\n", i+1, d.settings[i].key);
				key_down(d.settings[i].key);
			}
		}
		else if (d.settings[i].hold_mode == 2)
		{
			guideDepressed[i] = hrc::now();

		}
	});

	d.guideReleased.connect([&](int i)
	{
		if (d.settings[i].hold_mode == 1)
		{
			printf ("Player %i guide up, will stop holding key %lli\n", i+1, d.settings[i].key);
			key_up(d.settings[i].key);
		}
		else if (d.settings[i].hold_mode == 2)
		{
			int64_t ms = (std::chrono::duration_cast<std::chrono::milliseconds>(hrc::now() - guideDepressed[i])).count();
			if (ms >= d.settings[i].longpress_duration)
			{
				if (d.settings[i].longpress_key != 0)
				{
					printf ("Player %i guide pressed for longer than %lli ms, will now tap key %lli\n", i+1, d.settings[i].longpress_duration, d.settings[i].longpress_key);
					key_tap(d.settings[i].longpress_key, 0, 1);
				}

			}
			else
			{
				if (d.settings[i].key != 0)
				{
					printf ("Player %i guide pressed for less than %lli ms, will now tap key %lli\n", i+1, d.settings[i].longpress_duration, d.settings[i].key);
					key_tap(d.settings[i].key, 0, 1);
				}

			}
		}
	});

	for (;;)
	{
		d.update();
		Sleep(50);
	}

    return 0;
}
