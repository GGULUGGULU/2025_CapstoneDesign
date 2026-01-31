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
	COLLISION1, // √Êµπ
	COLLISION2, // √Êµπ
	COLLISION3, // √Êµπ
	DUST, // »Î∏’¡ˆ
	ITEM1, // æ∆¿Ã≈€ »πµÊ
	ITEM2, // æ∆¿Ã≈€ »πµÊ
	ITEM3, // æ∆¿Ã≈€ »πµÊ
	ITEM4, // æ∆¿Ã≈€ »πµÊ
	ITEM5, // æ∆¿Ã≈€ »πµÊ
	ITEM6, // æ∆¿Ã≈€ »πµÊ
	ITEM7, // æ∆¿Ã≈€ »πµÊ
	ITEM8, // æ∆¿Ã≈€ »πµÊ
	ITEM9, // æ∆¿Ã≈€ »πµÊ
	BOOSTER, // ∫ŒΩ∫≈Õ
	WIND_EFFECT,
	COUNT // ∞≥ºˆ
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
		/////////////////////////////////////////////////
		L"Asset/DDS_File/Dust.dds",
		/////////////////////////////////////////////////
		L"Asset/DDS_File/LongPinkRibbon.dds",
		L"Asset/DDS_File/LongRedRibbon.dds",
		L"Asset/DDS_File/LongYellowRibbon.dds",
		L"Asset/DDS_File/PinkCircle.dds",
		L"Asset/DDS_File/PinkTriangle.dds",
		L"Asset/DDS_File/ShortPinkRibbon.dds",
		L"Asset/DDS_File/ShortYellowRibbon.dds",
		L"Asset/DDS_File/YellowCircle.dds",
		L"Asset/DDS_File/YellowTriangle.dds"
		/////////////////////////////////////////////////
		L"Asset/DDS_File/Booster.dds",
		L"Asset/DDS_File/WindShield.dds",
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

