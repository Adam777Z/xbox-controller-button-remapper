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

#include <iostream>

#include "SDL.h"

using namespace std;

std::string GetExeFileName()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	return std::string(buffer);
}

std::string GetExePath()
{
	std::string f = GetExeFileName();
	return f.substr(0, f.find_last_of("\\/"));
}

#define MAX_CONTROLLERS 4

static int num_controllers = 0;
static SDL_GameController* controllers[MAX_CONTROLLERS];

static void add_controller_mapping(char* guid)
{
	char mapping_string[1024];

	SDL_strlcpy(mapping_string, guid, sizeof(mapping_string));
	SDL_strlcat(mapping_string, ",Xbox Controller,platform:Windows,", sizeof(mapping_string));
	SDL_strlcat(mapping_string, "a:b0,b:b1,x:b2,y:b3,back:b6,guide:b10,start:b7,leftstick:b8,rightstick:b9,leftshoulder:b4,rightshoulder:b5,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,", sizeof(mapping_string));
	SDL_strlcat(mapping_string, "misc1:b11,", sizeof(mapping_string));

	SDL_GameControllerAddMapping(mapping_string);
}

static int find_controller(SDL_JoystickID controller_id)
{
	for (int i = 0; i < num_controllers; ++i) {
		if (controller_id == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers[i]))) {
			return i;
		}
	}
	return -1;
}

static void add_controller(int i)
{
	SDL_JoystickID controller_id = SDL_JoystickGetDeviceInstanceID(i);
	if (controller_id < 0) {
		//SDL_Log("Couldn't get controller ID: %s\n", SDL_GetError());
		return;
	}

	if (find_controller(controller_id) >= 0) {
		// We already have this controller
		return;
	}

	char guid[64];

	SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof(guid));

	add_controller_mapping(guid);

	//SDL_Log("Mapping: %s\n", SDL_GameControllerMappingForGUID(SDL_JoystickGetDeviceGUID(i)));

	SDL_GameController* controller = SDL_GameControllerOpen(i);
	if (!controller) {
		//SDL_Log("Couldn't open controller: %s\n", SDL_GetError());
		return;
	}

	controllers[i] = controller;

	num_controllers = SDL_NumJoysticks() > MAX_CONTROLLERS ? MAX_CONTROLLERS : SDL_NumJoysticks();

	//SDL_Log("Opened controller: %s\n", SDL_GameControllerName(controller));
}

static void add_controllers()
{
	num_controllers = SDL_NumJoysticks() > MAX_CONTROLLERS ? MAX_CONTROLLERS : SDL_NumJoysticks();

	for (int i = 0; i < num_controllers; ++i)
	{
		add_controller(i);
	}
}

static void close_controller(int i)
{
	if (controllers[i] == NULL)
	{
		return;
	}

	const char* controller_name = SDL_GameControllerName(controllers[i]);

	SDL_GameControllerClose(controllers[i]);

	controllers[i] = NULL;

	num_controllers = SDL_NumJoysticks() > MAX_CONTROLLERS ? MAX_CONTROLLERS : SDL_NumJoysticks();

	//SDL_Log("Closed controller: %s\n", controller_name);
}

static void close_controllers()
{
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		close_controller(i);
	}
}

static void update_controllers()
{
	int num_controllers_now = SDL_NumJoysticks() > MAX_CONTROLLERS ? MAX_CONTROLLERS : SDL_NumJoysticks();

	if (num_controllers_now != num_controllers)
	{
		close_controllers();
		add_controllers();
	}
}


void error(const std::string&& text)
{
	MessageBox(NULL, text.c_str(), "Error", MB_OK);
}

void fatal_error(const std::string&& text)
{
	MessageBox(NULL, text.c_str(), "Fatal Error", MB_OK);
	exit(1);
}

void key_down(int code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = code; // hardware scan code for key
	inp.ki.wVk = 0; // virtual-key code, we're doing scan codes instead
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	inp.ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));
}

void key_up(int code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = code; // hardware scan code for key
	inp.ki.wVk = 0; // virtual-key code, we're doing scan codes instead
	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press
	SendInput(1, &inp, sizeof(INPUT));
}

void key_tap(int code, int64_t delay, int64_t duration)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	key_down(code);
	std::this_thread::sleep_for(std::chrono::milliseconds(duration));
	key_up(code);
}

void open_xbox_game_bar()
{
	INPUT inp[4];

	// Press LEFT WINDOWS key
	inp[0].type = INPUT_KEYBOARD;
	inp[0].ki.wScan = 91; // hardware scan code for LEFT WINDOWS key
	inp[0].ki.wVk = 91; // virtual-key code for LEFT WINDOWS key
	inp[0].ki.time = 0;
	inp[0].ki.dwExtraInfo = 0;
	inp[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_SCANCODE; // 0 for key press

	// Press G key
	inp[1] = inp[0];
	inp[1].ki.wScan = 34; // hardware scan code for G key
	inp[1].ki.wVk = 0; // virtual-key code, we're doing scan codes instead
	inp[1].ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press

	// Release LEFT WINDOWS key
	inp[2] = inp[0];
	//inp[2].ki.wScan = 91; // hardware scan code for LEFT WINDOWS key
	//inp[2].ki.wVk = 91; // virtual-key code for LEFT WINDOWS key
	inp[2].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press

	// Release G key
	inp[3] = inp[1];
	//inp[3].ki.wScan = 34; // hardware scan code for G key
	//inp[3].ki.wVk = 0; // virtual-key code, we're doing scan codes instead
	inp[3].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press

	SendInput(4, inp, sizeof(INPUT));
}



struct PlayerSettings
{
	int64_t button = 0;
	int64_t xbox_game_bar = 1;
	std::vector<int> key = { 1 };
	int64_t hold_mode = 1;
	std::vector<int> longpress_key = { 1 };
	int64_t longpress_duration = 1000;
	int64_t delay = 0;
	int64_t duration = 1;
};

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
	handy::arch::Signal<void(int which)> button_pressed;
	handy::arch::Signal<void(int which)> button_released;


	// Singleton
	static inline GlobalData& get()
	{
		static GlobalData dat;
		return dat;
	}

	bool inline is_xbox_button_pressed(int i)
	{
		return SDL_GameControllerGetButton(controllers[i], SDL_CONTROLLER_BUTTON_GUIDE) == SDL_PRESSED;
	}

	bool inline is_share_button_pressed(int i)
	{
		return SDL_GameControllerGetButton(controllers[i], SDL_CONTROLLER_BUTTON_MISC1) == SDL_PRESSED;
	}

	void update()
	{
		SDL_JoystickUpdate();
		update_controllers();

		GlobalData& d = GlobalData::get();

		for (int i = 0; i < num_controllers; ++i)
		{
			if (d.settings[i].button == 0 && d.settings[i].xbox_game_bar == 1)
			{
				bool xbox_button_pressed = is_xbox_button_pressed(i);

				if (!xbox_button_pressed && was_xbox_button_pressed_prev[i])
				{
					//SDL_Log("Xbox button released.\n\n");
					open_xbox_game_bar();
				}

				was_xbox_button_pressed_prev[i] = xbox_button_pressed;
			}

			bool button_pressed_now = d.settings[i].button == 1 ? is_xbox_button_pressed(i) : is_share_button_pressed(i);

			if (button_pressed_now && !was_button_pressed_prev[i])
			{
				//SDL_Log("Button pressed.\n");
				button_pressed.call(i);
			}
			else if (!button_pressed_now && was_button_pressed_prev[i])
			{
				//SDL_Log("Button released.\n");
				button_released.call(i);
			}

			was_button_pressed_prev[i] = button_pressed_now;
		}
	}

private:


	bool was_xbox_button_pressed_prev[4] = { false };
	bool was_button_pressed_prev[4] = { false };

	GlobalData()
	{
		std::string path = GetExePath();
		handy::io::INIFile ini;

		if (!ini.loadFile(path + "\\config.ini"))
		{
			error("Couldn't load config.ini. Using defaults.");
		}

		settings[0].button             = ini.getInteger("player1", "button", 0);
		settings[0].xbox_game_bar      = ini.getInteger("player1", "xbox_game_bar", 1);
		settings[0].key                = ini.getIntegers("player1", "key", { 1 });
		settings[0].hold_mode          = ini.getInteger("player1", "hold_mode", 1);
		settings[0].longpress_key      = ini.getIntegers("player1", "longpress_key", { 1 });
		settings[0].longpress_duration = ini.getInteger("player1", "longpress_duration", 1000);
		settings[0].delay              = ini.getInteger("player1", "delay", 0);
		settings[0].duration           = ini.getInteger("player1", "duration", 1);

		settings[1].button             = ini.getInteger("player2", "button", 0);
		settings[1].xbox_game_bar      = ini.getInteger("player2", "xbox_game_bar", 1);
		settings[1].key                = ini.getIntegers("player2", "key", { 1 });
		settings[1].hold_mode          = ini.getInteger("player2", "hold_mode", 1);
		settings[1].longpress_key      = ini.getIntegers("player2", "longpress_key", { 1 });
		settings[1].longpress_duration = ini.getInteger("player2", "longpress_duration", 1000);
		settings[1].delay              = ini.getInteger("player2", "delay", 0);
		settings[1].duration           = ini.getInteger("player2", "duration", 1);

		settings[2].button             = ini.getInteger("player3", "button", 0);
		settings[2].xbox_game_bar      = ini.getInteger("player3", "xbox_game_bar", 1);
		settings[2].key                = ini.getIntegers("player3", "key", { 1 });
		settings[2].hold_mode          = ini.getInteger("player3", "hold_mode", 1);
		settings[2].longpress_key      = ini.getIntegers("player3", "longpress_key", { 1 });
		settings[2].longpress_duration = ini.getInteger("player3", "longpress_duration", 1000);
		settings[2].delay              = ini.getInteger("player3", "delay", 0);
		settings[2].duration           = ini.getInteger("player3", "duration", 1);

		settings[3].button             = ini.getInteger("player4", "button", 0);
		settings[3].xbox_game_bar      = ini.getInteger("player4", "xbox_game_bar", 1);
		settings[3].key                = ini.getIntegers("player4", "key", { 1 });
		settings[3].hold_mode          = ini.getInteger("player4", "hold_mode", 1);
		settings[3].longpress_key      = ini.getIntegers("player4", "longpress_key", { 1 });
		settings[3].longpress_duration = ini.getInteger("player4", "longpress_duration", 1000);
		settings[3].delay              = ini.getInteger("player4", "delay", 0);
		settings[3].duration           = ini.getInteger("player4", "duration", 1);


	}
};



#define WM_SYSICON (WM_USER + 1)

// Variables
HWND hWnd;
HINSTANCE hCurrentInstance;
HMENU hMenu;
NOTIFYICONDATA notifyIconData;
TCHAR szTIP[64] = TEXT("Xbox Controller button remapper");
char szClassName[] = "Xbox Controller button remapper";

// Procedures
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);


int WINAPI WinMain(_In_ HINSTANCE hThisInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszArgument, _In_ int nCmdShow)
{
	// Try to open the mutex.
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, "Xbox-Controller-button-remapper");

	if (!hMutex)
	{
		// Mutex doesn't exist. This is the first instance so create the mutex.
		hMutex = CreateMutex(0, 0, "Xbox-Controller-button-remapper");
	}
	else
	{
		// The mutex exists so this is the second instance so return.
		return 0;
	}

	// This is the handle for our window
	MSG messages; // Here messages to the application are saved
	hCurrentInstance = hThisInstance;

	WNDCLASS wincl;
	ZeroMemory(&wincl, sizeof(wincl));
	wincl.hInstance = hThisInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WindowProc; // This function is called by windows
	ATOM szClassName = RegisterClass(&wincl);
	hWnd = CreateWindow((LPCTSTR)szClassName, "", 0, 0, 0, 0, 0, NULL, NULL, hThisInstance, NULL);

	// Main

	GlobalData& d = GlobalData::get();

	typedef std::chrono::high_resolution_clock hrc;

	auto now = hrc::now();
	hrc::time_point button_depressed[4] = { now, now, now, now };


	d.button_pressed.connect([&](int i)
	{
		if (d.settings[i].hold_mode == 1)
		{
			if (d.settings[i].key.size() != 0)
			{
				for (std::size_t j = 0; j < d.settings[i].key.size(); ++j) {
					key_down(d.settings[i].key[j]);
				}
			}
		}
		else if (d.settings[i].hold_mode == 2)
		{
			button_depressed[i] = hrc::now();
		}
	});

	d.button_released.connect([&](int i)
	{
		if (d.settings[i].hold_mode == 1)
		{
			if (d.settings[i].key.size() != 0)
			{
				for (std::size_t j = 0; j < d.settings[i].key.size(); ++j) {
					key_up(d.settings[i].key[j]);
				}
			}
		}
		else if (d.settings[i].hold_mode == 2)
		{
			int64_t ms = (std::chrono::duration_cast<std::chrono::milliseconds>(hrc::now() - button_depressed[i])).count();
			if (ms >= d.settings[i].longpress_duration)
			{
				if (d.settings[i].longpress_key.size() != 0)
				{
					for (std::size_t j = 0; j < d.settings[i].longpress_key.size(); ++j) {
						key_tap(d.settings[i].longpress_key[j], d.settings[i].delay, d.settings[i].duration);
					}
				}
			}
			else
			{
				if (d.settings[i].key.size() != 0)
				{
					for (std::size_t j = 0; j < d.settings[i].key.size(); ++j) {
						key_tap(d.settings[i].key[j], d.settings[i].delay, d.settings[i].duration);
					}
				}
			}
		}
	});

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	// Initialize SDL
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
		fatal_error(strcat("Couldn't initialize SDL: %s", SDL_GetError()));
	}

	//SDL_GameControllerAddMappingsFromFile("gamecontrollermapping.txt");

	add_controllers();

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

	return messages.wParam;
}


// This function is called by the Windows function DispatchMessage()

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle the messages
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
		notifyIconData.uCallbackMessage = WM_SYSICON; // Set up our invented Windows Message
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
		close_controllers();

		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);

		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
