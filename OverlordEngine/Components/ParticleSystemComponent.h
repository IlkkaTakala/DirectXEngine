#pragma once
class ParticleMaterial;
class ParticleEmitter;
class ParticleSystemComponent;

class CurveData
{
public:
	CurveData(const std::vector<std::pair<float, float>>& data);

	float EvaluateCurve(float delta) const;

private:
	std::vector<std::pair<float, float>> points;
};

class VectorCurveData
{
public:
	VectorCurveData(const std::vector<std::pair<float, XMFLOAT3>>& data) : points(data) {}

	XMVECTOR EvaluateCurve(float delta) const;

private:
	std::vector<std::pair<float, XMFLOAT3>> points;
};

struct Particle
{
	VertexParticle vertexInfo{};

	bool isActive{ false };

	float rotationRate;
	XMFLOAT3 velocity;
	float alpha;
	float lifetime;

	float max_lifetime;
	float initialSize;
	XMFLOAT3 initialColour;
};

class ParticleSystemConstruction
{
public:
	struct Spawner {
		Spawner() : system(nullptr) {}
		ParticleEmitter* system;
		virtual ~Spawner() {}
		virtual bool Check(float) = 0;
		virtual int GetMaxCount() = 0;
		virtual void Reset() {}
	};

	struct Constructor {
		static void BoxLocation(Particle&, float x, float y, float z);
		static void SphereLocation(Particle&, float radius, bool onSurface = false);

	};

	struct Updator {
		Updator(ParticleEmitter* s) : system(s) {}
		ParticleEmitter* system;

		void UpdateVelocities(float delta) const;
		void UpdateLifetime(float delta) const;
		void Color(float delta, const VectorCurveData& curve) const;
		void Alpha(float delta, const CurveData& curve) const;
		void SpriteSize(float delta, const CurveData& curve) const;
		void SpriteRotationRate(float delta, const CurveData& curve) const;
		void ColorBySpeed(float delta, const VectorCurveData& curve) const;
		void AddVelocity(float delta, const XMFLOAT3& velocity) const;

	};

private:
	ParticleSystemConstruction() {}

	struct RateSpawner : public Spawner {
		RateSpawner(float rate) : Spawner(), SpawnInterval(1.f / rate), SpawnLast(0.f) {}
		float SpawnLast;
		float SpawnInterval;
		virtual bool Check(float) override;
		virtual int GetMaxCount() override;
	};

	struct BurstSpawner : public Spawner {
		BurstSpawner(int count) : Spawner(), SpawnCount(count), Spawned(false) {}
		int SpawnCount;
		bool Spawned;
		bool Check(float) override;
		int GetMaxCount() override;
		void Reset() { Spawned = false; }
	};

public:
	static Spawner* MakeSpawnRate(float rate) { return new RateSpawner(rate); }
	static Spawner* MakeSpawnBurst(int count) { return new BurstSpawner(count); }
};

typedef std::function<void(ParticleEmitter*, float)> DefaultUpdate;
typedef std::function<void(ParticleEmitter*, Particle&)> DefaultParticle;

struct ParticleEmitterData
{
	std::wstring m_AssetFile{};
	DefaultParticle Constructor{ nullptr };
	DefaultUpdate Updator{ nullptr };
};

class ParticleSystemComponent : public BaseComponent
{
	struct BaseVariable
	{
		virtual ~BaseVariable() {}
	};
public:
	ParticleSystemComponent();
	~ParticleSystemComponent() override;
	ParticleSystemComponent(const ParticleSystemComponent& other) = delete;
	ParticleSystemComponent(ParticleSystemComponent&& other) noexcept = delete;
	ParticleSystemComponent& operator=(const ParticleSystemComponent& other) = delete;
	ParticleSystemComponent& operator=(ParticleSystemComponent&& other) noexcept = delete;

	template <typename T>
	struct Variable : BaseVariable
	{
		T Value{};

		T& Get() { return Value; }
		void Set(const T& val) { Value = val; }
	};

	void Warmup(float duration);
	void Reset();

	template <typename T>
	void SetVariable(std::string name, T value) {
		if (!Variables[name])
			Variables[name] = new Variable<T>();
		if (auto ptr = dynamic_cast<Variable<T>*>(Variables[name]); ptr)
			ptr->Set(value);
	}

	template <typename T>
	T* GetVariable(std::string name) {
		if (!Variables[name])
			Variables[name] = new Variable<T>();
		if (auto ptr = dynamic_cast<Variable<T>*>(Variables[name]); ptr)
			return &ptr->Get();
		return nullptr;
	}

	ParticleEmitter* AddEmitter(const ParticleEmitterData& emitter, std::function<ParticleSystemConstruction::Spawner * ()> spawn);

protected:
	void Initialize(const SceneContext&) override;
	void Update(const SceneContext&) override;
	void PostDraw(const SceneContext&) override;

private:

	std::unordered_map<std::string, BaseVariable*> Variables;
	std::vector<ParticleEmitter*> Emitters;

};

class ParticleEmitter
{
	friend class ParticleSystemConstruction;
	friend class ParticleSystemComponent;

	~ParticleEmitter();
	void Initialize();
	void CreateVertexBuffer(const SceneContext& sceneContext, UINT newMax); //Method to create the vertex buffer
	void InternalUpdate(const SceneContext& sceneContext, float delta);
	void Draw(const SceneContext&);

	TextureData* m_pParticleTexture{};
	static ParticleMaterial* m_pParticleMaterial; //Material used to render the particles (static >> shared by all emitters)

	ID3D11Buffer* m_pVertexBuffer{}; //The vertex buffer, containing ParticleVertex information for each Particle

	std::deque<int> FreeIdx;
	Particle* Particles{nullptr}; //Array of particle objects
	UINT MaxParticleCount{ 0 };
	float AverageLifespan{ 0.f };
	UINT ActiveParticles{}; //The active particles for the current frame

	bool Autoplay{ true };
	bool IsLocalspace{ false };

	ParticleSystemComponent* system;
	ParticleSystemConstruction::Spawner* Spawner{nullptr};
	ParticleEmitterData data;

public:
	ParticleEmitter(ParticleEmitterData data);

	UINT ParticleCount{}; //The total amount of particles
	UINT GetMaxParticleCount() const { return MaxParticleCount; }

	void SetIsLocalSpace(bool value) { IsLocalspace = value; }
};

