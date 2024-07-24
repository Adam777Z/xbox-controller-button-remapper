#include <string>
#include <format>
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
//#include "ScreenCapture.h"
#include "ScreenCapture2.cpp"
#include <codecvt>
#include <filesystem>
//#include <Shlwapi.h>
//#pragma comment(lib, "Shlwapi.lib")
//#include <PathCch.h>
//#pragma comment(lib, "PathCch.lib")
//#include <tchar.h>
//#include <shlobj.h>
//#include <algorithm>
//#include <regex>
#include <atomic>
#include <thread>
#include <future>
#include <endpointvolume.h>

// Need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

using namespace std;

typedef std::chrono::high_resolution_clock hrc;

const wchar_t szProgramName[] = L"Xbox Controller Button Remapper";

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
	SDL_Gamepad* controller{};
	ControllerSettings settings;
	hrc::time_point button_depressed = hrc::now();
};

static std::vector<Controller> controllers;
handy::io::INIFile ini;
bool debug = false;

std::wstring file_location = L"C:\\Windows\\explorer.exe";
int file_key = 0;
int OpenFileHotKeyID = 3;

std::wstring folder = L"";
std::wstring file_path = L"";

std::wstring captures_location = L"C:\\Screenshots";
int screenshot_key = 0;
int video_key = 0;
int ScreenshotHotKeyID = 1;
int VideoHotKeyID = 2;

bool capture_cursor = false;
int adapter = 0;
int monitor = 0;
std::wstring video_format = L"H264";
int fps = 60;
int video_bitrate = 9000;
int video_factor = -1;
int vbrm = 0;
int vbrq = 0;
int threads = 0;
std::wstring audio_format = L"AAC";
int audio_channels = 2;
int audio_sample_rate = 48000;
int audio_bitrate = 192;

DESKTOPCAPTUREPARAMS dp_screenshot;
DESKTOPCAPTUREPARAMS dp_video;

bool sdl_initialized = false;
bool screen_capture_initialized = false;
bool input_device_initialized = false;

std::mutex mtx;
std::thread worker;
std::atomic<bool> recording_video{ false }; // This needs to be atomic to avoid data races

IMMDeviceEnumerator* pEnumerator = NULL;
IMMDevice* pDevice = NULL;
static IAudioEndpointVolume* g_pEndptVol = NULL;

static void message(const LPCWSTR&& text)
{
	MessageBox(NULL, text, szProgramName, MB_OK);
}

static void error(const LPCWSTR&& text)
{
	MessageBox(NULL, text, L"Error", MB_OK);
}

static void fatal_error(const LPCWSTR&& text)
{
	MessageBox(NULL, text, L"Fatal Error", MB_OK);
	exit(1);
}

static std::wstring get_exe_filename()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	return std::wstring(buffer);
}

static std::wstring get_exe_path()
{
	std::wstring f = get_exe_filename();
	return f.substr(0, f.find_last_of(L"\\/"));
}

static std::string get_date_time()
{
	std::time_t t = std::time(nullptr);
	char date_time[20];
	std::strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
	return date_time;
}

// Source: https://gist.github.com/utilForever/fdf1540cea0de65cfc0a1a69d8cafb63
// Replace some pattern in std::wstring with another pattern
static std::wstring replace_wstring_with_pattern(__in const std::wstring& message, __in const std::wstring& pattern, __in const std::wstring& replace)
{
	std::wstring result = message;
	std::wstring::size_type pos = 0;
	std::wstring::size_type offset = 0;

	while ((pos = result.find(pattern, offset)) != std::wstring::npos)
	{
		result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
		offset = pos + replace.size();
	}

	return result;
}

static const std::wstring forbidden_chars = L"\\/:*?\"<>|";
static bool is_forbidden(wchar_t c)
{
	return std::wstring::npos != forbidden_chars.find(c);
}

// Source: https://gist.github.com/danzek/d6a0e4a48a5439e7f808ed1497f6268e
static std::wstring string_to_wstring(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

static void set_file_path(std::wstring extension)
{
	HWND foreground = GetForegroundWindow();
	WCHAR window_title[256];
	GetWindowText(foreground, window_title, _countof(window_title));

	std::wstring window_title2(window_title);
	//window_title2 = L"Test\\/:*?\"<>|";
	//window_title2 = replace_wstring_with_pattern(window_title2, L":", L"꞉"); // Replace forbidden character with allowed character that looks similar
	window_title2 = replace_wstring_with_pattern(window_title2, L":", L" -");
	std::replace_if(window_title2.begin(), window_title2.end(), is_forbidden, '_');

	if (window_title2.empty())
	{
		window_title2 = L"Screenshot"; // Default title
	}

	//LPWSTR wnd_title{};
	//HWND hWnd = GetForegroundWindow(); // Get handle of currently active window
	//GetWindowText(hWnd, wnd_title, sizeof(wnd_title));

	/*TCHAR current_path[MAX_PATH];
	GetModuleFileName(NULL, current_path, MAX_PATH);
	//PathRemoveFileSpec(current_path);
	PathCchRemoveFileSpec(current_path, MAX_PATH);*/

	/*std::string s1 = std::format("{:%F %T}", std::chrono::system_clock::now());
	std::string s2 = std::format("{:%F %T %Z}", std::chrono::system_clock::now());
	std::string s3 = std::format("{:%F %T %Z}", std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() });*/

	std::string datetime = std::format("{:%Y-%m-%d %H-%M-%S}", std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() });
	//std::wstring datetime2 = std::filesystem::path(datetime).wstring(); // Convert from string to wstring
	std::wstring datetime2 = string_to_wstring(datetime); // Convert from string to wstring
	datetime2 = datetime2.substr(0, datetime2.find('.') + 4); // Keep last 3 digits only
	std::replace(datetime2.begin(), datetime2.end(), '.', '-');
	//std::wstring file_path = std::wstring(captures_location) + L"\\" + window_title2 + L" Screenshot " + datetime2 + extension;
	folder = std::wstring(captures_location) + L"\\" + window_title2;

	if (!std::filesystem::exists(folder)) // Check if folder exists
	{
		if (!std::filesystem::create_directories(folder)) // Create folder
		{
			if (debug)
			{
				SDL_Log("%s: Could not create folder for screenshot.\n", get_date_time().c_str());
			}

			return;
		}
	}

	file_path = folder + L"\\" + window_title2 + L" " + datetime2 + extension;
}

static HRESULT save_png(const BYTE* pSource, size_t pSourceSize, UINT pWidth, UINT pHeight)
{
	LPCWSTR Path = file_path.c_str();
	HRESULT hr;
	int multi = 4;
	
	//WICPixelFormatGUID format = GUID_WICPixelFormat32bppRGB;
	//WICPixelFormatGUID format = GUID_WICPixelFormat32bppRGBA; // DXGI_FORMAT_R8G8B8A8_UNORM
	//WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
	//WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGR; // DXGI_FORMAT_B8G8R8X8_UNORM
	WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA; // DXGI_FORMAT_B8G8R8A8_UNORM
	
	CComPtr<IWICImagingFactory2> m_pWICFactory;
	//CComPtr<IWICBitmapSource> pSource2(pSource);
	//CComPtr<IWICBitmapSource> pSource2;
	CComPtr<IWICBitmap> pSource2;
	CComPtr<IWICStream> pStream;
	CComPtr<IWICBitmapEncoder> pEncoder;
	CComPtr<IWICBitmapFrameEncode> pFrame;

	/*hr = CoCreateInstance(
		CLSID_WICImagingFactory2,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory2,
		reinterpret_cast<void**>(&m_pWICFactory)
	);*/
	hr = m_pWICFactory.CoCreateInstance(CLSID_WICImagingFactory2);
	//hr = CoCreateInstance(CLSID_WICImagingFactory2, 0, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory2), (void**)&m_pWICFactory);

	if (SUCCEEDED(hr))
	{
		hr = m_pWICFactory->CreateBitmapFromMemory(pWidth, pHeight, format, pWidth * multi, pWidth * pHeight * multi, (BYTE*)pSource, &pSource2);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pWICFactory->CreateStream(&pStream);
	}

	if (SUCCEEDED(hr))
	{
		hr = pStream->InitializeFromFilename(Path, GENERIC_WRITE);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pWICFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &pEncoder);
	}

	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
	}

	if (SUCCEEDED(hr))
	{
		hr = pEncoder->CreateNewFrame(&pFrame, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->Initialize(NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->SetSize(pWidth, pHeight);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->SetPixelFormat(&format);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->WritePixels(pHeight, pWidth * multi, (UINT)pSourceSize, (BYTE*)pSource);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->Commit();
	}

	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Commit();
	}

	return hr;
}

static HRESULT save_jxr(const BYTE* pSource, size_t pSourceSize, UINT pWidth, UINT pHeight)
{
	LPCWSTR Path = file_path.c_str();
	HRESULT hr;
	int multi = 8; // 64-bit HDR
	WICPixelFormatGUID format = GUID_WICPixelFormat64bppRGBAHalf; // DXGI_FORMAT_R16G16B16A16_FLOAT
	CComPtr<IWICImagingFactory2> m_pWICFactory;
	//CComPtr<IWICBitmapSource> pSource2(pSource);
	//CComPtr<IWICBitmapSource> pSource2;
	CComPtr<IWICBitmap> pSource2;
	CComPtr<IWICStream> pStream;
	CComPtr<IWICBitmapEncoder> pEncoder;
	CComPtr<IWICBitmapFrameEncode> pFrame;

	hr = m_pWICFactory.CoCreateInstance(CLSID_WICImagingFactory2);
	//hr = CoCreateInstance(CLSID_WICImagingFactory2, 0, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory2), (void**)&m_pWICFactory);

	if (SUCCEEDED(hr))
	{
		hr = m_pWICFactory->CreateBitmapFromMemory(pWidth, pHeight, format, pWidth * multi, pWidth * pHeight * multi, (BYTE*)pSource, &pSource2);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pWICFactory->CreateStream(&pStream);
	}

	if (SUCCEEDED(hr))
	{
		hr = pStream->InitializeFromFilename(Path, GENERIC_WRITE);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pWICFactory->CreateEncoder(GUID_ContainerFormatWmp, NULL, &pEncoder);
	}

	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
	}

	if (SUCCEEDED(hr))
	{
		hr = pEncoder->CreateNewFrame(&pFrame, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->Initialize(NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->SetSize(pWidth, pHeight);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->SetPixelFormat(&format);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->WritePixels(pHeight, pWidth * multi, (UINT)pSourceSize, (BYTE*)pSource);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrame->Commit();
	}

	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Commit();
	}

	return hr;
}

static void initialize_screen_capture()
{
	if (!screen_capture_initialized)
	{
		dp_screenshot.HasVideo = 1;
		dp_screenshot.HasAudio = 0;
		dp_screenshot.Cursor = capture_cursor;
		dp_screenshot.ad = reinterpret_cast<IDXGIAdapter1*>(static_cast<uintptr_t>(adapter));
		dp_screenshot.nOutput = monitor;
		dp_screenshot.f = L"";

		// To callback frame
		dp_screenshot.Framer = [](const BYTE* b, size_t sz, UINT wi, UINT he, void* cb)
			{
				if (b && sz)
				{
					/*if (IsHDR)
					{
						set_file_path(L".jxr");
						save_jxr(b, sz, wi, he);
					}
					else
					{*/
						set_file_path(L".png");
						save_png(b, sz, wi, he);
					//}

					if (debug)
					{
						SDL_Log("%s: Captured screenshot.\n", get_date_time().c_str());
					}

					return S_OK;
				}

				return S_FALSE;
			};

		dp_video.HasVideo = 1;
		dp_video.HasAudio = 1;
		dp_video.Cursor = capture_cursor;
		dp_video.ad = reinterpret_cast<IDXGIAdapter1*>(static_cast<uintptr_t>(adapter));
		dp_video.nOutput = monitor;

		if (video_format == L"AV1")
		{
			dp_video.VIDEO_ENCODING_FORMAT = MFVideoFormat_AV1;
		}
		else if (video_format == L"HEVC")
		{
			dp_video.VIDEO_ENCODING_FORMAT = MFVideoFormat_HEVC;
		}
		else
		{
			dp_video.VIDEO_ENCODING_FORMAT = MFVideoFormat_H264;
		}

		dp_video.fps = fps;
		dp_video.BR = video_bitrate;
		dp_video.Qu = video_factor;
		dp_video.vbrm = vbrm;
		dp_video.vbrq = vbrq;
		dp_video.NumThreads = threads;

		if (audio_format == L"FLAC")
		{
			dp_video.AUDIO_ENCODING_FORMAT = MFAudioFormat_FLAC;
		}
		else if (audio_format == L"MP3")
		{
			dp_video.AUDIO_ENCODING_FORMAT = MFAudioFormat_MP3;
		}
		else
		{
			dp_video.AUDIO_ENCODING_FORMAT = MFAudioFormat_AAC;
		}

		dp_video.NCH = audio_channels;
		dp_video.SR = audio_sample_rate;
		dp_video.ABR = audio_bitrate;

		screen_capture_initialized = true;
	}
}

static void take_screenshot()
{
	/*
	// Set config
	tagScreenCaptureFilterConfig config;
	RtlZeroMemory(&config, sizeof(config));
	config.MonitorIdx = 0;
	config.ShowCursor = 0;

	HRESULT hr = S_OK;

	//hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		if (debug)
		{
			SDL_Log("%s: Error[0x%08X]: CoInitialize failed.\n", get_date_time().c_str(), hr);
		}

		return;
	}

	// First initialize
	CDXGICapture dxgiCapture;
	hr = dxgiCapture.Initialize();
	if (FAILED(hr))
	{
		if (debug)
		{
			SDL_Log("%s: Error[0x%08X]: CDXGICapture::Initialize failed.\n", get_date_time().c_str(), hr);
		}

		return;
	}

	// Select monitor by ID
	const tagDublicatorMonitorInfo* pMonitorInfo = dxgiCapture.FindDublicatorMonitorInfo(config.MonitorIdx);
	if (nullptr == pMonitorInfo)
	{
		if (debug)
		{
			SDL_Log("%s: Error: Monitor '%d' was not found.\n", get_date_time().c_str(), config.MonitorIdx);
		}

		return;
	}

	hr = dxgiCapture.SetConfig(config);
	if (FAILED(hr))
	{
		if (debug)
		{
			SDL_Log("%s: Error[0x%08X]: CDXGICapture::SetConfig failed.\n", get_date_time().c_str(), hr);
		}

		return;
	}

	Sleep(50);

	set_file_path(L".png");

	hr = dxgiCapture.CaptureToFile(file_path.c_str(), NULL);

	//dxgiCapture.Terminate();

	if (FAILED(hr))
	{
		if (debug)
		{
			SDL_Log("%s: Error[0x%08X]: CDXGICapture::CaptureToFile failed.\n", get_date_time().c_str(), hr);
		}

		return;
	}
	*/

	if (!screen_capture_initialized)
	{
		initialize_screen_capture();
	}

	DesktopCapture(dp_screenshot);
}

static void record_video()
{
	if (!screen_capture_initialized)
	{
		initialize_screen_capture();
	}

	std::lock_guard<std::mutex> guard(mtx);

	if (!recording_video)
	{
		// Start recording video

		set_file_path(L".mp4");

		dp_video.f = file_path;
		dp_video.MustEnd = false;

		worker = std::thread([] { DesktopCapture(dp_video); }); // Run DesktopCapture in a separate thread
		worker.detach(); // Detach the thread

		recording_video = true;

		if (debug)
		{
			SDL_Log("%s: Video recording started.\n", get_date_time().c_str());
		}
	}
	else
	{
		// Stop recording video
		dp_video.MustEnd = true;
		recording_video = false;

		if (debug)
		{
			SDL_Log("%s: Video recording stopped.\n", get_date_time().c_str());
		}
	}
}

static void initialize_default_input_device()
{
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
	//hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&g_pEndptVol);
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&g_pEndptVol);
	//hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void**>(&g_pEndptVol));

	input_device_initialized = true;
}

static void toggle_mute()
{
	if (!input_device_initialized)
	{
		initialize_default_input_device();
	}

	BOOL bMute;

	g_pEndptVol->GetMute(&bMute); // Get current mute state
	g_pEndptVol->SetMute(!bMute, NULL); // Toggle mute
}

static void set_mute(bool mute)
{
	if (!input_device_initialized)
	{
		initialize_default_input_device();
	}

	g_pEndptVol->SetMute(mute, NULL);
}

static void add_controller_mapping(char* guid)
{
	char mapping_string[1024];

	SDL_strlcpy(mapping_string, guid, sizeof(mapping_string));
	SDL_strlcat(mapping_string, ",Xbox Controller,platform:Windows,", sizeof(mapping_string));
	SDL_strlcat(mapping_string, "a:b0,b:b1,x:b2,y:b3,back:b6,guide:b10,start:b7,leftstick:b8,rightstick:b9,leftshoulder:b4,rightshoulder:b5,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,", sizeof(mapping_string));
	SDL_strlcat(mapping_string, "misc1:b11,", sizeof(mapping_string));

	SDL_AddGamepadMapping(mapping_string);
}

static void load_controller_config(int i)
{
	std::wstring controller = L"controller" + std::to_wstring(i + 1);

	controllers[i].settings.share.key = ini.getIntegers(controller, L"share_key", { 901 });
	controllers[i].settings.share.hold_mode = ini.getInteger(controller, L"share_hold_mode", 2);
	controllers[i].settings.share.longpress_key = ini.getIntegers(controller, L"share_longpress_key", { 902 });
	controllers[i].settings.share.longpress_duration = ini.getInteger(controller, L"share_longpress_duration", 1000);
	controllers[i].settings.share.delay = ini.getInteger(controller, L"share_delay", 0);
	controllers[i].settings.share.duration = ini.getInteger(controller, L"share_duration", 1);
	controllers[i].settings.xbox.key = ini.getIntegers(controller, L"xbox_key", { 0 });
	controllers[i].settings.xbox.hold_mode = ini.getInteger(controller, L"xbox_hold_mode", 2);
	controllers[i].settings.xbox.longpress_key = ini.getIntegers(controller, L"xbox_longpress_key", { 0 });
	controllers[i].settings.xbox.longpress_duration = ini.getInteger(controller, L"xbox_longpress_duration", 1000);
	controllers[i].settings.xbox.delay = ini.getInteger(controller, L"xbox_delay", 0);
	controllers[i].settings.xbox.duration = ini.getInteger(controller, L"xbox_duration", 1);
}

static void add_controllers()
{
	int num_gamepads;
	SDL_JoystickID* gamepads = SDL_GetGamepads(&num_gamepads);

	if (gamepads)
	{
		if (controllers.size() != num_gamepads)
		{
			controllers.resize(num_gamepads);
		}

		for (int i = 0; i < num_gamepads; ++i)
		{
			SDL_JoystickID instance_id = gamepads[i];

			char guid[64];
			SDL_GetJoystickGUIDString(SDL_GetJoystickInstanceGUID(instance_id), guid, sizeof(guid));
			add_controller_mapping(guid);

			if (debug)
			{
				SDL_Log("%s: Mapping: %s\n", get_date_time().c_str(), SDL_GetGamepadMappingForGUID(SDL_GetJoystickInstanceGUID(instance_id)));
			}

			SDL_Gamepad* controller = SDL_OpenGamepad(instance_id);
			if (!controller)
			{
				if (debug)
				{
					SDL_Log("%s: Could not open controller: %s\n", get_date_time().c_str(), SDL_GetError());
				}

				continue;
			}

			controllers[i].controller = controller;
			load_controller_config(i);

			if (debug)
			{
				SDL_Log("%s: Opened controller %d: %s\n", get_date_time().c_str(), (i + 1), SDL_GetGamepadName(controller));
			}
		}

		SDL_free(gamepads);
	}
}

static void close_controllers()
{
	for (int i = 0; i < controllers.size(); ++i)
	{
		SDL_CloseGamepad(controllers[i].controller);
	}

	controllers.clear();

	if (debug)
	{
		SDL_Log("%s: Closed controllers.\n", get_date_time().c_str());
	}
}

static void update_controllers()
{
	int num_gamepads;
	SDL_GetGamepads(&num_gamepads);

	int num_controllers = controllers.size();

	// Update only if the number of controllers changed
	if (num_gamepads != num_controllers)
	{
		// On change close and open all controllers to ensure correct order
		close_controllers();
		add_controllers();
	}
}

static int find_controller(SDL_JoystickID controller_id)
{
	for (int i = 0; i < controllers.size(); ++i)
	{
		if (controller_id == SDL_GetGamepadInstanceID(controllers[i].controller))
		{
			return i;
		}
	}
	return -1;
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

	sdl_initialized = SDL_Init(SDL_INIT_GAMEPAD) == 0;

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

static void key_down(int code)
{
	if (code == 901 || code == 902 || code == 903 || code == 904)
	{
		// Take screenshot / record video / open file / toggle mute/unmute on button release only
		return;
	}
	else if (code == 905)
	{
		set_mute(false);
		return;
	}

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
		inp.ki.wVk = 0; // virtual-key code, doing scan codes instead
		inp.ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press
	}

	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}

static void key_up(int code)
{
	if (code == 901)
	{
		take_screenshot();
		return;
	}
	else if (code == 902)
	{
		record_video();
		return;
	}
	else if (code == 903)
	{
		ShellExecute(NULL, L"open", file_location.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return;
	}
	else if (code == 904)
	{
		toggle_mute();
		return;
	}
	else if (code == 905)
	{
		set_mute(true);
		return;
	}

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
		inp.ki.wVk = 0; // virtual-key code, doing scan codes instead
		inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // 0 for key press
	}

	inp.ki.time = 0;
	inp.ki.dwExtraInfo = 0;
	
	SendInput(1, &inp, sizeof(INPUT));
}

static void key_tap(std::vector<int> code, int64_t delay, int64_t duration)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(delay));

	for (std::size_t i = 0; i < code.size(); ++i)
	{
		key_down(code[i]);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(duration));

	for (std::size_t i = 0; i < code.size(); ++i)
	{
		key_up(code[i]);
	}
}

// Console I/O in a Windows GUI App
// Maximum number of lines the output console should have
static const WORD MAX_CONSOLE_LINES = 500;

static void RedirectIOToConsole()
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

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog, and clog point to console as well
	ios::sync_with_stdio();
}

static void loop()
{
	SDL_Event event;
	int i; // Controller ID
	KeySettings settings;
	//const Uint8* keyboard = SDL_GetKeyboardState(NULL);

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_GAMEPAD_ADDED:
			case SDL_EVENT_GAMEPAD_REMOVED:
				update_controllers();

				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				if (debug)
				{
					SDL_Log("%s: Controller %d button %s %s.\n", get_date_time().c_str(), find_controller(event.gdevice.which) + 1, SDL_GetGamepadStringForButton((SDL_GamepadButton)event.gbutton.button), event.gbutton.state ? "pressed" : "released");
				}

				if (event.gbutton.button == SDL_GAMEPAD_BUTTON_MISC1 || event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE)
				{
					if (debug)
					{
						SDL_Log("%s: %s button pressed.\n", get_date_time().c_str(), (event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE ? "Xbox" : "Share"));
					}

					i = find_controller(event.gdevice.which);
					settings = event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE ? controllers[i].settings.xbox : controllers[i].settings.share;

					if (settings.hold_mode == 1)
					{
						if (settings.key.size() != 0)
						{
							for (std::size_t k = 0; k < settings.key.size(); ++k)
							{
								key_down(settings.key[k]);
							}
						}
					}
					else if (settings.hold_mode == 2)
					{
						controllers[i].button_depressed = hrc::now();
					}
				}

				break;

			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				if (debug)
				{
					SDL_Log("%s: Controller %d button %s %s.\n", get_date_time().c_str(), find_controller(event.gdevice.which) + 1, SDL_GetGamepadStringForButton((SDL_GamepadButton)event.gbutton.button), event.gbutton.state ? "pressed" : "released");
				}

				if (event.gbutton.button == SDL_GAMEPAD_BUTTON_MISC1 || event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE)
				{
					if (debug)
					{
						SDL_Log("%s: %s button released.\n", get_date_time().c_str(), (event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE ? "Xbox" : "Share"));
					}

					i = find_controller(event.gdevice.which);
					settings = event.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE ? controllers[i].settings.xbox : controllers[i].settings.share;

					if (settings.hold_mode == 1)
					{
						if (settings.key.size() != 0)
						{
							for (std::size_t k = 0; k < settings.key.size(); ++k)
							{
								key_up(settings.key[k]);
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

			/*case SDL_EVENT_KEY_DOWN:
				if (debug)
				{
					SDL_Log("%s: Keyboard key pressed.\n", get_date_time().c_str());
				}

				break;

			case SDL_EVENT_KEY_UP:
				if (debug)
				{
					SDL_Log("%s: Keyboard key released.\n", get_date_time().c_str());
				}

				break;*/

			case SDL_EVENT_GAMEPAD_REMAPPED:
				if (debug)
				{
					SDL_Log("%s: Mapping was updated for Controller %d.\n", get_date_time().c_str(), find_controller(event.gdevice.which) + 1);
				}

				break;
			
			case SDL_EVENT_JOYSTICK_ADDED:
				if (debug)
				{
					SDL_Log("%s: Controller connected.\n", get_date_time().c_str());
				}

				break;

			case SDL_EVENT_JOYSTICK_REMOVED:
				if (debug)
				{
					SDL_Log("%s: Controller disconnected.\n", get_date_time().c_str());
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
HWND hWnd;
MSG msg; // Messages to the application are saved here
NOTIFYICONDATA notifyIconData;
const UINT WM_NOTIFYICON = WM_APP + 1;
HMENU hMenu;

// Forward declarations of functions
// Procedures
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

static void AddNotificationIcon(HWND hWnd)
{
	//NOTIFYICONDATA notifyIconData = { sizeof(notifyIconData) };
	//notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hWnd;
	// Add the icon, setting the icon, tooltip, and callback message
	// The icon will be identified with the uID
	notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uCallbackMessage = WM_NOTIFYICON;
	LoadIconMetric(hCurrentInstance, MAKEINTRESOURCE(IDI_ICON), LIM_SMALL, &notifyIconData.hIcon);
	// This text will be shown as the icon's tooltip
	//LoadString(hCurrentInstance, szProgramName, notifyIconData.szTip, ARRAYSIZE(notifyIconData.szTip));
	//wcsncpy(notifyIconData.szTip, szProgramName, sizeof(szProgramName));
	StringCchCopy(notifyIconData.szTip, ARRAYSIZE(notifyIconData.szTip), szProgramName);
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);

	// NOTIFYICON_VERSION_4 is preferred
	notifyIconData.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_SETVERSION, &notifyIconData);

	/*hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");*/
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	MFStartup(MF_VERSION);

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

	// This is the handle for the window
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

	file_location = ini.getString(L"settings", L"file_location", L"C:\\Windows\\explorer.exe");

	file_key = ini.getInteger(L"settings", L"file_key", 0);

	if (file_key != 0)
	{
		UINT file_key_vk = MapVirtualKey(file_key, MAPVK_VSC_TO_VK_EX);
		RegisterHotKey(hWnd, OpenFileHotKeyID, MOD_NOREPEAT, file_key_vk);
	}

	captures_location = ini.getString(L"screen_capture", L"location", L"C:\\Screenshots");

	screenshot_key = ini.getInteger(L"screen_capture", L"screenshot_key", 0);

	if (screenshot_key != 0)
	{
		UINT screenshot_key_vk = MapVirtualKey(screenshot_key, MAPVK_VSC_TO_VK_EX);
		RegisterHotKey(hWnd, ScreenshotHotKeyID, MOD_NOREPEAT, screenshot_key_vk);
	}

	video_key = ini.getInteger(L"screen_capture", L"video_key", 0);

	if (video_key != 0)
	{
		UINT video_key_vk = MapVirtualKey(video_key, MAPVK_VSC_TO_VK_EX);
		RegisterHotKey(hWnd, VideoHotKeyID, MOD_NOREPEAT, video_key_vk);
	}

	capture_cursor = ini.getInteger(L"screen_capture", L"cursor", 0) == 1;
	adapter = ini.getInteger(L"screen_capture", L"adapter", 0);
	monitor = ini.getInteger(L"screen_capture", L"monitor", 0);
	video_format = ini.getString(L"screen_capture", L"video_format", L"H264");
	fps = ini.getInteger(L"screen_capture", L"fps", 60);
	video_bitrate = ini.getInteger(L"screen_capture", L"video_bitrate", 9000);
	video_factor = ini.getInteger(L"screen_capture", L"video_factor", -1);
	vbrm = ini.getInteger(L"screen_capture", L"vbrm", 0);
	vbrq = ini.getInteger(L"screen_capture", L"vbrq", 0);
	threads = ini.getInteger(L"screen_capture", L"threads", 0);
	audio_format = ini.getString(L"screen_capture", L"audio_format", L"AAC");
	audio_channels = ini.getInteger(L"screen_capture", L"audio_channels", 2);
	audio_sample_rate = ini.getInteger(L"screen_capture", L"audio_sample_rate", 48000);
	audio_bitrate = ini.getInteger(L"screen_capture", L"audio_bitrate", 192);

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
			else if (msg.message == WM_HOTKEY)
			{
				if (msg.wParam == ScreenshotHotKeyID)
				{
					if (debug)
					{
						SDL_Log("%s: Screenshot HotKey pressed.\n", get_date_time().c_str());
					}

					take_screenshot();
				}
				else if (msg.wParam == VideoHotKeyID)
				{
					if (debug)
					{
						SDL_Log("%s: Video HotKey pressed.\n", get_date_time().c_str());
					}

					record_video();
				}
				else if (msg.wParam == OpenFileHotKeyID)
				{
					if (debug)
					{
						SDL_Log("%s: Open file HotKey pressed.\n", get_date_time().c_str());
					}

					ShellExecute(NULL, L"open", file_location.c_str(), NULL, NULL, SW_SHOWNORMAL);
				}
			}
		}

		// Do a tick of any intensive stuff here such as graphics processing
		loop();

		// Fix for controller disconnect on subsequent connections
		XINPUT_CAPABILITIES capabilities;
		XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &capabilities);

		/*if (!Shell_NotifyIcon(NIM_MODIFY, &notifyIconData))
		{
			AddNotificationIcon(hWnd);
		}*/

		Sleep(50);
	}

	return msg.wParam;
}

// This function is called by DispatchMessage()
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT s_uTaskbarRestart;

	// Handle the messages
	switch (uMsg)
	{
		case WM_CREATE:
		{
			s_uTaskbarRestart = RegisterWindowMessage(L"TaskbarCreated");
			AddNotificationIcon(hWnd);
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
					DestroyWindow(hWnd);
					break;
				}

				default:
				{
					return DefWindowProc(hWnd, uMsg, wParam, lParam);
				}
			}
		}
		break;

		case WM_NOTIFYICON:
		{
			/*if (lParam == WM_RBUTTONDOWN)
			{
				// Get current mouse cursor position
				POINT curPoint;
				GetCursorPos(&curPoint);
				SetForegroundWindow(hWnd);

				// TrackPopupMenu blocks the app until TrackPopupMenu returns
				UINT clicked = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hWnd, NULL);

				if (clicked == ID_TRAY_EXIT)
				{
					// Quit the application
					Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
					PostQuitMessage(0);
				}
			}*/

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
							// The window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
							SetForegroundWindow(hWnd);

							// Respect menu drop alignment
							UINT uFlags = TPM_RIGHTBUTTON;
							if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
							{
								uFlags |= TPM_RIGHTALIGN;
							}
							else
							{
								uFlags |= TPM_LEFTALIGN;
							}

							//TrackPopupMenu(hSubMenu, uFlags, pt.x, pt.y, 0, hWnd, NULL);
							TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, NULL);
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

			CoUninitialize();

			if (file_key != 0)
			{
				UnregisterHotKey(hWnd, OpenFileHotKeyID);
			}

			if (screenshot_key != 0)
			{
				UnregisterHotKey(hWnd, ScreenshotHotKeyID);
			}

			if (video_key != 0)
			{
				UnregisterHotKey(hWnd, VideoHotKeyID);
			}

			if (input_device_initialized)
			{
				pEnumerator->Release();
				pDevice->Release();
				g_pEndptVol->Release();

				input_device_initialized = false;
			}

			close_controllers();

			//SDL_Quit();
			atexit(SDL_Quit);
			sdl_initialized = false;

			// Remove the tray icon

			/*NOTIFYICONDATA notifyIconData2;

			notifyIconData2.cbSize = sizeof(NOTIFYICONDATA);
			notifyIconData2.hWnd = hWnd;
			notifyIconData2.uID = ID_TRAY_APP_ICON;*/

			Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

			PostQuitMessage(0);

			if (hMutex != NULL)
			{
				// The application is closing so release and close the mutex
				ReleaseMutex(hMutex);
				CloseHandle(hMutex);
			}
		}
		break;

		default:
		{
			if (uMsg == s_uTaskbarRestart)
			{
				AddNotificationIcon(hWnd);
			}

			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

	return 0;
}
