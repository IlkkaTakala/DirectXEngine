#pragma once
class Pickable : public GameObject
{
public:
	Pickable();
	~Pickable() override;

	Pickable(const Pickable& other) = delete;
	Pickable(Pickable&& other) noexcept = delete;
	Pickable& operator=(const Pickable& other) = delete;
	Pickable& operator=(Pickable&& other) noexcept = delete;

protected:
	void Initialize(const SceneContext&) override;
	void Update(const SceneContext&) override;

private:

	void RemovePickup();

	static UINT MaterialIdx;

	float m_Rotation{};
	FMOD::Sound* pickSound{};

	RigidBodyComponent* Collider{};
	ModelComponent* Mesh{};
	ParticleSystemComponent* AliveParticles{};
	ParticleSystemComponent* DeathParticles{};
};

