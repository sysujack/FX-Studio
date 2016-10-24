#include "Skybox.h"
#include "Model.h"
#include "Mesh.h"
#include "Scene.h"
#include "../Utilities/UtilityString.h"
#include <DDSTextureLoader.h>

SkyboxMaterial::SkyboxMaterial()
	: Material("main11"),
	m_pWorldViewProjection(nullptr),
	m_pSkyboxTexutre(nullptr)
{

}

SkyboxMaterial::~SkyboxMaterial()
{

}

void SkyboxMaterial::Initialize(Effect* pEffect)
{
	Material::Initialize(pEffect);

	m_pWorldViewProjection = pEffect->GetVariablesByName().at("WorldViewProjection");
	m_pSkyboxTexutre = pEffect->GetVariablesByName().at("SkyboxTexture");

	D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	CreateInputLayout("main11", "p0", inputElementDescriptions, ARRAYSIZE(inputElementDescriptions));
}

void SkyboxMaterial::CreateVertexBuffer(const Mesh* pMesh, ID3D11Buffer** ppVertexBuffer) const
{
	GCC_ASSERT(pMesh != nullptr);
	const std::vector<Vector3>& sourceVertices = pMesh->GetVertices();

	std::vector<XMFLOAT4> vertices;
	vertices.reserve(sourceVertices.size());
	for (UINT i = 0; i < sourceVertices.size(); i++)
	{
		XMFLOAT3 position = sourceVertices.at(i);
		vertices.push_back(XMFLOAT4(position.x, position.y, position.z, 1.0f));
	}

	CreateVertexBuffer(&vertices[0], vertices.size(), ppVertexBuffer);
}

void SkyboxMaterial::CreateVertexBuffer(XMFLOAT4* vertices, uint32_t vertexCount, ID3D11Buffer** ppVertexBuffer) const
{
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.ByteWidth = VertexSize() * vertexCount;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubResourceData;
	ZeroMemory(&vertexSubResourceData, sizeof(vertexSubResourceData));
	vertexSubResourceData.pSysMem = vertices;
	if (FAILED(DXUTGetD3D11Device()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, ppVertexBuffer)))
	{
		GCC_ERROR("ID3D11Device::CreateBuffer() failed.");
	}
}

UINT SkyboxMaterial::VertexSize() const
{
	return sizeof(XMFLOAT4);
}

SkyboxNode::SkyboxNode(const std::wstring& textureFile)
	: SceneNode(INVALID_ACTOR_ID, WeakBaseRenderComponentPtr(), RenderPass_Sky),
	m_pEffect(nullptr),
	m_pMaterial(nullptr),
	m_pCubeMapShaderResourceView(nullptr),
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_WorldMatrix(Matrix::Identity),
	m_ScaleMatrix(Matrix::Identity),
	m_TextureFile(textureFile),
	m_IndexCount(0),
	m_IsActive(true)
{
	m_ScaleMatrix = Matrix::CreateScale(10);
}

SkyboxNode::~SkyboxNode()
{
	Reset();
}

HRESULT SkyboxNode::VPreRender(Scene* pScene)
{
	return SceneNode::VPreRender(pScene);
}

bool SkyboxNode::VIsVisible(Scene* pScene)
{
	return m_IsActive;
}

HRESULT SkyboxNode::VOnRestore(Scene* pScene)
{
	Reset();

	SetCurrentDirectory(Utility::ExecutableDirectory().c_str());
	std::unique_ptr<Model> model(new Model("Assets\\Models\\Sphere.obj", true));

	m_pEffect = new Effect();
	m_pEffect->LoadCompiledEffect(L"Assets\\Effects\\Skybox.cso");

	m_pMaterial = new SkyboxMaterial();
	m_pMaterial->Initialize(m_pEffect);

	Mesh* pMesh = model->GetMeshes().at(0);
	m_pMaterial->CreateVertexBuffer(pMesh, &m_pVertexBuffer);
	pMesh->CreateIndexBuffer(&m_pIndexBuffer);
	m_IndexCount = pMesh->GetIndices().size();

	HRESULT hr = DirectX::CreateDDSTextureFromFile(DXUTGetD3D11Device(), m_TextureFile.c_str(), nullptr, &m_pCubeMapShaderResourceView);
	if (FAILED(hr))
	{
		GCC_ERROR("CreateDDSTextureFromFile() failed.");
	}

	return hr;
}

HRESULT SkyboxNode::VRender(Scene* pScene, double fTime, float fElapsedTime)
{
	ID3D11DeviceContext* direct3DDeviceContext = DXUTGetD3D11DeviceContext();
	direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Pass* pass = m_pMaterial->GetCurrentTechnique()->GetPasses().at(0);
	ID3D11InputLayout* inputLayout = m_pMaterial->GetInputLayouts().at(pass);
	direct3DDeviceContext->IASetInputLayout(inputLayout);

	UINT stride = m_pMaterial->VertexSize();
	UINT offset = 0;
	direct3DDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	direct3DDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX wvp = XMMATRIX(m_WorldMatrix * pScene->GetCamera()->GetViewMatrix() * pScene->GetCamera()->GetProjectMatrix());
	m_pMaterial->WorldViewProjection() << wvp;
	m_pMaterial->SkyboxTexture() << m_pCubeMapShaderResourceView;

	pass->Apply(0);

	direct3DDeviceContext->DrawIndexed(m_IndexCount, 0, 0);

	return S_OK;
}


HRESULT SkyboxNode::VOnUpdate(Scene* pScene, double fTime, float fElapsedTime)
{
	m_WorldMatrix = m_ScaleMatrix * Matrix::CreateTranslation(pScene->GetCamera()->GetPostion());
	return S_OK;
}

void SkyboxNode::Reset()
{
	SAFE_RELEASE(m_pCubeMapShaderResourceView);
	SAFE_DELETE(m_pMaterial);
	SAFE_DELETE(m_pEffect);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}