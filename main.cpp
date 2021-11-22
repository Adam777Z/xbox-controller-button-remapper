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

#include <fcntl.h>
#include <io.h>
#include <fstream>

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
bool debug = false;

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
	for (int i = 0; i < num_controllers; ++i)
	{
		if (controller_id == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers[i])))
		{
			return i;
		}
	}
	return -1;
}

static void add_controller(int i)
{
	SDL_JoystickID controller_id = SDL_JoystickGetDeviceInstanceID(i);
	if (controller_id < 0)
	{
		if (debug)
		{
			SDL_Log("Couldn't get controller ID: %s\n", SDL_GetError());
		}

		return;
	}

	if (find_controller(controller_id) >= 0)
	{
		// We already have this controller
		return;
	}

	char guid[64];

	SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof(guid));

	add_controller_mapping(guid);

	if (debug)
	{
		SDL_Log("Mapping: %s\n", SDL_GameControllerMappingForGUID(SDL_JoystickGetDeviceGUID(i)));
	}

	SDL_GameController* controller = SDL_GameControllerOpen(i);
	if (!controller)
	{
		if (debug)
		{
			SDL_Log("Couldn't open controller: %s\n", SDL_GetError());
		}

		return;
	}

	controllers[i] = controller;

	num_controllers = SDL_NumJoysticks() > MAX_CONTROLLERS ? MAX_CONTROLLERS : SDL_NumJoysticks();

	if (debug)
	{
		SDL_Log("Opened controller: %s\n", SDL_GameControllerName(controller));
	}
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

	if (debug)
	{
		SDL_Log("Closed controller: %s\n", controller_name);
	}
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

bool is_open_xbox_game_bar_keys(std::vector<int> code)
{
	bool key1 = false;
	bool key2 = false;

	if (code.size() == 2)
	{
		for (std::size_t i = 0; i < code.size(); ++i)
		{
			if (code[i] == 91 || code[i] == 92)
			{
				key1 = true;
			}
			else if (code[i] == 34)
			{
				key2 = true;
			}
		}
	}

	return key1 && key2;
}

void key_down(int code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = code; // hardware scan code for key

	if (code == 91 || code == 92 || code == 93)
	{
		inp.ki.wVk = code; // virtual-key code
		inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_SCANCODE; // 0 for key press
	}
	else
	{
		inp.ki.wVk = 0; // virtual-key code, we're doing scan codes instead
		inp.ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press
	}

	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}

void key_up(int code)
{
	INPUT inp;
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = code; // hardware scan code for key

	if (code == 91 || code == 92 || code == 93)
	{
		inp.ki.wVk = code; // virtual-key code
		inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press
	}
	else
	{
		inp.ki.wVk = 0; // virtual-key code, we're doing scan codes instead
		inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press
	}

	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}

void key_tap(std::vector<int> code, int64_t delay, int64_t duration)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(delay));

	bool open_xbox_game_bar_keys = is_open_xbox_game_bar_keys(code);

	// Open Xbox Game Bar on button release only
	if (!open_xbox_game_bar_keys)
	{
		for (std::size_t i = 0; i < code.size(); ++i)
		{
			key_down(code[i]);
		}
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(duration));

	if (open_xbox_game_bar_keys)
	{
		for (std::size_t i = 0; i < code.size(); ++i)
		{
			key_down(code[i]);
		}

		for (std::size_t i = 0; i < code.size(); ++i)
		{
			key_up(code[i]);
		}

		if (debug)
		{
			SDL_Log("Opened Xbox Game Bar.\n");
		}
	}
	else
	{
		for (std::size_t i = 0; i < code.size(); ++i)
		{
			key_up(code[i]);
		}
	}
}



struct KeySettings
{
	std::vector<int> key = { 1 };
	int64_t hold_mode = 1;
	std::vector<int> longpress_key = { 1 };
	int64_t longpress_duration = 1000;
	int64_t delay = 0;
	int64_t duration = 1;
};

struct ControllerSettings
{
	KeySettings share;
	KeySettings xbox;
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

// Console I/O in a Win32 GUI App
// maximum mumber of lines the output console should have
static const WORD MAX_CONSOLE_LINES = 500;

void RedirectIOToConsole()
{
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	ios::sync_with_stdio();
}

struct GlobalData
{
	ControllerSettings settings[4];
	handy::arch::Signal<void(int controller, int button)> button_pressed;
	handy::arch::Signal<void(int controller, int button)> button_released;

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
			bool button_pressed_now[2] = { is_share_button_pressed(i), is_xbox_button_pressed(i) };

			for (int j = 0; j < 2; ++j)
			{
				if (button_pressed_now[j] && !was_button_pressed_prev[i][j])
				{
					if (debug)
					{
						SDL_Log("%s button pressed.\n", (j == 1 ? "Xbox" : "Share"));
					}

					button_pressed.call(i, j);
				}
				else if (!button_pressed_now[j] && was_button_pressed_prev[i][j])
				{
					if (debug)
					{
						SDL_Log("%s button released.\n", (j == 1 ? "Xbox" : "Share"));
					}

					button_released.call(i, j);
				}

				was_button_pressed_prev[i][j] = button_pressed_now[j];
			}
		}
	}

private:

	bool was_xbox_button_pressed_prev[4] = { false };
	bool was_button_pressed_prev[4][2] = { false };

	GlobalData()
	{
		std::string path = GetExePath();
		handy::io::INIFile ini;

		if (!ini.loadFile(path + "\\config.ini"))
		{
			error("Couldn't load config.ini. Using defaults.");
		}

		debug = ini.getInteger("settings", "debug", 0) == 1;

		if (debug)
		{
			RedirectIOToConsole();
			SDL_Log("Debug mode enabled.\n");
		}

		for (int i = 0; i < MAX_CONTROLLERS; ++i)
		{
			std::string controller = "controller" + std::to_string(i + 1);

			settings[i].share.key                = ini.getIntegers(controller, "share_key", { 91,56,84 });
			settings[i].share.hold_mode          = ini.getInteger(controller,  "share_hold_mode", 2);
			settings[i].share.longpress_key      = ini.getIntegers(controller, "share_longpress_key", { 91,56,19 });
			settings[i].share.longpress_duration = ini.getInteger(controller,  "share_longpress_duration", 1000);
			settings[i].share.delay              = ini.getInteger(controller,  "share_delay", 0);
			settings[i].share.duration           = ini.getInteger(controller,  "share_duration", 1);
			settings[i].xbox.key                 = ini.getIntegers(controller, "xbox_key", { 91,34 });
			settings[i].xbox.hold_mode           = ini.getInteger(controller,  "xbox_hold_mode", 1);
			settings[i].xbox.longpress_key       = ini.getIntegers(controller, "xbox_longpress_key", { 1 });
			settings[i].xbox.longpress_duration  = ini.getInteger(controller,  "xbox_longpress_duration", 1000);
			settings[i].xbox.delay               = ini.getInteger(controller,  "xbox_delay", 0);
			settings[i].xbox.duration            = ini.getInteger(controller,  "xbox_duration", 1);
		}
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

	if (!hMutex)
	{
		// Cannot create application mutex.
		fatal_error("Error");
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


	d.button_pressed.connect([&](int i, int j)
		{
			KeySettings settings = j == 1 ? d.settings[i].xbox : d.settings[i].share;

			if (settings.hold_mode == 1)
			{
				if (settings.key.size() != 0)
				{
					// Open Xbox Game Bar on button release only
					if (!is_open_xbox_game_bar_keys(settings.key))
					{
						for (std::size_t k = 0; k < settings.key.size(); ++k)
						{
							key_down(settings.key[k]);
						}
					}
				}
			}
			else if (settings.hold_mode == 2)
			{
				button_depressed[i] = hrc::now();
			}
		}
	);

	d.button_released.connect([&](int i, int j)
		{
			KeySettings settings = j == 1 ? d.settings[i].xbox : d.settings[i].share;

			if (settings.hold_mode == 1)
			{
				if (settings.key.size() != 0)
				{
					if (is_open_xbox_game_bar_keys(settings.key))
					{
						key_tap(settings.key, 0, 1);
					}
					else
					{
						for (std::size_t k = 0; k < settings.key.size(); ++k)
						{
							key_up(settings.key[k]);
						}
					}
				}
			}
			else if (settings.hold_mode == 2)
			{
				int64_t ms = (std::chrono::duration_cast<std::chrono::milliseconds>(hrc::now() - button_depressed[i])).count();

				if (ms >= settings.longpress_duration)
				{
					if (settings.longpress_key.size() != 0)
					{
						key_tap(settings.longpress_key, settings.delay, settings.duration);
					}
				}
				else
				{
					if (settings.key.size() != 0)
					{
						key_tap(settings.key, settings.delay, settings.duration);
					}
				}
			}
		}
	);

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	// Initialize SDL
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
	{
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
