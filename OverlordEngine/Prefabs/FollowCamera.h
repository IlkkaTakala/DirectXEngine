#pragma once
class FollowCamera : public GameObject
{
public:
	FollowCamera() = default;
	~FollowCamera() override = default;
	FollowCamera(const FollowCamera& other) = delete;
	FollowCamera(FollowCamera&& other) noexcept = delete;
	FollowCamera& operator=(const FollowCamera& other) = delete;
	FollowCamera& operator=(FollowCamera&& other) noexcept = delete;

	void SetOffset(const XMFLOAT3& offset, float radius);

	void SetActive(bool value);
	bool IsActiveCamera() const;

protected:

	void Initialize(const SceneContext& sceneContext) override;
	void Update(const SceneContext& sceneContext) override;

private:

	XMFLOAT3 m_Offset{};
	float m_Radius{};
	float m_Distance{};
	float m_CurrentDistance{};
	bool m_HasCollider{false};
	CameraComponent* m_pCamera{};
};

