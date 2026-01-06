#pragma once

#define FRAME_BUFFER_WIDTH		640
#define FRAME_BUFFER_HEIGHT		480

#include "Timer.h"
#include "Player.h"
#include "Scene.h"


class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateDepthStencilView();

	void ChangeSwapChainState();

    void BuildObjectGameStart();
    void ReleaseObjects();

    void ProcessInput();
    void AnimateObjects();
    void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);


	//
	void ShowFrameRate();
	void BeforeTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList);
	void AfterTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList);
	void ClearRTVDSV(ID3D12DescriptorHeap* d3dRtvDescriptorHeap, ID3D12DescriptorHeap* d3dDsvDescriptorHeap, ID3D12GraphicsCommandList* d3dCommandList);
	void SetUIInfo();
	void BuildGameObjects();
	void CollisionProcess();
	void ProcessInputGameStage();
	void CreateD3D11On12Device();
	void CreateD2DDevice();
	void CreateRenderTargetView();
	void CreateTextResources();
	void RenderUI();
	void BuildObjectEnd();
	
	void CreateShadowMap();
	void RenderShadowPass();
	void SetMainViewport(); // [추가] 메인 화면용 뷰포트 설정 함수

	void CreatePostProcessResource();
	void CreateComputeRootSignature();
	void RenderBlur();

	CB_RADIAL_BLUR cbData;
	//
private:
	HINSTANCE					m_hInstance;
	HWND						m_hWnd; 

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;
        
	IDXGIFactory4				*m_pdxgiFactory = NULL;
	IDXGISwapChain3				*m_pdxgiSwapChain = NULL;
	ID3D12Device				*m_pd3dDevice = NULL;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12DescriptorHeap		*m_pd3dRtvDescriptorHeap = NULL;
	UINT						m_nRtvDescriptorIncrementSize;

	ID3D12Resource				*m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap		*m_pd3dDsvDescriptorHeap = NULL;
	UINT						m_nDsvDescriptorIncrementSize;

	ID3D12CommandQueue			*m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList	*m_pd3dCommandList = NULL;

	ID3D12Fence					*m_pd3dFence = NULL;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;

#if defined(_DEBUG)
	ID3D12Debug					*m_pd3dDebugController;
#endif

	CGameTimer					m_GameTimer;

	CScene						*m_pScene = NULL;
	CPlayer						*m_pPlayer = NULL;
	CCamera						*m_pCamera = NULL;

	POINT						m_ptOldCursorPos;

	_TCHAR						m_pszFrameRate[80];


	ID3D11DeviceContext* m_d3d11DeviceContext;
	ComPtr<ID3D11On12Device> m_d3d11On12Device;
	ComPtr<ID2D1Factory3> m_d2dFactory;
	ComPtr<ID2D1Device> m_d2dDevice;
	ComPtr<ID2D1DeviceContext> m_d2dDeviceContext;
	ComPtr<IDWriteFactory> m_dWriteFactory;

	ComPtr<ID3D11Resource> m_wrappedBackBuffers[m_nSwapChainBuffers];
	ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[m_nSwapChainBuffers];

	ComPtr<ID3D12Resource> m_d3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ComPtr<ID3D12CommandAllocator> m_d3dCommandAllocators[m_nSwapChainBuffers];

	ComPtr<IDWriteTextFormat> m_textTimeFormat;  // 글꼴, 크기, 정렬
	ComPtr<ID2D1SolidColorBrush> m_textTimeBrush; // 글자 색상

	ComPtr<IDWriteTextFormat> m_textSpeedFormat;  // 글꼴, 크기, 정렬
	ComPtr<ID2D1SolidColorBrush> m_textSpeedBrush; // 글자 색상

	ComPtr<IDWriteTextFormat> m_textEndTimeFormat;  // 글꼴, 크기, 정렬
	ComPtr<ID2D1SolidColorBrush> m_textEndTimeBrush; // 글자 색상

	ID3D12Resource* m_pd3dShadowMap;
	ID3D12DescriptorHeap* m_pd3dShadowDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dCPUShadowDSVHandle;

	CRadialBlurShader* m_pRadialBlurShader;
	ID3D12RootSignature* m_pd3dComputeRootSignature;

	ID3D12Resource* m_pSceneRenderTexture;
	ID3D12Resource* m_pBlurTexture;

	ID3D12DescriptorHeap* m_pd3dCbvSrvUavHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSrvGpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dUavGpuHandle;

	ID3D12DescriptorHeap* m_pd3dPostProcessRtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSceneRtvCpuHandle;
public:
	// UI
	TCHAR m_timeBuffer[1024];
	float m_fTotalTime{ 0.f };
	float m_fCollisionCurrentTime{ 0.f };
	float m_fJumpCurrentTime{ 0.f };

	TCHAR m_speedBuffer[1024];
	int m_nPlayerCurrentSpeed{ 0 };
	
	// stage
	int m_nStage{ 0 };
	bool m_bFlag{ false };
	bool m_bIsStun{ false };
	bool m_bJump{ false };
	int m_nScore{ 0 };
	
};

