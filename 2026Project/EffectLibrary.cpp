#include "stdafx.h"
#include "EffectLibrary.h"
#include "ParticleSystem.h"
#include "d3dx12.h"

// 쉐이더 컴파일 헬퍼 함수 (내부 사용용)
D3D12_SHADER_BYTECODE CompileShaderHelper(LPCWSTR filename, LPCSTR entrypoint, LPCSTR target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* byteCode = nullptr;
	ID3DBlob* errors = nullptr;

	HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint, target, compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
		errors->Release();
	}

	if (FAILED(hr))
	{
		return { nullptr, 0 };
	}

	return { byteCode->GetBufferPointer(), byteCode->GetBufferSize() };
}

CEffectLibrary* CEffectLibrary::Instance()
{
	static CEffectLibrary inst;
	return &inst;
}

void CEffectLibrary::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pd3dSrvHeap != nullptr)
	{
		Release();
	}
	BuildRootSignature(pd3dDevice);
	BuildPipelineState(pd3dDevice);

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 1; // 텍스처 수만큼
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;

	pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pd3dSrvHeap));
	
	m_d3dSrvCpuHandleStart = m_pd3dSrvHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dSrvGpuHandleStart = m_pd3dSrvHeap->GetGPUDescriptorHandleForHeapStart();
	
	LoadAssets(pd3dDevice, pd3dCommandList);

	for (int i = 0; i < 50; ++i) {
		CParticleSystem* pSys = new CParticleSystem(pd3dDevice, pd3dCommandList);
		
		ActiveEffect* pEffect = new ActiveEffect{ pSys, false, 0.0f };
		m_vEffectPool[(int)EFFECT_TYPE::EXPLOSION].push_back(pEffect);
	}// 이펙트 풀 생성
}

void CEffectLibrary::BuildRootSignature(ID3D12Device* pd3dDevice)
{
	CD3DX12_ROOT_PARAMETER rootParameters[3];

	CD3DX12_DESCRIPTOR_RANGE ranges[1];

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

	rootParameters[1].InitAsConstants(32, 6, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstants(16, 7, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_STATIC_SAMPLER_DESC sampler(
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(3, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	pd3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));

	if (signature) signature->Release();
	if (error) error->Release();
}

void CEffectLibrary::LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresource;

	LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/GreenStar.dds", &m_pExplosionTexture, ddsData, subresource);

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_pExplosionTexture, 0, static_cast<UINT>(subresource.size()));

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	pd3dDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pExplosionUploadBuffer));

	UpdateSubresources(pd3dCommandList, m_pExplosionTexture, m_pExplosionUploadBuffer, 0, 0, static_cast<UINT>(subresource.size()), subresource.data());

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pExplosionTexture,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	pd3dCommandList->ResourceBarrier(1, &barrier);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_pExplosionTexture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = m_pExplosionTexture->GetDesc().MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	pd3dDevice->CreateShaderResourceView(m_pExplosionTexture, &srvDesc, m_d3dSrvCpuHandleStart);
}

void CEffectLibrary::Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
	// [디버그 5] 이 함수가 호출은 되는지, 그리고 왜 리턴되는지 확인
	if (m_vActiveEffects.empty()) {
		OutputDebugStringA("[DEBUG] EffectLibrary::Render -> ActiveEffects is EMPTY (No effects to draw)\n");
		return;
	}
	if (m_pd3dSrvHeap == nullptr) {
		OutputDebugStringA("[DEBUG] EffectLibrary::Render -> Heap is NULL (Initialize not called!)\n");
		return;
	}

	// [디버그 6] 여기까지 오면 렌더링 시작
	OutputDebugStringA("[DEBUG] EffectLibrary::Render -> Rendering Start!\n");

	if (m_vActiveEffects.empty() || m_pd3dSrvHeap == nullptr) return;

	pd3dCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pd3dCommandList->SetPipelineState(m_pPipelineState);

	ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dSrvHeap };
	pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	pd3dCommandList->SetGraphicsRootDescriptorTable(0, m_d3dSrvGpuHandleStart);

	// 1. 행렬을 로드하고 전치(Transpose)합니다.
	XMMATRIX mView = XMLoadFloat4x4(&view);
	XMMATRIX mProj = XMLoadFloat4x4(&proj);

	XMFLOAT4X4 tMats[2];
	XMStoreFloat4x4(&tMats[0], XMMatrixTranspose(mView)); // View 전치
	XMStoreFloat4x4(&tMats[1], XMMatrixTranspose(mProj)); // Projection 전치
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 32, tMats, 0);
	
	for (auto eff : m_vActiveEffects) {
		eff->pParticleSys->Render(pd3dCommandList);
	}
}

void CEffectLibrary::BuildPipelineState(ID3D12Device* pd3dDevice)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	// 1) Input Layout 설정 (VS_VB_INSTANCE_PARTICLE 구조체와 일치해야 함)
	// Position(3) + Size(2)
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

	// 2) 쉐이더 컴파일 (파일 이름과 엔트리 포인트는 실제 HLSL 파일에 맞춰 수정하세요)
	// 보통 파티클은 PointList -> Geometry Shader에서 Quad로 확장합니다.
	psoDesc.VS = CompileShaderHelper(L"Shaders.hlsl", "VS_Particle", "vs_5_1");
	psoDesc.GS = CompileShaderHelper(L"Shaders.hlsl", "GS_Particle", "gs_5_1"); // 기하 쉐이더 필수 (Point->Quad)
	psoDesc.PS = CompileShaderHelper(L"Shaders.hlsl", "PS_Particle", "ps_5_1");

	psoDesc.pRootSignature = m_pRootSignature;

	// 3) 블렌드 스테이트 (이펙트는 보통 Alpha Blending이나 Additive Blending 사용)
	// 여기서는 Additive(더하기) 블렌딩 예시 (폭발/불꽃에 적합)
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // Additive
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState = blendDesc;

	// 4) 뎁스 스테이트 (깊이 판정은 하되, 깊이 버퍼에 쓰지는 않음 -> 투명 물체 국룰)
	D3D12_DEPTH_STENCIL_DESC depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 깊이 기록 끔 (반투명 겹침 문제 해결)
	psoDesc.DepthStencilState = depthDesc;

	// 5) 나머지 설정
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 양면 렌더링
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; // PointList로 그림
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
}

void CEffectLibrary::Play(EFFECT_TYPE type, XMFLOAT3 position)
{
	// [디버그 1] Play 호출 여부 확인
	OutputDebugStringA("[DEBUG] EffectLibrary::Play Called!\n");

	// 1. 해당 타입의 풀이 비어있는지 확인
	if (m_vEffectPool[(int)type].empty())
	{
		// [디버그 2] 풀이 비어서 리턴되는지 확인 (Initialize 안 된 경우)
		OutputDebugStringA("[DEBUG] Pool is Empty! (Initialization Failed?)\n");
		// 풀이 비었으면 생성을 못하므로 리턴 (혹은 여기서 동적 생성해도 됨)
		return;
	}

	// 2. 풀의 가장 마지막 녀석을 꺼냄 (가장 효율적)
	ActiveEffect* pEffectData = m_vEffectPool[(int)type].back();
	m_vEffectPool[(int)type].pop_back();

	// 3. 데이터 초기화
	if (pEffectData && pEffectData->pParticleSys)
	{
		
		pEffectData->bActive = true;
		pEffectData->fAge = 0.0f;

		// 파티클 시스템 자체 리셋 (위치 이동 및 파티클 재생성)
		pEffectData->pParticleSys->SetPosition(position);
		pEffectData->pParticleSys->ResetParticles();

		// 4. 활성 리스트에 등록
		m_vActiveEffects.push_back(pEffectData);
	}
}

void CEffectLibrary::Update(float fTimeElapsed)
{
	// 활성 리스트를 순회하며 업데이트
	auto it = m_vActiveEffects.begin();
	while (it != m_vActiveEffects.end())
	{
		ActiveEffect* pEffectData = *it;

		// 1. 수명 체크 (여기서는 2초로 하드코딩 했으나, EffectData에 MaxLifeTime 변수를 두는 게 좋음)
		pEffectData->fAge += fTimeElapsed;
		float fMaxLifeTime = 2.0f; // 예시

		if (pEffectData->fAge > fMaxLifeTime)
		{
			// 수명 종료: 비활성화하고 풀로 반납
			pEffectData->bActive = false;

			// 원래 있던 타입의 풀로 되돌림 (현재는 EXPLOSION 하나라고 가정)
			// 나중에 ActiveEffect 구조체에 'type' 멤버를 추가해서 구분하면 좋음
			m_vEffectPool[(int)EFFECT_TYPE::EXPLOSION].push_back(pEffectData);

			// 활성 리스트에서 제거
			it = m_vActiveEffects.erase(it);
		}
		else
		{
			// 2. 살아있다면 파티클 시뮬레이션 진행
			pEffectData->pParticleSys->Animate(fTimeElapsed);
			++it;
		}
	}
}

void CEffectLibrary::Release()
{
	// 1. 텍스처 및 업로드 버퍼 해제
	if (m_pExplosionTexture) m_pExplosionTexture->Release();
	if (m_pExplosionUploadBuffer) m_pExplosionUploadBuffer->Release();

	// 2. 힙 해제 및 nullptr 초기화
	if (m_pd3dSrvHeap) {
		m_pd3dSrvHeap->Release();
		m_pd3dSrvHeap = nullptr; // ★ 필수
	}

	// 3. 파이프라인 객체 해제
	if (m_pRootSignature) m_pRootSignature->Release();
	if (m_pPipelineState) m_pPipelineState->Release();

	m_pRootSignature = nullptr;
	m_pPipelineState = nullptr;
	m_pExplosionTexture = nullptr;
	m_pExplosionUploadBuffer = nullptr;

	// 4. [중요] 이펙트 풀 메모리 해제 (누수 수정됨)
	// 활성 리스트 해제
	for (auto eff : m_vActiveEffects)
	{
		if (eff->pParticleSys) delete eff->pParticleSys; // ★ 알맹이 삭제
		delete eff; // 껍데기 삭제
	}
	m_vActiveEffects.clear();

	// 대기 풀 해제
	for (int i = 0; i < (int)EFFECT_TYPE::COUNT; ++i)
	{
		for (auto eff : m_vEffectPool[i])
		{
			if (eff->pParticleSys) delete eff->pParticleSys; // ★ 알맹이 삭제
			delete eff; // 껍데기 삭제
		}
		m_vEffectPool[i].clear();
	}
}