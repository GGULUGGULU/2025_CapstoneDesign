//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"
#include "Player.h"

#define MAX_LIGHTS			16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

class CScene
{
public:
    CScene();
    ~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	void BuildDefaultLightsAndMaterials();
	void BuildObjectsGameStart(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	bool ProcessInput(UCHAR *pKeysBuffer);
    void AnimateObjects(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);

	void ReleaseUploadBuffers();

	CPlayer* m_pPlayer = NULL;

	//
	void PickObject(XMFLOAT3& fWorldRayOrigin, XMFLOAT3& fWorldRayDirection);
	void BuildGameObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void BuildObjectsGameEnd(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateParticle(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateAABBWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateMirror(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateRockBillboard(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateFlowerBillboard(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateTreeBillboard(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateShadowMapSRV(ID3D12Device* pd3dDevice, ID3D12Resource* pShadowMapResource);
	void RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle); 
	XMMATRIX GetShadowLightViewProj(); 

	CGameObject* m_pSelectedObject = NULL; // 클릭된 객체 저장용 
	CGameObject* m_pCollidedObject = NULL;
	
	CGameObject* m_pWireframeBoxObject = NULL;

	bool m_bShowWireframeBox{ false };

	CGameObject* m_pCombinedAABBBoxObject = NULL;
	bool m_bShowCombinedAABB{ false };

	bool m_bIsCollision{ false };

	bool CheckCollision();

	bool CheckGroundCollision();

	void LoadTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	ID3D12Resource* m_pTreeTexture = NULL;
	ID3D12Resource* m_pTreeTextureUploadBuffer = NULL;

	ID3D12Resource* m_pFlowerTexture = NULL;
	ID3D12Resource* m_pFlowerTextureUploadBuffer = NULL;

	ID3D12Resource* m_pRockTexture = NULL;
	ID3D12Resource* m_pRockTextureUploadBuffer = NULL;

	ID3D12DescriptorHeap* m_pd3dCbvSrvHeap = NULL;

	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuTreeSrvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuFlowerSrvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuRockSrvHandle;

	CParticleEmitter* m_pParticleEmitter = NULL;

	ID3D12Resource* m_pExplosionTexture = NULL;
	ID3D12Resource* m_pExplosionTextureUploadBuffer = NULL;

	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuExplosionSrvHandle;

	CGameObject* m_pMirrorObject = NULL; // 거울 객체
	CReflectedObjectShader* m_pReflectedShader = NULL; // 반사된 물체 그릴 셰이더

	CShadowShader* m_pShadowShader = NULL; // 쉐도우 맵 생성용 셰이더

	UINT m_nDescriptorIncrementSize;

	CTerrainShader* m_pTerrainShader = NULL;
	//
public:
	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;

	CGameObject					**m_ppGameObjects = NULL;
	int							m_nGameObjects = 0;

	LIGHT						*m_pLights = NULL;
	int							m_nLights = 0;

	XMFLOAT4					m_xmf4GlobalAmbient;

	ID3D12Resource				*m_pd3dcbLights = NULL;
	LIGHTS						*m_pcbMappedLights = NULL;

	float						m_fElapsedTime = 0.0f;

	int m_nGFStage{};
};
