#include "DebugGizmosNode.h"
#include "CameraNode.h"
#include "Scene.h"
#include "Material.h"
#include "../Actors/Actor.h"
#include "../Actors/RenderComponent.h"
#include "../Actors/TransformComponent.h"
#include "../ResourceCache/ResCache.h"
#include "../ResourceCache/TextureResource.h"
#include "../ResourceCache/ShaderResource.h"
#include "../AppFramework/BaseGameApp.h"
#include "../AppFramework/BaseGameLogic.h"

DebugGizmosNode::DebugGizmosNode()
	: SceneNode(INVALID_ACTOR_ID, nullptr, RenderPass_Debug, Matrix::CreateTranslation(0.0f, 0.5f, 0.0f)),
	m_pEffect(nullptr),
	m_pCurrentPass(nullptr),
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_Transform(TT_None),
	m_MousePos(),
	m_PickedTransform(PT_None),
	m_IsVisible(false)
{

}

DebugGizmosNode::~DebugGizmosNode()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}

HRESULT DebugGizmosNode::VOnInitSceneNode(Scene* pScene)
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	Resource effectRes("Effects\\DebugAssist.fx");
	shared_ptr<ResHandle> pEffectResHandle = g_pApp->GetResCache()->GetHandle(&effectRes);
	if (pEffectResHandle == nullptr)
	{
		return S_FALSE;
	}
	shared_ptr<HlslResourceExtraData> extra = static_pointer_cast<HlslResourceExtraData>(pEffectResHandle->GetExtraData());
	if (extra == nullptr)
	{
		return S_FALSE;
	}

	m_pEffect = extra->GetEffect();

	Technique* pCurrentTechnique = m_pEffect->GetTechniquesByName().at("main11");
	if (pCurrentTechnique == nullptr)
	{
		DEBUG_ERROR(std::string("technique is not exist: ") + "main11");
		return S_FALSE;
	}
	m_pCurrentPass = pCurrentTechnique->GetPassesByName().at("p0");
	if (m_pCurrentPass == nullptr)
	{
		DEBUG_ERROR(std::string("technique is not exist: ") + "p0");
		return S_FALSE;
	}

	CreateGeometryBuffers();

	return S_OK;
}

HRESULT DebugGizmosNode::VOnDeleteSceneNode(Scene *pScene)
{
	return S_OK;
}

HRESULT DebugGizmosNode::VOnUpdate(Scene* pScene, const GameTime& gameTime)
{
	return S_OK;
}

HRESULT DebugGizmosNode::VRender(Scene* pScene, const GameTime& gameTime)
{
	shared_ptr<ISceneNode> pPickedNode = pScene->FindActor(pScene->GetPickedActor());
	if (pPickedNode == nullptr)
	{
		return S_OK;
	}

	uint32_t stride = m_pCurrentPass->GetVertexSize();
	uint32_t offset = 0;
	pScene->GetRenderder()->VSetVertexBuffers(m_pVertexBuffer, &stride, &offset);
	pScene->GetRenderder()->VSetIndexBuffer(m_pIndexBuffer, IRenderer::Format_uint16, 0);

	pScene->GetRenderder()->VInputSetup(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, m_pCurrentPass->GetInputLayout());
	RenderBoundingBox(pScene, pPickedNode->VGet()->GetBoundingBox(), pPickedNode->VGet()->GetWorldMatrix());

	shared_ptr<IRenderState> debugPass = pScene->GetRenderder()->VPrepareDebugPass();

	pScene->GetRenderder()->VInputSetup(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_pCurrentPass->GetInputLayout());
	switch (m_Transform)
	{
	case DebugGizmosNode::TT_None:
		break;
	case DebugGizmosNode::TT_Translation:
		RenderTranslateAxes(pScene, pPickedNode->VGet()->GetBoundingBox(), pPickedNode->VGet()->GetWorldMatrix());
		break;
	case DebugGizmosNode::TT_Rotation:
		RenderRotateRings(pScene, pPickedNode->VGet()->GetBoundingBox(), pPickedNode->VGet()->GetWorldMatrix());
		break;
	case DebugGizmosNode::TT_Scale:
		RenderScaleAxes(pScene, pPickedNode->VGet()->GetBoundingBox(), pPickedNode->VGet()->GetWorldMatrix());
		break;
	default:
		break;
	}

	return S_OK;
}

bool DebugGizmosNode::VIsVisible(Scene* pScene) const
{
	return pScene->GetPickedActor() != INVALID_ACTOR_ID;
}

bool DebugGizmosNode::IsXAxisPicked(Scene* pScene, const Matrix& world)
{
	const Matrix& projectMat = pScene->GetCamera()->GetProjectMatrix();
	float viewX = (2.0f * m_MousePos.x / g_pApp->GetGameConfig().m_ScreenWidth - 1.0f) / projectMat.m[0][0];
	float viewY = (1.0f - 2.0f * m_MousePos.y / g_pApp->GetGameConfig().m_ScreenHeight) / projectMat.m[1][1];

	Matrix toLocal = (world * pScene->GetCamera()->GetViewMatrix()).Invert();
	Vector3 rayPostition = toLocal.Translation();
	//use right-hand coordinates, z should be -1
	Vector3 rayDir = Vector3::TransformNormal(Vector3(viewX, viewY, -1.0f), toLocal);
	rayDir.Normalize();

	Ray ray(rayPostition, rayDir);

	float distance = 0.0f;
	return ray.Intersects(m_Properties.GetBoundingBox(), distance);
}

HRESULT DebugGizmosNode::RenderBoundingBox(Scene* pScene, const BoundingBox& aaBox, const Matrix& world)
{
	Variable* variable = m_pEffect->GetVariablesByName().at("WorldViewProjection");
	Matrix boxWorld = world * Matrix::CreateScale(aaBox.Extents * 2.0f);
	boxWorld.Translation(world.Translation());
	boxWorld = boxWorld * Matrix::CreateTranslation(aaBox.Center);
	const XMMATRIX& wvp = boxWorld * pScene->GetCamera()->GetViewMatrix() * pScene->GetCamera()->GetProjectMatrix();
	variable->SetMatrix(wvp);

	Variable* ambientColor = m_pEffect->GetVariablesByName().at("AmbientColor");
	ambientColor->SetVector(Color(Colors::LightSkyBlue.f));

	pScene->GetRenderder()->VDrawMesh(m_AABoxIndexCount, m_AABoxIndexOffset, m_AABoxVertexOffset, m_pCurrentPass->GetEffectPass());

	return S_OK;
}

HRESULT DebugGizmosNode::RenderTranslateAxes(Scene* pScene, const BoundingBox& aaBox, const Matrix& world)
{
	Variable* objectWvp = m_pEffect->GetVariablesByName().at("WorldViewProjection");
	Variable* ambientColor = m_pEffect->GetVariablesByName().at("AmbientColor");

	Vector3 cameraPos = pScene->GetCamera()->GetPosition();
	Matrix aixsWorld = world * Matrix::CreateScale(Vector3::Distance(cameraPos, world.Translation()) * 0.2f);
	aixsWorld.Translation(world.Translation());
	aixsWorld = aixsWorld * Matrix::CreateTranslation(aaBox.Center);
	const XMMATRIX& wvp = aixsWorld * pScene->GetCamera()->GetViewMatrix() * pScene->GetCamera()->GetProjectMatrix();

	bool isPicked = false;
	if (IsXAxisPicked(pScene,
		Matrix::CreateRotationZ(XMConvertToRadians(-90)) * Matrix::CreateTranslation(Vector3(0.3f, 0.0f, 0.0f)) * aixsWorld))
	{
		ambientColor->SetVector(Color(1.0f, 1.0f, 1.0f));
		isPicked = true;
		m_PickedTransform = PT_TranslateX;
	}
	else
		ambientColor->SetVector(Color(1.0f, 0.0f, 0.0f));

	objectWvp->SetMatrix(Matrix::CreateRotationZ(XMConvertToRadians(-90)) * Matrix::CreateTranslation(Vector3(0.3f, 0.0f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset, m_pCurrentPass->GetEffectPass());
	objectWvp->SetMatrix(Matrix::CreateRotationZ(XMConvertToRadians(-90)) * Matrix::CreateTranslation(Vector3(0.6f, 0.0f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_ConeIndexCount, m_ConeIndexOffset, m_ConeVertexOffset, m_pCurrentPass->GetEffectPass());

	if (!isPicked && IsXAxisPicked(pScene,
		Matrix::CreateTranslation(Vector3(0.0f, 0.3f, 0.0f)) * aixsWorld))
	{
		ambientColor->SetVector(Color(1.0f, 1.0f, 1.0f));
		isPicked = true;
		m_PickedTransform = PT_TranslateY;
	}
	else
		ambientColor->SetVector(Color(0.0f, 1.0f, 0.0f));

	objectWvp->SetMatrix(Matrix::CreateTranslation(Vector3(0.0f, 0.3f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset, m_pCurrentPass->GetEffectPass());
	objectWvp->SetMatrix(Matrix::CreateTranslation(Vector3(0.0f, 0.6f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_ConeIndexCount, m_ConeIndexOffset, m_ConeVertexOffset, m_pCurrentPass->GetEffectPass());

	if (!isPicked && IsXAxisPicked(pScene,
		Matrix::CreateRotationX(XMConvertToRadians(90)) * Matrix::CreateTranslation(Vector3(0.0f, 0.0f, 0.3f)) * aixsWorld))
	{
		ambientColor->SetVector(Color(1.0f, 1.0f, 1.0f));
		isPicked = true;
		m_PickedTransform = PT_TranslateZ;
	}
	else
		ambientColor->SetVector(Color(0.0f, 0.0f, 1.0f));

	objectWvp->SetMatrix(Matrix::CreateRotationX(XMConvertToRadians(90)) * Matrix::CreateTranslation(Vector3(0.0f, 0.0f, 0.3f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset, m_pCurrentPass->GetEffectPass());
	objectWvp->SetMatrix(Matrix::CreateRotationX(XMConvertToRadians(90)) * Matrix::CreateTranslation(Vector3(0.0f, 0.0f, 0.6f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_ConeIndexCount, m_ConeIndexOffset, m_ConeVertexOffset, m_pCurrentPass->GetEffectPass());

	return S_OK;
}

HRESULT DebugGizmosNode::RenderRotateRings(Scene* pScene, const BoundingBox& aaBox, const Matrix& world)
{
	Variable* objectWvp = m_pEffect->GetVariablesByName().at("WorldViewProjection");
	Vector3 cameraPos = pScene->GetCamera()->GetPosition();
	Matrix ringWorld = world * Matrix::CreateScale(Vector3::Distance(cameraPos, world.Translation()) * 0.2f);
	ringWorld.Translation(world.Translation());
	ringWorld = ringWorld * Matrix::CreateTranslation(aaBox.Center);
	const XMMATRIX& wvp = ringWorld * pScene->GetCamera()->GetViewMatrix() * pScene->GetCamera()->GetProjectMatrix();

	objectWvp->SetMatrix(Matrix::CreateRotationZ(XMConvertToRadians(-90)) * wvp);
	Variable* ambientColor = m_pEffect->GetVariablesByName().at("AmbientColor");
	ambientColor->SetVector(Color(1.0f, 0.0f, 0.0f));
	pScene->GetRenderder()->VDrawMesh(
		m_TorusIndexCount, m_TorusIndexOffset, m_TorusVertexOffset, m_pCurrentPass->GetEffectPass());

	objectWvp->SetMatrix(wvp);
	ambientColor->SetVector(Color(0.0f, 1.0f, 0.0f));

	pScene->GetRenderder()->VDrawMesh(
		m_TorusIndexCount, m_TorusIndexOffset, m_TorusVertexOffset, m_pCurrentPass->GetEffectPass());

	objectWvp->SetMatrix(Matrix::CreateRotationX(XMConvertToRadians(90)) * wvp);
	ambientColor->SetVector(Color(0.0f, 0.0f, 1.0f));

	pScene->GetRenderder()->VDrawMesh(
		m_TorusIndexCount, m_TorusIndexOffset, m_TorusVertexOffset, m_pCurrentPass->GetEffectPass());

	return S_OK;
}

HRESULT DebugGizmosNode::RenderScaleAxes(Scene* pScene, const BoundingBox& aaBox, const Matrix& world)
{
	Variable* objectWvp = m_pEffect->GetVariablesByName().at("WorldViewProjection");
	Vector3 cameraPos = pScene->GetCamera()->GetPosition();
	Matrix aixsWorld = world * Matrix::CreateScale(Vector3::Distance(cameraPos, world.Translation()) * 0.2f);
	aixsWorld.Translation(world.Translation());
	aixsWorld = aixsWorld * Matrix::CreateTranslation(aaBox.Center);
	const XMMATRIX& wvp = aixsWorld * pScene->GetCamera()->GetViewMatrix() * pScene->GetCamera()->GetProjectMatrix();

	Variable* ambientColor = m_pEffect->GetVariablesByName().at("AmbientColor");
	ambientColor->SetVector(Color(1.0f, 0.0f, 0.0f));
	objectWvp->SetMatrix(Matrix::CreateRotationZ(XMConvertToRadians(-90)) * Matrix::CreateTranslation(Vector3(0.3f, 0.0f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset, m_pCurrentPass->GetEffectPass());
	objectWvp->SetMatrix(Matrix::CreateRotationZ(XMConvertToRadians(-90)) * Matrix::CreateTranslation(Vector3(0.6f, 0.0f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CubeIndexCount, m_CubeIndexOffset, m_CubeVertexOffset, m_pCurrentPass->GetEffectPass());

	ambientColor->SetVector(Color(0.0f, 1.0f, 0.0f));
	objectWvp->SetMatrix(Matrix::CreateTranslation(Vector3(0.0f, 0.3f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset, m_pCurrentPass->GetEffectPass());
	objectWvp->SetMatrix(Matrix::CreateTranslation(Vector3(0.0f, 0.6f, 0.0f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CubeIndexCount, m_CubeIndexOffset, m_CubeVertexOffset, m_pCurrentPass->GetEffectPass());

	ambientColor->SetVector(Color(0.0f, 0.0f, 1.0f));
	objectWvp->SetMatrix(Matrix::CreateRotationX(XMConvertToRadians(90)) * Matrix::CreateTranslation(Vector3(0.0f, 0.0f, 0.3f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset, m_pCurrentPass->GetEffectPass());
	objectWvp->SetMatrix(Matrix::CreateRotationX(XMConvertToRadians(90)) * Matrix::CreateTranslation(Vector3(0.0f, 0.0f, 0.6f)) * wvp);
	pScene->GetRenderder()->VDrawMesh(
		m_CubeIndexCount, m_CubeIndexOffset, m_CubeVertexOffset, m_pCurrentPass->GetEffectPass());
	return S_OK;
}

void DebugGizmosNode::CreateAABox(std::vector<Vector3>& vertices, std::vector<uint16_t>& indices)
{
	vertices.clear();
	vertices.reserve(8);
	Color color(Colors::LightSkyBlue.f);

	vertices.push_back(Vector3(-0.5f, -0.5f, -0.5f));
	vertices.push_back(Vector3(0.5f, -0.5f, -0.5f));
	vertices.push_back(Vector3(0.5f, -0.5f, 0.5f));
	vertices.push_back(Vector3(-0.5f, -0.5f, 0.5f));
	vertices.push_back(Vector3(-0.5f, 0.5f, -0.5f));
	vertices.push_back(Vector3(0.5f, 0.5f, -0.5f));
	vertices.push_back(Vector3(0.5f, 0.5f, 0.5f));
	vertices.push_back(Vector3(-0.5f, 0.5f, 0.5f));

	uint16_t arrayIndices[] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };
	indices.clear();
	indices.resize(ARRAYSIZE(arrayIndices));
	memcpy_s(&indices.front(), sizeof(arrayIndices), arrayIndices, sizeof(arrayIndices));
}

void DebugGizmosNode::AddVertex(std::vector<Vector3>& vertices, const std::vector<struct VertexPositionNormalTexture>& inputVertices)
{
	for (auto& vertex : inputVertices)
	{
		vertices.push_back(vertex.position);
	}
}

void DebugGizmosNode::CreateGeometryBuffers()
{
	std::vector<Vector3> boxVertices;
	std::vector<uint16_t> boxIndices;
	CreateAABox(boxVertices, boxIndices);

	std::vector<VertexPositionNormalTexture> cylinderVertices;
	std::vector<uint16_t> cylinderIndices;
	GeometricPrimitive::CreateCylinder(cylinderVertices, cylinderIndices, 0.6f, 0.008f);

	std::vector<VertexPositionNormalTexture> coneVertices;
	std::vector<uint16_t> coneIndices;
	GeometricPrimitive::CreateCone(coneVertices, coneIndices, 0.06f, 0.1f);

	std::vector<VertexPositionNormalTexture> torusVertices;
	std::vector<uint16_t> torusIndices;
	GeometricPrimitive::CreateTorus(torusVertices, torusIndices, 1.0f, 0.008f);

	std::vector<VertexPositionNormalTexture> cubeVertices;
	std::vector<uint16_t> cubeIndices;
	GeometricPrimitive::CreateCube(cubeVertices, cubeIndices, 0.06f);

	m_AABoxVertexOffset = 0;
	m_CylinderVertexOffset = boxVertices.size();
	m_ConeVertexOffset = m_CylinderVertexOffset + cylinderVertices.size();
	m_TorusVertexOffset = m_ConeVertexOffset + coneVertices.size();
	m_CubeVertexOffset = m_TorusVertexOffset + torusVertices.size();

	m_AABoxIndexCount = boxIndices.size();
	m_CylinderIndexCount = cylinderIndices.size();
	m_ConeIndexCount = coneIndices.size();
	m_TorusIndexCount = torusIndices.size();
	m_CubeIndexCount = cubeIndices.size();

	m_AABoxIndexOffset = 0;
	m_CylinderIndexOffset = m_AABoxIndexCount;
	m_ConeIndexOffset = m_CylinderIndexOffset + m_CylinderIndexCount;
	m_TorusIndexOffset = m_ConeIndexOffset + m_ConeIndexCount;
	m_CubeIndexOffset = m_TorusIndexOffset + m_TorusIndexCount;

	uint32_t vertexCount = 
		boxVertices.size() + cylinderVertices.size() + coneVertices.size() + torusVertices.size() + cubeVertices.size();
	std::vector<Vector3> vertices;
	vertices.reserve(vertexCount);

	vertices.insert(vertices.end(), boxVertices.begin(), boxVertices.end());

	AddVertex(vertices, cylinderVertices);
	AddVertex(vertices, coneVertices);
	AddVertex(vertices, torusVertices);
	AddVertex(vertices, cubeVertices);

	std::vector<uint16_t> indices;
	indices.insert(indices.end(), boxIndices.begin(), boxIndices.end());
	indices.insert(indices.end(), cylinderIndices.begin(), cylinderIndices.end());
	indices.insert(indices.end(), coneIndices.begin(), coneIndices.end());
	indices.insert(indices.end(), torusIndices.begin(), torusIndices.end());
	indices.insert(indices.end(), cubeIndices.begin(), cubeIndices.end());

	m_pCurrentPass->CreateVertexBuffer(&vertices.front(), vertices.size() * sizeof(Vector3), &m_pVertexBuffer);
	m_pCurrentPass->CreateIndexBuffer(&indices.front(), indices.size() * sizeof(uint16_t), &m_pIndexBuffer);

	vertices.clear();
	AddVertex(vertices, cylinderVertices);
	AddVertex(vertices, coneVertices);
	SetBoundingBox(vertices);
}