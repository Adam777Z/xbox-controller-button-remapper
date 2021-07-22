#include <string>

#include <chrono>
#include <thread>

#include "INI.hpp"

#include "targetver.h"
#include <windows.h>
#include "Signal.hpp"

#include <shellapi.h>
#include "resource.h"

#include "qsb.hpp"
#include <stdio.h>

using namespace std;



void fatal_error(const std::string&& text)
{
	MessageBox(NULL, text.c_str(), "Fatal Error", MB_OK);
	exit(1);
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
		if (!ini.loadFile("config.ini"))
			fatal_error("Couldn't load config.ini");

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



#define WM_SYSICON (WM_USER + 1)

/* Variables */
HWND hWnd;
HINSTANCE hCurrentInstance;
HMENU hMenu;
NOTIFYICONDATA notifyIconData;
TCHAR szTIP[64] = TEXT("Xbox Guide button map to key");
char szClassName[] = "Xbox Guide button map to key";

/* Procedures */
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);


int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow)
{
	// Try to open the mutex.
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, "XboxGuideButtonMapToKey1.0");

	if (!hMutex)
	{
		// Mutex doesn't exist. This is the first instance so create the mutex.
		hMutex = CreateMutex(0, 0, "XboxGuideButtonMapToKey1.0");
	}
	else
	{
		// The mutex exists so this is the second instance so return.
		return 0;
	}

	/* This is the handle for our window */
	MSG messages; /* Here messages to the application are saved */
	hCurrentInstance = hThisInstance;

	WNDCLASS wincl;
	ZeroMemory(&wincl, sizeof(wincl));
	wincl.hInstance = hThisInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WindowProc; /* This function is called by windows */
	ATOM szClassName = RegisterClass(&wincl);
	hWnd = CreateWindow((LPCTSTR)szClassName, "", 0, 0, 0, 0, 0, NULL, NULL, hThisInstance, NULL);

	// Main

	GlobalData& d = GlobalData::get();

	typedef std::chrono::high_resolution_clock hrc;

	auto now = hrc::now();
	hrc::time_point guideDepressed[4] = { now,now,now,now };


	d.guidePressed.connect([&](int i)
	{
		if (d.settings[i].hold_mode == 1)
		{
			if (d.settings[i].key != 0)
			{
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
			key_up(d.settings[i].key);
		}
		else if (d.settings[i].hold_mode == 2)
		{
			int64_t ms = (std::chrono::duration_cast<std::chrono::milliseconds>(hrc::now() - guideDepressed[i])).count();
			if (ms >= d.settings[i].longpress_duration)
			{
				if (d.settings[i].longpress_key != 0)
				{
					key_tap(d.settings[i].longpress_key, 0, 1);
				}
			}
			else
			{
				if (d.settings[i].key != 0)
				{
					key_tap(d.settings[i].key, 0, 1);
				}
			}
		}
	});

	/*for (;;)
	{
		d.update();
		Sleep(50);
	}*/

	while (true)
	{
		while (PeekMessage(&messages, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&messages);
			DispatchMessage(&messages);
		} // Will get messages until queue is clear
		
		if (messages.message == WM_QUIT)
		{
			// The app is closing so release the mutex.
			ReleaseMutex(hMutex);

			break;
		}

		// Do a tick of any intensive stuff here such as graphics processing
		d.update();
		Sleep(50);
	}

	/* Run the message loop. It will run until GetMessage() returns 0 */
	//while (GetMessage(&messages, NULL, 0, 0))
	//{
	//	TranslateMessage(&messages); // Translate virtual-key messages into character messages
	//	DispatchMessage(&messages); // Send message to WindowProc
	//}

	return messages.wParam;
}


/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/* handle the messages */
	switch (uMsg)
	{
	case WM_CREATE:
	{
		//memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
		ZeroMemory(&notifyIconData, sizeof(notifyIconData));
		notifyIconData.cbSize = sizeof(notifyIconData);
		notifyIconData.hWnd = hwnd;
		notifyIconData.uID = ID_TRAY_APP_ICON;
		notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		notifyIconData.uCallbackMessage = WM_SYSICON; //Set up our invented Windows Message
		notifyIconData.hIcon = (HICON)LoadImage(hCurrentInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		strncpy(notifyIconData.szTip, szTIP, sizeof(szTIP));
		Shell_NotifyIcon(NIM_ADD, &notifyIconData);

		hMenu = CreatePopupMenu();
		AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
	}
	return 0;

	// Our user defined WM_SYSICON message
	case WM_SYSICON:
	{
		if (lParam == WM_RBUTTONDOWN)
		{
			// Get current mouse position
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hwnd);

			// TrackPopupMenu blocks the app until TrackPopupMenu returns
			UINT clicked = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);

			if (clicked == ID_TRAY_EXIT)
			{
				// Quit the application
				Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
				PostQuitMessage(0);
			}
		}
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
