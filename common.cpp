#include "common.h"

const wchar_t szProgramName[] = L"Xbox Controller Button Remapper";

bool debug = false;

std::wstring folder_path = L"";
std::wstring filename = L"";
std::wstring file_path = L"";

std::wstring captures_location = L"C:\\Screenshots";

bool capture_border = true;
bool capture_cursor = false;

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

std::string get_date_time()
{
	std::time_t t = std::time(nullptr);
	char date_time[20];
	std::strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
	return date_time;
}

// Source: https://gist.github.com/utilForever/fdf1540cea0de65cfc0a1a69d8cafb63
// Replace some pattern in std::wstring with another pattern
std::wstring replace_wstring_with_pattern(__in const std::wstring& message, __in const std::wstring& pattern, __in const std::wstring& replace)
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

const std::wstring forbidden_chars = L"\\/:*?\"<>|";
bool is_forbidden(wchar_t c)
{
	return std::wstring::npos != forbidden_chars.find(c);
}

// Source: https://gist.github.com/danzek/d6a0e4a48a5439e7f808ed1497f6268e
std::wstring string_to_wstring(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

// Source: https://stackoverflow.com/a/217605
void ltrim(std::wstring& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}));
}

void rtrim(std::wstring& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}).base(), s.end());
}

void trim(std::wstring& s)
{
	rtrim(s);
	ltrim(s);
}

/*const std::string currentDateTime()
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[20];
	tstruct = *localtime(&now);

	strftime(buf, sizeof(buf), "%Y-%m-%d %H-%M-%S", &tstruct);

	return buf;
}*/

/*const std::wstring get_current_date_time()
{
	time_t     now = time(0);
	struct tm  tstruct;
	wchar_t    buf[40];
	tstruct = *localtime(&now);

	wcsftime(buf, (sizeof(buf) / sizeof(wchar_t)), L"%Y-%m-%d %H-%M-%S", &tstruct);

	return buf;
}*/

/*std::wstring get_current_date_time2()
{
	SYSTEMTIME ltime;
	wchar_t TimeStamp[30];

	GetLocalTime(&ltime);

	swprintf(TimeStamp, (sizeof(TimeStamp) / sizeof(wchar_t)), L"%d-%02d-%02d %02d-%02d-%02d-%03d", ltime.wYear, ltime.wMonth, ltime.wDay, ltime.wHour, ltime.wMinute, ltime.wSecond, ltime.wMilliseconds);

	return TimeStamp;
}*/

/*std::string get_current_date_time3()
{
	std::string date_time = DateTime.Now.ToString("yyyy-MM-dd HH-mm-ss-fff");
	return date_time;
}*/

void set_file_path(std::wstring extension)
{
	HWND foreground = GetForegroundWindow();
	WCHAR window_title[256];
	GetWindowText(foreground, window_title, _countof(window_title));

	std::wstring window_title2(window_title);
	//window_title2 = L"Test\\/:*?\"<>|";
	//window_title2 = replace_wstring_with_pattern(window_title2, L":", L"꞉"); // Replace forbidden character with allowed character that looks similar
	window_title2 = replace_wstring_with_pattern(window_title2, L":", L" -");
	std::replace_if(window_title2.begin(), window_title2.end(), is_forbidden, '_');
	trim(window_title2);

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
	//std::time_t t = std::time(nullptr);
	//auto time = std::chrono::system_clock::now();
	//std::string datetime0 = fmt::format("{:%Y-%m-%d %H-%M-%S}", fmt::localtime(t));
	//std::string datetime = fmt::format("{:%Y-%m-%d %H-%M-%S}", std::chrono::system_clock::now());
	//std::string datetime = get_current_date_time();
	//std::wstring datetime2 = std::filesystem::path(datetime).wstring(); // Convert from string to wstring
	std::wstring datetime2 = string_to_wstring(datetime); // Convert from string to wstring
	//std::wstring datetime2 = get_current_date_time2();
	datetime2 = datetime2.substr(0, datetime2.find('.') + 4); // Keep last 3 digits only
	std::replace(datetime2.begin(), datetime2.end(), '.', '-');
	//std::wstring file_path = std::wstring(captures_location) + L"\\" + window_title2 + L" Screenshot " + datetime2 + extension;
	folder_path = std::wstring(captures_location) + L"\\" + window_title2;

	if (!std::filesystem::exists(folder_path)) // Check if folder exists
	{
		if (!std::filesystem::create_directories(folder_path)) // Create folder
		{
			if (debug)
			{
				SDL_Log("%s: Could not create folder for screenshot.\n", get_date_time().c_str());
			}

			return;
		}
	}

	filename = window_title2 + L" " + datetime2 + extension;
	file_path = folder_path + L"\\" + filename;
}
