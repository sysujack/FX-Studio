#include "stdafx.h"
#include "FXStudioView.h"
#include "FXStudioApp.h"

extern FXStudioApp globalApp;

FXStudioView::FXStudioView(shared_ptr<IRenderer> pRenderer) : HumanView(pRenderer)
{
}


FXStudioView::~FXStudioView()
{
}

void FXStudioView::VOnUpdate(double fTime, float fElapsedTime)
{
	HumanView::VOnUpdate(fTime, fElapsedTime);

// 	if (m_pFreeCameraController)
// 	{
// 		m_pFreeCameraController->OnUpdate(deltaMilliseconds);
// 	}
}

bool FXStudioView::VLoadGameDelegate(TiXmlElement* pProjectData)
{
	if (!HumanView::VLoadGameDelegate(pProjectData))
		return false;

// 	m_pFreeCameraController.reset(GCC_NEW MovementController(m_pCamera, 90, 0, true));
// 	m_pCamera->ClearTarget();
// 
// 	m_KeyboardHandler = m_pFreeCameraController;
// 	m_PointerHandler = m_pFreeCameraController;

	m_pScene->VOnRestore();
	return true;
}
