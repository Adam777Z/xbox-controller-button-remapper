// Source: https://github.com/robmikh/ScreenshotSample
// With modifications
// Take and save screenshots using Windows.Graphics.Capture

#include "ScreenCapture.h"

namespace util
{
    using namespace robmikh::common::desktop;
    using namespace robmikh::common::uwp;
    using namespace robmikh::common::wcli;
}

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::Graphics::Imaging;
    using namespace Windows::Storage;
    using namespace Windows::Storage::Streams;
    using namespace Windows::System;
}

std::map<HMONITOR, float> BuildDisplayHandleToMaxLuminanceMap()
{
    std::map<HMONITOR, float> maxLuminances;

    winrt::com_ptr<IDXGIFactory1> factory;
    winrt::check_hresult(CreateDXGIFactory1(winrt::guid_of<IDXGIFactory1>(), factory.put_void()));

    UINT adapterCount = 0;
    winrt::com_ptr<IDXGIAdapter1> adapter;

    while (SUCCEEDED(factory->EnumAdapters1(adapterCount, adapter.put())))
    {
        UINT outputCount = 0;
        winrt::com_ptr<IDXGIOutput> output;

        while (SUCCEEDED(adapter->EnumOutputs(outputCount, output.put())))
        {
            auto output6 = output.as<IDXGIOutput6>();
            DXGI_OUTPUT_DESC1 desc = {};
            winrt::check_hresult(output6->GetDesc1(&desc));

            if (desc.AttachedToDesktop)
            {
                auto displayHandle = desc.Monitor;
                auto maxLuminance = desc.MaxLuminance;
                maxLuminances.insert({ displayHandle, maxLuminance });
            }

            output = nullptr;
            outputCount++;
        }

        adapter = nullptr;
        adapterCount++;
    }

    return maxLuminances;
}

std::vector<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPathInfos()
{
    uint32_t numPaths = 0;
    uint32_t numModes = 0;

    winrt::check_win32(GetDisplayConfigBufferSizes(
        QDC_ONLY_ACTIVE_PATHS,
        &numPaths,
        &numModes));

    std::vector<DISPLAYCONFIG_PATH_INFO> pathInfos(numPaths, DISPLAYCONFIG_PATH_INFO{});
    std::vector<DISPLAYCONFIG_MODE_INFO> modeInfos(numModes, DISPLAYCONFIG_MODE_INFO{});

    winrt::check_win32(QueryDisplayConfig(
        QDC_ONLY_ACTIVE_PATHS,
        &numPaths,
        pathInfos.data(),
        &numModes,
        modeInfos.data(),
        nullptr));

    pathInfos.resize(numPaths);

    return pathInfos;
}

struct DisplayHDRInfo
{
    bool IsHDR = false;
    // Only valid if IsHDR is true
    float SDRWhiteLevelInNits = 0.0f;
};

std::map<std::wstring, DisplayHDRInfo> BuildDeviceNameToHDRInfoMap()
{
    auto pathInfos = GetDisplayConfigPathInfos();
    std::map<std::wstring, DisplayHDRInfo> namesToHDRInfos;

    for (auto&& pathInfo : pathInfos)
    {
        // Get the device name
        DISPLAYCONFIG_SOURCE_DEVICE_NAME deviceName = {};
        deviceName.header.size = sizeof(deviceName);
        deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        deviceName.header.adapterId = pathInfo.sourceInfo.adapterId;
        deviceName.header.id = pathInfo.sourceInfo.id;
        winrt::check_win32(DisplayConfigGetDeviceInfo(&deviceName.header));
        std::wstring name(deviceName.viewGdiDeviceName);

        // Check to see if the display is in HDR mode
        DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO colorInfo = {};
        colorInfo.header.size = sizeof(colorInfo);
        colorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
        colorInfo.header.adapterId = pathInfo.targetInfo.adapterId;
        colorInfo.header.id = pathInfo.targetInfo.id;
        winrt::check_win32(DisplayConfigGetDeviceInfo(&colorInfo.header));
        bool isHDR = colorInfo.advancedColorEnabled && !colorInfo.wideColorEnforced;

        // Get the SDR white level
        float sdrWhiteLevelInNits = 0.0f;

        if (isHDR)
        {
            DISPLAYCONFIG_SDR_WHITE_LEVEL whiteLevel = {};
            whiteLevel.header.size = sizeof(whiteLevel);
            whiteLevel.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
            whiteLevel.header.adapterId = pathInfo.targetInfo.adapterId;
            whiteLevel.header.id = pathInfo.targetInfo.id;
            winrt::check_win32(DisplayConfigGetDeviceInfo(&whiteLevel.header));
            sdrWhiteLevelInNits = static_cast<float>((whiteLevel.SDRWhiteLevel / 1000.0) * 80.0);
        }

        namesToHDRInfos.insert({ name, { isHDR, sdrWhiteLevelInNits } });
    }

    return namesToHDRInfos;
}

std::vector<Display> Display::GetAllDisplays()
{
    // Get all the display handles
    std::vector<HMONITOR> displayHandles;

    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
        {
            auto& displayHandles = *reinterpret_cast<std::vector<HMONITOR>*>(lparam);
            displayHandles.push_back(hmon);

            return TRUE;
        }, reinterpret_cast<LPARAM>(&displayHandles));

    // The maximum luminance of the display is required, obtained from DXGI
    auto maxLuminances = BuildDisplayHandleToMaxLuminanceMap();

    // Build a mapping of device names to HDR information
    auto namesToHDRInfos = BuildDeviceNameToHDRInfoMap();

    // Go through each display and find the matching HDR info
    std::vector<Display> displays;

    for (auto&& displayHandle : displayHandles)
    {
        // Get the monitor rectangle and device name
        MONITORINFOEXW monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);
        winrt::check_bool(GetMonitorInfoW(displayHandle, &monitorInfo));
        std::wstring name(monitorInfo.szDevice);

        // This sample assumes the displays will be found
        // If the display information is not found, it can be assumed the display isn't HDR
        auto hdrInfo = namesToHDRInfos[name];
        auto maxLuminance = maxLuminances[displayHandle];
        displays.push_back(Display(displayHandle, monitorInfo.rcMonitor, hdrInfo.IsHDR, hdrInfo.SDRWhiteLevelInNits, maxLuminance));
    }

    return displays;
}

Display::Display(HMONITOR handle, RECT rect, bool isHDR, float sdrWhiteLevelInNits, float maxLuminance)
{
    m_handle = handle;
    m_rect = rect;
    m_isHDR = isHDR;
    m_sdrWhiteLevelInNits = sdrWhiteLevelInNits;
    m_maxLuminance = maxLuminance;
}


wil::task<Snapshot> Snapshot::TakeAsync(
    winrt::IDirect3DDevice const& device,
    Display const& display,
    std::shared_ptr<ToneMapper> const& toneMapper)
{
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(device);
    winrt::com_ptr<ID3D11DeviceContext> d3dContext;
    d3dDevice->GetImmediateContext(d3dContext.put());

    // Get the information needed from the display
    auto displayHandle = display.Handle();
    auto displayRect = display.Rect();
    auto isHDR = display.IsHDR();
    auto sdrWhiteLevel = display.SDRWhiteLevelInNits();
    auto maxLuminance = display.MaxLuminance();

    // Debug options
    if (!isHDR && Options::ForceHDR())
    {
        isHDR = true;
        sdrWhiteLevel = D2D1_SCENE_REFERRED_SDR_WHITE_LEVEL;
    }

    if (Options::ClipHDR())
    {
        isHDR = false;
        sdrWhiteLevel = 0.0f;
    }

    // Grab a reference to the tone mapper so that it survives the coming coroutines
    auto hdrToneMapper = toneMapper;

    // HDR captures use an FP16 pixel format, SDR uses BGRA8
    auto capturePixelFormat = isHDR ? winrt::DirectXPixelFormat::R16G16B16A16Float : winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;

    // Set up the capture objects
    // If wanted, this is where to adjust any properties of the GraphicsCaptureSession (e.g. IsCursorCaptureEnabled, IsBorderRequired)
    // The CreateCaptureItemForMonitor helper can be found here: https://github.com/robmikh/robmikh.common/blob/f2311df8de56f31410d14f55de7307464d9a673d/robmikh.common/include/robmikh.common/capture.desktop.interop.h#L16-L23
    auto item = util::CreateCaptureItemForMonitor(displayHandle);
    auto framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
        device,
        capturePixelFormat,
        1,
        item.Size());
    auto session = framePool.CreateCaptureSession(item);

    session.IsBorderRequired(capture_border); // Whether to show a colored border around the display to indicate that a capture is in progress
    session.IsCursorCaptureEnabled(capture_cursor);

    // Get one frame and then end the capture
    winrt::com_ptr<ID3D11Texture2D> captureTexture;
    wil::shared_event captureEvent(wil::EventOptions::ManualReset);

    framePool.FrameArrived([session, d3dDevice, d3dContext, captureEvent, &captureTexture](auto&& framePool, auto&&) -> void
        {
            auto frame = framePool.TryGetNextFrame();
            auto surface = frame.Surface();
            auto frameTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(surface);

            framePool.Close();
            session.Close();

            captureTexture.copy_from(frameTexture.get());
            captureEvent.SetEvent();
        });

    session.StartCapture();

    // Wait for the next frame to show up
    co_await winrt::resume_on_signal(captureEvent.get());

    // The caller is expecting a BGRA8 texture
    // If captured in HDR, tone map the texture and return the result
    winrt::com_ptr<ID3D11Texture2D> resultTexture;

    if (isHDR)
    {
        // Tonemap the texture
        resultTexture.copy_from(hdrToneMapper->ProcessTexture(captureTexture, sdrWhiteLevel, maxLuminance).get());
    }
    else
    {
        // If an SDR display is captured, the capture can be used directly
        resultTexture.copy_from(captureTexture.get());
    }

    co_return Snapshot{ resultTexture, displayRect };
}


ToneMapper::ToneMapper(winrt::com_ptr<ID3D11Device> const& d3dDevice)
{
    auto d2dDebugFlag = D2D1_DEBUG_LEVEL_NONE;

    if (Options::DxDebug())
    {
        d2dDebugFlag = D2D1_DEBUG_LEVEL_INFORMATION;
    }

    // These helpers can be found in the robmikh.common package:
    // CreateD2DFactory: https://github.com/robmikh/robmikh.common/blob/f2311df8de56f31410d14f55de7307464d9a673d/robmikh.common/include/robmikh.common/d3dHelpers.h#L81-L89
    // CreateD2DDevice: https://github.com/robmikh/robmikh.common/blob/f2311df8de56f31410d14f55de7307464d9a673d/robmikh.common/include/robmikh.common/d3dHelpers.h#L53-L58
    m_d3dDevice = d3dDevice;
    m_d3dDevice->GetImmediateContext(m_d3dContext.put());
    m_d3dMultithread = m_d3dDevice.as<ID3D11Multithread>();
    m_d2dFactory = util::CreateD2DFactory(d2dDebugFlag);
    auto d2dDevice = util::CreateD2DDevice(m_d2dFactory, d3dDevice);
    m_d2dDevice = d2dDevice.as<ID2D1Device1>();
    winrt::com_ptr<ID2D1DeviceContext1> d2dContext;
    winrt::check_hresult(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2dContext.put()));
    m_d2dContext = d2dContext.as<ID2D1DeviceContext5>();

    // Create the effects
    winrt::check_hresult(m_d2dContext->CreateEffect(CLSID_D2D1WhiteLevelAdjustment, m_sdrWhiteScaleEffect.put()));
    winrt::check_hresult(m_d2dContext->CreateEffect(CLSID_D2D1HdrToneMap, m_hdrTonemapEffect.put()));
    winrt::check_hresult(m_d2dContext->CreateEffect(CLSID_D2D1ColorManagement, m_colorManagementEffect.put()));

    // Set up the effect graph connections
    m_sdrWhiteScaleEffect->SetInputEffect(0, m_hdrTonemapEffect.get());
    m_colorManagementEffect->SetInputEffect(0, m_sdrWhiteScaleEffect.get());

    // Set up the tone map effect
    winrt::check_hresult(m_hdrTonemapEffect->SetValue(D2D1_HDRTONEMAP_PROP_DISPLAY_MODE, D2D1_HDRTONEMAP_DISPLAY_MODE_SDR));

    // Set up the output color management effect
    {
        winrt::check_hresult(m_colorManagementEffect->SetValue(D2D1_COLORMANAGEMENT_PROP_QUALITY, D2D1_COLORMANAGEMENT_QUALITY_BEST));

        winrt::com_ptr<ID2D1ColorContext> inputColorContext;
        winrt::check_hresult(m_d2dContext->CreateColorContext(D2D1_COLOR_SPACE_SCRGB, nullptr, 0, inputColorContext.put()));
        winrt::check_hresult(m_colorManagementEffect->SetValue(D2D1_COLORMANAGEMENT_PROP_SOURCE_COLOR_CONTEXT, inputColorContext.get()));

        winrt::com_ptr<ID2D1ColorContext1> outputColorContext;
        winrt::check_hresult(m_d2dContext->CreateColorContextFromDxgiColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, outputColorContext.put()));
        winrt::check_hresult(m_colorManagementEffect->SetValue(D2D1_COLORMANAGEMENT_PROP_DESTINATION_COLOR_CONTEXT, outputColorContext.get()));
    }
}

winrt::com_ptr<ID3D11Texture2D> ToneMapper::ProcessTexture(winrt::com_ptr<ID3D11Texture2D> const& hdrTexture, float sdrWhiteLevelInNits, float maxLuminance)
{
    // The D3D11DeviceLock RAII wrapper can be found here:
    // https://github.com/robmikh/robmikh.common/blob/f2311df8de56f31410d14f55de7307464d9a673d/robmikh.common/include/robmikh.common/d3dHelpers.h#L30-L46
    auto multithreadLock = util::D3D11DeviceLock(m_d3dMultithread.get());

    D3D11_TEXTURE2D_DESC desc = {};
    hdrTexture->GetDesc(&desc);
    auto dxgiSurface = hdrTexture.as<IDXGISurface>();

    // Create a D2D image from the texture
    winrt::com_ptr<ID2D1ImageSource> d2dImageSource;
    std::vector<IDXGISurface*> surfaces = { dxgiSurface.get() };
    winrt::check_hresult(m_d2dContext->CreateImageSourceFromDxgi(
        surfaces.data(),
        static_cast<uint32_t>(surfaces.size()),
        // Will properly adjust the color space using the color management effect
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,
        D2D1_IMAGE_SOURCE_FROM_DXGI_OPTIONS_NONE,
        d2dImageSource.put()));

    // Finish setting up the HDR tone map effect
    winrt::check_hresult(m_hdrTonemapEffect->SetValue(D2D1_HDRTONEMAP_PROP_OUTPUT_MAX_LUMINANCE, sdrWhiteLevelInNits));
    winrt::check_hresult(m_hdrTonemapEffect->SetValue(D2D1_HDRTONEMAP_PROP_INPUT_MAX_LUMINANCE, maxLuminance));

    // Set up the white scale effect
    // 10% of the range is reserved for highlights
    // Reserving more for highlights will make "paper white" dimmer
    winrt::check_hresult(m_sdrWhiteScaleEffect->SetValue(D2D1_WHITELEVELADJUSTMENT_PROP_OUTPUT_WHITE_LEVEL, sdrWhiteLevelInNits));
    winrt::check_hresult(m_sdrWhiteScaleEffect->SetValue(D2D1_WHITELEVELADJUSTMENT_PROP_INPUT_WHITE_LEVEL, D2D1_SCENE_REFERRED_SDR_WHITE_LEVEL * 0.90f));

    // Hookup the HDR texture to the effect graph
    m_hdrTonemapEffect->SetInput(0, d2dImageSource.get());

    // Get the image from the last effect to use to draw
    winrt::com_ptr<ID2D1Image> effectImage;
    m_colorManagementEffect->GetOutput(effectImage.put());

    // Create the output texture
    winrt::com_ptr<ID3D11Texture2D> outputTexture;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    winrt::check_hresult(m_d3dDevice->CreateTexture2D(&desc, nullptr, outputTexture.put()));

    // Create a render target
    auto outputDxgiSurface = outputTexture.as<IDXGISurface>();
    winrt::com_ptr<ID2D1Bitmap1> d2dTargetBitmap;
    winrt::check_hresult(m_d2dContext->CreateBitmapFromDxgiSurface(outputDxgiSurface.get(), nullptr, d2dTargetBitmap.put()));

    // Set the render target as the current target
    m_d2dContext->SetTarget(d2dTargetBitmap.get());

    // Draw to the render target
    m_d2dContext->BeginDraw();
    m_d2dContext->Clear(D2D1::ColorF(0, 0));
    m_d2dContext->DrawImage(effectImage.get(), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    winrt::check_hresult(m_d2dContext->EndDraw());

    m_d2dContext->SetTarget(nullptr);

    return outputTexture;
}


Options Options::s_options = {};

void Options::InitOptions(bool dxDebug, bool forceHDR, bool clipHDR)
{
    s_options.m_dxDebug = dxDebug;
    s_options.m_forceHDR = forceHDR;
    s_options.m_clipHDR = clipHDR;
}


float CLEARCOLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // RGBA

wil::task<winrt::com_ptr<ID3D11Texture2D>> ComposeSnapshotsAsync(
    winrt::IDirect3DDevice const& device,
    std::vector<Display> const& displays,
    std::shared_ptr<ToneMapper> const& toneMapper);
//winrt::IAsyncOperation<winrt::StorageFile> CreateLocalFileAsync(std::wstring const& fileName);
winrt::IAsyncOperation<winrt::StorageFile> CreateLocalFileAsync();
winrt::IAsyncAction SaveTextureToFileAsync(
    winrt::com_ptr<ID3D11Texture2D> const& texture,
    winrt::StorageFile const& file);
void LoadOptions(bool dxDebug, bool forceHDR, bool clipHDR);

winrt::IAsyncAction MainAsync()
{
    // Init D3D
    uint32_t d3dFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    if (Options::DxDebug())
    {
        d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    // These helpers can be found in the robmikh.common package:
    // CreateD3D11Device: https://github.com/robmikh/robmikh.common/blob/5b17ef46d28e0d188baaa1d091ef1354eeb3c42c/robmikh.common/include/robmikh.common/d3d11Helpers.h#L34-L45
    // CreateDirect3DDevice: https://github.com/robmikh/robmikh.common/blob/f2311df8de56f31410d14f55de7307464d9a673d/robmikh.common/include/robmikh.common/direct3d11.interop.h#L19-L24
    auto d3dDevice = util::CreateD3D11Device(d3dFlags);
    auto device = CreateDirect3DDevice(d3dDevice.as<IDXGIDevice>().get());

    // Create the tone mapper
    auto toneMapper = std::make_shared<ToneMapper>(d3dDevice);

    // Enumerate displays
    auto displays = Display::GetAllDisplays();

    for (auto&& display : displays)
    {
        if (debug)
        {
            if (display.IsHDR())
            {
                SDL_Log("%s: Found HDR display with white level: %f and max luminance: %f.\n", get_date_time().c_str(), display.SDRWhiteLevelInNits(), display.MaxLuminance());
            }
            else
            {
                SDL_Log("%s: Found SDR display.\n", get_date_time().c_str());
            }
        }
    }

    // Compose the displays
    auto composedTexture = co_await ComposeSnapshotsAsync(device, displays, toneMapper);

    // Save the texture to a file
    //auto file = co_await CreateLocalFileAsync(L"screenshot.png");
    auto file = co_await CreateLocalFileAsync();
    co_await SaveTextureToFileAsync(composedTexture, file);

    if (debug)
    {
        SDL_Log("%s: Screenshot saved.\n", get_date_time().c_str());
    }

    //co_await winrt::Launcher::LaunchFileAsync(file); // Open file

    co_return;
}

wil::task<winrt::com_ptr<ID3D11Texture2D>> ComposeSnapshotsAsync(
    winrt::IDirect3DDevice const& device,
    std::vector<Display> const& displays,
    std::shared_ptr<ToneMapper> const& toneMapper)
{
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(device);
    winrt::com_ptr<ID3D11DeviceContext> d3dContext;
    d3dDevice->GetImmediateContext(d3dContext.put());

    // Determine the union of all displays
    RECT unionRect = {};
    unionRect.left = LONG_MAX;
    unionRect.top = LONG_MAX;
    unionRect.right = LONG_MIN;
    unionRect.bottom = LONG_MIN;

    for (auto&& display : displays)
    {
        auto& displayRect = display.Rect();

        if (unionRect.left > displayRect.left)
        {
            unionRect.left = displayRect.left;
        }

        if (unionRect.top > displayRect.top)
        {
            unionRect.top = displayRect.top;
        }

        if (unionRect.right < displayRect.right)
        {
            unionRect.right = displayRect.right;
        }

        if (unionRect.bottom < displayRect.bottom)
        {
            unionRect.bottom = displayRect.bottom;
        }
    }

    // Capture each display
    std::vector<wil::task<Snapshot>> futures;

    for (auto&& display : displays)
    {
        auto future = Snapshot::TakeAsync(device, display, toneMapper);
        futures.push_back(std::move(future));
    }

    // Create the texture to compose everything onto
    winrt::com_ptr<ID3D11Texture2D> composedTexture;
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = static_cast<uint32_t>(unionRect.right - unionRect.left);
    textureDesc.Height = static_cast<uint32_t>(unionRect.bottom - unionRect.top);
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    winrt::check_hresult(d3dDevice->CreateTexture2D(&textureDesc, nullptr, composedTexture.put()));
    // Clear to black
    winrt::com_ptr<ID3D11RenderTargetView> composedRenderTargetView;
    winrt::check_hresult(d3dDevice->CreateRenderTargetView(composedTexture.get(), nullptr, composedRenderTargetView.put()));
    d3dContext->ClearRenderTargetView(composedRenderTargetView.get(), CLEARCOLOR);

    // Compose the textures into one texture
    for (auto&& future : futures)
    {
        auto snapshot = co_await std::move(future);

        D3D11_TEXTURE2D_DESC desc = {};
        snapshot.Texture->GetDesc(&desc);

        auto destX = snapshot.DisplayRect.left - unionRect.left;
        auto destY = snapshot.DisplayRect.top - unionRect.top;

        D3D11_BOX region = {};
        region.left = 0;
        region.right = desc.Width;
        region.top = 0;
        region.bottom = desc.Height;
        region.back = 1;

        d3dContext->CopySubresourceRegion(composedTexture.get(), 0, destX, destY, 0, snapshot.Texture.get(), 0, &region);
    }

    co_return composedTexture;
}

//winrt::IAsyncOperation<winrt::StorageFile> CreateLocalFileAsync(std::wstring const& fileName)
winrt::IAsyncOperation<winrt::StorageFile> CreateLocalFileAsync()
{
    //auto currentPath = std::filesystem::current_path();
    //auto folder = co_await winrt::StorageFolder::GetFolderFromPathAsync(currentPath.wstring());
    auto folder = co_await winrt::StorageFolder::GetFolderFromPathAsync(folder_path);
    auto file = co_await folder.CreateFileAsync(filename, winrt::CreationCollisionOption::GenerateUniqueName);
    co_return file;
}

winrt::IAsyncAction SaveTextureToFileAsync(
    winrt::com_ptr<ID3D11Texture2D> const& texture,
    winrt::StorageFile const& file)
{
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);

    // These helpers can be found in the robmikh.common package:
    // CopyBytesFromTexture: https://github.com/robmikh/robmikh.common/blob/f2311df8de56f31410d14f55de7307464d9a673d/robmikh.common/include/robmikh.common/d3dHelpers.h#L250-L282
    auto bytes = util::CopyBytesFromTexture(texture);

    auto stream = co_await file.OpenAsync(winrt::FileAccessMode::ReadWrite);
    auto encoder = co_await winrt::BitmapEncoder::CreateAsync(winrt::BitmapEncoder::PngEncoderId(), stream);

    encoder.SetPixelData(
        winrt::BitmapPixelFormat::Bgra8,
        winrt::BitmapAlphaMode::Premultiplied,
        desc.Width,
        desc.Height,
        1.0,
        1.0,
        bytes);

    co_await encoder.FlushAsync();

    co_return;
}

void LoadOptions(
    bool dxDebug = false, // Use the D3D and D2D debug layers
    bool forceHDR = false, // Force all monitors to be captured as HDR, used for debugging
    bool clipHDR = false // Clip HDR content instead of tone mapping
)
{
    if (clipHDR && forceHDR)
    {
        error(L"Cannot simultaneously clip and force HDR!\n");
        return;
    }

    Options::InitOptions(dxDebug, forceHDR, clipHDR);

    if (debug)
    {
        if (dxDebug)
        {
            SDL_Log("%s: Using D3D and D2D debug layers...\n", get_date_time().c_str());
        }

        if (forceHDR)
        {
            SDL_Log("%s: Forcing HDR capture for all monitors...\n", get_date_time().c_str());
        }

        if (clipHDR)
        {
            SDL_Log("%s: Clipping HDR content...\n", get_date_time().c_str());
        }
    }
}

void take_screenshot()
{
    auto isCaptureSupported = winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();

    if (!isCaptureSupported)
    {
        if (debug)
        {
            SDL_Log("%s: Screen capture is not supported on this device for this release of Windows.\n", get_date_time().c_str());
        }

        return;
    }

    // Check that the minimum required features are available
    /*auto isCaptureSupported = winrt::GraphicsCaptureSession::IsSupported();
    auto sampleSupported = winrt::ApiInformation::IsMethodPresent(winrt::name_of<winrt::MediaStreamSample>(), L"CreateFromDirect3D11Surface");
    if (!isCaptureSupported || !sampleSupported)
    {
        if (debug)
        {
            SDL_Log("%s: Screen capture is not supported on this device for this release of Windows.\n", get_date_time().c_str());
            //SDL_Log("%s: This release of Windows does not have the minimum required features. Please update to a newer release.\n", get_date_time().c_str());
        }

        return;
    }*/

    // Init COM
    //winrt::init_apartment();

    // Don't want virtualized coordinates
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    //LoadOptions();

    // Run synchronously
    try
    {
        set_file_path(L".png");
        MainAsync().get();
    }
    catch (winrt::hresult_error const& error)
    {
        if (debug)
        {
            SDL_Log("%s: 0x%08x - %s\n", get_date_time().c_str(), error.code().value, winrt::to_string(error.message()).c_str());
        }
    }
}
