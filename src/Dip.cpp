#include "Dip.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "ParticleManager.h"
#include "Map.h"
#include <cmath> // Para sqrt()

Dip::Dip() : Enemy(EntityType::UNKNOWN) // Reemplaza UNKNOWN por EntityType::DIP cuando lo tengas
{
	name = "Dip";
}

Dip::~Dip() {}

bool Dip::Awake() {
	return true;
}

bool Dip::Start()
{
	// 1. Cargar las 5 animaciones reales de Ninfa para realizar las pruebas (Placeholder)
	std::unordered_map<int, std::string> aliases = {
		{0, "idle"},
		{3, "walk"},
		{13, "jumpFwd"},
		{26, "claw"},
		{39, "howl"} // Usado temporalmente para la animación de muerte
	};
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/Ninfa.tsx", aliases);
	anims.SetCurrent("idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/Ninfa.png");

	// 2. Físicas (Círculo Dinámico)
	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	// 3. Inicializar Pathfinding
	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	// 4. Estadísticas del enemigo
	vision = 15;
	speed = 3.0f;
	knockbackForce = 5.0f;
	maxHealth = 20;
	currentHealth = 20;
	velocity = { 0.0f, 0.0f };

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Iniciar el temporizador para el cooldown de 5 segundos del ataque de salto
	startAttack.Start();

	return true;
}

bool Dip::Update(float dt)
{
	if (!active) return true;

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		// Actualizar el Pathfinding cada 500ms si no está atacando
		if (pathFindingCooldown.ReadMSec() > 500 && !isJumpingFwd && !isJumpingBack && !isClawing && !isKnockedback)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();

		Player* player = Engine::GetInstance().entityManager->GetPlayer();

		// Lógica Principal de Acción y Distancia
		if (player != nullptr && !isJumpingFwd && !isJumpingBack && !isClawing && !isKnockedback)
		{
			// ¡Cálculo de distancia EXACTA en píxeles! (Evita el bug del tipo int con los tiles)
			float dist = sqrt(position.distanceSquared(player->GetPosition()));

			// Ataque Cercano: Zarpazo (< 80 píxeles)
			if (dist < 80.0f)
			{
				isClawing = true;
				anims.SetCurrent("claw");
			}
			// Ataque Lejano: Salto/Embestida (Entre 80 y 250 píxeles) -> SOLO SI HAN PASADO 5 SEGUNDOS
			else if (dist < 250.0f && startAttack.ReadSec() >= 5.0f)
			{
				isJumpingFwd = true;
				initialJumpPos = { position.getX(), position.getY() }; // Guardar posición original para poder volver
				actionTimer.Start();
				anims.SetCurrent("jumpFwd");

				// Darle un pequeño impulso inicial hacia arriba al saltar
				velocity.y = -4.0f;
			}
			// Fuera de rango de ataque: Moverse hacia el jugador
			else
			{
				Move();
			}
		}
		// Ejecutar la acción si ya se encuentra en un estado de ataque
		else if (player != nullptr)
		{
			if (isJumpingFwd || isJumpingBack) JumpAttack();
			if (isClawing) ClawAttack();
		}

		Knockback();
		ApplyPhysics();
	}

	// Lógica de Muerte Temporal (Placeholder)
	if (isdead)
	{
		if (anims.GetCurrentName() != "howl")
		{
			Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
			anims.GetAnim("howl")->SetLoop(false);
			anims.SetCurrent("howl");
			pbody->ctype = ColliderType::UNKNOWN;
			isKnockedback = false;

			std::shared_ptr<Entity> healthOrb = Engine::GetInstance().entityManager->CreateEntity(EntityType::HEALTH_ORB);
			healthOrb->position.setX(this->position.getX());
			healthOrb->position.setY(this->position.getY() - 100.0f);
			healthOrb->Start();
		}

		if (anims.GetAnim("howl")->HasFinishedOnce()) {
			pendingToDelete = true;
		}
	}

	Draw(dt);
	return true;
}

void Dip::PerformPathfinding()
{
	pathfinding->ResetPath(GetTilePos());
	Vector2D pos = GetPosition();
	Player* player = Engine::GetInstance().entityManager->GetPlayer();

	// Limitar el cálculo pesado de A* si el jugador está demasiado lejos
	float dist = sqrt(pos.distanceSquared(player->GetPosition())) / 128.0f;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && dist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void Dip::GetPhysicsValues()
{
	b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0.0f, vel.y }; // Resetear X en cada frame, conservar la gravedad (Y)
}

void Dip::Move()
{
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (!player) return;

	// 1. Si el jugador está fuera de la visión absoluta (15 tiles * 128px = 1920px), se detiene por completo
	float dist = sqrt(position.distanceSquared(player->GetPosition()));
	if (dist > vision * 128.0f) {
		anims.SetCurrent("idle");
		velocity.x = 0;
		return;
	}

	if (anims.GetCurrentName() != "walk") anims.SetCurrent("walk");

	// 2. CASO NORMAL: Si el Pathfinding (A*) encuentra una ruta válida, sigue los nodos
	if (!pathfinding->pathTiles.empty())
	{
		Vector2D tilePos = GetTilePos();

		// Consumir el nodo actual de la ruta si ya lo hemos alcanzado
		if (pathfinding->pathTiles.back() == tilePos) {
			pathfinding->pathTiles.pop_back();
		}

		if (!pathfinding->pathTiles.empty()) {
			Vector2D nextTile = pathfinding->pathTiles.back();

			if (nextTile.getX() > tilePos.getX()) {
				velocity.x = speed;
				lookingRight = true;
			}
			else if (nextTile.getX() < tilePos.getX()) {
				velocity.x = -speed;
				lookingRight = false;
			}

			// Compensación de velocidad para evitar quedarse atascado en rampas o bajadas
			if (pathfinding->IsWalkable(nextTile.getX(), nextTile.getY() + 1) && !pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 1)) {
				velocity.x *= 5;
			}
		}
	}
	// 3. FIX CENTRAL (FALLBACK TRACKING): 
	// Si el A* falla (porque el jugador saltó o superó el límite de iteraciones),
	// el enemigo correrá agresivamente hacia la coordenada X del jugador sin dudarlo.
	else
	{
		float playerX = player->GetPosition().getX();
		float myX = position.getX();

		// Margen de 10 píxeles para evitar que el sprite tiemble cuando coinciden las coordenadas
		if (playerX > myX + 10.0f) {
			velocity.x = speed;
			lookingRight = true;
		}
		else if (playerX < myX - 10.0f) {
			velocity.x = -speed;
			lookingRight = false;
		}
		else {
			velocity.x = 0;
		}
	}
}

void Dip::JumpAttack()
{
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	float dashSpeed = 6.0f;

	// FASE 1: Saltar y embestir hacia el jugador
	if (isJumpingFwd)
	{
		lookingRight = (player->GetPosition().getX() > position.getX());
		velocity.x = lookingRight ? dashSpeed : -dashSpeed;

		// A los 0.4s termina la embestida, golpea y cambia a la fase de retroceso
		if (actionTimer.ReadSec() > 0.4f)
		{
			// Empujar y repeler al jugador (Knockback)
			float pushX = lookingRight ? 15.0f : -15.0f;
			Engine::GetInstance().physics->SetLinearVelocity(player->pbody, { pushX, -5.0f });

			isJumpingFwd = false;
			isJumpingBack = true; // Iniciar el viaje de vuelta
			actionTimer.Start();
		}
	}
	// FASE 2: Volver a la posición original
	else if (isJumpingBack)
	{
		bool goRight = (initialJumpPos.x > position.getX());
		velocity.x = goRight ? dashSpeed : -dashSpeed;
		lookingRight = !goRight; // Mirar hacia donde ataca, no hacia donde retrocede (efecto visual)

		// A los 0.4s llega a su sitio original y aterriza
		if (actionTimer.ReadSec() > 0.4f)
		{
			isJumpingBack = false;
			startAttack.Start(); // ¡REINICIAR EL COOLDOWN DE 5 SEGUNDOS AQUÍ!
			anims.SetCurrent("idle");
			velocity.x = 0;
		}
	}
}

void Dip::ClawAttack()
{
	velocity.x = 0.0f; // Quedarse quieto en el sitio para arañar

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player != nullptr) {
		lookingRight = (player->GetPosition().getX() > position.getX());
	}

	// Cuando termine la animación, salir del estado de ataque cercano
	if (anims.GetAnim("claw")->HasFinishedOnce()) {
		isClawing = false;
		anims.SetCurrent("idle");
	}
}

void Dip::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isJumpingFwd = false;
		isJumpingBack = false;
		isClawing = false;

		// No tenemos animación de daño (hurt) en este placeholder, usamos idle
		if (anims.GetCurrentName() != "idle") anims.SetCurrent("idle");
		velocity.x = lookingRight ? knockbackForce : -knockbackForce;
	}

	if (knockbackTime <= 0) {
		isKnockedback = false;
		knockbackTime = 500.0f;
	}
	else {
		knockbackTime -= Engine::GetInstance().GetDt() * 1000.0f;
	}
}

void Dip::ApplyPhysics()
{
	// Aplica el movimiento horizontal modificado y respeta la gravedad vertical (Box2D)
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, velocity.y });
}

void Dip::Draw(float dt)
{
	if (!Engine::GetInstance().sceneManager->isGamePaused) {
		anims.Update(dt);
	}

	const SDL_Rect& animFrame = anims.GetCurrentFrame();
	SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (Engine::GetInstance().physics->GetDebug()) {
		pathfinding->DrawPath();
	}

	if (isKnockedback) {
		Uint8 r = 255, g = 25, b = 25;
		Engine::GetInstance().render->SetColorMod(texture, &r, &g, &b, 255, 25, 25);
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, 255, 255, 255);
	}
	else {
		Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1);
	}
}

void Dip::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (isdead) return;

	// Recibir daño si choca con el ataque del jugador
	if (physB->ctype == ColliderType::PLAYER_ATTACK) {
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
		Engine::GetInstance().particleManager->EmitHitSparks(position.getX(), position.getY(), false);
	}
}

void Dip::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {}