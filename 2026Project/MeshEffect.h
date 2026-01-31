#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <vector>
#include "d3dx12.h"

using namespace DirectX;

struct EFFECT_VERTEX
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

struct CB_EFFECT_DATA
{
	XMFLOAT4X4 m_xmf4x4World;
	float m_fTime;
	XMFLOAT3 m_fScrollSpeed;
};

class CMeshEffect
{public:
	CMeshEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	~CMeshEffect();

	void CreateMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fRadius, int nSlices, int nStacks);
	void CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName);

	void CreateProceduralTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

	void Render(ID3D12GraphicsCommandList* pd3dCommandList);

	void SetPosition(XMFLOAT3& pos) { m_xmf3Position = pos; }
	void SetRotation(XMFLOAT3& rot) { m_xmf3Rotation = rot; }
	void SetScale(XMFLOAT3& scale) { m_xmf3Scale = scale; }

	void SetActive(bool flag) { m_bActive = flag; }
	bool IsActive() const { return m_bActive; }

	void Update(float fTimeElapsed);

private:
	bool m_bActive = false;
	float m_fCurrentTime = 0.0f;

	XMFLOAT3 m_xmf3Position{ 0,0,0 };
	XMFLOAT3 m_xmf3Rotation{ 0,0,0 };
	XMFLOAT3 m_xmf3Scale{ 1,1,1 };
	XMFLOAT4X4 m_xmf4x4World;

	ID3D12Resource* m_pVertexBuffer = nullptr;
	ID3D12Resource* m_pVertexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView;

	ID3D12Resource* m_pIndexBuffer = nullptr;
	ID3D12Resource* m_pIndexUploadBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_d3dIndexBufferView;

	UINT m_nIndices = 0;

	ID3D12Resource* m_pTexture = nullptr;
	ID3D12Resource* m_pTextureUploadBuffer = nullptr;
	ID3D12DescriptorHeap* m_pSrvHeap = nullptr;
};

