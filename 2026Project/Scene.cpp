//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "WireframeBoxMesh.h"
#include "EffectLibrary.h"

#include <random>

std::random_device rd;
std::default_random_engine dre{ rd() };
std::uniform_int_distribution<int> uid(0, 180); 
std::uniform_int_distribution<int> uid1(-2500,-1500); 
std::uniform_int_distribution<int> uid2(1500,2500); 
std::uniform_int_distribution<int> uid3(-2500,2500);
std::uniform_int_distribution<int> uid4(-1500,1500);

CScene::CScene()
{
	m_d3dGpuTreeSrvHandle.ptr = 0;
	m_d3dGpuFlowerSrvHandle.ptr = 0;
	m_d3dGpuRockSrvHandle.ptr = 0;
}

CScene::~CScene()
{
	if (m_pd3dCbvSrvHeap) m_pd3dCbvSrvHeap->Release();
	if (m_pTreeTexture) m_pTreeTexture->Release();
	if (m_pFlowerTexture) m_pFlowerTexture->Release();
	if (m_pRockTexture) m_pRockTexture->Release();
}

void CScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 4;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = POINT_LIGHT;
	m_pLights[0].m_fRange = 1000.0f;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(30.0f, 30.0f, 30.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.001f, 0.0001f);
	m_pLights[1].m_bEnable = true;

	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 500.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-50.0f, 20.0f, -5.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[1].m_fFalloff = 8.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(20.0f));
	m_pLights[2].m_bEnable = true;

	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(1.0f, -1.0f, 0.5f); // 위에서 아래로 방향-> 사선7시방향
	m_pLights[3].m_bEnable = true;

	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(50.0f, 30.0f, 30.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));
}

void CScene::BuildObjectsGameStart(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	m_nGameObjects = 1;
	m_ppGameObjects = new CGameObject*[m_nGameObjects];

	CGameObject* pGameStartModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/GameStart.bin");
	CGameObject* pGameStartObject = new CGameObject();
	pGameStartObject->SetChild(pGameStartModel);
	pGameStartObject->SetPosition(-100.0f, 0.0f, 150.0f);
	pGameStartObject->Rotate(0.0f, -90.0f, 0.0f);
	pGameStartObject->SetScale(5, 5, 5);
	pGameStartObject->Rotate(0.0f,-135.f, 0.0f);
	m_ppGameObjects[0] = pGameStartObject;

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}//

void CScene::BuildObjectsGameEnd(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}//

void SetTerrainRecursive(CGameObject* pObject, D3D12_PRIMITIVE_TOPOLOGY topology, CMaterial* pMaterial)
{
	if (!pObject) return;
	if (pObject->m_pMesh)
	{
		pObject->m_pMesh->SetPrimitiveTopology(topology);

		if (pMaterial)
		{
			if (pObject->m_ppMaterials)
			{
				for (int i = 0; i < pObject->m_nMaterials; i++)
				{
					if (pObject->m_ppMaterials[i]) pObject->m_ppMaterials[i]->Release();
				}
				delete[] pObject->m_ppMaterials;
				pObject->m_ppMaterials = NULL;
			}

			pObject->m_nMaterials = 1;
			pObject->m_ppMaterials = new CMaterial * [1];
			pObject->m_ppMaterials[0] = NULL;
			pObject->SetMaterial(0, pMaterial);
		}
	}

	if (pObject->m_pSibling) SetTerrainRecursive(pObject->m_pSibling, topology, pMaterial);
	if (pObject->m_pChild) SetTerrainRecursive(pObject->m_pChild, topology, pMaterial);
}

void CScene::BuildGameObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pTerrainShader = new CTerrainShader();
	m_pTerrainShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	LoadTexture(pd3dDevice, pd3dCommandList);
	
	m_nGameObjects = 1 + 1 + 12 + 12 + 12 +1 +20 +20 +15 +15 + 4;
	m_ppGameObjects = new CGameObject * [m_nGameObjects];

	CGameObject* pGroundModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Ground.bin");
	CGameObject* pGroundObject = new CGameObject();
	pGroundObject->SetChild(pGroundModel);
	pGroundObject->SetPosition(0.0f, 0.0f, 0.0f);
	pGroundObject->Rotate(0.0f, 0.0f, 0.0f);
	pGroundObject->SetScale(100, 5, 100);
	pGroundObject->Rotate(0.0f, 0.f, 0.0f);
	pGroundObject->ComputeCombinedAABB();

	CMaterial* pTerrainMaterial = new CMaterial();
	pTerrainMaterial->SetShader(m_pTerrainShader);

	CMaterialColors* pTerrainColors = new CMaterialColors();
	pTerrainColors->m_xmf4Diffuse = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	pTerrainColors->m_xmf4Ambient = XMFLOAT4(0.1f, 0.3f, 0.1f, 1.0f);
	pTerrainMaterial->SetMaterialColors(pTerrainColors);

	SetTerrainRecursive(pGroundModel, D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST, pTerrainMaterial);

	m_ppGameObjects[0] = pGroundObject;

	CGameObject* pSuperCobraModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SuperCobra.bin");
	CSuperCobraObject* pSuperCobraObject = NULL;

	pSuperCobraObject = new CSuperCobraObject();
	pSuperCobraObject->SetChild(pSuperCobraModel, true);
	pSuperCobraObject->OnInitialize();
	pSuperCobraObject->SetPosition(20.0f, 8.0f, 100.0f);
	pSuperCobraObject->SetScale(4.5f, 4.5f, 4.5f);
	pSuperCobraObject->Rotate(0.0f, -90.0f, 0.0f);
	pSuperCobraObject->ComputeCombinedAABB();
	m_ppGameObjects[1] = pSuperCobraObject;

	{
		CreateTreeBillboard(pd3dDevice, pd3dCommandList);
		CreateFlowerBillboard(pd3dDevice, pd3dCommandList);
		CreateRockBillboard(pd3dDevice, pd3dCommandList);
	}

	CGameObject* pObstacleModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Cube.bin");
	CGameObject* pObstacleObject = new CGameObject();
	pObstacleObject->SetChild(pObstacleModel);
	pObstacleObject->SetPosition(0.0f, 0.0f, 0.0f);
	pObstacleObject->Rotate(0.0f, 0.0f, 0.0f);
	pObstacleObject->SetScale(100, 100, 100);
	pObstacleObject->Rotate(0.0f, 0.f, 0.0f);
	pObstacleObject->ComputeCombinedAABB();
	m_ppGameObjects[38] = pObstacleObject;
	// 가운데 장애물

	{
		for (int i = 0; i < 20; ++i) {
			CGameObject* pSuperCobraModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SuperCobra.bin");
			CSuperCobraObject* pSuperCobraObject = NULL;
			pSuperCobraObject = new CSuperCobraObject();
			pSuperCobraObject->SetChild(pSuperCobraModel, true);
			pSuperCobraObject->OnInitialize();
			pSuperCobraObject->SetPosition(uid1(dre), 18.0f, uid3(dre));
			pSuperCobraObject->SetScale(4.5f, 4.5f, 4.5f);
			pSuperCobraObject->Rotate(0.0f, uid(dre), 0.0f);
			pSuperCobraObject->ComputeCombinedAABB();
			m_ppGameObjects[39+i] = pSuperCobraObject;
		}

		for (int i = 0; i < 20; ++i) {
			CGameObject* pSuperCobraModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SuperCobra.bin");
			CSuperCobraObject* pSuperCobraObject = NULL;
			pSuperCobraObject = new CSuperCobraObject();
			pSuperCobraObject->SetChild(pSuperCobraModel, true);
			pSuperCobraObject->OnInitialize();
			pSuperCobraObject->SetPosition(uid2(dre), 18.0f, uid3(dre));
			pSuperCobraObject->SetScale(4.5f, 4.5f, 4.5f);
			pSuperCobraObject->Rotate(0.0f, uid(dre), 0.0f);
			pSuperCobraObject->ComputeCombinedAABB();
			m_ppGameObjects[59 + i] = pSuperCobraObject;
		}

		for (int i = 0; i < 15; ++i) {
			CGameObject* pSuperCobraModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SuperCobra.bin");
			CSuperCobraObject* pSuperCobraObject = NULL;
			pSuperCobraObject = new CSuperCobraObject();
			pSuperCobraObject->SetChild(pSuperCobraModel, true);
			pSuperCobraObject->OnInitialize();
			pSuperCobraObject->SetPosition(uid4(dre), 18.0f, uid2(dre));
			pSuperCobraObject->SetScale(4.5f, 4.5f, 4.5f);
			pSuperCobraObject->Rotate(0.0f, uid(dre), 0.0f);
			pSuperCobraObject->ComputeCombinedAABB();
			m_ppGameObjects[79 + i] = pSuperCobraObject;
		}

		for (int i = 0; i < 15; ++i) {
			CGameObject* pSuperCobraModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SuperCobra.bin");
			CSuperCobraObject* pSuperCobraObject = NULL;
			pSuperCobraObject = new CSuperCobraObject();
			pSuperCobraObject->SetChild(pSuperCobraModel, true);
			pSuperCobraObject->OnInitialize();
			pSuperCobraObject->SetPosition(uid4(dre), 18.0f, uid1(dre));
			pSuperCobraObject->SetScale(4.5f, 4.5f, 4.5f);
			pSuperCobraObject->Rotate(0.0f, uid(dre), 0.0f);
			pSuperCobraObject->ComputeCombinedAABB();
			m_ppGameObjects[94 + i] = pSuperCobraObject;
		}

		
		CGameObject* pItemModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item.bin");
		CGameObject* pItemObject = new CGameObject();
		pItemObject->SetChild(pItemModel);
		pItemObject->SetPosition(-2300.0f, 50.0f, 2300.0f);
		pItemObject->Rotate(0.0f, 0.0f, 0.0f);
		pItemObject->SetScale(10, 10, 10);
		pItemObject->Rotate(0.0f, 0.f, 0.0f);
		pItemObject->ComputeCombinedAABB();
		m_ppGameObjects[109] = pItemObject;

		CGameObject* pItemModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item.bin");
		CGameObject* pItemObject1 = new CGameObject();
		pItemObject1->SetChild(pItemModel1);
		pItemObject1->SetPosition(-2200.0f, 20.0f, -2300.0f);
		pItemObject1->Rotate(0.0f, 0.0f, 0.0f);
		pItemObject1->SetScale(10, 10, 10);
		pItemObject1->Rotate(0.0f, 0.f, 0.0f);
		pItemObject1->ComputeCombinedAABB();
		m_ppGameObjects[110] = pItemObject1;

		CGameObject* pItemModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item.bin");
		CGameObject* pItemObject2 = new CGameObject();
		pItemObject2->SetChild(pItemModel2);
		pItemObject2->SetPosition(2300.0f, 40.0f, -2300.0f);
		pItemObject2->Rotate(0.0f, 0.0f, 0.0f);
		pItemObject2->SetScale(10, 10, 10);
		pItemObject2->Rotate(0.0f, 0.f, 0.0f);
		pItemObject2->ComputeCombinedAABB();
		m_ppGameObjects[111] = pItemObject2;

		CGameObject* pItemModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item.bin");
		CGameObject* pItemObject3 = new CGameObject();
		pItemObject3->SetChild(pItemModel3);
		pItemObject3->SetPosition(2200.0f, 50.0f, 2300.0f);
		pItemObject3->Rotate(0.0f, 0.0f, 0.0f);
		pItemObject3->SetScale(10, 10, 10);
		pItemObject3->Rotate(0.0f, 0.f, 0.0f);
		pItemObject3->ComputeCombinedAABB();
		m_ppGameObjects[112] = pItemObject3;
	}

	CreateMirror(pd3dDevice, pd3dCommandList);
	CreateWireFrameBox(pd3dDevice, pd3dCommandList);
	CreateAABBWireFrameBox(pd3dDevice, pd3dCommandList);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CEffectLibrary::Instance()->Initialize(pd3dDevice, pd3dCommandList);

	m_pShadowShader = new CShadowShader();
	m_pShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
}

void CScene::CreateTreeBillboard(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CBillboardVertex* pTreeMesh = new CBillboardVertex(pd3dDevice, pd3dCommandList);

	CMaterial* pTreeMaterial = new CMaterial();

	CMaterialColors* pTreeColors = new CMaterialColors();
	pTreeColors->m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pTreeColors->m_xmf4Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pTreeMaterial->SetMaterialColors(pTreeColors);
	pTreeMaterial->SetShader(CMaterial::m_pBillboardShader);
	pTreeMaterial->SetTexture(m_d3dGpuTreeSrvHandle);

	for (int i = 0; i < 12; ++i) {
		CGameObject* pTreeObject = new CGameObject();

		pTreeObject->SetMesh(pTreeMesh); 
		pTreeObject->m_nMaterials = 1;
		pTreeObject->m_ppMaterials = new CMaterial * [pTreeObject->m_nMaterials];
		pTreeObject->m_ppMaterials[0] = NULL;
		pTreeObject->SetMaterial(0, pTreeMaterial); 
		pTreeObject->SetPosition(-2750.0f, 0.0f, 2750.0f - 500 * i);

		m_ppGameObjects[2 + i] = pTreeObject; // index = 13
	}
}

void CScene::CreateShadowMapSRV(ID3D12Device* pd3dDevice, ID3D12Resource* pShadowMapResource)
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dSrvCpuHandle = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();

	UINT nDescriptorSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	d3dSrvCpuHandle.ptr += (nDescriptorSize * 4);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; 
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	pd3dDevice->CreateShaderResourceView(pShadowMapResource, &srvDesc, d3dSrvCpuHandle);
}

void CScene::CreateFlowerBillboard(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CBillboardVertex* pFlowerMesh = new CBillboardVertex(pd3dDevice, pd3dCommandList);

	CMaterial* pFlowerMaterial = new CMaterial();

	CMaterialColors* pFlowerColors = new CMaterialColors();
	pFlowerColors->m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pFlowerColors->m_xmf4Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pFlowerMaterial->SetMaterialColors(pFlowerColors);
	pFlowerMaterial->SetShader(CMaterial::m_pBillboardShader);
	pFlowerMaterial->SetTexture(m_d3dGpuFlowerSrvHandle);

	for (int i = 0; i < 12; ++i) {
		CGameObject* pFlowerObject = new CGameObject();

		pFlowerObject->SetMesh(pFlowerMesh);
		pFlowerObject->m_nMaterials = 1;
		pFlowerObject->m_ppMaterials = new CMaterial * [pFlowerObject->m_nMaterials];
		pFlowerObject->m_ppMaterials[0] = NULL;
		pFlowerObject->SetScale(1, 0.5, 1);
		pFlowerObject->SetMaterial(0, pFlowerMaterial); 
		pFlowerObject->SetPosition(-2750.0f + 500 * i, 0.0f, -2750.0f);

		m_ppGameObjects[14 + i] = pFlowerObject; // index = 13
	}
}

void CScene::CreateRockBillboard(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CMaterial* pRockMaterial = new CMaterial();

	CMaterialColors* pRockColors = new CMaterialColors();
	pRockColors->m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); 
	pRockColors->m_xmf4Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	pRockMaterial->SetMaterialColors(pRockColors);
	pRockMaterial->SetShader(CMaterial::m_pBillboardShader);
	pRockMaterial->SetTexture(m_d3dGpuRockSrvHandle);

	for (int i = 0; i < 12; ++i) {
		
		int temp = rand() / 150;
		if (temp < 70) temp = 75;

		CBillboardVertex* pRockMesh = new CBillboardVertex(pd3dDevice, pd3dCommandList, 100, temp);

		CGameObject* pRockObject = new CGameObject();
		pRockObject->SetMesh(pRockMesh); 
		pRockObject->m_nMaterials = 1;
		pRockObject->m_ppMaterials = new CMaterial * [pRockObject->m_nMaterials];
		pRockObject->m_ppMaterials[0] = NULL;
		pRockObject->SetMaterial(0, pRockMaterial); 
		pRockObject->SetPosition(2750.0f, 10.0f, -2750.0f + 500 * i);

		m_ppGameObjects[26 + i] = pRockObject; // index = 13
	}
}

void CScene::CreateMirror(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pReflectedShader = new CReflectedObjectShader();
	m_pReflectedShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pReflectedShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	{
		CGameObject* pMirrorModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/MirrorObject.bin");

		if (pMirrorModel->GetMesh() == NULL && pMirrorModel->m_pChild != NULL)
		{
			m_pMirrorObject = pMirrorModel->m_pChild;
		}
		else
		{
			m_pMirrorObject = pMirrorModel;
		}

		m_pMirrorObject->SetPosition(0.0f, 50.0f, 2500.0f);
		m_pMirrorObject->SetScale(600.0f, 10.0f, 10.0f);
		m_pMirrorObject->Rotate(90.0f, 180.0f, 0.0f);

		CMirrorShader* pMirrorSurfaceShader = new CMirrorShader();
		pMirrorSurfaceShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

		CMaterial* pMirrorMaterial = new CMaterial();
		CMaterialColors* pMirrorColors = new CMaterialColors();

		pMirrorColors->m_xmf4Diffuse = XMFLOAT4(0.0f, 0.1f, 0.5f, 0.5f); 
		pMirrorMaterial->SetMaterialColors(pMirrorColors);
		pMirrorMaterial->SetShader(pMirrorSurfaceShader);

		m_pMirrorObject->m_nMaterials = 1;
		m_pMirrorObject->m_ppMaterials = new CMaterial * [1];
		m_pMirrorObject->m_ppMaterials[0] = NULL;
		m_pMirrorObject->SetMaterial(0, pMirrorMaterial);
	}
}

void CScene::CreateWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CWireframeBoxMesh* pDebugBoxMesh = new CWireframeBoxMesh(pd3dDevice, pd3dCommandList);

	CMaterial* pDebugMaterial = new CMaterial();

	CMaterialColors* pDebugColors = new CMaterialColors();
	pDebugColors->m_xmf4Diffuse = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f); 
	pDebugColors->m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); 
	pDebugColors->m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); 
	pDebugMaterial->SetMaterialColors(pDebugColors);

	pDebugMaterial->SetShader(CMaterial::m_pDiffusedShader);

	m_pWireframeBoxObject = new CGameObject();
	m_pWireframeBoxObject->SetMesh(pDebugBoxMesh);
	m_pWireframeBoxObject->m_nMaterials = 1;
	m_pWireframeBoxObject->m_ppMaterials = new CMaterial * [m_pWireframeBoxObject->m_nMaterials];
	m_pWireframeBoxObject->m_ppMaterials[0] = NULL;
	m_pWireframeBoxObject->SetMaterial(0, pDebugMaterial);
	m_pWireframeBoxObject->SetScale(0.0f, 0.0f, 0.0f);
	m_pWireframeBoxObject->UpdateTransform(NULL);
}

void CScene::CreateAABBWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CWireframeBoxMesh* pRedBoxMesh = new CWireframeBoxMesh(pd3dDevice, pd3dCommandList);

	CMaterial* pRedMaterial = new CMaterial();
	CMaterialColors* pRedColors = new CMaterialColors();
	pRedColors->m_xmf4Diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); 
	pRedColors->m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	pRedColors->m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	pRedMaterial->SetMaterialColors(pRedColors);
	pRedMaterial->SetShader(CMaterial::m_pDiffusedShader); //

	m_pCombinedAABBBoxObject = new CGameObject();
	m_pCombinedAABBBoxObject->SetMesh(pRedBoxMesh);
	m_pCombinedAABBBoxObject->m_nMaterials = 1;
	m_pCombinedAABBBoxObject->m_ppMaterials = new CMaterial * [1];
	m_pCombinedAABBBoxObject->m_ppMaterials[0] = NULL;
	m_pCombinedAABBBoxObject->SetMaterial(0, pRedMaterial);

	m_pCombinedAABBBoxObject->SetScale(0.0f, 0.0f, 0.0f);
	m_pCombinedAABBBoxObject->UpdateTransform(NULL);
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
	}

	if (m_pWireframeBoxObject) m_pWireframeBoxObject->Release();
	if (m_pCombinedAABBBoxObject) m_pCombinedAABBBoxObject->Release();

	if (m_pTreeTexture) m_pTreeTexture->Release();
	if (m_pTreeTextureUploadBuffer) m_pTreeTextureUploadBuffer->Release();

	if (m_pFlowerTexture) m_pFlowerTexture->Release();
	if (m_pFlowerTextureUploadBuffer) m_pFlowerTextureUploadBuffer->Release();

	if (m_pRockTexture) m_pRockTexture->Release();
	if (m_pRockTextureUploadBuffer) m_pRockTextureUploadBuffer->Release();

	if (m_pd3dCbvSrvHeap) m_pd3dCbvSrvHeap->Release();

	if (m_pShadowShader) { m_pShadowShader->Release(); m_pShadowShader = NULL; }

	m_pTreeTexture = NULL;
	m_pTreeTextureUploadBuffer = NULL;

	m_pFlowerTexture = NULL;
	m_pFlowerTextureUploadBuffer = NULL;

	m_pRockTexture = NULL;
	m_pRockTextureUploadBuffer = NULL;

	m_pd3dCbvSrvHeap = NULL;

	// [추가] 라이브러리 리소스 해제 호출
	CEffectLibrary::Instance()->Release();

	ReleaseShaderVariables();

	if (m_pLights) delete[] m_pLights;
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[1];
	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 0; // t0 
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[6];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; // b1 (Camera)
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 32;
	pd3dRootParameters[1].Constants.ShaderRegister = 2; // b2 (GameObject)
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; // b4 (Lights)
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = pd3dDescriptorRanges;
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[4].Constants.Num32BitValues = 16; // 4x4 행렬 (16 floats)
	pd3dRootParameters[4].Constants.ShaderRegister = 5;  // b5
	pd3dRootParameters[4].Constants.RegisterSpace = 0;
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	
	D3D12_DESCRIPTOR_RANGE shadowRange[1];
	shadowRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	shadowRange[0].NumDescriptors = 1;
	shadowRange[0].BaseShaderRegister = 4; // t4 
	shadowRange[0].RegisterSpace = 0;
	shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = shadowRange;
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC d3dStaticSamplerDescs[2];
	::ZeroMemory(&d3dStaticSamplerDescs, sizeof(D3D12_STATIC_SAMPLER_DESC)*2);
	d3dStaticSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dStaticSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dStaticSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dStaticSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dStaticSamplerDescs[0].MipLODBias = 0;
	d3dStaticSamplerDescs[0].MaxAnisotropy = 16;
	d3dStaticSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dStaticSamplerDescs[0].MinLOD = 0;
	d3dStaticSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dStaticSamplerDescs[0].ShaderRegister = 0; // s0
	d3dStaticSamplerDescs[0].RegisterSpace = 0;
	d3dStaticSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	///////////////////////////////////////////
	d3dStaticSamplerDescs[1] = d3dStaticSamplerDescs[0];
	d3dStaticSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dStaticSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dStaticSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dStaticSamplerDescs[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; 
	d3dStaticSamplerDescs[1].ShaderRegister = 1; // s1 

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;//


	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));

	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 2;
	d3dRootSignatureDesc.pStaticSamplers = d3dStaticSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256의 배수
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void **)&m_pcbMappedLights);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}
}

void CScene::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->ReleaseUploadBuffers();

	if (m_pWireframeBoxObject) m_pWireframeBoxObject->ReleaseUploadBuffers();
	if (m_pCombinedAABBBoxObject) m_pCombinedAABBBoxObject->ReleaseUploadBuffers();

	if (m_pTreeTextureUploadBuffer) m_pTreeTextureUploadBuffer->Release();
	m_pTreeTextureUploadBuffer = NULL;

	if (m_pFlowerTextureUploadBuffer) m_pFlowerTextureUploadBuffer->Release();
	m_pFlowerTextureUploadBuffer = NULL;

	if (m_pRockTextureUploadBuffer) m_pRockTextureUploadBuffer->Release();
	m_pRockTextureUploadBuffer = NULL;
}

void CScene::PickObject(XMFLOAT3& fWorldRayOrigin, XMFLOAT3& fWorldRayDirection)
{
	XMVECTOR vWorldRayOrigin = XMLoadFloat3(&fWorldRayOrigin);
	XMVECTOR vWorldRayDirection = XMLoadFloat3(&fWorldRayDirection);
	XMVECTOR vWorldRayTarget = vWorldRayOrigin + vWorldRayDirection * 10000.0f;

	CGameObject* pSelectedObject = NULL; 
	float fMinDistance = FLT_MAX;      

	for (int i = 0; i < m_nGameObjects; ++i)
	{
		if (!m_ppGameObjects[i]) continue;

		CGameObject* pClosestHit = m_ppGameObjects[i]->PickObject(vWorldRayOrigin, vWorldRayTarget, fMinDistance);

		if (pClosestHit)
		{
			pSelectedObject = pClosestHit;
		}
	}

	m_pSelectedObject = pSelectedObject; 
}


bool CScene::CheckCollision()
{
	if (!m_pPlayer) return false;

	BoundingBox localPlayerAABB = m_pPlayer->GetCombinedAABB();
	
	BoundingBox worldPlayerAABB;
	localPlayerAABB.Transform(worldPlayerAABB, XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix()));

	m_pCollidedObject = NULL; 

	for (int i = 1; i < m_nGameObjects; i++)
	{
		CGameObject* pObject = m_ppGameObjects[i];
		if (!pObject || !pObject->m_bIsActive) continue;

		BoundingBox localObjectAABB = pObject->GetCombinedAABB();

		if (localObjectAABB.Extents.x == 0 && localObjectAABB.Extents.y == 0 && localObjectAABB.Extents.z == 0) continue;

		BoundingBox worldObjectAABB;
		localObjectAABB.Transform(worldObjectAABB, XMLoadFloat4x4(&pObject->GetWorldMatrix()));

		if (worldPlayerAABB.Intersects(worldObjectAABB))
		{
			m_pCollidedObject = pObject;

			return true;
		}
	}
	return false;
}

bool CScene::CheckGroundCollision()
// 지면과 충돌 판정
{
	if (!m_pPlayer) return false;

	BoundingBox localPlayerAABB = m_pPlayer->GetCombinedAABB();

	BoundingBox worldPlayerAABB;
	localPlayerAABB.Transform(worldPlayerAABB, XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix()));
	
	CGameObject* pObject = m_ppGameObjects[0];
	
	BoundingBox localObjectAABB = pObject->GetCombinedAABB();

	BoundingBox worldObjectAABB;
	localObjectAABB.Transform(worldObjectAABB, XMLoadFloat4x4(&pObject->GetWorldMatrix()));

	if (worldPlayerAABB.Intersects(worldObjectAABB))
	{
		return true;
	}
	return false;
}

void CScene::LoadTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc = {};
	d3dDescriptorHeapDesc.NumDescriptors = 10;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dCbvSrvHeap);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandleStart = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuSrvHandleStart = m_pd3dCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

	m_nDescriptorIncrementSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	{

		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle_Slot0 = d3dCpuSrvHandleStart;
		m_d3dGpuTreeSrvHandle = d3dGpuSrvHandleStart;

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/Tree.dds", &m_pTreeTexture, ddsData, subresources);
		UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pTreeTexture, 0, (UINT)subresources.size());
		pd3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			__uuidof(ID3D12Resource), (void**)&m_pTreeTextureUploadBuffer);
		UpdateSubresources(pd3dCommandList, m_pTreeTexture, m_pTreeTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
		pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTreeTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = m_pTreeTexture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_pTreeTexture->GetDesc().MipLevels;
		pd3dDevice->CreateShaderResourceView(m_pTreeTexture, &srvDesc, d3dCpuSrvHandle_Slot0); 
	}

	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle_Slot1 = d3dCpuSrvHandleStart;
		d3dCpuSrvHandle_Slot1.ptr += (m_nDescriptorIncrementSize * 1);
		m_d3dGpuFlowerSrvHandle = d3dGpuSrvHandleStart;
		m_d3dGpuFlowerSrvHandle.ptr += (m_nDescriptorIncrementSize * 1);

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/Flower.dds", &m_pFlowerTexture, ddsData, subresources);
		UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pFlowerTexture, 0, (UINT)subresources.size());
		pd3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			__uuidof(ID3D12Resource), (void**)&m_pFlowerTextureUploadBuffer);
		UpdateSubresources(pd3dCommandList, m_pFlowerTexture, m_pFlowerTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
		pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pFlowerTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = m_pFlowerTexture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_pFlowerTexture->GetDesc().MipLevels;
		pd3dDevice->CreateShaderResourceView(m_pFlowerTexture, &srvDesc, d3dCpuSrvHandle_Slot1); 
	}

	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle_Slot3 = d3dCpuSrvHandleStart;
		d3dCpuSrvHandle_Slot3.ptr += (m_nDescriptorIncrementSize * 3);
		m_d3dGpuRockSrvHandle = d3dGpuSrvHandleStart;
		m_d3dGpuRockSrvHandle.ptr += (m_nDescriptorIncrementSize * 3);

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/Rock.dds", &m_pRockTexture, ddsData, subresources);
		UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pRockTexture, 0, (UINT)subresources.size());
		pd3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			__uuidof(ID3D12Resource), (void**)&m_pRockTextureUploadBuffer);
		UpdateSubresources(pd3dCommandList, m_pRockTexture, m_pRockTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
		pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRockTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = m_pRockTexture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_pRockTexture->GetDesc().MipLevels;
		pd3dDevice->CreateShaderResourceView(m_pRockTexture, &srvDesc, d3dCpuSrvHandle_Slot3); 
	}
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return(false);
}


bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'R':
			m_ppGameObjects[1]->MoveUp(10);
			break;
		case 'W':
			m_ppGameObjects[1]->MoveForward(10);
			break;
		case 'S':
			m_ppGameObjects[1]->MoveForward(-10);
			break;
		case 'A':
			m_ppGameObjects[1]->MoveStrafe(-10);
			break;
		case 'D':
			m_ppGameObjects[1]->MoveStrafe(10);
			break;
		case 'Z':
			m_ppGameObjects[1]->Rotate(0,10,0);
			break;
		case VK_F7:
			m_bShowWireframeBox = !m_bShowWireframeBox;
			break;
		case VK_F8:
			m_bShowCombinedAABB = !m_bShowCombinedAABB;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return(false);
}

bool CScene::ProcessInput(UCHAR *pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;

	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->Animate(fTimeElapsed, NULL);

	if (m_pLights)
	{
		m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
		m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
	}

}

void CScene::RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	if (m_pd3dGraphicsRootSignature)
		pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 8192.0f, 8192.0f, 0.0f, 1.0f };
	D3D12_RECT scissorRect = { 0, 0, 8192, 8192 };
	pd3dCommandList->RSSetViewports(1, &viewport);
	pd3dCommandList->RSSetScissorRects(1, &scissorRect);

	pd3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	pd3dCommandList->OMSetRenderTargets(0, NULL, FALSE, &dsvHandle);

	if (m_pShadowShader) m_pShadowShader->Render(pd3dCommandList, NULL);

	XMMATRIX mLightViewProj = GetShadowLightViewProj();
	XMFLOAT4X4 xmf4x4LightViewProj;
	XMStoreFloat4x4(&xmf4x4LightViewProj, XMMatrixTranspose(mLightViewProj));
	pd3dCommandList->SetGraphicsRoot32BitConstants(4, 16, &xmf4x4LightViewProj, 0);

	for (int i = 0; i < m_nGameObjects; ++i)
	{
		if (i == 0) continue;

		CGameObject* pObj = m_ppGameObjects[i];
		if (!pObj || !pObj->m_bIsActive) continue;

		bool bIsBillboard = false;
		if (pObj->m_nMaterials > 0 && pObj->m_ppMaterials && pObj->m_ppMaterials[0])
		{
			if (pObj->m_ppMaterials[0]->m_pShader == CMaterial::m_pBillboardShader)
				bIsBillboard = true;
		}
		if (bIsBillboard) continue;

		pObj->Render(pd3dCommandList, NULL, NULL);
	}

	if (m_pPlayer) m_pPlayer->Render(pd3dCommandList, NULL, NULL);
}

XMMATRIX CScene::GetShadowLightViewProj()
{
	XMFLOAT3 lightPos = m_pLights[2].m_xmf3Position;
	XMFLOAT3 lightDir = m_pLights[2].m_xmf3Direction;


	if (m_pPlayer)
	{
		XMFLOAT3 playerPos = m_pPlayer->GetPosition();

		XMVECTOR vLightDir = XMLoadFloat3(&lightDir);
		XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);

		XMVECTOR vLightPos = vPlayerPos - (vLightDir * 6000.0f);
		XMStoreFloat3(&lightPos, vLightPos);
	}

	XMVECTOR vEye = XMLoadFloat3(&lightPos);
	XMVECTOR vAt = XMLoadFloat3(&lightPos) + XMLoadFloat3(&lightDir);
	XMVECTOR vUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX mLightView = XMMatrixLookAtLH(vEye, vAt, vUp);

	XMMATRIX mLightProj = XMMatrixOrthographicLH(8000.0f, 8000.0f, 1.0f, 18000.f);

	return mLightView * mLightProj;
}

void RenderReflectedObject(ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pObject, XMMATRIX matReflect)
{
	if (!pObject || !pObject->m_bIsActive) return;

	bool bIsBillboard = false;
	if (pObject->m_nMaterials > 0 && pObject->m_ppMaterials && pObject->m_ppMaterials[0] &&
		pObject->m_ppMaterials[0]->m_pShader == CMaterial::m_pBillboardShader)
	{
		bIsBillboard = true;
	}

	if (!bIsBillboard)
	{
		XMFLOAT4X4 originalWorld = pObject->m_xmf4x4World;
		XMMATRIX world = XMLoadFloat4x4(&originalWorld);

		XMMATRIX reflectedWorld = world * matReflect;
		XMFLOAT4X4 xmfReflectedWorld;
		XMStoreFloat4x4(&xmfReflectedWorld, reflectedWorld);

		pObject->UpdateShaderVariable(pd3dCommandList, &xmfReflectedWorld);

		if (pObject->m_nMaterials > 0 && pObject->m_ppMaterials && pObject->m_ppMaterials[0])
		{
			pObject->m_ppMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		}

		if (pObject->m_pMesh)
		{
			pObject->m_pMesh->Render(pd3dCommandList, 0);
		}
	}

	if (pObject->m_pSibling) RenderReflectedObject(pd3dCommandList, pObject->m_pSibling, matReflect);
	if (pObject->m_pChild) RenderReflectedObject(pd3dCommandList, pObject->m_pChild, matReflect);
}


void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvHeap) {
		ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dCbvSrvHeap };
		pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	}
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);
	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress);

	if (m_pd3dCbvSrvHeap)
	{
		pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvHeap);

		D3D12_GPU_DESCRIPTOR_HANDLE d3dShadowMapHandle = m_pd3dCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

		d3dShadowMapHandle.ptr += (m_nDescriptorIncrementSize * 4);

		pd3dCommandList->SetGraphicsRootDescriptorTable(5, d3dShadowMapHandle);

	}

	XMMATRIX mLightViewProj = GetShadowLightViewProj();

	XMFLOAT4X4 xmf4x4LightViewProj;
	XMStoreFloat4x4(&xmf4x4LightViewProj, XMMatrixTranspose(mLightViewProj));

	pd3dCommandList->SetGraphicsRoot32BitConstants(4, 16, &xmf4x4LightViewProj, 0);

	CGameObject* pDebugBoxToRender = NULL;
	if (m_bShowWireframeBox && m_pWireframeBoxObject) pDebugBoxToRender = m_pWireframeBoxObject;

	for (int i = 0; i < m_nGameObjects; i++)
	{

		if (m_ppGameObjects[i] && m_ppGameObjects[i] != m_pMirrorObject)
		{
			m_ppGameObjects[i]->Animate(m_fElapsedTime, NULL);
			m_ppGameObjects[i]->UpdateTransform(NULL);
			m_ppGameObjects[i]->Render(pd3dCommandList, pDebugBoxToRender, pCamera);
		}
	}

	if (m_pReflectedShader) m_pReflectedShader->UpdateShaderVariables(pd3dCommandList);

	if (m_pMirrorObject && m_pMirrorObject->m_bIsActive)
	{
		m_pMirrorObject->UpdateTransform(NULL);

		pd3dCommandList->OMSetStencilRef(1);

		if (m_pMirrorObject->m_ppMaterials[0] && m_pMirrorObject->m_ppMaterials[0]->m_pShader)
		{
			m_pMirrorObject->m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera, 0);

			m_pMirrorObject->UpdateShaderVariable(pd3dCommandList, &m_pMirrorObject->m_xmf4x4World);
			m_pMirrorObject->m_ppMaterials[0]->UpdateShaderVariable(pd3dCommandList);

			if (m_pMirrorObject->m_pMesh)
				m_pMirrorObject->m_pMesh->Render(pd3dCommandList, 0);
		}

		XMFLOAT3 mirrorPos = m_pMirrorObject->GetPosition();
		XMFLOAT3 mirrorNormal = m_pMirrorObject->GetUp();
		XMVECTOR vMirrorPlane = XMPlaneFromPointNormal(XMLoadFloat3(&mirrorPos), XMLoadFloat3(&mirrorNormal));
		XMMATRIX matReflect = XMMatrixReflect(vMirrorPlane);

		if (m_pReflectedShader)
		{
			m_pReflectedShader->Render(pd3dCommandList, pCamera, 0);

			for (int i = 0; i < m_nGameObjects; i++)
			{
				if (!m_ppGameObjects[i] || m_ppGameObjects[i] == m_pMirrorObject) continue;

				if (0 == i) continue;

				RenderReflectedObject(pd3dCommandList, m_ppGameObjects[i], matReflect);
			}

			if (m_pPlayer)
			{
				RenderReflectedObject(pd3dCommandList, m_pPlayer, matReflect);
			}
		}
		if (m_pMirrorObject->m_ppMaterials[0] && m_pMirrorObject->m_ppMaterials[0]->m_pShader)
		{
			m_pMirrorObject->m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera, 1);

			m_pMirrorObject->UpdateShaderVariable(pd3dCommandList, &m_pMirrorObject->m_xmf4x4World);
			m_pMirrorObject->m_ppMaterials[0]->UpdateShaderVariable(pd3dCommandList);

			if (m_pMirrorObject->m_pMesh)
				m_pMirrorObject->m_pMesh->Render(pd3dCommandList, 0);
		}

		pd3dCommandList->OMSetStencilRef(0);
	}

	if (m_bShowCombinedAABB && m_pCombinedAABBBoxObject)
	{
		if (m_pCombinedAABBBoxObject->m_ppMaterials[0])
		{
			if (m_pCombinedAABBBoxObject->m_ppMaterials[0]->m_pShader)
				m_pCombinedAABBBoxObject->m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera);

			m_pCombinedAABBBoxObject->m_ppMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		}

		if (m_pPlayer)
		{
			BoundingBox localAABB = m_pPlayer->GetCombinedAABB();
			XMMATRIX matWorld = XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix());

			XMMATRIX matScale = XMMatrixScaling(localAABB.Extents.x * 2.0f, localAABB.Extents.y * 2.0f, localAABB.Extents.z * 2.0f);
			XMMATRIX matTranslate = XMMatrixTranslation(localAABB.Center.x, localAABB.Center.y, localAABB.Center.z);
			XMMATRIX matFinalWorld = matScale * matTranslate * matWorld;

			XMStoreFloat4x4(&m_pCombinedAABBBoxObject->m_xmf4x4World, matFinalWorld);
			m_pCombinedAABBBoxObject->UpdateShaderVariable(pd3dCommandList, &m_pCombinedAABBBoxObject->m_xmf4x4World);
			if (m_pCombinedAABBBoxObject->m_pMesh) m_pCombinedAABBBoxObject->m_pMesh->Render(pd3dCommandList);
		}

		for (int i = 0; i < m_nGameObjects; i++)
		{
			if (m_ppGameObjects[i])
			{
				BoundingBox localAABB = m_ppGameObjects[i]->GetCombinedAABB();
				XMMATRIX matWorld = XMLoadFloat4x4(&m_ppGameObjects[i]->GetWorldMatrix());

				XMMATRIX matScale = XMMatrixScaling(localAABB.Extents.x * 2.0f, localAABB.Extents.y * 2.0f, localAABB.Extents.z * 2.0f);
				XMMATRIX matTranslate = XMMatrixTranslation(localAABB.Center.x, localAABB.Center.y, localAABB.Center.z);
				XMMATRIX matFinalWorld = matScale * matTranslate * matWorld;

				XMStoreFloat4x4(&m_pCombinedAABBBoxObject->m_xmf4x4World, matFinalWorld);
				m_pCombinedAABBBoxObject->UpdateShaderVariable(pd3dCommandList, &m_pCombinedAABBBoxObject->m_xmf4x4World);
				if (m_pCombinedAABBBoxObject->m_pMesh) m_pCombinedAABBBoxObject->m_pMesh->Render(pd3dCommandList);
			}
		}
	}
}

