#pragma once

#include <windows.h>
#include <string>
#include <codecvt>
#include <filesystem>
//#include <fmt/base.h>
//#include <fmt/core.h>
//#include <fmt/format.h>
//#include <fmt/chrono.h>
#include "SDL.h"

extern const wchar_t szProgramName[];

extern bool debug;

extern std::wstring folder_path;
extern std::wstring filename;
extern std::wstring file_path;

extern std::wstring captures_location;

extern bool capture_border;
extern bool capture_cursor;

void message(const LPCWSTR&& text);
void error(const LPCWSTR&& text);
void fatal_error(const LPCWSTR&& text);

std::string get_date_time();

std::wstring replace_wstring_with_pattern(__in const std::wstring& message, __in const std::wstring& pattern, __in const std::wstring& replace);

extern const std::wstring forbidden_chars;
bool is_forbidden(wchar_t c);

std::wstring string_to_wstring(const std::string& str);

void ltrim(std::wstring& s);
void rtrim(std::wstring& s);
void trim(std::wstring& s);

//const std::string currentDateTime();
//const std::wstring get_current_date_time();
//std::wstring get_current_date_time2();
//std::string get_current_date_time3();

void set_file_path(std::wstring extension);
