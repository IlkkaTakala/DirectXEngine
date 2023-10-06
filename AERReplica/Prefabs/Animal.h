#pragma once

class Animal : public GameObject
{
public:
	Animal();
	~Animal() override;

	Animal(const Animal& other) = delete;
	Animal(Animal&& other) noexcept = delete;
	Animal& operator=(const Animal& other) = delete;
	Animal& operator=(Animal&& other) noexcept = delete;

protected:
	void Initialize(const SceneContext&) override;
	void Update(const SceneContext&) override;

	AnimationInstance Idle;

	FMOD::Sound* sounds[3];
};

