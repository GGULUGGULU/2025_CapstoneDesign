//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Mesh.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
CMeshLoadInfo::~CMeshLoadInfo()
{
	if (m_pxmf3Positions) delete[] m_pxmf3Positions;
	if (m_pxmf4Colors) delete[] m_pxmf4Colors;
	if (m_pxmf3Normals) delete[] m_pxmf3Normals;

	if (m_pnIndices) delete[] m_pnIndices;
	
	if (m_pnSubSetIndices) delete[] m_pnSubSetIndices;

	for (int i = 0; i < m_nSubMeshes; i++) if (m_ppnSubSetIndices[i]) delete[] m_ppnSubSetIndices[i];
	if (m_ppnSubSetIndices) delete[] m_ppnSubSetIndices;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
CMeshFromFile::CMeshFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CMeshLoadInfo* pMeshInfo)
{
	m_nVertices = pMeshInfo->m_nVertices;
	m_nType = pMeshInfo->m_nType;

	if (pMeshInfo->m_nVertices > 0 && pMeshInfo->m_pxmf3Positions)
	{
		m_vPositions.assign(pMeshInfo->m_pxmf3Positions, pMeshInfo->m_pxmf3Positions + pMeshInfo->m_nVertices);
	}

	if (!m_vPositions.empty())
	{
		BoundingBox::CreateFromPoints(m_xmBoundingBox, m_vPositions.size(), m_vPositions.data(), sizeof(XMFLOAT3));
	}

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMeshInfo->m_pxmf3Positions, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;

	m_nSubMeshes = pMeshInfo->m_nSubMeshes;
	if (m_nSubMeshes > 0)
	{
		m_ppd3dSubSetIndexBuffers = new ID3D12Resource * [m_nSubMeshes];
		m_ppd3dSubSetIndexUploadBuffers = new ID3D12Resource * [m_nSubMeshes];
		m_pd3dSubSetIndexBufferViews = new D3D12_INDEX_BUFFER_VIEW[m_nSubMeshes];

		m_pnSubSetIndices = new int[m_nSubMeshes];

		for (int i = 0; i < m_nSubMeshes; i++)
		{
			m_ppd3dSubSetIndexBuffers[i] = NULL;
			m_ppd3dSubSetIndexUploadBuffers[i] = NULL;
			m_pd3dSubSetIndexBufferViews[i] = {};
		}

		for (int i = 0; i < m_nSubMeshes; i++)
		{

			m_pnSubSetIndices[i] = pMeshInfo->m_pnSubSetIndices[i];
			if (pMeshInfo->m_pnSubSetIndices[i] > 0 && pMeshInfo->m_ppnSubSetIndices[i])
			{
				m_vIndices.insert(m_vIndices.end(), pMeshInfo->m_ppnSubSetIndices[i], pMeshInfo->m_ppnSubSetIndices[i] + pMeshInfo->m_pnSubSetIndices[i]);
			}

			if (m_pnSubSetIndices[i] > 0)
			{
				m_ppd3dSubSetIndexBuffers[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMeshInfo->m_ppnSubSetIndices[i], sizeof(UINT) * m_pnSubSetIndices[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_ppd3dSubSetIndexUploadBuffers[i]);
				if (m_ppd3dSubSetIndexBuffers[i])
				{
					m_pd3dSubSetIndexBufferViews[i].BufferLocation = m_ppd3dSubSetIndexBuffers[i]->GetGPUVirtualAddress();
					m_pd3dSubSetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
					m_pd3dSubSetIndexBufferViews[i].SizeInBytes = sizeof(UINT) * pMeshInfo->m_pnSubSetIndices[i];
				}
				else
				{
					m_pnSubSetIndices[i] = 0; 
					m_pd3dSubSetIndexBufferViews[i] = {};
				}
			}
			else
			{
				m_ppd3dSubSetIndexBuffers[i] = NULL; 
				m_pd3dSubSetIndexBufferViews[i] = {}; 
			}
		}
	}
}

CMeshFromFile::~CMeshFromFile()
{
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();

	if (m_nSubMeshes > 0)
	{
		for (int i = 0; i < m_nSubMeshes; i++)
		{
			if (m_ppd3dSubSetIndexBuffers[i]) m_ppd3dSubSetIndexBuffers[i]->Release();
		}
		if (m_ppd3dSubSetIndexBuffers) delete[] m_ppd3dSubSetIndexBuffers;
		if (m_pd3dSubSetIndexBufferViews) delete[] m_pd3dSubSetIndexBufferViews;

		if (m_pnSubSetIndices) delete[] m_pnSubSetIndices;
	}
}

void CMeshFromFile::ReleaseUploadBuffers()
{
	CMesh::ReleaseUploadBuffers();

	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	m_pd3dPositionUploadBuffer = NULL;

	if ((m_nSubMeshes > 0) && m_ppd3dSubSetIndexUploadBuffers)
	{
		for (int i = 0; i < m_nSubMeshes; i++)
		{
			if (m_ppd3dSubSetIndexUploadBuffers[i]) m_ppd3dSubSetIndexUploadBuffers[i]->Release();
		}
		if (m_ppd3dSubSetIndexUploadBuffers) delete[] m_ppd3dSubSetIndexUploadBuffers;
		m_ppd3dSubSetIndexUploadBuffers = NULL;
	}
}

void CMeshFromFile::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dPositionBufferView);
	if ((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_pnSubSetIndices[nSubSet], 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

int CMeshFromFile::CheckRayIntersection(XMFLOAT3& xmRayPosition, XMFLOAT3& xmRayDirection, float* pfNearHitDistance)
{
	XMVECTOR rayOrigin = XMLoadFloat3(&xmRayPosition);
	XMVECTOR rayDir = XMLoadFloat3(&xmRayDirection);

	float fHitDistance;
	if (!m_xmBoundingBox.Intersects(rayOrigin, rayDir, fHitDistance))
	{
		return false; 
	}

	int nIntersections = 0;
	float fMinHitDistance = FLT_MAX;

	for (size_t i = 0; i < m_vIndices.size(); i += 3)
	{
		UINT i0 = m_vIndices[i];
		UINT i1 = m_vIndices[i + 1];
		UINT i2 = m_vIndices[i + 2];

		XMVECTOR v0 = XMLoadFloat3(&m_vPositions[i0]);
		XMVECTOR v1 = XMLoadFloat3(&m_vPositions[i1]);
		XMVECTOR v2 = XMLoadFloat3(&m_vPositions[i2]);

		if (DirectX::TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, fHitDistance))
		{
			if (fHitDistance < fMinHitDistance)
			{
				fMinHitDistance = fHitDistance;
			}
			nIntersections++;
		}
	}

	if (nIntersections > 0)
	{
		*pfNearHitDistance = fMinHitDistance; 
		return true; 
	}

	return false; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
CMeshIlluminatedFromFile::CMeshIlluminatedFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CMeshLoadInfo *pMeshInfo) : CMeshFromFile::CMeshFromFile(pd3dDevice, pd3dCommandList, pMeshInfo)
{
	m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMeshInfo->m_pxmf3Normals, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);

	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
}

CMeshIlluminatedFromFile::~CMeshIlluminatedFromFile()
{
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
}

void CMeshIlluminatedFromFile::ReleaseUploadBuffers()
{
	CMeshFromFile::ReleaseUploadBuffers();

	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;
}

void CMeshIlluminatedFromFile::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[2] = { m_d3dPositionBufferView, m_d3dNormalBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 2, pVertexBufferViews);
	if ((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_pnSubSetIndices[nSubSet], 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

////////////////////////////////////////////////////
CBillboardVertex::CBillboardVertex(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT m_nStride = sizeof(BillboardVertexData);
	m_nVertices = 100;

	BillboardVertexData* pBillboardVertices = new BillboardVertexData[m_nVertices];

	XMFLOAT3 xmf3Position; 
	for (int i = 0; i < 100; ++i) {
		float xPos = (rand() % 500) - 250.0f;
		float zPos = (rand() % 500) - 250.0f;

		pBillboardVertices[i].m_xmf3Position = XMFLOAT3(xPos, 30.0f, zPos);
		pBillboardVertices[i].m_xmf2Size = XMFLOAT2(50, 50);
	}

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pBillboardVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);

	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	delete[] pBillboardVertices;
}

CBillboardVertex::CBillboardVertex(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int xSize, int ySize)
{
	UINT m_nStride = sizeof(BillboardVertexData);
	m_nVertices = 100;

	BillboardVertexData* pBillboardVertices = new BillboardVertexData[m_nVertices];

	XMFLOAT3 xmf3Position; 
	for (int i = 0; i < 100; ++i) {
		float xPos = (rand() % 500) - 250.0f;
		float zPos = (rand() % 500) - 250.0f;

		pBillboardVertices[i].m_xmf3Position = XMFLOAT3(xPos, 30.0f, zPos);
		pBillboardVertices[i].m_xmf2Size = XMFLOAT2(xSize, ySize);
	}

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pBillboardVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);

	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	delete[] pBillboardVertices;
}

void CBillboardVertex::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);

	pd3dCommandList->DrawInstanced(m_nVertices, 1, 0, 0);
}

void CBillboardVertex::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet)
{
	Render(pd3dCommandList);
}