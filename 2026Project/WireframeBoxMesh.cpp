#include "stdafx.h"
#include "WireframeBoxMesh.h"

CWireframeBoxMesh::CWireframeBoxMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
    XMFLOAT3 pxmf3Positions[8] =
    {
        XMFLOAT3(-0.5f, -0.5f, -0.5f),
        XMFLOAT3(-0.5f, -0.5f, +0.5f),
        XMFLOAT3(+0.5f, -0.5f, +0.5f),
        XMFLOAT3(+0.5f, -0.5f, -0.5f),
        XMFLOAT3(-0.5f, +0.5f, -0.5f),
        XMFLOAT3(-0.5f, +0.5f, +0.5f),
        XMFLOAT3(+0.5f, +0.5f, +0.5f),
        XMFLOAT3(+0.5f, +0.5f, -0.5f)
    };

    UINT pnIndices[24] = {
        0, 1, 1, 2, 2, 3, 3, 0, 
        4, 5, 5, 6, 6, 7, 7, 4, 
        0, 4, 1, 5, 2, 6, 3, 7  
    };

    m_nVertices = 8;
    m_nIndices = 24;
    m_nSlot = 0;
    m_nOffset = 0;

    m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

    m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pxmf3Positions,
        sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);

    m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
    m_d3dVertexBufferView.StrideInBytes = sizeof(XMFLOAT3);
    m_d3dVertexBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;

    m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pnIndices,
        sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);

    m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
    m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CWireframeBoxMesh::~CWireframeBoxMesh()
{
    if (m_pd3dVertexBuffer) m_pd3dVertexBuffer->Release();
    if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CWireframeBoxMesh::ReleaseUploadBuffers()
{
    if (m_pd3dVertexUploadBuffer) m_pd3dVertexUploadBuffer->Release();
    if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
    m_pd3dVertexUploadBuffer = NULL;
    m_pd3dIndexUploadBuffer = NULL;
}

void CWireframeBoxMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
    pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
    pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dVertexBufferView);
    pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
    pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}