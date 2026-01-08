#include "stdafx.h"
#include "ParticleSystem.h"
#include <random>

//std::default_random_engine dre;
//::uniform_int_distribution<int> uid;

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

//void CParticleSystem::ResetParticles()
//{
//	for (int i = 0; i < m_nMaxParticles; ++i) {
//		m_vCpuParticles[i].m_bIsActive = false;
//		m_vCpuParticles[i].m_fAge = 0.0f;
//	}
//	m_nActiveParticles = 0;
//}

// 전역 혹은 클래스 멤버로 랜덤 엔진 정의 (여기서는 함수 내 static으로 예시)
void CParticleSystem::ResetParticles()
{
    // 랜덤 설정
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> distDir(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distSpeed(10.0f, 50.0f); // 속도 범위
    std::uniform_real_distribution<float> distLife(0.5f, 1.5f);    // 수명 범위

    m_nActiveParticles = 0;

    for (int i = 0; i < m_nMaxParticles; ++i)
    {
        // 1. 활성화
        m_vCpuParticles[i].m_bIsActive = true;

        // 2. 나이 초기화
        m_vCpuParticles[i].m_fAge = 0.0f;
        m_vCpuParticles[i].m_fLifeTime = distLife(gen);

        // 3. 위치 초기화 (로컬 기준 0,0,0에서 시작)
        m_vCpuParticles[i].m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);

        // 4. 크기 설정
        m_vCpuParticles[i].m_xmf2MaxSize = XMFLOAT2(50.0f, 50.0f); // 별 크기

        // 5. 랜덤 속도 부여 (폭발 효과: 사방으로 퍼짐)
        XMFLOAT3 randomDir = XMFLOAT3(distDir(gen), distDir(gen), distDir(gen));

        // 방향 정규화 (필요시) 후 속도 곱하기
        XMVECTOR vDir = XMLoadFloat3(&randomDir);
        vDir = XMVector3Normalize(vDir);
        vDir *= distSpeed(gen);

        XMStoreFloat3(&m_vCpuParticles[i].m_xmf3Velocity, vDir);

        // 초기화하자마자 활성 카운트 증가시키면 안됨 (Animate에서 처리하거나 여기서 미리 증가)
        // 로직상 Animate가 호출되어야 GPU 버퍼로 넘어갑니다.
    }
    Animate(0.0);

    // [디버그 3] 카운트가 정상적으로 늘었는지 확인
    char debugBuf[64];
    sprintf_s(debugBuf, "[DEBUG] ResetParticles Done. Count: %d\n", m_nActiveParticles);
    OutputDebugStringA(debugBuf);

}

void CParticleSystem::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
    // [디버그 4] if 문 "바깥"에서 찍어보세요.
    // 아예 이 로그가 안 뜬다면 EffectLibrary::Render에서 반복문이 안 도는 겁니다.
    char debugBuf[64];
    sprintf_s(debugBuf, "[DEBUG] CParticleSystem::Render Called. Current Count: %d\n", m_nActiveParticles);
    OutputDebugStringA(debugBuf);

	if (0 == m_nActiveParticles) return;

    // [디버깅] 현재 활성화된 파티클 개수 출력
    if (m_nActiveParticles > 0) {
        char debugBuf[64];
        sprintf_s(debugBuf, "Active Particles: %d\n", m_nActiveParticles);
        OutputDebugStringA(debugBuf);
    }

	XMFLOAT4X4 xmf4x4World;
	XMMATRIX mWorld = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(mWorld));

	pd3dCommandList->SetGraphicsRoot32BitConstants(2, 16, &xmf4x4World, 0);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
	pd3dCommandList->DrawInstanced(m_nActiveParticles, 1, 0, 0);
}

void CParticleSystem::Animate(float fTimeElapsed)
{
    // 1. 월드 행렬 업데이트
    // 이 파티클 시스템 전체가 놓일 위치(m_xmf3Position)로 행렬을 만듭니다.
    // 렌더링할 때 이 행렬을 쉐이더로 보냅니다.
    XMMATRIX mWorld = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
    XMStoreFloat4x4(&m_xmf4x4World, XMMatrixTranspose(mWorld)); // 전치 행렬 저장

    // 이번 프레임에 그릴 파티클 개수 초기화
    m_nActiveParticles = 0;

    // 2. 모든 파티클 순회하며 업데이트
    for (int i = 0; i < m_nMaxParticles; ++i)
    {
        // 비활성 파티클은 무시
        if (!m_vCpuParticles[i].m_bIsActive) continue;

        // -------------------------------------------------------
        // A. 수명 관리
        // -------------------------------------------------------
        m_vCpuParticles[i].m_fAge += fTimeElapsed;

        if (m_vCpuParticles[i].m_fAge > m_vCpuParticles[i].m_fLifeTime)
        {
            m_vCpuParticles[i].m_bIsActive = false; // 수명 다함 -> 사망
            continue; // 다음 파티클로 넘어감 (GPU 복사 안 함)
        }

        // -------------------------------------------------------
        // B. 물리 시뮬레이션 (위치 = 기존위치 + 속도 * 시간)
        // -------------------------------------------------------
        // 중력 적용 (y축 속도 감소)
        const float GRAVITY = -9.8f * 2.0f; // 좀 더 무겁게 2배 중력
        m_vCpuParticles[i].m_xmf3Velocity.y += GRAVITY * fTimeElapsed;

        // 위치 갱신
        m_vCpuParticles[i].m_xmf3Position.x += m_vCpuParticles[i].m_xmf3Velocity.x * fTimeElapsed;
        m_vCpuParticles[i].m_xmf3Position.y += m_vCpuParticles[i].m_xmf3Velocity.y * fTimeElapsed;
        m_vCpuParticles[i].m_xmf3Position.z += m_vCpuParticles[i].m_xmf3Velocity.z * fTimeElapsed;

        // -------------------------------------------------------
        // C. 크기 애니메이션 (점점 커졌다가 작아지기 or 그냥 작아지기)
        // -------------------------------------------------------
        // 예시: 수명이 다해갈수록 0으로 줄어듦
        float fLifeRatio = m_vCpuParticles[i].m_fAge / m_vCpuParticles[i].m_fLifeTime;
        float fScale = 1.0f - fLifeRatio; // 1.0 -> 0.0으로 감소
        if (fScale < 0.0f) fScale = 0.0f;

        XMFLOAT2 currentSize;
        currentSize.x = m_vCpuParticles[i].m_xmf2MaxSize.x * fScale;
        currentSize.y = m_vCpuParticles[i].m_xmf2MaxSize.y * fScale;

        // -------------------------------------------------------
        // D. GPU 매핑 버퍼에 복사 (Active한 녀석들만 차곡차곡 쌓음)
        // -------------------------------------------------------
        // m_pMappedParticles는 생성자에서 Map() 해둔 포인터입니다.
        m_pMappedParticles[m_nActiveParticles].m_xmf3Position = m_vCpuParticles[i].m_xmf3Position;
        m_pMappedParticles[m_nActiveParticles].m_xmf2Size = currentSize;

        // 활성 파티클 개수 증가 (Render 함수에서 이 개수만큼만 그림)
        m_nActiveParticles++;
    }
}