// Source: https://github.com/robmikh/ScreenshotSample
// With modifications
// Take and save screenshots using Windows.Graphics.Capture

#pragma once

// Windows
#include <windows.h>

// Must come before C++/WinRT
#include <wil/cppwinrt.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>

// WIL
#include <wil/resource.h>
#include <wil/coroutine.h>

// DirectX
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// STL
#include <memory>
#include <filesystem>
#include <chrono>
#include <vector>
#include <future>
#include <string>
#include <map>

// robmikh.common
#include <robmikh.common/direct3d11.interop.h>
#include <robmikh.common/d2dHelpers.h>
#include <robmikh.common/d3d11Helpers.h>
#include <robmikh.common/d3d11Helpers.desktop.h>
#include <robmikh.common/graphics.interop.h>
#include <robmikh.common/capture.desktop.interop.h>
#include <robmikh.common/wcliparse.h>

#include "common.h"
#include <chrono>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxguid.lib")

struct Display
{
    static std::vector<Display> GetAllDisplays();
    Display(HMONITOR handle, RECT rect, bool isHDR, float sdrWhiteLevelInNits, float maxLuminance);

    HMONITOR Handle() const { return m_handle; }
    RECT const& Rect() const { return m_rect; }
    bool IsHDR() const { return m_isHDR; }
    // Only valid if IsHDR is true
    float SDRWhiteLevelInNits() const { return m_sdrWhiteLevelInNits; }
    float MaxLuminance() const { return m_maxLuminance; }

    bool operator==(const Display& display) { return m_handle == display.m_handle; }
    bool operator!=(const Display& display) { return !(*this == display); }

private:
    HMONITOR m_handle = nullptr;
    RECT m_rect = {};
    bool m_isHDR = false;
    float m_sdrWhiteLevelInNits = 0.0f;
    float m_maxLuminance = 0.0f;
};

class ToneMapper
{
public:
    ToneMapper(winrt::com_ptr<ID3D11Device> const& d3dDevice);
    ~ToneMapper() {}

    winrt::com_ptr<ID3D11Texture2D> ProcessTexture(winrt::com_ptr<ID3D11Texture2D> const& hdrTexture, float sdrWhiteLevelInNits, float maxLuminance);

private:
    winrt::com_ptr<ID3D11Device> m_d3dDevice;
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext;
    winrt::com_ptr<ID3D11Multithread> m_d3dMultithread;
    winrt::com_ptr<ID2D1Factory1> m_d2dFactory;
    winrt::com_ptr<ID2D1Device1> m_d2dDevice;
    winrt::com_ptr<ID2D1DeviceContext5> m_d2dContext;

    winrt::com_ptr<ID2D1Effect> m_sdrWhiteScaleEffect;
    winrt::com_ptr<ID2D1Effect> m_hdrTonemapEffect;
    winrt::com_ptr<ID2D1Effect> m_colorManagementEffect;
};

struct Snapshot
{
    static wil::task<Snapshot> TakeAsync(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        Display const& display,
        std::shared_ptr<ToneMapper> const& toneMapper);

    winrt::com_ptr<ID3D11Texture2D> Texture;
    RECT DisplayRect = {};
};

class Options
{
public:
    static void InitOptions(bool dxDebug, bool forceHDR, bool clipHDR);

    static bool DxDebug() { return s_options.m_dxDebug; }
    static bool ForceHDR() { return s_options.m_forceHDR; }
    static bool ClipHDR() { return s_options.m_clipHDR; }

private:
    static Options s_options;

    bool m_dxDebug = false;
    bool m_forceHDR = false;
    bool m_clipHDR = false;
};

void take_screenshot();
