#pragma once

#include <d3d12.h>
#include <DirectxMath.h>
#include <vector>
#include "DDSTextureLoader12.h"

using namespace DirectX;

class CParticleSystem;

enum class EFFECT_TYPE
{
	EXPLOSION,
	COUNT
};

struct ActiveEffect {
	CParticleSystem* pParticleSys;
	bool bActive;
	float fAge;
};

class CEffectLibrary
{
public:
	static CEffectLibrary* Instance();

	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void Release();

	void Update(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& view, const XMFLOAT4X4& proj);

	void Play(EFFECT_TYPE type, XMFLOAT3 position);

private:
	CEffectLibrary() {}
	~CEffectLibrary() {}

	std::vector<ActiveEffect*> m_vActiveEffects;
	std::vector<ActiveEffect*> m_vEffectPool[(int)EFFECT_TYPE::COUNT];

	ID3D12DescriptorHeap* m_pd3dSrvHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSrvCpuHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSrvGpuHandleStart;

	ID3D12Resource* m_pExplosionTexture = nullptr;
	ID3D12Resource* m_pExplosionUploadBuffer = nullptr;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;

	void BuildRootSignature(ID3D12Device* pd3dDevice);
	void BuildPipelineState(ID3D12Device* pd3dDevice);
	void LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
};

