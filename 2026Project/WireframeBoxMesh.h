#pragma once
#include "Mesh.h"

class CWireframeBoxMesh : public CMesh
{
public:
    CWireframeBoxMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
    virtual ~CWireframeBoxMesh();

public:
    virtual void ReleaseUploadBuffers();

protected:
    ID3D12Resource* m_pd3dVertexBuffer = NULL;
    ID3D12Resource* m_pd3dVertexUploadBuffer = NULL;
    D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView;

    ID3D12Resource* m_pd3dIndexBuffer = NULL;
    ID3D12Resource* m_pd3dIndexUploadBuffer = NULL;
    D3D12_INDEX_BUFFER_VIEW m_d3dIndexBufferView;

    UINT m_nIndices = 0;

public:
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);
};

