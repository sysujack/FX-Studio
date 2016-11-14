#include "Scene.h"
#include "SceneNode.h"
#include "CameraNode.h"
#include "../EventManager/EventManager.h"
#include "../EventManager/Events.h"

Scene::Scene(shared_ptr<IRenderer> pRenderer)
	: m_pRenderer(pRenderer),
	m_pCamera(nullptr)
{
	m_pRootNode.reset(DEBUG_NEW RootNode());

	IEventManager* pEventMgr = IEventManager::Get();
	pEventMgr->VAddListener(
		boost::bind(&Scene::NewRenderComponentDelegate, this, _1), EvtData_New_Render_Component::sk_EventType);
	pEventMgr->VAddListener(
		boost::bind(&Scene::DestroyActorDelegate, this, _1), EvtData_Destroy_Actor::sk_EventType);
	pEventMgr->VAddListener(
		boost::bind(&Scene::MoveActorDelegate, this, _1), EvtData_Move_Actor::sk_EventType);
	pEventMgr->VAddListener(
		boost::bind(&Scene::ModifiedRenderComponentDelegate, this, _1), EvtData_Modified_Render_Component::sk_EventType);
}

Scene::~Scene()
{
	IEventManager* pEventMgr = IEventManager::Get();
	pEventMgr->VRemoveListener(
		boost::bind(&Scene::NewRenderComponentDelegate, this, _1), EvtData_New_Render_Component::sk_EventType);
	pEventMgr->VRemoveListener(
		boost::bind(&Scene::DestroyActorDelegate, this, _1), EvtData_Destroy_Actor::sk_EventType);
	pEventMgr->VRemoveListener(
		boost::bind(&Scene::MoveActorDelegate, this, _1), EvtData_Move_Actor::sk_EventType);
	pEventMgr->VRemoveListener(
		boost::bind(&Scene::ModifiedRenderComponentDelegate, this, _1), EvtData_Modified_Render_Component::sk_EventType);
}

HRESULT Scene::OnUpdate(double fTime, float fElapsedTime)
{
	if (m_pRootNode != nullptr)
	{
		return m_pRootNode->VOnUpdate(this, fTime, fElapsedTime);
	}

	return S_OK;
}

HRESULT Scene::OnRender(double fTime, float fElapsedTime)
{
	if (m_pRootNode != nullptr && m_pCamera != nullptr)
	{
		m_pCamera->SetViewTransform(this);

		if (m_pRootNode->VPreRender(this) == S_OK)
		{
			m_pRootNode->VRender(this, fTime, fElapsedTime);
			m_pRootNode->VRenderChildren(this, fTime, fElapsedTime);
			m_pRootNode->VPostRender(this);
		}

		RenderAlphaPass();
	}

	return S_OK;
}

HRESULT Scene::OnRestore()
{
	if (!m_pRootNode)
		return S_OK;

// 	HRESULT hr;
// 	V_RETURN(m_pRenderer->VOnRestore());

	return m_pRootNode->VOnRestore(this);
}

HRESULT Scene::OnDestoryDevice()
{
	if (m_pRootNode != nullptr)
	{
		return m_pRootNode->VOnDestoryDevice(this);
	}
	return S_OK;
}

shared_ptr<ISceneNode> Scene::FindActor(ActorId actorId)
{
	auto actor = m_ActorMap.find(actorId);
	if (actor != m_ActorMap.end())
	{
		return actor->second;
	}
	else
	{
		return shared_ptr<ISceneNode>();
	}
}

bool Scene::AddChild(ActorId actorId, shared_ptr<ISceneNode> pChild)
{
	if (actorId != INVALID_ACTOR_ID)
	{
		m_ActorMap[actorId] = pChild;
	}

	return m_pRootNode->VAddChild(pChild);
}

bool Scene::RemoveChild(ActorId actorId)
{
	if (actorId != INVALID_ACTOR_ID)
	{
		m_ActorMap.erase(actorId);
		return m_pRootNode->VRemoveChild(actorId);
	}

	return false;
}

void Scene::NewRenderComponentDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_New_Render_Component> pCastEventData = static_pointer_cast<EvtData_New_Render_Component>(pEventData);

	ActorId actorId = pCastEventData->GetActorId();
	shared_ptr<SceneNode> pSceneNode(pCastEventData->GetSceneNode());
	if (pSceneNode != nullptr)
	{
		if (FAILED(pSceneNode->VOnRestore(this)))
		{
// 			std::string error = "Failed to restore scene node to the scene for actorid " + boost::lex(actorId);
// 			DEBUG_ERROR(error);
			return;
		}

		AddChild(actorId, pSceneNode);
	}
}

void Scene::ModifiedRenderComponentDelegate(IEventDataPtr pEventData)
{

}

void Scene::DestroyActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Destroy_Actor> pCastEventData = static_pointer_cast<EvtData_Destroy_Actor>(pEventData);
	RemoveChild(pCastEventData->GetActorId());
}

void Scene::MoveActorDelegate(IEventDataPtr pEventData)
{
	shared_ptr<EvtData_Move_Actor> pCastEventData = static_pointer_cast<EvtData_Move_Actor>(pEventData);

	ActorId id = pCastEventData->GetActorId();
	Matrix transform = pCastEventData->GetMatrix();

	shared_ptr<ISceneNode> pNode = FindActor(id);
	if (pNode)
	{
// 		pNode->VSetTransform(transform);
	}
}