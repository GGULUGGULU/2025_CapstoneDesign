#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;
struct VS_VB_INSTANCE_PARTICLE
{
	XMFLOAT3 m_xmf3Position;
	XMFLOAT2 m_xmf2Size;
};

struct ParticleCPUData
{
	XMFLOAT3 m_xmf3Position;
	XMFLOAT3 m_xmf3Velocity;
	XMFLOAT2 m_xmf2MaxSize;
	float m_fAge;
	float m_fLifeTime;
	bool m_bIsActive;
};

class CParticleSystem{
public:
	CParticleSystem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nMaxParticles = 1000);
	virtual ~CParticleSystem();

	void Animate(float fTimeElapsed);

	void Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projMatrix);

	void ResetParticles();

	void SetPosition(const XMFLOAT3& pos) { m_xmf3Position = pos; }

private:
	XMFLOAT3 m_xmf3Position;
	XMFLOAT4X4 m_xmf4x4World;

	int m_nMaxParticles;
	int m_nActiveParticles;
	std::vector<ParticleCPUData> m_vCpuParticles;

	ID3D12Resource* m_pd3dVertexBuffer = nullptr;
	ID3D12Resource* m_pd3dVertexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView;
	VS_VB_INSTANCE_PARTICLE* m_pMappedParticles = nullptr;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;

	friend class CEffectLibrary;
};

