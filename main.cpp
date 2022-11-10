#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <strsafe.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include "INI.hpp"
#include "SDL.h"
#include "resource.h"
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

// we need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

using namespace std;

typedef std::chrono::high_resolution_clock hrc;

const wchar_t szProgramName[] = L"Xbox Controller button remapper";

struct KeySettings
{
	std::vector<int> key = { 0 };
	int64_t hold_mode = 1;
	std::vector<int> longpress_key = { 0 };
	int64_t longpress_duration = 1000;
	int64_t delay = 0;
	int64_t duration = 1;
};

struct ControllerSettings
{
	KeySettings share;
	KeySettings xbox;
};

struct Controller
{
	SDL_GameController* controller{};
	ControllerSettings settings;
	hrc::time_point button_depressed = hrc::now();
};

static std::vector<Controller> controllers;
handy::io::INIFile ini;
bool debug = false;
bool sdl_initialized = false;

void message(const LPCWSTR&& text)
{
	MessageBox(NULL, text, szProgramName, MB_OK);
}

void error(const LPCWSTR&& text)
{
	MessageBox(NULL, text, L"Error", MB_OK);
}

void fatal_error(const LPCWSTR&& text)
{
	MessageBox(NULL, text, L"Fatal Error", MB_OK);
	exit(1);
}

std::wstring get_exe_filename()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	return std::wstring(buffer);
}

std::wstring get_exe_path()
{
	std::wstring f = get_exe_filename();
	return f.substr(0, f.find_last_of(L"\\/"));
}

std::string get_date_time() {
	std::time_t t = std::time(nullptr);
	char date_time[20];
	std::strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
	return date_time;
}

static void add_controller_mapping(char* guid)
{
	char mapping_string[1024];

	SDL_strlcpy(mapping_string, guid, sizeof(mapping_string));
	SDL_strlcat(mapping_string, ",Xbox Controller,platform:Windows,", sizeof(mapping_string));
	SDL_strlcat(mapping_string, "a:b0,b:b1,x:b2,y:b3,back:b6,guide:b10,start:b7,leftstick:b8,rightstick:b9,leftshoulder:b4,rightshoulder:b5,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,", sizeof(mapping_string));
	SDL_strlcat(mapping_string, "misc1:b11,", sizeof(mapping_string));

	SDL_GameControllerAddMapping(mapping_string);
}

static void load_controller_config(int i)
{
	std::wstring controller = L"controller" + std::to_wstring(i + 1);

	controllers[i].settings.share.key = ini.getIntegers(controller, L"share_key", { 91,56,84 });
	controllers[i].settings.share.hold_mode = ini.getInteger(controller, L"share_hold_mode", 2);
	controllers[i].settings.share.longpress_key = ini.getIntegers(controller, L"share_longpress_key", { 91,56,19 });
	controllers[i].settings.share.longpress_duration = ini.getInteger(controller, L"share_longpress_duration", 1000);
	controllers[i].settings.share.delay = ini.getInteger(controller, L"share_delay", 0);
	controllers[i].settings.share.duration = ini.getInteger(controller, L"share_duration", 1);
	controllers[i].settings.xbox.key = ini.getIntegers(controller, L"xbox_key", { 91,34 });
	controllers[i].settings.xbox.hold_mode = ini.getInteger(controller, L"xbox_hold_mode", 1);
	controllers[i].settings.xbox.longpress_key = ini.getIntegers(controller, L"xbox_longpress_key", { 1 });
	controllers[i].settings.xbox.longpress_duration = ini.getInteger(controller, L"xbox_longpress_duration", 1000);
	controllers[i].settings.xbox.delay = ini.getInteger(controller, L"xbox_delay", 0);
	controllers[i].settings.xbox.duration = ini.getInteger(controller, L"xbox_duration", 1);
}

static int find_controller(SDL_JoystickID controller_id)
{
	for (int i = 0; i < controllers.size(); ++i)
	{
		if (controller_id == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers[i].controller)))
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
			SDL_Log("%s: Could not get controller ID: %s\n", get_date_time().c_str(), SDL_GetError());
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
		SDL_Log("%s: Mapping: %s\n", get_date_time().c_str(), SDL_GameControllerMappingForGUID(SDL_JoystickGetDeviceGUID(i)));
	}

	SDL_GameController* controller = SDL_GameControllerOpen(i);
	if (!controller)
	{
		if (debug)
		{
			SDL_Log("%s: Could not open controller: %s\n", get_date_time().c_str(), SDL_GetError());
		}

		return;
	}

	controllers.resize((int64_t)(i) + 1);
	controllers[i].controller = controller;

	load_controller_config(i);

	if (debug)
	{
		SDL_Log("%s: Opened controller %d: %s\n", get_date_time().c_str(), (i + 1), SDL_GameControllerName(controller));
	}
}

static void close_controller(SDL_JoystickID controller_id)
{
	int i = find_controller(controller_id);

	if (i < 0)
	{
		// Can not find this controller
		return;
	}

	const char* controller_name = SDL_GameControllerName(controllers[i].controller);

	SDL_GameControllerClose(controllers[i].controller);

	controllers.erase(controllers.begin() + i);

	if (debug)
	{
		SDL_Log("%s: Closed controller %d: %s\n", get_date_time().c_str(), (i + 1), controller_name);
	}
}

static void close_controllers()
{
	for (int i = 0; i < controllers.size(); ++i)
	{
		SDL_GameControllerClose(controllers[i].controller);
	}

	controllers.clear();

	if (debug)
	{
		SDL_Log("%s: Closed controllers.\n", get_date_time().c_str());
	}
}

static void initialize_sdl()
{
	if (sdl_initialized)
	{
		// Reinitialize SDL
		close_controllers();
		SDL_Quit();
		sdl_initialized = false;
	}

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	sdl_initialized = SDL_Init(SDL_INIT_GAMECONTROLLER) == 0;

	if (!sdl_initialized)
	{
		//SDL_SetError("Test error.");
		const char* error_msg = SDL_GetError();
		const size_t size = strlen(error_msg) + 1;
		wchar_t error_msg2[] = L"";
		mbstowcs(error_msg2, error_msg, size);
		fatal_error((L"Could not initialize SDL: " + std::wstring(error_msg2)).c_str());
	}
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
	INPUT inp{};
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = code; // hardware scan code for key

	if (code == 91 || code == 92 || code == 93)
	{
		inp.ki.wVk = code; // virtual-key code
		inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_SCANCODE; // 0 for key press
	}
	else
	{
		inp.ki.wVk = 0; // virtual-key code, we are doing scan codes instead
		inp.ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press
	}

	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}

void key_up(int code)
{
	INPUT inp{};
	inp.type = INPUT_KEYBOARD;
	inp.ki.wScan = code; // hardware scan code for key

	if (code == 91 || code == 92 || code == 93)
	{
		inp.ki.wVk = code; // virtual-key code
		inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press
	}
	else
	{
		inp.ki.wVk = 0; // virtual-key code, we are doing scan codes instead
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

	// Open/close Xbox Game Bar on button release only
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
			SDL_Log("%s: Xbox Game Bar opened/closed.\n", get_date_time().c_str());
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

//bool inline is_xbox_button_pressed(int i)
//{
//	return SDL_GameControllerGetButton(controllers[i].controller, SDL_CONTROLLER_BUTTON_GUIDE) == SDL_PRESSED;
//}

//bool inline is_share_button_pressed(int i)
//{
//	return SDL_GameControllerGetButton(controllers[i].controller, SDL_CONTROLLER_BUTTON_MISC1) == SDL_PRESSED;
//}

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

	SetConsoleTitle(szProgramName);

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (int64_t)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console
	lStdHandle = (int64_t)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	lStdHandle = (int64_t)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	ios::sync_with_stdio();
}

void loop()
{
	SDL_Event event;
	int i;
	KeySettings settings;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_CONTROLLERDEVICEADDED:
				if (debug)
				{
					//SDL_Log("%s: Controller %d added.\n", get_date_time().c_str(), (int)SDL_JoystickGetDeviceInstanceID(event.cdevice.which));
					SDL_Log("%s: Controller %d added.\n", get_date_time().c_str(), (int)event.cdevice.which + 1);
				}

				add_controller(event.cdevice.which);

				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				if (debug)
				{
					//SDL_Log("%s: Controller %d removed.\n", get_date_time().c_str(), (int)event.cdevice.which);
					SDL_Log("%s: Controller %d removed.\n", get_date_time().c_str(), (int)find_controller(event.cdevice.which) + 1);
				}

				close_controller(event.cdevice.which);

				break;

			case SDL_CONTROLLERBUTTONDOWN:
				if (debug)
				{
					SDL_Log("%s: Controller %d button %s %s.\n", get_date_time().c_str(), (int)find_controller(event.cdevice.which) + 1, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.cbutton.state ? "pressed" : "released");
				}

				if (event.cbutton.button == SDL_CONTROLLER_BUTTON_MISC1 || event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE)
				{
					if (debug)
					{
						SDL_Log("%s: %s button pressed.\n", get_date_time().c_str(), (event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE ? "Xbox" : "Share"));
					}

					i = find_controller(event.cdevice.which);
					settings = event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE ? controllers[i].settings.xbox : controllers[i].settings.share;

					if (settings.hold_mode == 1)
					{
						if (settings.key.size() != 0)
						{
							// Open/close Xbox Game Bar on button release only
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
						controllers[i].button_depressed = hrc::now();
					}
				}

				break;

			case SDL_CONTROLLERBUTTONUP:
				if (debug)
				{
					SDL_Log("%s: Controller %d button %s %s.\n", get_date_time().c_str(), (int)find_controller(event.cdevice.which) + 1, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.cbutton.state ? "pressed" : "released");
				}

				if (event.cbutton.button == SDL_CONTROLLER_BUTTON_MISC1 || event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE)
				{
					if (debug)
					{
						SDL_Log("%s: %s button released.\n", get_date_time().c_str(), (event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE ? "Xbox" : "Share"));
					}

					i = find_controller(event.cdevice.which);
					settings = event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE ? controllers[i].settings.xbox : controllers[i].settings.share;

					if (settings.hold_mode == 1)
					{
						if (settings.key.size() != 0)
						{
							// Open/close Xbox Game Bar on button release only
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
						int64_t ms = (std::chrono::duration_cast<std::chrono::milliseconds>(hrc::now() - controllers[i].button_depressed)).count();

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

				break;

			default:
				if (debug)
				{
					SDL_Log("%s: Event: %d\n", get_date_time().c_str(), event.type);
				}

				break;
		}
	}
}

// Variables
HANDLE hMutex;
HINSTANCE hCurrentInstance = NULL;
HWND hwnd;
MSG msg; // Here messages to the application are saved
NOTIFYICONDATA notifyIconData;
const UINT WM_NOTIFYICON = WM_APP + 1;
HMENU hMenu;

// Forward declarations of functions
// Procedures
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

static void AddNotificationIcon(HWND hwnd)
{
	//NOTIFYICONDATA notifyIconData = { sizeof(notifyIconData) };
	//notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hwnd;
	// Add the icon, setting the icon, tooltip, and callback message
	// The icon will be identified with the uID
	notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uCallbackMessage = WM_NOTIFYICON;
	LoadIconMetric(hCurrentInstance, MAKEINTRESOURCE(IDI_ICON), LIM_SMALL, &notifyIconData.hIcon);
	// This text will be shown as the icon's tooltip.
	//LoadString(hCurrentInstance, szProgramName, notifyIconData.szTip, ARRAYSIZE(notifyIconData.szTip));
	//wcsncpy(notifyIconData.szTip, szProgramName, sizeof(szProgramName));
	StringCchCopy(notifyIconData.szTip, ARRAYSIZE(notifyIconData.szTip), szProgramName);
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);

	// NOTIFYICON_VERSION_4 is prefered
	notifyIconData.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_SETVERSION, &notifyIconData);

	/*hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");*/
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	// Try to create the mutex
	hMutex = CreateMutex(NULL, FALSE, szProgramName);

	if (hMutex == NULL)
	{
		// Can not create application mutex
		fatal_error(L"Error starting the program.");
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// The mutex exists so this is the second instance so return
		message(L"Program is already running.");
		return 0;
	}

	// This is the handle for our window
	hCurrentInstance = hInstance;

	WNDCLASSEX wcex = { sizeof(wcex) };
	//wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.hInstance = hInstance;
	//wcex.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON));
	//wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = pszMenuName;
	wcex.lpszClassName = szProgramName;
	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindow(szProgramName, L"", 0, 0, 0, 0, 0, NULL, NULL, hCurrentInstance, NULL);

	if (!hWnd)
	{
		return 0;
	}

	// Main

	// Initialize SDL
	initialize_sdl();

	std::wstring path = get_exe_path();

	if (!ini.loadFile(path + L"\\config.ini"))
	{
		error(L"Could not load config.ini. Using defaults.");
	}

	debug = ini.getInteger(L"settings", L"debug", 0) == 1;

	if (debug)
	{
		RedirectIOToConsole();
		SDL_Log("%s: Debug mode enabled.\n", get_date_time().c_str());
	}

	while (true)
	{
		// Main message loop
		// Will get messages until queue is clear
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				return 0;
			}
		}

		// Do a tick of any intensive stuff here such as graphics processing
		loop();

		// Fix for controller disconnect on subsequent connections
		XINPUT_CAPABILITIES capabilities;
		XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &capabilities);

		/*if (!Shell_NotifyIcon(NIM_MODIFY, &notifyIconData))
		{
			AddNotificationIcon(hwnd);
		}*/

		Sleep(50);
	}

	return msg.wParam;
}

// This function is called by DispatchMessage()
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hwnd = hwnd;
	static UINT s_uTaskbarRestart;

	// Handle the messages
	switch (uMsg)
	{
		case WM_CREATE:
		{
			s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
			AddNotificationIcon(hwnd);
		}
		break;

		case WM_COMMAND:
		{
			int const wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_EXIT:
				{
					DestroyWindow(hwnd);
					break;
				}

				default:
				{
					return DefWindowProc(hwnd, uMsg, wParam, lParam);
				}
			}
		}
		break;

		case WM_NOTIFYICON:
		{
			//if (lParam == WM_RBUTTONDOWN)
			//{
			//	// Get current mouse position
			//	POINT curPoint;
			//	GetCursorPos(&curPoint);
			//	SetForegroundWindow(hwnd);

			//	// TrackPopupMenu blocks the app until TrackPopupMenu returns
			//	UINT clicked = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);

			//	if (clicked == ID_TRAY_EXIT)
			//	{
			//		// Quit the application
			//		Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
			//		PostQuitMessage(0);
			//	}
			//}

			switch (LOWORD(lParam))
			{
				case WM_CONTEXTMENU:
					POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
					HMENU hMenu = LoadMenu(hCurrentInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));

					if (hMenu)
					{
						HMENU hSubMenu = GetSubMenu(hMenu, 0);

						if (hSubMenu)
						{
							// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
							SetForegroundWindow(hwnd);

							// respect menu drop alignment
							UINT uFlags = TPM_RIGHTBUTTON;
							if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
							{
								uFlags |= TPM_RIGHTALIGN;
							}
							else
							{
								uFlags |= TPM_LEFTALIGN;
							}

							//TrackPopupMenu(hSubMenu, uFlags, pt.x, pt.y, 0, hwnd, NULL);
							TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
						}

						DestroyMenu(hMenu);
					}

					break;
			}
		}
		break;

		case WM_DESTROY:
		{
			// Quit the application

			close_controllers();

			//SDL_Quit();
			atexit(SDL_Quit);
			sdl_initialized = false;

			// Remove the tray icon

			/*NOTIFYICONDATA notifyIconData2;

			notifyIconData2.cbSize = sizeof(NOTIFYICONDATA);
			notifyIconData2.hWnd = hwnd;
			notifyIconData2.uID = ID_TRAY_APP_ICON;*/

			Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

			PostQuitMessage(0);

			if (hMutex != NULL)
			{
				// The app is closing so release and close the mutex
				ReleaseMutex(hMutex);
				CloseHandle(hMutex);
			}
		}
		break;

		default:
		{
			if (uMsg == s_uTaskbarRestart)
			{
				AddNotificationIcon(hwnd);
			}

			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	return 0;
}
