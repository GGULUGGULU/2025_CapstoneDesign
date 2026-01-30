//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"
#include "EffectLibrary.h"

CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_nRtvDescriptorIncrementSize = 0;
	m_nDsvDescriptorIncrementSize = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	_tcscpy_s(m_pszFrameRate, _T("2026Project ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();

	CreateD3D11On12Device();
	CreateD2DDevice();
	CreateTextResources(); // D2D 장치 생성 후 텍스트 리소스 생성
	CreateRenderTargetView();

	CreateDepthStencilView();

	// 1. 명령 리스트 리셋 (초기화 명령 기록용)
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	// 2. [추가] 이펙트 라이브러리 초기화 (여기서 딱 한 번만!)
	CEffectLibrary::Instance()->Initialize(m_pd3dDevice, m_pd3dCommandList);

	// 3. 명령 실행 및 대기
	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

	BuildObjectGameStart();

	CreateComputeRootSignature();
	CreatePostProcessResource();

	m_pRadialBlurShader = new CRadialBlurShader();
	m_pRadialBlurShader->CreateShader(m_pd3dDevice, m_pd3dComputeRootSignature);

	return(true);
}

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1 **)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	//CreateRenderTargetView();
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug *pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void **)&m_pdxgiFactory);

	IDXGIAdapter1 *pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice))) break;
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void **)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice);
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);

	//hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)m_d3dCommandAllocators);
	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_d3dCommandAllocators[0].GetAddressOf()));
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3dCommandAllocators[0].Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	// hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3dCommandAllocators, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dRtvDescriptorHeap);
	m_nRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dDsvDescriptorHeap);
	m_nDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_d3dSwapChainBackBuffers[i]) m_d3dSwapChainBackBuffers[i]->Release();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetView();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	int mouseX = LOWORD(lParam);
	int mouseY = HIWORD(lParam);

	float ndcX = (2.0f * mouseX / m_pCamera->GetViewport().Width) - 1.0f;
	float ndcY = 1.0f - (2.0f * mouseY / m_pCamera->GetViewport().Height);

	XMFLOAT4X4 fMatProj = m_pCamera->GetProjectionMatrix();
	XMMATRIX matProj = XMLoadFloat4x4(&fMatProj);
	XMMATRIX matInvProj = XMMatrixInverse(NULL, matProj);

	XMVECTOR vViewRayTarget = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	vViewRayTarget = XMVector3TransformCoord(vViewRayTarget, matInvProj);

	XMFLOAT4X4 fMatView = m_pCamera->GetViewMatrix();
	XMMATRIX matView = XMLoadFloat4x4(&fMatView);
	XMMATRIX matInvView = XMMatrixInverse(NULL, matView);

	XMVECTOR vWorldRayOrigin = XMLoadFloat3(&m_pCamera->GetPosition());

	XMVECTOR vWorldRayTarget = XMVector3TransformCoord(vViewRayTarget, matInvView);

	XMVECTOR vWorldRayDirection = XMVector3Normalize(vWorldRayTarget - vWorldRayOrigin);

	XMFLOAT3 fWorldRayOrigin, fWorldRayDirection;
	XMStoreFloat3(&fWorldRayOrigin, vWorldRayOrigin);
	XMStoreFloat3(&fWorldRayDirection, vWorldRayDirection);

	if (m_pScene)
	{
		if (nMessageID == WM_LBUTTONDOWN)
		{
			m_pScene->PickObject(fWorldRayOrigin, fWorldRayDirection);
			if (m_pScene->m_pSelectedObject != NULL && m_nStage != 2)
				++m_nStage;
		}
		else
		{
			m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		}
	}
	switch (nMessageID)
	{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			::SetCapture(hWnd);
			::GetCursorPos(&m_ptOldCursorPos);
			break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			::ReleaseCapture();
			break;
		case WM_MOUSEMOVE:
			break;
		default:
			break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	switch (nMessageID)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					::PostQuitMessage(0);
					break;
				case VK_RETURN:
					break;
				case VK_F1:
				case VK_F2:
				case VK_F3:
					m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
					break;
				case VK_F9:
					ChangeSwapChainState();
					break;
				case VK_SPACE:
				{
					// 1단 점프
					if (m_nJumpCount == 0)
					{
						m_nJumpCount = 1;
						m_fFirstJumpTime = m_fTotalTime;

						m_fJumpCurrentTime = m_fTotalTime;
						m_pPlayer->SetGravity(XMFLOAT3(0, 2.0f, 0));
						m_bJump = true;
					}
					// 2단 점프
					else if (m_nJumpCount == 1)
					{
						float dt = m_fTotalTime - m_fFirstJumpTime;
						if (dt <= m_fSecondJumpWindow)
						{
							m_nJumpCount = 2;

							m_fJumpCurrentTime = m_fTotalTime;

							m_pPlayer->SetGravity(XMFLOAT3(0, 2.0f, 0));
							m_bJump = true;
						}
					}
					
				}
				break;

				case 'S':
					m_pPlayer->SetVelocity(XMFLOAT3(0, 0, 0));
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
				m_GameTimer.Stop();
			else
				m_GameTimer.Start();
			break;
		}
		case WM_SIZE:
			break;
		case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
			OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
	}
	return(0);
}

void CGameFramework::OnDestroy()
{
    ReleaseObjects();

	CEffectLibrary::Instance()->Release();//

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_d3dSwapChainBackBuffers[i].Get()) m_d3dSwapChainBackBuffers[i].Reset();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_d3d11DeviceContext) m_d3d11DeviceContext->Release();

	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
    if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1	*pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void **)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

void CGameFramework::BuildObjectGameStart()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	m_pScene = new CScene();
	
	if (m_pScene) m_pScene->BuildObjectsGameStart(m_pd3dDevice, m_pd3dCommandList);

	CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	pCarPlayer->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;
	m_pCamera = m_pPlayer->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) m_pPlayer->Release();

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		DWORD dwDirection = 0;
		if (pKeysBuffer[VK_UP] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeysBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeysBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeysBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;

		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		if (GetCapture() == m_hWnd)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
				else
					m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection) m_pPlayer->Move(dwDirection, 1.5f, true);
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::ProcessInputGameStage()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		DWORD dwDirection = 0;
		if (pKeysBuffer[VK_UP] & 0xF0) {dwDirection |= DIR_FORWARD; if(m_nPlayerCurrentSpeed< m_pPlayer->m_fMaxVelocityXZ) ++m_nPlayerCurrentSpeed;}
		else if (pKeysBuffer[VK_DOWN] & 0xF0) { dwDirection |= DIR_BACKWARD; if (m_nPlayerCurrentSpeed < m_pPlayer->m_fMaxVelocityXZ) ++m_nPlayerCurrentSpeed; }
		else { if (m_nPlayerCurrentSpeed > 0)--m_nPlayerCurrentSpeed; }

		if (pKeysBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeysBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;

		if ((dwDirection != 0))
		{
			if (dwDirection == DIR_LEFT || dwDirection == DIR_RIGHT)
			{
				if (dwDirection == DIR_RIGHT)
					m_pPlayer->Rotate(0.0f, +1.0f, 0.0f);
				else
					m_pPlayer->Rotate(0.0f, -1.0f, 0.0f);
			}
			else if (dwDirection) m_pPlayer->Move(dwDirection, m_nPlayerCurrentSpeed, true);

			CEffectLibrary::Instance()->Play(EFFECT_TYPE::DUST, XMFLOAT3(m_pPlayer->GetPosition().x, m_pPlayer->GetPosition().y-50, m_pPlayer->GetPosition().z-20), XMFLOAT2(10,10));
		}
	}

	if (::GetAsyncKeyState('Z') & 0x8000)
	{
		CEffectLibrary::Instance()->ToggleBooster(true);
	}
	else
	{
		CEffectLibrary::Instance()->ToggleBooster(false);
	}///////////////////

	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
} // 게임 스테이지 입력방식 

void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();

	if (m_pScene) m_pScene->AnimateObjects(fTimeElapsed);

	m_pPlayer->Animate(fTimeElapsed, NULL);

	if (m_pPlayer)
	{
		CEffectLibrary::Instance()->UpdateBoosterPosition(
			m_pPlayer->GetPosition(),
			m_pPlayer->GetLookVector()
		);
	}

	CEffectLibrary::Instance()->Update(fTimeElapsed);
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGameFramework::ShowFrameRate()
{
	m_GameTimer.GetFrameRate(m_pszFrameRate + 13, 37);
	size_t nLength = _tcslen(m_pszFrameRate);
	XMFLOAT3 xmf3Position = m_pPlayer->GetPosition();
	_stprintf_s(m_pszFrameRate + nLength, 70 - nLength, _T("(%2f, %2f, %2f)"), xmf3Position.x, xmf3Position.y, xmf3Position.z);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}

void CGameFramework::BeforeTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList)
{
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get();
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
}

void CGameFramework::AfterTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList)
{
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
}

void CGameFramework::ClearRTVDSV(ID3D12DescriptorHeap* d3dRtvDescriptorHeap, ID3D12DescriptorHeap* d3dDsvDescriptorHeap, ID3D12GraphicsCommandList* d3dCommandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	d3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	d3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);
}

void CGameFramework::SetUIInfo()
{
	m_fTotalTime = m_GameTimer.GetTotalTime();
}

void CGameFramework::BuildGameObjects()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	if (m_pPlayer)
	{
		m_pPlayer->Release();
		m_pPlayer = NULL;
	}
	if (m_pScene)
	{
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = NULL;
	}

	m_pScene = new CScene();
	if (m_pScene) m_pScene->BuildGameObjects(m_pd3dDevice, m_pd3dCommandList);

	CreateShadowMap();

	CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	pCarPlayer->SetScale(10.2f, 10.2f, 10.2f);
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;

	m_pPlayer->ComputeCombinedAABB();
	m_pPlayer->m_xmCombinedLocalAABB.Extents.x = 1;
	m_pPlayer->m_xmCombinedLocalAABB.Extents.y = 0.5;
	m_pPlayer->m_xmCombinedLocalAABB.Extents.z = 2;
	// AABB사이즈 

	m_pPlayer->Rotate(0, 180, 0);
	m_pPlayer->SetPosition(XMFLOAT3(0.0f, 10.0f, 0.0f)); // 플레이어 위치 조정
	m_pPlayer->SetGravity(XMFLOAT3(0, -1, 0));

	m_pPlayer->OnPrepareRender();

	m_pCamera = m_pPlayer->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

void CGameFramework::CollisionProcess()
{
	if (m_pPlayer) m_pPlayer->OnPrepareRender();
	
	XMFLOAT3 colDirection;

	if (2 == m_nStage && m_pScene->CheckGroundCollision() && !m_bJump)
	{
		m_pPlayer->SetGravity(XMFLOAT3(0, 0, 0));
		XMFLOAT3 currentVel = m_pPlayer->GetVelocity();
		m_pPlayer->SetVelocity(XMFLOAT3(currentVel.x, 0.0f, currentVel.z));

		m_nJumpCount = 0;  

	}

	if (2 == m_nStage && m_pScene->CheckCollision() && !m_bIsStun)
	{
		CGameObject* pCollidedObject = m_pScene->m_pCollidedObject;

		if (pCollidedObject == m_pScene->m_ppGameObjects[113] || pCollidedObject == m_pScene->m_ppGameObjects[114])
			return;


		if (pCollidedObject == m_pScene->m_ppGameObjects[109] || pCollidedObject == m_pScene->m_ppGameObjects[110] ||
			pCollidedObject == m_pScene->m_ppGameObjects[111] || pCollidedObject == m_pScene->m_ppGameObjects[112])
		{

			XMFLOAT3 vPos = pCollidedObject->GetPosition();
			//m_pScene->m_pParticleEmitter->SpawnExplosion(XMFLOAT3(vPos.x, vPos.y + 10, vPos.z));
			CEffectLibrary::Instance()->Play(EFFECT_TYPE::COLLISION1, vPos, XMFLOAT2(50, 50));
			CEffectLibrary::Instance()->Play(EFFECT_TYPE::COLLISION2, vPos, XMFLOAT2(50, 50));
			CEffectLibrary::Instance()->Play(EFFECT_TYPE::COLLISION3, vPos, XMFLOAT2(50, 50));

			pCollidedObject->Disable();

			++m_nScore;
			
			m_pPlayer->m_fMaxVelocityXZ += 50;
		}
		else if (pCollidedObject != m_pScene->m_ppGameObjects[38])
		{

			XMFLOAT3 vPos = pCollidedObject->GetPosition();
			//m_pScene->m_pParticleEmitter->SpawnExplosion(XMFLOAT3(vPos.x, vPos.y + 10, vPos.z));
			CEffectLibrary::Instance()->Play(EFFECT_TYPE::COLLISION1, XMFLOAT3(vPos.x, vPos.y+10, vPos.z), XMFLOAT2(50, 50));
			CEffectLibrary::Instance()->Play(EFFECT_TYPE::COLLISION2, XMFLOAT3(vPos.x, vPos.y+10, vPos.z), XMFLOAT2(50, 50));
			CEffectLibrary::Instance()->Play(EFFECT_TYPE::COLLISION3, XMFLOAT3(vPos.x, vPos.y+10, vPos.z), XMFLOAT2(50, 50));

			pCollidedObject->Disable();

			colDirection = Vector3::Subtract(m_pPlayer->GetPosition(), pCollidedObject->GetPosition());
			colDirection.y = 0.0f;
			colDirection = Vector3::Normalize(colDirection);

			float fReboundPower = m_nPlayerCurrentSpeed; 
			XMFLOAT3 velocity = Vector3::ScalarProduct(colDirection, fReboundPower, false);

			m_pPlayer->SetVelocity(velocity);

			m_bIsStun = true;
			m_fCollisionCurrentTime = m_fTotalTime;
		}
		else
		{
			colDirection = Vector3::Subtract(m_pPlayer->GetPosition(), pCollidedObject->GetPosition());
			colDirection.y = 0.0f;
			colDirection = Vector3::Normalize(colDirection);

			float fReboundPower = m_nPlayerCurrentSpeed;
			XMFLOAT3 velocity = Vector3::ScalarProduct(colDirection, fReboundPower, false);

			m_pPlayer->SetVelocity(velocity);

			m_bIsStun = true;
			m_fCollisionCurrentTime = m_fTotalTime;
		}
	}

	if (m_bIsStun && m_fTotalTime - m_fCollisionCurrentTime > 1.0f)
	{
		m_bIsStun = false;
	}

	if (m_bJump && m_fTotalTime - m_fJumpCurrentTime > 1.0f)
	{
		m_pPlayer->SetGravity(XMFLOAT3(0, -1.5, 0));
		m_bJump = false;
	}
}

void CGameFramework::CreateD3D11On12Device()
{
	// Create an 11 device wrapped around the 12 device and share 12's command queue.
	ComPtr<ID3D11Device> d3d11Device;
	D3D11On12CreateDevice(
		m_pd3dDevice,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr,
		0,
		reinterpret_cast<IUnknown**>(&m_pd3dCommandQueue),
		1,
		0,
		&d3d11Device,
		&m_d3d11DeviceContext,
		nullptr
	);

	// Query the 11On12 device from the 11 device.
	//d3d11Device.As(&m_d3d11On12Device);
	d3d11Device.Get()->QueryInterface(
		__uuidof(ID3D11On12Device),
		reinterpret_cast<void**>(m_d3d11On12Device.GetAddressOf())
	);
}

void CGameFramework::CreateD2DDevice()
{
	// Create D2D/DWrite components.
	D2D1_FACTORY_OPTIONS d2dFactoryOptions{};
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, reinterpret_cast<void**>(m_d2dFactory.GetAddressOf()));
	
	ComPtr<IDXGIDevice> dxgiDevice;
	m_d3d11On12Device.As(&dxgiDevice);
	m_d2dFactory->CreateDevice(dxgiDevice.Get(), m_d2dDevice.GetAddressOf());
	m_d2dDevice->CreateDeviceContext(deviceOptions, m_d2dDeviceContext.GetAddressOf());
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dWriteFactory.GetAddressOf()));
}

void CGameFramework::CreateRenderTargetView()
{
	// Query the desktop's dpi settings, which will be used to create D2D's render targets.
	float dpiX;
	float dpiY;
#pragma warning(push)
#pragma warning(disable : 4996) // GetDesktopDpi is deprecated.
	m_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
#pragma warning(pop)

	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	for (UINT i = 0; i < m_nSwapChainBuffers; ++i)
	{
		m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_d3dSwapChainBackBuffers[i]));
		m_pd3dDevice->CreateRenderTargetView(m_d3dSwapChainBackBuffers[i].Get(), NULL, rtvHandle);

		// Create a wrapped 11On12 resource of this back buffer. Since we are 
		// rendering all D3D12 content first and then all D2D content, we specify 
		// the In resource state as RENDER_TARGET - because D3D12 will have last 
		// used it in this state - and the Out resource state as PRESENT. When 
		// ReleaseWrappedResources() is called on the 11On12 device, the resource 
		// will be transitioned to the PRESENT state.
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		m_d3d11On12Device->CreateWrappedResource(
			m_d3dSwapChainBackBuffers[i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(m_wrappedBackBuffers[i].GetAddressOf())
		);

		// Create a render target for D2D to draw directly to this back buffer.
		ComPtr<IDXGISurface> surface;
		m_wrappedBackBuffers[i].As(&surface);
		m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			m_d2dRenderTargets[i].GetAddressOf()
		);

		rtvHandle.Offset(m_nRtvDescriptorIncrementSize);

		if( i > 0)
			m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_d3dCommandAllocators[i].GetAddressOf()));
	}
}

void CGameFramework::CreateTextResources()
{
	m_dWriteFactory->CreateTextFormat(
			L"Arial",              
			NULL,                  
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			24.0f,                  
			L"en-us",              
			m_textTimeFormat.GetAddressOf()
		);

	m_textTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	m_textTimeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	
		m_d2dDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Gray),
			m_textTimeBrush.GetAddressOf()
		);

		m_dWriteFactory->CreateTextFormat(
			L"Arial",              
			NULL,                 
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			24.0f,                 
			L"en-us",             
			m_textSpeedFormat.GetAddressOf()
		);

		m_textSpeedFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_textSpeedFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		m_d2dDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Yellow),
			m_textSpeedBrush.GetAddressOf()
		);

		m_dWriteFactory->CreateTextFormat(
			L"Arial",               
			NULL,                  
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			24.0f,                  
			L"en-us",              
			m_textEndTimeFormat.GetAddressOf()
		);

		m_textEndTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_textEndTimeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

		m_d2dDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Yellow),
			m_textEndTimeBrush.GetAddressOf()
		);
}

void CGameFramework::RenderUI()
{
	
	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_nSwapChainBufferIndex].GetAddressOf(), 1);

	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_nSwapChainBufferIndex].Get());
	m_d2dDeviceContext->BeginDraw();

	int minutes = static_cast<int>(m_fTotalTime / 60);
	int seconds = static_cast<int>(m_fTotalTime) % 60;
	int milliseconds = static_cast<int>((m_fTotalTime - (minutes * 60 + seconds)) * 100);
	swprintf_s(m_timeBuffer, 1024, L"%02d:%02d:%02d", minutes, seconds, milliseconds);
	swprintf_s(m_speedBuffer, 1024, L"%d Km/h", m_nPlayerCurrentSpeed);
	if (4 != m_nScore && 2 == m_nStage)
	{
		m_d2dDeviceContext->DrawTextW(
			m_timeBuffer,                        
			wcslen(m_timeBuffer),                
			m_textTimeFormat.Get(),                   
			D2D1::RectF(10.0f, 10.0f,
				(float)m_nWndClientWidth - 10.0f,
				(float)m_nWndClientHeight - 10.0f),
			m_textTimeBrush.Get()                     
		);

		m_d2dDeviceContext->DrawTextW(
			m_speedBuffer,                         
			wcslen(m_speedBuffer),                
			m_textSpeedFormat.Get(),                   
			D2D1::RectF(10, 10,
				(float)m_nWndClientWidth - 10.0f,
				(float)m_nWndClientHeight - 10.0f), 
			m_textSpeedBrush.Get()                     
		);
	}
	
	if(100 == m_nStage)
	{
		m_d2dDeviceContext->DrawTextW(
			m_timeBuffer,
			wcslen(m_timeBuffer),
			m_textEndTimeFormat.Get(),
			D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight),
			m_textEndTimeBrush.Get()
		);
	}

	m_d2dDeviceContext->EndDraw();

	m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_nSwapChainBufferIndex].GetAddressOf(), 1);

	m_d3d11DeviceContext->Flush();
}

void CGameFramework::BuildObjectEnd()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	if (m_pPlayer)
	{
		m_pPlayer->Release();
		m_pPlayer = NULL;
	}
	if (m_pScene)
	{
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = NULL;
	}

	m_pScene = new CScene();

	if (m_pScene) m_pScene->BuildObjectsGameEnd(m_pd3dDevice, m_pd3dCommandList);

	CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	pCarPlayer->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;
	m_pCamera = m_pPlayer->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

}

void CGameFramework::CreateShadowMap()
{
	D3D12_RESOURCE_DESC d3dShadowMapDesc = {};
	d3dShadowMapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dShadowMapDesc.Alignment = 0;
	d3dShadowMapDesc.Width = 8192;
	d3dShadowMapDesc.Height = 8192;
	d3dShadowMapDesc.DepthOrArraySize = 1;
	d3dShadowMapDesc.MipLevels = 1;
	d3dShadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	d3dShadowMapDesc.SampleDesc.Count = 1;
	d3dShadowMapDesc.SampleDesc.Quality = 0;
	d3dShadowMapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dShadowMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; 
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue = {};
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; 
	d3dClearValue.DepthStencil.Depth = 1.0f; 
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(
		&d3dHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&d3dShadowMapDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		&d3dClearValue,
		__uuidof(ID3D12Resource),
		(void**)&m_pd3dShadowMap
	);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; 
	dsvHeapDesc.NodeMask = 0;

	m_pd3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pd3dShadowDSVHeap));

	m_d3dCPUShadowDSVHandle = m_pd3dShadowDSVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

	m_pd3dDevice->CreateDepthStencilView(m_pd3dShadowMap, &dsvDesc, m_d3dCPUShadowDSVHandle);

	if (m_pScene)
	{
		m_pScene->CreateShadowMapSRV(m_pd3dDevice, m_pd3dShadowMap);
	}
}

void CGameFramework::RenderShadowPass()
{
	if (!m_pScene || !m_pd3dShadowMap) return;

	D3D12_RESOURCE_BARRIER toShadowWrite = {};
	toShadowWrite.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toShadowWrite.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toShadowWrite.Transition.pResource = m_pd3dShadowMap;
	toShadowWrite.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	toShadowWrite.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	toShadowWrite.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pd3dCommandList->ResourceBarrier(1, &toShadowWrite);

	m_pScene->RenderShadowMap(m_pd3dCommandList, m_d3dCPUShadowDSVHandle);

	D3D12_RESOURCE_BARRIER toGenericRead = {};
	toGenericRead.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toGenericRead.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toGenericRead.Transition.pResource = m_pd3dShadowMap;
	toGenericRead.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	toGenericRead.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	toGenericRead.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pd3dCommandList->ResourceBarrier(1, &toGenericRead);
}

void CGameFramework::SetMainViewport()
{
	D3D12_VIEWPORT viewport = { 0, 0, (float)m_nWndClientWidth, (float)m_nWndClientHeight, 0.0f, 1.0f };
	D3D12_RECT scissorRect = { 0, 0, m_nWndClientWidth, m_nWndClientHeight };

	m_pd3dCommandList->RSSetViewports(1, &viewport);
	m_pd3dCommandList->RSSetScissorRects(1, &scissorRect);
}

void CGameFramework::CreatePostProcessResource()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	m_pd3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pd3dPostProcessRtvHeap));
	m_d3dSceneRtvCpuHandle = m_pd3dPostProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	m_pd3dDevice->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&m_pd3dCbvSrvUavHeap));

	UINT nIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pd3dCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dSrvGpuHandle = m_pd3dCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	m_d3dUavGpuHandle = m_d3dSrvGpuHandle;
	m_d3dUavGpuHandle.ptr += nIncrementSize;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = m_nWndClientWidth;
	resDesc.Height = m_nWndClientHeight;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; 

	D3D12_CLEAR_VALUE clearVal = { DXGI_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.125f, 0.3f, 1.0f } };

	m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &clearVal, IID_PPV_ARGS(&m_pSceneRenderTexture));

	m_pd3dDevice->CreateRenderTargetView(m_pSceneRenderTexture, NULL, m_d3dSceneRtvCpuHandle);

	m_pd3dDevice->CreateShaderResourceView(m_pSceneRenderTexture, NULL, cpuHandle);

	cpuHandle.ptr += nIncrementSize;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, NULL, IID_PPV_ARGS(&m_pBlurTexture));

	m_pd3dDevice->CreateUnorderedAccessView(m_pBlurTexture, NULL, NULL, cpuHandle);
}

void CGameFramework::CreateComputeRootSignature()
{
	D3D12_ROOT_PARAMETER pd3dRootParameters[3];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[0].Constants.Num32BitValues = 4; 
	pd3dRootParameters[0].Constants.ShaderRegister = 0; // b0
	pd3dRootParameters[0].Constants.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE srvRange[1];
	srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange[0].NumDescriptors = 1;
	srvRange[0].BaseShaderRegister = 0; // t0
	srvRange[0].RegisterSpace = 0;
	srvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[1].DescriptorTable.pDescriptorRanges = srvRange;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE uavRange[1];
	uavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRange[0].NumDescriptors = 1;
	uavRange[0].BaseShaderRegister = 0; // u0
	uavRange[0].RegisterSpace = 0;
	uavRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[2].DescriptorTable.pDescriptorRanges = uavRange;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	
	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	m_pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_pd3dComputeRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();
}

void CGameFramework::RenderBlur()
{
	D3D12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pSceneRenderTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pd3dCommandList->ResourceBarrier(1, &toRT);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(m_d3dSceneRtvCpuHandle, pfClearColor, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	m_pd3dCommandList->OMSetRenderTargets(1, &m_d3dSceneRtvCpuHandle, TRUE, &dsvHandle);
	SetMainViewport();

	if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);

	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pSceneRenderTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pBlurTexture, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_pd3dCommandList->ResourceBarrier(2, barriers);

	m_pd3dCommandList->SetComputeRootSignature(m_pd3dComputeRootSignature);
	m_pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvUavHeap);

	m_pd3dCommandList->SetComputeRootDescriptorTable(1, m_d3dSrvGpuHandle); // t0
	m_pd3dCommandList->SetComputeRootDescriptorTable(2, m_d3dUavGpuHandle); // u0

	if (m_nPlayerCurrentSpeed > 0)
	{
		cbData.strength = min((float)m_nPlayerCurrentSpeed * 0.0005f, 0.15f);
	}
	else
	{
		cbData.strength = 0.0f;
	}
	cbData.cx = 0.5f; cbData.cy = 0.5f;
	cbData.aspect = (float)m_nWndClientWidth / (float)m_nWndClientHeight;

	m_pd3dCommandList->SetComputeRoot32BitConstants(0, 4, &cbData, 0);

	m_pRadialBlurShader->Dispatch(m_pd3dCommandList, m_nWndClientWidth, m_nWndClientHeight,
		(m_nWndClientWidth + 15) / 16, (m_nWndClientHeight + 15) / 16, 1);

	D3D12_RESOURCE_BARRIER copyBarriers[2];
	copyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	copyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pBlurTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pd3dCommandList->ResourceBarrier(2, copyBarriers);

	m_pd3dCommandList->CopyResource(m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(), m_pBlurTexture);

	D3D12_RESOURCE_BARRIER toRTBack = CD3DX12_RESOURCE_BARRIER::Transition(
		m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pd3dCommandList->ResourceBarrier(1, &toRTBack);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

	m_pd3dCommandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	SetMainViewport();

	if (m_pPlayer)
	{
		m_pPlayer->Render(m_pd3dCommandList, NULL, m_pCamera);
	}
}

//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{    
	m_GameTimer.Tick(0.0f);
	
	if (1 == m_nStage)
	{
		m_pScene->m_nGFStage = m_nStage = 2;
		BuildGameObjects();
	} // 게임스테이지로 변경

	if (0 == m_nStage)
	{
		ProcessInput();
	}
	else if (2 == m_nStage)
	{

		CollisionProcess();

		if (!m_bIsStun)
		{
			ProcessInputGameStage(); 
		}
		else
		{
			m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
		}
	}// 게임 스테이지 진입 시 키 변경

	if (4 == m_nScore)
	{
		// 게임 종료
		BuildObjectEnd();
		m_nStage = 100;
		++m_nScore;
	}
	else if(100 != m_nStage)
	{
		SetUIInfo();
	}

    AnimateObjects();

	// 커맨드 리스트 초기화
	HRESULT hResult = m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get()->Reset();
	hResult = m_pd3dCommandList->Reset(m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get(), NULL);

	if (2 == m_nStage)
	{
		RenderShadowPass();
	}

	if (2 == m_nStage)
	{
		RenderBlur();
		CEffectLibrary::Instance()->Render(m_pd3dCommandList, m_pCamera->GetViewMatrix(), m_pCamera->GetProjectionMatrix());
	}
	else
	{
		D3D12_RESOURCE_BARRIER d3dResourceBarrier;
		BeforeTransformBarrier(d3dResourceBarrier, m_pd3dCommandList);

		ClearRTVDSV(m_pd3dRtvDescriptorHeap, m_pd3dDsvDescriptorHeap, m_pd3dCommandList);
		SetMainViewport();

		if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);
		if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, NULL, m_pCamera);
	
	}

	CGameObject* pDebugBoxToRender = NULL;
	if (m_pScene && m_pScene->m_bShowWireframeBox && m_pScene->m_pWireframeBoxObject)
	{
		pDebugBoxToRender = m_pScene->m_pWireframeBoxObject;
	}

	//if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);

#ifdef _WITH_PLAYER_TOP
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
	//if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, pDebugBoxToRender, m_pCamera);

	//if (0 == m_nStage) AfterTransformBarrier(d3dResourceBarrier, m_pd3dCommandList);

	hResult = m_pd3dCommandList->Close();
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	// [수정 코드]
	if (0 != m_nStage)
	{
		RenderUI();
	}
	hResult = m_pd3dCommandList->Reset(m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get(), NULL);
	
	if(0 == m_nStage)
	{
		D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_pd3dCommandList->ResourceBarrier(1, &presentBarrier);
	}

	m_pd3dCommandList->Close();

	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

	MoveToNextFrame();

	ShowFrameRate();
}
