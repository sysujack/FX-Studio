#pragma once
#include "../TinyEngine/TinyEngine.h"

class SceneNode;

enum CameraType
{
	CT_OrbitView = 0, CT_FirstPerson = 1
};

class ModelController : public IPointerHandler, public IKeyboardHandler
{
public:
	ModelController(shared_ptr<CameraNode> pEditorCamera, shared_ptr<DebugGizmosNode> pGizmosNode,
		const Vector3& cameraPos, float cameraYaw, float cameraPitch);

	void SetCameraType(CameraType type);
	void OnUpdate(const GameTime& gameTime);

	virtual bool VOnPointerLeftButtonDown(const Vector2 &pos, int radius) override;
	virtual bool VOnPointerLeftButtonUp(const Vector2 &pos, int radius) override;
	virtual bool VOnPointerRightButtonDown(const Vector2 &pos, int radius) override;
	virtual bool VOnPointerRightButtonUp(const Vector2 &pos, int radius) override;
	virtual bool VOnPointerMove(const Vector2 &pos, int radius) override;
	virtual bool VOnPointerWheel(int16_t delta) override;

	virtual bool VOnKeyDown(uint8_t c) override { m_Keys[c] = true; return true; }
	virtual bool VOnKeyUp(uint8_t c) override { m_Keys[c] = false; return true; }

protected:
	shared_ptr<CameraNode> m_pEditorCamera;
	shared_ptr<DebugGizmosNode> m_pGizmosNode;
	CameraType m_CameraType;

	Vector3 m_Position;
	float m_Yaw;
	float m_Pitch;
	float m_TargetYaw;
	float m_TargetPitch;
	float m_Delta;
	float m_MoveX;
	float m_MoveY;
	float m_MaxSpeed;
	float m_CurrentSpeed;

	bool m_IsChanged;
	bool m_IsLButtonDown;
	Vector2 m_LastMousePos;
	bool m_Keys[256];
};

