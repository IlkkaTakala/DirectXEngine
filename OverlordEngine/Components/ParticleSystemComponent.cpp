#include "stdafx.h"
#include "ParticleSystemComponent.h"
#include "Misc/ParticleMaterial.h"

ParticleMaterial* ParticleEmitter::m_pParticleMaterial{};


CurveData::CurveData(const std::vector<std::pair<float, float>>& data) : points(data)
{
}

float CurveData::EvaluateCurve(float delta) const
{
	for (int i = 0; i < points.size(); i++) {
		if (points[i].first <= delta) {
			if (i + 1 < points.size()) {
				if (points[i + 1].first >= delta) {
					float dist = points[i + 1].first - points[i].first;
					float change = points[i + 1].second - points[i].second;
					float p = (delta - points[i].first) / dist;
					return points[i].second + p * change;
				}
			}
			else return points[i].second;
		}
	}
	return 0.f;
}

XMVECTOR VectorCurveData::EvaluateCurve(float delta) const
{
	for (int i = 0; i < points.size(); i++) {
		if (points[i].first <= delta) {
			if (i + 1 < points.size()) {
				if (points[i].first >= delta) {
					float dist = points[i + 1].first - points[i].first;
					XMVECTOR change = XMLoadFloat3(&points[i + 1].second) - XMLoadFloat3(&points[i].second);
					float p = (delta - points[i].first) / dist;
					return XMLoadFloat3(&points[i].second) + p * change;
				}
			}
			else return XMLoadFloat3(&points[i].second);
		}
	}
	return { 0.f };
}


bool ParticleSystemConstruction::RateSpawner::Check(float delta)
{
	SpawnLast += delta;
	if (SpawnLast < SpawnInterval) return false;
	SpawnLast = 0.f;
	UINT idx = 0;
	if (system->FreeIdx.empty()) {
		if (system->ParticleCount >= system->MaxParticleCount) return false;
		idx = system->ParticleCount++;
	}
	else {
		idx = system->FreeIdx.front();
		system->FreeIdx.pop_front();
	}
	if (idx >= system->MaxParticleCount) {
		Logger::LogWarning(L"Particle count over limit");
		return false;
	}
	system->data.Constructor(system, system->Particles[idx]);
	auto& c = system->Particles[idx].initialColour;
	system->Particles[idx].vertexInfo.Color = { c.x, c.y, c.z, system->Particles[idx].alpha };
	system->Particles[idx].vertexInfo.Size = system->Particles[idx].initialSize;
	if (!system->IsLocalspace) XMStoreFloat3(&system->Particles[idx].vertexInfo.Position, 
		XMLoadFloat3(&system->Particles[idx].vertexInfo.Position) + 
		XMLoadFloat3(&system->system->GetTransform()->GetWorldPosition()));
	system->Particles[idx].vertexInfo.Rotation;
	system->Particles[idx].isActive = true;
	system->Particles[idx].lifetime = 0.f;
	system->ParticleCount = system->ParticleCount > idx ? system->ParticleCount : idx;
	return true;
}

int ParticleSystemConstruction::RateSpawner::GetMaxCount()
{
	if (SpawnInterval > 0.0) {
		return int(1 / SpawnInterval);
	}
	return 0;
}

bool ParticleSystemConstruction::BurstSpawner::Check(float)
{
	if (!Spawned) {
		Spawned = true;

		for (int i = 0; i < SpawnCount; i++) {
			UINT idx = 0;
			if (system->FreeIdx.empty()) {
				if (system->ParticleCount >= system->MaxParticleCount) return false;
				idx = system->ParticleCount++;
			}
			else {
				idx = system->FreeIdx.front();
				system->FreeIdx.pop_front();
			}
			if (idx >= system->MaxParticleCount) {
				Logger::LogWarning(L"Particle count over limit");
				return false;
			}
			system->data.Constructor(system, system->Particles[idx]);
			auto& c = system->Particles[idx].initialColour;
			system->Particles[idx].vertexInfo.Color = { c.x, c.y, c.z, system->Particles[idx].alpha };
			system->Particles[idx].vertexInfo.Size = system->Particles[idx].initialSize;
			if (!system->IsLocalspace) 
				XMStoreFloat3(&system->Particles[idx].vertexInfo.Position, 
					XMLoadFloat3(&system->Particles[idx].vertexInfo.Position) + 
					XMLoadFloat3(&system->system->GetTransform()->GetWorldPosition()));
			system->Particles[idx].vertexInfo.Rotation;
			system->Particles[idx].isActive = true;
			system->Particles[idx].lifetime = 0.f;
			system->ParticleCount = system->ParticleCount > idx ? system->ParticleCount : idx;
		}
		return true;
	}
	return false;
}

int ParticleSystemConstruction::BurstSpawner::GetMaxCount()
{
	return int(SpawnCount * 1.5f);
}

void ParticleSystemConstruction::Updator::UpdateVelocities(float delta) const
{
	for (UINT i = 0; i <= system->ParticleCount && i < system->MaxParticleCount; i++) {
		Particle& p = system->Particles[i];

		if (!p.isActive) continue;

		XMStoreFloat3(&p.vertexInfo.Position, XMLoadFloat3(&p.vertexInfo.Position) + XMLoadFloat3(&p.velocity) * delta);
		p.vertexInfo.Rotation += p.rotationRate * delta; // TODO fix
		if (p.vertexInfo.Rotation > 360.f)
			p.vertexInfo.Rotation -= 360.f;
	}
}

void ParticleSystemConstruction::Updator::UpdateLifetime(float delta) const
{
	UINT last_active = 0;
	for (UINT i = 0; i < system->ParticleCount && i < system->MaxParticleCount; i++) {
		Particle& p = system->Particles[i];

		if (!p.isActive) continue;

		p.lifetime += delta;
		if (p.lifetime >= p.max_lifetime)
		{
			p.isActive = false;
			system->FreeIdx.push_back(i);
			continue;
		}

		last_active = i;
	}
	system->ParticleCount = last_active + 1;
}

void ParticleSystemConstruction::Updator::Alpha(float /*delta*/, const CurveData& curve) const
{
	for (UINT i = 0; i < system->ParticleCount; i++) {
		Particle& p = system->Particles[i];
		if (!p.isActive) continue;

		p.alpha = curve.EvaluateCurve(p.lifetime / p.max_lifetime);

	}
}

void ParticleSystemConstruction::Updator::SpriteSize(float /*delta*/, const CurveData& curve) const
{
	for (UINT i = 0; i < system->ParticleCount; i++) {
		Particle& p = system->Particles[i];
		if (!p.isActive) continue;

		p.vertexInfo.Size = p.initialSize * curve.EvaluateCurve(p.lifetime / p.max_lifetime);

	}
}

void ParticleSystemConstruction::Updator::SpriteRotationRate(float delta, const CurveData& curve) const
{
	for (UINT i = 0; i < system->ParticleCount; i++) {
		Particle& p = system->Particles[i];
		if (!p.isActive) continue;

		p.vertexInfo.Rotation += delta * curve.EvaluateCurve(p.lifetime / p.max_lifetime);
		if (p.vertexInfo.Rotation > 360.f)
			p.vertexInfo.Rotation -= 360.f;
	}
}

void ParticleSystemConstruction::Updator::ColorBySpeed(float /*delta*/, const VectorCurveData& curve) const
{
	for (UINT i = 0; i < system->ParticleCount; i++) {
		Particle& p = system->Particles[i];
		if (!p.isActive) continue;

		float Speed;
		XMStoreFloat(&Speed, XMVector3Length(XMLoadFloat3(&p.velocity)));
		XMStoreFloat4(&p.vertexInfo.Color, XMLoadFloat3(&p.initialColour) * curve.EvaluateCurve(Speed));
	}
}

void ParticleSystemConstruction::Updator::AddVelocity(float delta, const XMFLOAT3& velocity) const
{
	for (UINT i = 0; i < system->ParticleCount; i++) {
		Particle& p = system->Particles[i];
		if (!p.isActive) continue;

		XMStoreFloat3(&p.velocity, XMLoadFloat3(&p.velocity) + XMLoadFloat3(&velocity) * delta);
	}
}

void ParticleSystemConstruction::Updator::Color(float /*delta*/, const VectorCurveData& curve) const
{
	for (UINT i = 0; i < system->ParticleCount; i++) {
		Particle& p = system->Particles[i];
		if (!p.isActive) continue;

		XMStoreFloat4(&p.vertexInfo.Color, XMLoadFloat3(&p.initialColour) * curve.EvaluateCurve(p.lifetime / p.max_lifetime));
	}
}

void ParticleSystemConstruction::Constructor::BoxLocation(Particle& p, float x, float y, float z)
{
	p.vertexInfo.Position = { MathHelper::randF(-x, x), MathHelper::randF(-y, y), MathHelper::randF(-z, z) };
}

void ParticleSystemConstruction::Constructor::SphereLocation(Particle& p, float radius, bool onSurface)
{
	if (onSurface) {
		float x = MathHelper::randF(-1.0, 1.0);
		float y = MathHelper::randF(-1.0, 1.0);
		float z = MathHelper::randF(-1.0, 1.0);

		float mag = sqrt(x * x + y * y + z * z);
		x /= mag; y /= mag; z /= mag;

		p.vertexInfo.Position = { radius * x, radius * y, radius * z };
	}
	else {
		float u = MathHelper::randF(0.f, 1.f);
		float v = MathHelper::randF(0.f, 1.f);
		float theta = u * 2.f * PxPi;
		float phi = acos(2.f * v - 1.f);
		float r = radius;
		float sinTheta = sin(theta);
		float cosTheta = cos(theta);
		float sinPhi = sin(phi);
		float cosPhi = cos(phi);
		float x = r * sinPhi * cosTheta;
		float y = r * sinPhi * sinTheta;
		float z = r * cosPhi;
		p.vertexInfo.Position = { x, y, z };
	}
}

ParticleSystemComponent::ParticleSystemComponent()
{
	m_enablePostDraw = true; //This enables the PostDraw function for the component
}

ParticleSystemComponent::~ParticleSystemComponent()
{
	for (auto& e : Emitters) {
		delete e;
	}

	for (auto& v : Variables) {
		delete v.second;
	}
}

ParticleEmitter::~ParticleEmitter()
{
	delete[] Particles;
 	delete Spawner;

	SafeRelease(m_pVertexBuffer);
}

ParticleEmitter* ParticleSystemComponent::AddEmitter(const ParticleEmitterData& emitter, std::function<ParticleSystemConstruction::Spawner* ()> spawn)
{
	auto e = new ParticleEmitter(emitter);
	Emitters.push_back(e);
	e->system = this;
	e->Spawner = spawn();
	e->Spawner->system = e;
	e->Initialize();
	return e;
}

void ParticleSystemComponent::Initialize(const SceneContext& /*sceneContext*/)
{
	
}

void ParticleEmitter::Initialize()
{
	if (!m_pParticleMaterial) m_pParticleMaterial = MaterialManager::Get()->CreateMaterial<ParticleMaterial>();

	m_pParticleTexture = ContentManager::Load<TextureData>(data.m_AssetFile);
}

void ParticleEmitter::CreateVertexBuffer(const SceneContext& sceneContext, UINT newMax)
{
	if (m_pVertexBuffer) SafeRelease(m_pVertexBuffer);

	D3D11_BUFFER_DESC vertexBuffDesc{};
	vertexBuffDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	vertexBuffDesc.ByteWidth = sizeof(VertexParticle) * newMax;
	vertexBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBuffDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
	vertexBuffDesc.MiscFlags = 0;

	HANDLE_ERROR(sceneContext.d3dContext.pDevice->CreateBuffer(&vertexBuffDesc, nullptr, &m_pVertexBuffer));

	Particle* temp = new Particle[newMax]();

	if (Particles) {
		memcpy(temp, Particles, MaxParticleCount);
		delete[] Particles;
	}

	MaxParticleCount = newMax;
	Particles = temp;
}

void ParticleSystemComponent::Update(const SceneContext& sceneContext)
{
	for (auto& e : Emitters)
		e->InternalUpdate(sceneContext, sceneContext.pGameTime->GetElapsed());
}

void ParticleEmitter::InternalUpdate(const SceneContext& sceneContext, float delta)
{
	if (!Spawner || !data.Constructor) return;

	UINT Max = UINT(Spawner->GetMaxCount() * AverageLifespan + 1);

	if (Max > MaxParticleCount) {
		CreateVertexBuffer(sceneContext, Max);
	}

	if (system->IsActive()) Spawner->Check(delta);

	UINT t_idx = 0;
	UINT last_active = 0;

	if (data.Updator) data.Updator(this, delta);

	D3D11_MAPPED_SUBRESOURCE Data;
	auto error = sceneContext.d3dContext.pDeviceContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Data);

	if (error != S_OK) {
		HANDLE_ERROR(error);
		return;
	}

	VertexParticle* Verts = (VertexParticle*)Data.pData;

	auto world = XMLoadFloat3(&system->GetTransform()->GetWorldPosition());

	for (UINT i = 0; i < ParticleCount; i++) {
		if (!Particles[i].isActive) continue;

		Particle& p = Particles[i];

		p.vertexInfo.Color.w = p.alpha;
		Verts[t_idx] = p.vertexInfo;
		if (IsLocalspace) {
			XMStoreFloat3(&Verts[t_idx].Position, XMLoadFloat3(&Verts[t_idx].Position) + world);
		}

		t_idx++;
		last_active = i;
	}

	sceneContext.d3dContext.pDeviceContext->Unmap(m_pVertexBuffer, 0);

	ActiveParticles = t_idx;
	ParticleCount = last_active + 1;
}

void ParticleEmitter::Draw(const SceneContext& sceneContext)
{
	auto d3d11 = sceneContext.d3dContext;

	m_pParticleMaterial->SetVariable_Matrix(L"gWorldViewProj", sceneContext.pCamera->GetViewProjection());
	m_pParticleMaterial->SetVariable_Matrix(L"gViewInverse", sceneContext.pCamera->GetViewInverse());
	m_pParticleMaterial->SetVariable_Texture(L"gParticleTexture", m_pParticleTexture);

	auto& Context = m_pParticleMaterial->GetTechniqueContext();

	d3d11.pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	d3d11.pDeviceContext->IASetInputLayout(Context.pInputLayout);

	constexpr UINT offset = 0;
	constexpr UINT stride = sizeof(VertexParticle);
	d3d11.pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	D3DX11_TECHNIQUE_DESC techDesc{};
	Context.pTechnique->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		Context.pTechnique->GetPassByIndex(p)->Apply(0, d3d11.pDeviceContext);
		d3d11.pDeviceContext->Draw(ActiveParticles, 0);
	}
}

ParticleEmitter::ParticleEmitter(ParticleEmitterData data) :
	data(data), Spawner(nullptr), system(nullptr)
{
	float maxLife = 0.f;

	Particle base;
	for (int i = 0; i < 20; i++) {
		data.Constructor(this, base);
		maxLife += base.max_lifetime;
	}

	AverageLifespan = maxLife / 20.f * 1.3f;
}

void ParticleSystemComponent::PostDraw(const SceneContext& sceneContext)
{
	for (auto& c : Emitters) {
		c->Draw(sceneContext);
	}
}

void ParticleSystemComponent::Warmup(float duration)
{
	for (auto& e : Emitters) {
		if (!m_IsActive || !e->Spawner || !e->data.Constructor) return;
		UINT Max = UINT(e->Spawner->GetMaxCount() * e->AverageLifespan + 1);

		if (Max > e->MaxParticleCount) {
			auto& scene = m_pScene->GetSceneContext();
			e->CreateVertexBuffer(scene, Max);
		}
		float remaining = duration;
		constexpr float tick = 0.016f;
		while (remaining > 0.f) {


			e->Spawner->Check(tick);
			if (e->data.Updator) e->data.Updator(e, tick);
			remaining -= tick;
		}
	}
}

void ParticleSystemComponent::Reset()
{
	for (auto& e : Emitters) {
		if (e->Spawner) e->Spawner->Reset();
		e->ParticleCount = 0;
		e->FreeIdx.clear();
	}
}
