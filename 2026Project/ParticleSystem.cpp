#include "stdafx.h"
#include "ParticleSystem.h"
#include <random>

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

void CParticleSystem::ResetParticles(const XMFLOAT2& size)
{
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> distDir(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distSpeed(10.0f, 50.0f);
    std::uniform_real_distribution<float> distLife(0.5f, 1.5f);   

    m_nActiveParticles = 0;

    for (int i = 0; i < m_nMaxParticles; ++i)
    {
        m_vCpuParticles[i].m_bIsActive = true;

        m_vCpuParticles[i].m_fAge = 0.0f;
        m_vCpuParticles[i].m_fLifeTime = distLife(gen);

        m_vCpuParticles[i].m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);

        m_vCpuParticles[i].m_xmf2MaxSize = XMFLOAT2(size); // Å©±â

        XMFLOAT3 randomDir = XMFLOAT3(distDir(gen), distDir(gen), distDir(gen));

        XMVECTOR vDir = XMLoadFloat3(&randomDir);
        vDir = XMVector3Normalize(vDir);
        vDir *= distSpeed(gen);

        XMStoreFloat3(&m_vCpuParticles[i].m_xmf3Velocity, vDir);
    }
    CollisionAnimate(0.0);
    DustAnimate(0.0);
}

void CParticleSystem::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (0 == m_nActiveParticles) return;

	XMFLOAT4X4 xmf4x4World;
	XMMATRIX mWorld = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(mWorld));

	pd3dCommandList->SetGraphicsRoot32BitConstants(2, 16, &xmf4x4World, 0);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
	pd3dCommandList->DrawInstanced(m_nActiveParticles, 1, 0, 0);
}

void CParticleSystem::CollisionAnimate(float fTimeElapsed)
{

    XMMATRIX mWorld = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
    XMStoreFloat4x4(&m_xmf4x4World, XMMatrixTranspose(mWorld));

    m_nActiveParticles = 0;

    for (int i = 0; i < m_nMaxParticles; ++i)
    {
        if (!m_vCpuParticles[i].m_bIsActive) continue;

        m_vCpuParticles[i].m_fAge += fTimeElapsed;

        if (m_vCpuParticles[i].m_fAge > m_vCpuParticles[i].m_fLifeTime)
        {
            m_vCpuParticles[i].m_bIsActive = false;
            continue;
        }
            const float GRAVITY = -9.8f * 2.0f;
            m_vCpuParticles[i].m_xmf3Velocity.y += GRAVITY * fTimeElapsed;

            m_vCpuParticles[i].m_xmf3Position.x += m_vCpuParticles[i].m_xmf3Velocity.x * fTimeElapsed;
            m_vCpuParticles[i].m_xmf3Position.y += m_vCpuParticles[i].m_xmf3Velocity.y * fTimeElapsed;
            m_vCpuParticles[i].m_xmf3Position.z += m_vCpuParticles[i].m_xmf3Velocity.z * fTimeElapsed;

            float fLifeRatio = m_vCpuParticles[i].m_fAge / m_vCpuParticles[i].m_fLifeTime;
            float fScale = 1.0f - fLifeRatio;
            if (fScale < 0.0f) fScale = 0.0f;

            XMFLOAT2 currentSize;
            currentSize.x = m_vCpuParticles[i].m_xmf2MaxSize.x * fScale;
            currentSize.y = m_vCpuParticles[i].m_xmf2MaxSize.y * fScale;

            m_pMappedParticles[m_nActiveParticles].m_xmf3Position = m_vCpuParticles[i].m_xmf3Position;
            m_pMappedParticles[m_nActiveParticles].m_xmf2Size = currentSize;

            m_nActiveParticles++;
        }

}

void CParticleSystem::DustAnimate(float fTimeElapsed)
{
    XMMATRIX mWorld = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
    XMStoreFloat4x4(&m_xmf4x4World, XMMatrixTranspose(mWorld)); 

    m_nActiveParticles = 0;

    for (int i = 0; i < m_nMaxParticles; ++i)
    {
        if (!m_vCpuParticles[i].m_bIsActive) continue;

        m_vCpuParticles[i].m_fAge += fTimeElapsed;

        if (m_vCpuParticles[i].m_fAge > m_vCpuParticles[i].m_fLifeTime)
        {
            m_vCpuParticles[i].m_bIsActive = false;
            continue;
        }

        const float GRAVITY = 9.8f;
        m_vCpuParticles[i].m_xmf3Velocity.y += GRAVITY * fTimeElapsed;

        m_vCpuParticles[i].m_xmf3Position.x += m_vCpuParticles[i].m_xmf3Velocity.x * fTimeElapsed;
        m_vCpuParticles[i].m_xmf3Position.y += m_vCpuParticles[i].m_xmf3Velocity.y * fTimeElapsed;
        m_vCpuParticles[i].m_xmf3Position.z += m_vCpuParticles[i].m_xmf3Velocity.z * fTimeElapsed;


        float fLifeRatio = m_vCpuParticles[i].m_fAge / m_vCpuParticles[i].m_fLifeTime;
        float fScale = 1.0f - fLifeRatio; 
        if (fScale < 0.0f) fScale = 0.0f;

        XMFLOAT2 currentSize;
        currentSize.x = m_vCpuParticles[i].m_xmf2MaxSize.x * fScale;
        currentSize.y = m_vCpuParticles[i].m_xmf2MaxSize.y * fScale;


        m_pMappedParticles[m_nActiveParticles].m_xmf3Position = m_vCpuParticles[i].m_xmf3Position;
        m_pMappedParticles[m_nActiveParticles].m_xmf2Size = currentSize;

        m_nActiveParticles++;
    }
}

void CParticleSystem::BoosterAnimate(float fTimeElapsed)
{
    m_nActiveParticles = 0;

    int nParticlesToEmit = 5; 

    for (int i = 0; i < m_nMaxParticles; i++)
    {

        if (m_vCpuParticles[i].m_bIsActive)
        {
            m_vCpuParticles[i].m_fAge += fTimeElapsed;
            if (m_vCpuParticles[i].m_fAge > m_vCpuParticles[i].m_fLifeTime)
            {
                m_vCpuParticles[i].m_bIsActive = false;
                continue; 
            }

            m_vCpuParticles[i].m_xmf3Position.x += m_vCpuParticles[i].m_xmf3Velocity.x * fTimeElapsed;
            m_vCpuParticles[i].m_xmf3Position.y += m_vCpuParticles[i].m_xmf3Velocity.y * fTimeElapsed;
            m_vCpuParticles[i].m_xmf3Position.z += m_vCpuParticles[i].m_xmf3Velocity.z * fTimeElapsed;
        }

        else if (nParticlesToEmit > 0)
        {
            m_vCpuParticles[i].m_bIsActive = true;
            m_vCpuParticles[i].m_fAge = 0.0f;
            m_vCpuParticles[i].m_fLifeTime = 0.5f;

            float fRandX = ((float)(rand() % 100) / 50.0f) - 1.0f;
            float fRandY = ((float)(rand() % 100) / 50.0f) - 1.0f;
            m_vCpuParticles[i].m_xmf3Position = XMFLOAT3(fRandX * 0.5f, fRandY * 0.5f, 0.0f);

            float speed = 20.0f + (rand() % 10);
            m_vCpuParticles[i].m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, -speed);

            m_vCpuParticles[i].m_xmf2MaxSize = XMFLOAT2(10.5f, 20.5f);

            nParticlesToEmit--;
        }

        if (m_vCpuParticles[i].m_bIsActive)
        {
            float fLifeRatio = m_vCpuParticles[i].m_fAge / m_vCpuParticles[i].m_fLifeTime;
            float fScale = 1.0f - fLifeRatio;
            if (fScale < 0.0f) fScale = 0.0f;

            m_pMappedParticles[m_nActiveParticles].m_xmf3Position = m_vCpuParticles[i].m_xmf3Position;
            m_pMappedParticles[m_nActiveParticles].m_xmf2Size.x = m_vCpuParticles[i].m_xmf2MaxSize.x * fScale;
            m_pMappedParticles[m_nActiveParticles].m_xmf2Size.y = m_vCpuParticles[i].m_xmf2MaxSize.y * fScale;

            m_nActiveParticles++;
        }
    }
}
