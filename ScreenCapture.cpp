// Source: https://github.com/GERD0GDU/dxgi_desktop_capture
// With modifications
/*****************************************************************************
* DXGI Desktop Capture
*
* Copyright (C) 2020 Gokhan Erdogdu <gokhan_erdogdu - at - yahoo - dot - com>
*
* DXGICapture is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; either version 2.1 of the License, or (at your option)
* any later version.
*
* DXGICapture is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
* details.
*
******************************************************************************/

#include "ScreenCapture.h"

#include <chrono>

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")

#pragma comment(lib, "shcore.lib")     // SetProcessDpiAwareness

// Driver types supported
const D3D_DRIVER_TYPE g_DriverTypes[] =
{
	D3D_DRIVER_TYPE_HARDWARE,
	D3D_DRIVER_TYPE_WARP,
	D3D_DRIVER_TYPE_REFERENCE,
};
const UINT g_NumDriverTypes = ARRAYSIZE(g_DriverTypes);

// Feature levels supported
const D3D_FEATURE_LEVEL g_FeatureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_1
};
const UINT g_NumFeatureLevels = ARRAYSIZE(g_FeatureLevels);

#define AUTOLOCK()                  ATL::CComCritSecLock<ATL::CComAutoCriticalSection> auto_lock((ATL::CComAutoCriticalSection&)(m_csLock))

//
// class CDXGICapture
//
CDXGICapture::CDXGICapture()
	: m_csLock()
	, m_bInitialized(FALSE)
	, m_lD3DFeatureLevel(D3D_FEATURE_LEVEL_INVALID)
{
	RtlZeroMemory(&m_rendererInfo, sizeof(m_rendererInfo));
	RtlZeroMemory(&m_mouseInfo, sizeof(m_mouseInfo));
	RtlZeroMemory(&m_tempMouseBuffer, sizeof(m_tempMouseBuffer));
	RtlZeroMemory(&m_desktopOutputDesc, sizeof(m_desktopOutputDesc));
}

CDXGICapture::~CDXGICapture()
{
	this->Terminate();
}

HRESULT CDXGICapture::loadMonitorInfos(ID3D11Device* pDevice)
{
	CHECK_POINTER(pDevice);

	HRESULT hr = S_OK;
	CComPtr<ID3D11Device> ipDevice(pDevice);

	// Get DXGI device
	CComPtr<IDXGIDevice> ipDxgiDevice;
	hr = ipDevice->QueryInterface(IID_PPV_ARGS(&ipDxgiDevice));
	if (FAILED(hr)) {
		return hr;
	}

	// Get DXGI adapter
	CComPtr<IDXGIAdapter> ipDxgiAdapter;
	hr = ipDxgiDevice->GetParent(IID_PPV_ARGS(&ipDxgiAdapter));
	if (FAILED(hr)) {
		return hr;
	}
	ipDxgiDevice = nullptr;

	CComPtr<IDXGIOutput> ipDxgiOutput;
	for (UINT i = 0; SUCCEEDED(hr); ++i)
	{
		ipDxgiOutput = nullptr;
		hr = ipDxgiAdapter->EnumOutputs(i, &ipDxgiOutput);
		if ((nullptr != ipDxgiOutput) && (hr != DXGI_ERROR_NOT_FOUND))
		{
			DXGI_OUTPUT_DESC DesktopDesc;
			hr = ipDxgiOutput->GetDesc(&DesktopDesc);
			if (FAILED(hr)) {
				continue;
			}

			tagDublicatorMonitorInfo* pInfo;
			pInfo = new (std::nothrow) tagDublicatorMonitorInfo;
			if (nullptr == pInfo) {
				return E_OUTOFMEMORY;
			}

			hr = DXGICaptureHelper::ConvertDxgiOutputToMonitorInfo(&DesktopDesc, i, pInfo);
			if (FAILED(hr)) {
				delete pInfo;
				continue;
			}

			m_monitorInfos.push_back(pInfo);
		}
	}

	ipDxgiOutput = nullptr;
	ipDxgiAdapter = nullptr;

	return S_OK;
}

void CDXGICapture::freeMonitorInfos()
{
	size_t nCount = m_monitorInfos.size();
	if (nCount == 0) {
		return;
	}
	DublicatorMonitorInfoVec::iterator it = m_monitorInfos.begin();
	DublicatorMonitorInfoVec::iterator end = m_monitorInfos.end();
	for (size_t i = 0; (i < nCount) && (it != end); i++, it++) {
		tagDublicatorMonitorInfo* pInfo = *it;
		if (nullptr != pInfo) {
			delete pInfo;
		}
	}
	m_monitorInfos.clear();
}

HRESULT CDXGICapture::createDeviceResource(
	const tagScreenCaptureFilterConfig* pConfig,
	const tagDublicatorMonitorInfo* pSelectedMonitorInfo
)
{
	HRESULT hr = S_OK;

	CComPtr<IDXGIOutputDuplication> ipDxgiOutputDuplication;
	CComPtr<ID3D11Texture2D>        ipCopyTexture2D;
	CComPtr<ID2D1Device>            ipD2D1Device;
	CComPtr<ID2D1DeviceContext>     ipD2D1DeviceContext;
	CComPtr<ID2D1Factory>           ipD2D1Factory;
	CComPtr<IWICImagingFactory>     ipWICImageFactory;
	CComPtr<IWICBitmap>             ipWICOutputBitmap;
	CComPtr<ID2D1RenderTarget>      ipD2D1RenderTarget;
	DXGI_OUTPUT_DESC                dgixOutputDesc;
	tagRendererInfo                 rendererInfo;

	RtlZeroMemory(&dgixOutputDesc, sizeof(dgixOutputDesc));
	RtlZeroMemory(&rendererInfo, sizeof(rendererInfo));

	// copy configuration to renderer info
	rendererInfo.MonitorIdx = pConfig->MonitorIdx;
	rendererInfo.ShowCursor = pConfig->ShowCursor;
	// default
	rendererInfo.ScaleX = 1.0f;
	rendererInfo.ScaleY = 1.0f;

	do
	{
		// Get DXGI factory
		CComPtr<IDXGIDevice> ipDxgiDevice;
		hr = m_ipD3D11Device->QueryInterface(IID_PPV_ARGS(&ipDxgiDevice));
		CHECK_HR_BREAK(hr);

		CComPtr<IDXGIAdapter> ipDxgiAdapter;
		hr = ipDxgiDevice->GetParent(IID_PPV_ARGS(&ipDxgiAdapter));
		CHECK_HR_BREAK(hr);

		// Get output
		CComPtr<IDXGIOutput> ipDxgiOutput;
		hr = ipDxgiAdapter->EnumOutputs(rendererInfo.MonitorIdx, &ipDxgiOutput);
		CHECK_HR_BREAK(hr);

		// Get output description		
		hr = ipDxgiOutput->GetDesc(&dgixOutputDesc);
		CHECK_HR_BREAK(hr);

		tagDublicatorMonitorInfo curMonInfo;
		hr = DXGICaptureHelper::ConvertDxgiOutputToMonitorInfo(&dgixOutputDesc, rendererInfo.MonitorIdx, &curMonInfo);
		CHECK_HR_BREAK(hr);

		if (!DXGICaptureHelper::IsEqualMonitorInfo(pSelectedMonitorInfo, &curMonInfo)) {
			hr = E_INVALIDARG; // Monitor settings have changed ???
			break;
		}

		// QI for Output 1
		CComPtr<IDXGIOutput1> ipDxgiOutput1;
		hr = ipDxgiOutput->QueryInterface(IID_PPV_ARGS(&ipDxgiOutput1));
		CHECK_HR_BREAK(hr);

		// Create desktop duplication
		hr = ipDxgiOutput1->DuplicateOutput(m_ipD3D11Device, &ipDxgiOutputDuplication);
		CHECK_HR_BREAK(hr);

		DXGI_OUTDUPL_DESC dxgiOutputDuplDesc;
		ipDxgiOutputDuplication->GetDesc(&dxgiOutputDuplDesc);

		hr = DXGICaptureHelper::CalculateRendererInfo(&dxgiOutputDuplDesc, &rendererInfo);
		CHECK_HR_BREAK(hr);

		// Create CPU access texture
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = rendererInfo.SrcBounds.Width;
		desc.Height = rendererInfo.SrcBounds.Height;
		desc.Format = rendererInfo.SrcFormat;
		desc.ArraySize = 1;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_STAGING;

		hr = m_ipD3D11Device->CreateTexture2D(&desc, NULL, &ipCopyTexture2D);
		CHECK_HR_BREAK(hr);

		if (nullptr == ipCopyTexture2D)
		{
			hr = E_OUTOFMEMORY;
			break;
		}

#pragma region <For_2D_operations>

		// Create D2D1 device
		UINT uiFlags = m_ipD3D11Device->GetCreationFlags();
		D2D1_CREATION_PROPERTIES d2d1Props = D2D1::CreationProperties
		(
			(uiFlags & D3D11_CREATE_DEVICE_SINGLETHREADED)
			? D2D1_THREADING_MODE_SINGLE_THREADED
			: D2D1_THREADING_MODE_MULTI_THREADED,
			D2D1_DEBUG_LEVEL_NONE,
			(uiFlags & D3D11_CREATE_DEVICE_SINGLETHREADED)
			? D2D1_DEVICE_CONTEXT_OPTIONS_NONE
			: D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS
		);
		hr = D2D1CreateDevice(ipDxgiDevice, d2d1Props, &ipD2D1Device);
		CHECK_HR_BREAK(hr);

		// Get D2D1 factory
		ipD2D1Device->GetFactory(&ipD2D1Factory);

		if (nullptr == ipD2D1Factory)
		{
			hr = D2DERR_INVALID_CALL;
			break;
		}

		//create WIC factory
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			reinterpret_cast<void**>(&ipWICImageFactory)
		);
		CHECK_HR_BREAK(hr);

		// create D2D1 target bitmap for render
		hr = ipWICImageFactory->CreateBitmap(
			(UINT)rendererInfo.DstBounds.Width,
			(UINT)rendererInfo.DstBounds.Height,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapCacheOnDemand,
			&ipWICOutputBitmap);
		CHECK_HR_BREAK(hr);

		if (nullptr == ipWICOutputBitmap)
		{
			hr = E_OUTOFMEMORY;
			break;
		}

		// create a D2D1 render target (for D2D1 drawing)
		D2D1_RENDER_TARGET_PROPERTIES d2d1RenderTargetProp = D2D1::RenderTargetProperties
		(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			0.0f, // default dpi
			0.0f, // default dpi
			D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE
		);
		hr = ipD2D1Factory->CreateWicBitmapRenderTarget(
			ipWICOutputBitmap,
			d2d1RenderTargetProp,
			&ipD2D1RenderTarget
		);
		CHECK_HR_BREAK(hr);

#pragma endregion </For_2D_operations>

	} while (false);

	if (SUCCEEDED(hr))
	{
		// copy output parameters
		memcpy_s((void*)&m_rendererInfo, sizeof(m_rendererInfo), (const void*)&rendererInfo, sizeof(m_rendererInfo));

		// set parameters
		m_desktopOutputDesc = dgixOutputDesc;

		m_ipDxgiOutputDuplication = ipDxgiOutputDuplication;
		m_ipCopyTexture2D = ipCopyTexture2D;

		m_ipD2D1Device = ipD2D1Device;
		m_ipD2D1Factory = ipD2D1Factory;
		m_ipWICImageFactory = ipWICImageFactory;
		m_ipWICOutputBitmap = ipWICOutputBitmap;
		m_ipD2D1RenderTarget = ipD2D1RenderTarget;
	}

	return S_OK;
}

void CDXGICapture::terminateDeviceResource()
{
	m_ipDxgiOutputDuplication = nullptr;
	m_ipCopyTexture2D = nullptr;

	m_ipD2D1Device = nullptr;
	m_ipD2D1Factory = nullptr;
	m_ipWICImageFactory = nullptr;
	m_ipWICOutputBitmap = nullptr;
	m_ipD2D1RenderTarget = nullptr;

	// clear config parameters
	RtlZeroMemory(&m_rendererInfo, sizeof(m_rendererInfo));

	// clear mouse information parameters
	if (m_mouseInfo.PtrShapeBuffer != nullptr) {
		delete[] m_mouseInfo.PtrShapeBuffer;
		m_mouseInfo.PtrShapeBuffer = nullptr;
	}
	RtlZeroMemory(&m_mouseInfo, sizeof(m_mouseInfo));

	// clear temp temp buffer
	if (m_tempMouseBuffer.Buffer != nullptr) {
		delete[] m_tempMouseBuffer.Buffer;
		m_tempMouseBuffer.Buffer = nullptr;
	}
	RtlZeroMemory(&m_tempMouseBuffer, sizeof(m_tempMouseBuffer));

	// clear desktop output desc
	RtlZeroMemory(&m_desktopOutputDesc, sizeof(m_desktopOutputDesc));
}

HRESULT CDXGICapture::Initialize()
{
	AUTOLOCK();
	if (m_bInitialized) {
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED); // already initialized
	}

	HRESULT hr = S_OK;
	D3D_FEATURE_LEVEL lFeatureLevel;
	CComPtr<ID3D11Device> ipDevice;
	CComPtr<ID3D11DeviceContext> ipDeviceContext;

	// required for monitor dpi problem (???)
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// Create device
	for (UINT i = 0; i < g_NumDriverTypes; ++i)
	{
		hr = D3D11CreateDevice(
			nullptr,
			g_DriverTypes[i],
			nullptr,
			/* D3D11_CREATE_DEVICE_BGRA_SUPPORT
			* This flag adds support for surfaces with a different
			* color channel ordering than the API default.
			* You need it for compatibility with Direct2D. */
			D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			g_FeatureLevels,
			g_NumFeatureLevels,
			D3D11_SDK_VERSION,
			&ipDevice,
			&lFeatureLevel,
			&ipDeviceContext);

		if (SUCCEEDED(hr))
		{
			// Device creation success, no need to loop anymore
			break;
		}

		ipDevice = nullptr;
		ipDeviceContext = nullptr;
	}

	if (FAILED(hr)) {
		return hr;
	}

	if (nullptr == ipDevice) {
		return E_UNEXPECTED;
	}

	// load all monitor informations
	hr = loadMonitorInfos(ipDevice);
	if (FAILED(hr)) {
		return hr;
	}

	// set common fields
	m_lD3DFeatureLevel = lFeatureLevel;
	m_ipD3D11Device = ipDevice;
	m_ipD3D11DeviceContext = ipDeviceContext;

	m_bInitialized = TRUE;

	return S_OK;
}

HRESULT CDXGICapture::Terminate()
{
	AUTOLOCK();
	if (!m_bInitialized) {
		return S_FALSE; // already terminated
	}

	this->terminateDeviceResource();

	m_ipD3D11Device = nullptr;
	m_ipD3D11DeviceContext = nullptr;
	m_lD3DFeatureLevel = D3D_FEATURE_LEVEL_INVALID;

	freeMonitorInfos();

	m_bInitialized = FALSE;
	return S_OK;
}

HRESULT CDXGICapture::SetConfig(const tagScreenCaptureFilterConfig* pConfig)
{
	AUTOLOCK();
	if (!m_bInitialized) {
		return D2DERR_NOT_INITIALIZED;
	}

	if (nullptr == pConfig) {
		return E_INVALIDARG;
	}

	// terminate old resources
	this->terminateDeviceResource();

	HRESULT hr = S_OK;
	const tagDublicatorMonitorInfo* pSelectedMonitorInfo = nullptr;

	pSelectedMonitorInfo = this->FindDublicatorMonitorInfo(pConfig->MonitorIdx);
	if (nullptr == pSelectedMonitorInfo) {
		return E_INVALIDARG;
	}

	hr = this->createDeviceResource(pConfig, pSelectedMonitorInfo);
	if (FAILED(hr)) {
		return hr;
	}

	return hr;
}

HRESULT CDXGICapture::SetConfig(const tagScreenCaptureFilterConfig& config)
{
	return this->SetConfig(&config);
}

BOOL CDXGICapture::IsInitialized() const
{
	AUTOLOCK();
	return m_bInitialized;
}

D3D_FEATURE_LEVEL CDXGICapture::GetD3DFeatureLevel() const
{
	AUTOLOCK();
	return m_lD3DFeatureLevel;
}

int CDXGICapture::GetDublicatorMonitorInfoCount() const
{
	AUTOLOCK();
	return (int)m_monitorInfos.size();
}

const tagDublicatorMonitorInfo* CDXGICapture::GetDublicatorMonitorInfo(int index) const
{
	AUTOLOCK();

	size_t nCount = m_monitorInfos.size();
	if ((index < 0) || (index >= (int)nCount)) {
		return nullptr;
	}

	return m_monitorInfos[index];
} // GetDublicatorMonitorInfo

const tagDublicatorMonitorInfo* CDXGICapture::FindDublicatorMonitorInfo(int monitorIdx) const
{
	AUTOLOCK();

	size_t nCount = m_monitorInfos.size();
	if (nCount == 0) {
		return nullptr;
	}
	DublicatorMonitorInfoVec::const_iterator it = m_monitorInfos.begin();
	DublicatorMonitorInfoVec::const_iterator end = m_monitorInfos.end();
	for (size_t i = 0; (i < nCount) && (it != end); i++, it++) {
		tagDublicatorMonitorInfo* pInfo = *it;
		if (monitorIdx == pInfo->Idx) {
			return pInfo;
		}
	}

	return nullptr;
} // FindDublicatorMonitorInfo

//
// CaptureToFile
//
HRESULT CDXGICapture::CaptureToFile(_In_ LPCWSTR lpcwOutputFileName, _Out_opt_ BOOL* pRetIsTimeout /*= NULL*/)
{
	AUTOLOCK();

	if (nullptr != pRetIsTimeout) {
		*pRetIsTimeout = FALSE;
	}

	if (!m_bInitialized) {
		return D2DERR_NOT_INITIALIZED;
	}

	CHECK_POINTER_EX(m_ipDxgiOutputDuplication, E_INVALIDARG);
	CHECK_POINTER_EX(lpcwOutputFileName, E_INVALIDARG);

	HRESULT hr = S_OK;

	hr = DXGICaptureHelper::IsRendererInfoValid(&m_rendererInfo);
	if (FAILED(hr)) {
		return hr;
	}

	// is valid?
	hr = DXGICaptureHelper::GetContainerFormatByFileName(lpcwOutputFileName);
	if (FAILED(hr)) {
		return hr;
	}

	DXGI_OUTDUPL_FRAME_INFO     FrameInfo;
	CComPtr<IDXGIResource>      ipDesktopResource;
	CComPtr<ID3D11Texture2D>    ipAcquiredDesktopImage;
	CComPtr<ID2D1Bitmap>        ipD2D1SourceBitmap;

	// Get new frame
	hr = m_ipDxgiOutputDuplication->AcquireNextFrame(1000, &FrameInfo, &ipDesktopResource);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		if (nullptr != pRetIsTimeout) {
			*pRetIsTimeout = TRUE;
		}
		return S_FALSE;
	}
	else if (FAILED(hr))
	{
		return hr;
	}

	// QI for ID3D11Texture2D
	hr = ipDesktopResource->QueryInterface(IID_PPV_ARGS(&ipAcquiredDesktopImage));
	ipDesktopResource = nullptr;
	CHECK_HR_RETURN(hr);

	if (nullptr == ipAcquiredDesktopImage)
	{
		// release frame
		m_ipDxgiOutputDuplication->ReleaseFrame();
		return E_OUTOFMEMORY;
	}

	// Copy needed full part of desktop image
	m_ipD3D11DeviceContext->CopyResource(m_ipCopyTexture2D, ipAcquiredDesktopImage);

	if (m_rendererInfo.ShowCursor) {
		hr = DXGICaptureHelper::GetMouse(m_ipDxgiOutputDuplication, &m_mouseInfo, &FrameInfo, (UINT)m_rendererInfo.MonitorIdx, m_desktopOutputDesc.DesktopCoordinates.left, m_desktopOutputDesc.DesktopCoordinates.top);
		if (SUCCEEDED(hr) && m_mouseInfo.Visible) {
			hr = DXGICaptureHelper::DrawMouse(&m_mouseInfo, &m_desktopOutputDesc, &m_tempMouseBuffer, m_ipCopyTexture2D);
		}

		if (FAILED(hr)) {
			// release frame
			m_ipDxgiOutputDuplication->ReleaseFrame();
			return hr;
		}
	}

	// release frame
	hr = m_ipDxgiOutputDuplication->ReleaseFrame();
	CHECK_HR_RETURN(hr);

	// create D2D1 source bitmap
	hr = DXGICaptureHelper::CreateBitmap(m_ipD2D1RenderTarget, m_ipCopyTexture2D, &ipD2D1SourceBitmap);
	CHECK_HR_RETURN(hr);

	D2D1_RECT_F rcSource = D2D1::RectF(
		(FLOAT)m_rendererInfo.SrcBounds.X,
		(FLOAT)m_rendererInfo.SrcBounds.Y,
		(FLOAT)(m_rendererInfo.SrcBounds.X + m_rendererInfo.SrcBounds.Width),
		(FLOAT)(m_rendererInfo.SrcBounds.Y + m_rendererInfo.SrcBounds.Height));
	D2D1_RECT_F rcTarget = D2D1::RectF(
		(FLOAT)m_rendererInfo.DstBounds.X,
		(FLOAT)m_rendererInfo.DstBounds.Y,
		(FLOAT)(m_rendererInfo.DstBounds.X + m_rendererInfo.DstBounds.Width),
		(FLOAT)(m_rendererInfo.DstBounds.Y + m_rendererInfo.DstBounds.Height));
	D2D1_POINT_2F ptTransformCenter = D2D1::Point2F(m_rendererInfo.DstBounds.Width / 2.0f, m_rendererInfo.DstBounds.Height / 2.0f);

	// Apply the rotation transform to the render target.
	D2D1::Matrix3x2F rotate = D2D1::Matrix3x2F::Rotation(
		m_rendererInfo.RotationDegrees,
		ptTransformCenter
	);

	D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(
		D2D1::SizeF(m_rendererInfo.ScaleX, m_rendererInfo.ScaleY),
		ptTransformCenter
	);

	// Priority: first rotate, after scale...
	m_ipD2D1RenderTarget->SetTransform(rotate * scale);

	m_ipD2D1RenderTarget->BeginDraw();
	// clear background color
	m_ipD2D1RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black, 1.0f));
	m_ipD2D1RenderTarget->DrawBitmap(ipD2D1SourceBitmap, rcTarget, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rcSource);
	// Reset transform
	//m_ipD2D1RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	// Logo draw sample
	//m_ipD2D1RenderTarget->DrawBitmap(ipBmpLogo, D2D1::RectF(0, 0, 2 * 200, 2 * 46));
	hr = m_ipD2D1RenderTarget->EndDraw();
	if (FAILED(hr)) {
		return hr;
	}

	hr = DXGICaptureHelper::SaveImageToFile(m_ipWICImageFactory, m_ipWICOutputBitmap, lpcwOutputFileName);
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
} // CaptureToFile

#undef AUTOLOCK
