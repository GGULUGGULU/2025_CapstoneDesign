#include "stdafx.h"
#include "ParticleSystem.h"
#include <random>

std::default_random_engine dre;
std::uniform_int_distribution<int> uid;

CParticleSystem::CParticleSystem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nMaxParticles)
{
	m_nMaxParticles = nMaxParticles;
	m_nActiveParticles = 0;
	m_vCpuParticles.resize(m_nMaxParticles);
	m_xmf3Position = XMFLOAT3(0, 0, 0);
	XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());

	UINT nStride = sizeof(VS_VB_INSTANCE_PARTICLE);
	UINT nBufferSize = nStride * m_nMaxParticles;

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(nBufferSize);

	pd3dDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_pd3dVertexBuffer));

	m_pd3dVertexBuffer->Map(0, NULL, (void**)&m_pMappedParticles);

	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = nStride;
	m_d3dVertexBufferView.SizeInBytes = nBufferSize;
}

CParticleSystem::~CParticleSystem()
{
	if (m_pd3dVertexBuffer)
	{
		m_pd3dVertexBuffer->Unmap(0, NULL);
		m_pd3dVertexBuffer->Release();
	}
}

void CParticleSystem::ResetParticles()
{
	for (int i = 0; i < m_nMaxParticles; ++i) {
		m_vCpuParticles[i].m_bIsActive = false;
		m_vCpuParticles[i].m_fAge = 0.0f;
	}
	m_nActiveParticles = 0;
}

