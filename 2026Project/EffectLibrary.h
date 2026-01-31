#pragma once

#include <d3d12.h>
#include <DirectxMath.h>
#include <vector>
#include "DDSTextureLoader12.h"

using namespace DirectX;

class CParticleSystem;
class CMeshEffect;

enum class EFFECT_TYPE
{
	COLLISION1, // 충돌
	COLLISION2, // 충돌
	COLLISION3, // 충돌
	DUST, // 흙먼지
	//ITEM, // 아이템 획득
	BOOSTER, // 부스터
	WIND_EFFECT,
	COUNT // 개수
};

struct ActiveEffect {
	EFFECT_TYPE type;
	bool bActive;
	float fAge;

	CParticleSystem* pParticleSys;
	CMeshEffect* pMeshEffect;
};

class CEffectLibrary
{
public:
	static CEffectLibrary* Instance();

	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void Release();

	void Update(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& view, const XMFLOAT4X4& proj);

	ActiveEffect* Play(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size);

	void ToggleBooster(bool flag);
	void UpdateBoosterPosition(const XMFLOAT3&, const XMFLOAT3&);

private:
	CEffectLibrary() {}
	~CEffectLibrary() {}

	std::vector<ActiveEffect*> m_vActiveEffects;
	std::vector<ActiveEffect*> m_vEffectPool[(int)EFFECT_TYPE::COUNT];

	ID3D12DescriptorHeap* m_pd3dSrvHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSrvCpuHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSrvGpuHandleStart;

	std::vector<ID3D12Resource*> m_vTextures;
	std::vector<ID3D12Resource*> m_vUploadBuffers;

	const std::wstring m_TextureFileNames[(int)EFFECT_TYPE::COUNT] = {
		L"Asset/DDS_File/GreenStar.dds",
		L"Asset/DDS_File/RedStar.dds",
		L"Asset/DDS_File/PurpleStar.dds",
		L"Asset/DDS_File/Dust.dds",
		L"Asset/DDS_File/Booster.dds",
		L"Asset/DDS_File/WindShield.dds"
		/*
		L"Asset/DDS_File/WindShield.dds"
		

		*/
	};

	UINT m_nSrvDescriptorIncrementSize = 0;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	ID3D12PipelineState* m_pMeshEffectPSO = nullptr;


	void BuildRootSignature(ID3D12Device* pd3dDevice);
	void BuildPipelineState(ID3D12Device* pd3dDevice);
	void LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	ActiveEffect* m_pBoosterEffect = nullptr;
	ActiveEffect* m_pWindShieldEffect = nullptr;
};

