#include "Dip.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "SceneManager.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include "Player.h" 
#include "Log.h" 
#include "ParticleManager.h" 
#include "Physics.h"

#include <cmath>
#include <cstdlib>

#include "tracy/Tracy.hpp"

Dip::Dip() : Enemy(EntityType::DIP)
{
	name = "Dip";
}

Dip::~Dip() {}

bool Dip::Awake() {
	return true;
}

bool Dip::Start()
{
	// ==========================================
	// TODO(Art): Cargar animaciones 
	// ==========================================
	// Inicializar parámetros del enemigo
	std::unordered_map<int, std::string> aliases = { {0,"Attack"},{22,"Dead"},{44,"Hit"},{66,"Still"},{88,"Idle"},{110,"Move"},{132,"Jump"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Dip/SS_Dip.tsx", aliases);
	anims.SetCurrent("Idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Dip/SS_Dip.png");

	// Cuerpo físico (Physbody)
	texW = 390;
	texH = 256;

	pbody = Engine::GetInstance().physics->CreateRectangle(
		(int)position.getX(),
		(int)position.getY(),
		texW,
		texH,
		bodyType::DYNAMIC
	);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	// MODIFICACIÓN: Rango de visión ajustado a 20 bloques
	vision = 20;
	speed = 8.0f;
	knockbackForce = 5.0f;
	maxHealth = 20;
	currentHealth = 20;

	attackDamage = 5;
	startAttack.Start();

	attackVisualTimer.Start();
	isAttackingVisual = false;
	isTouchingPlayer = false;
	playerContacts = 0;
	patrolTimer.Start(); // Iniciar temporizador de patrulla

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool Dip::Update(float dt)
{
	if (!active) return true;
	ZoneScoped;

	if (!Engine::GetInstance().render->IsOnScreenWorldRect(position.getX(), position.getY(), texW, texH, 5))
	{
		Engine::GetInstance().physics->SetLinearVelocity(pbody, b2Vec2_zero);
		return true;
	}

	if (isdead)
	{
		Draw(dt);
		return true;
	}

	if (Engine::GetInstance().sceneManager->isGamePaused == false && !isdead)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();
		Move();
		ApplyPhysics();

		if (isAttackingVisual && attackVisualTimer.ReadMSec() > 200) {
			isAttackingVisual = false;
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

	if (player == nullptr) return;
	Vector2D playerPos = player->GetPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 128.0f;
	int iter = 0;

	// ¡ATENCIÓN!: Si el pathTiles sigue vacío en el debug, asegúrate de que tu clase Pathfinding 
	// realmente está calculando la ruta hacia el jugador (ej. pasándole el destino a PropagateAStar).
	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void Dip::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
}

void Dip::ExecuteSpecialAttack(Vector2D playerPos)
{
	Vector2D pos = GetPosition();
	float attackOffset = 55.0f;

	if (specialPhase == 1)
	{
		if (phaseTimer.ReadMSec() < 400)
		{
			anims.SetCurrent("Jump");
			velocity.x = (pos.getX() > playerPos.getX()) ? speed * 2.5f : -speed * 2.5f;
			lookingRight = (playerPos.getX() > pos.getX());
		}
		else
		{
			leapStartPos = GetPosition();
			lockedPlayerPos = playerPos;
			specialPhase = 2;
			phaseTimer.Start();
			Engine::GetInstance().physics->SetYVelocity(pbody, -8.0f);
		}
	}
	else if (specialPhase == 2)
	{
		if (phaseTimer.ReadMSec() < 500)
		{
			anims.SetCurrent("Jump");
			bool jumpRight = (lockedPlayerPos.getX() > leapStartPos.getX());

			velocity.x = jumpRight ? (speed * 4.0f) : -(speed * 4.0f);
			lookingRight = jumpRight;

			float targetX = jumpRight ? (lockedPlayerPos.getX() - attackOffset) : (lockedPlayerPos.getX() + attackOffset);

			if ((jumpRight && pos.getX() >= targetX) ||
				(!jumpRight && pos.getX() <= targetX))
			{
				velocity.x = 0;
			}
		}
		else
		{
			specialPhase = 3;
			phaseTimer.Start();
			Engine::GetInstance().physics->SetYVelocity(pbody, -8.0f);
			lockedPlayerPos = GetPosition();
		}
	}
	else if (specialPhase == 3)
	{
		if (phaseTimer.ReadMSec() < 500)
		{
			anims.SetCurrent("Jump");
			bool returnRight = (leapStartPos.getX() > lockedPlayerPos.getX());
			velocity.x = returnRight ? (speed * 4.0f) : -(speed * 4.0f);
			lookingRight = (playerPos.getX() > pos.getX());

			if ((returnRight && pos.getX() >= leapStartPos.getX()) ||
				(!returnRight && pos.getX() <= leapStartPos.getX()))
			{
				velocity.x = 0;
			}
		}
		else
		{
			isSpecialAttacking = false;
			specialPhase = 0;
			specialAttackTimer.Start();
			this->damage = attackDamage;
			pbody->ctype = ColliderType::ENEMY;
			velocity.x = 0;
		}
	}
}

void Dip::Move() {

	// Si es empujado (knockback) o está muerto, no se mueve ni ataca
	if (isKnockedback || isdead) {
		// Cancelar ataque especial si es golpeado
		if (isSpecialAttacking) {
			isSpecialAttacking = false;
			specialPhase = 0;
			specialAttackTimer.Start();
			this->damage = attackDamage;
			pbody->ctype = ColliderType::ENEMY;
		}
		return;
	}

	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player == nullptr) return;

	Vector2D tilePos = GetTilePos();
	Vector2D pos = GetPosition();
	Vector2D playerPos = player->GetPosition();

	if (isTouchingPlayer)
	{
		velocity.x = 0;
		lookingRight = (playerPos.getX() > pos.getX());
		AttackPlayer();
		return;
	}

	if (!isSpecialAttacking && specialAttackTimer.ReadMSec() >= 6000 && playerTileDist <= vision)
	{
		isSpecialAttacking = true;
		specialPhase = 1;
		phaseTimer.Start();

		// Configurar daño a 1 y cambiar ctype a ATAQUE para el impacto
		this->damage = 1;
		pbody->ctype = ColliderType::ENEMY_ATTACK;

		Engine::GetInstance().physics->SetYVelocity(pbody, -6.0f); // Pequeño salto hacia atrás
	}

	if (isSpecialAttacking)
	{
		ExecuteSpecialAttack(playerPos);
		return;
	}

	float distX = std::abs(pos.getX() - playerPos.getX());
	float distY = std::abs(pos.getY() - playerPos.getY());

	// 2. Manejo cuando el array de pathfinding está vacío (Jugador lejos o no hay ruta)
	if (pathfinding->pathTiles.empty())
	{
		// Si está muy cerca pero el pathfinding está vacío, se acerca por fuerza bruta
		if (distX <= 256.0f && distY <= 128.0f)
		{
			if (playerPos.getX() > pos.getX()) {
				velocity.x = speed;
				lookingRight = true;
			}
			else {
				velocity.x = -speed;
				lookingRight = false;
			}
		}
		else
		{
			// ==========================================
			// MODIFICACIÓN: Lógica de Patrulla Simplificada
			// ==========================================
			anims.SetCurrent("Move");

			// Cambiar de dirección de forma estricta cada 4 segundos (4000 ms).
			// Evitamos comprobar si la velocidad es 0 para que no se buguee si el colisionador toca el suelo.
			if (patrolTimer.ReadMSec() > 4000)
			{
				lookingRight = !lookingRight; // Invertir dirección
				patrolTimer.Start(); // Reiniciar el temporizador
			}

			// Calcular la velocidad de patrulla (caminar más lento que cuando persigue)
			float patrolSpeed = speed * 0.4f;
			velocity.x = lookingRight ? patrolSpeed : -patrolSpeed;
		}
		return;
	}

	// 3. Movimiento normal siguiendo la ruta A*
	anims.SetCurrent("Move");
	if (pathfinding->pathTiles.back() == tilePos)
	{
		pathfinding->pathTiles.pop_back();
		if (pathfinding->pathTiles.empty()) { return; }
	}

	Vector2D nextTile = pathfinding->pathTiles.back();

	if (nextTile.getX() > tilePos.getX())
	{
		velocity.x = speed;
		lookingRight = true;
	}
	else if (nextTile.getX() < tilePos.getX())
	{
		velocity.x = -speed;
		lookingRight = false;
	}
	else
	{
		velocity.x = 0;
		anims.SetCurrent("Idle");
		lookingRight = (playerPos.getX() > pos.getX());
	}

	// Anti-atascos: Acelerar al encontrar un escalón para saltar
	if (pathfinding->IsWalkable(nextTile.getX(), nextTile.getY() + 1) && !pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 1))
	{
		velocity.x *= 5.0f;
	}
}

void Dip::AttackPlayer()
{
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	if (player == nullptr || isdead) return;

	if (player->godMode || player->isInvincible) {
		return;
	}

	// Frecuencia de ataque: cada 500 ms (2 veces por segundo)
	if (startAttack.ReadMSec() >= 500)
	{
		anims.SetCurrent("Attack");
		// 1. Reducir la salud del jugador
		player->currentHealth -= attackDamage;

		// Si la salud es <= 0, declarar al jugador muerto inmediatamente
		if (player->currentHealth <= 0)
		{
			player->isdead = true;
		}

		// 3. Reproducir efecto de chispas cuando el jugador es herido
		Engine::GetInstance().particleManager->EmitHitSparks(player->position.getX(), player->position.getY(), false);

		// 4. Activar el efecto visual rojo en sí mismo y reiniciar el temporizador
		isAttackingVisual = true;
		attackVisualTimer.Start();

		LOG("【DIP ATAQUE】Daño: %d. Salud Player: %d", attackDamage, player->currentHealth);

		startAttack.Start();
	}
}

void Dip::Knockback()
{
}

void Dip::ApplyPhysics() {
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	b2Vec2 newVel;
	newVel.x = velocity.x;
	newVel.y = currentVel.y;
	Engine::GetInstance().physics->SetLinearVelocity(pbody, newVel);
}

void Dip::Draw(float dt)
{
	if (isdead)
	{
	
		if (anims.GetCurrentName() != "Dead")
		{
			anims.SetCurrent("Dead");
			anims.GetAnim("Dead")->SetLoop(false);  
		}
		
		if (anims.GetAnim("Dead")->HasFinishedOnce())
		{
			
			pendingToDelete = true;
			active = false;
		}
	}

	anims.Update(dt);

	SDL_Rect animFrame = anims.GetCurrentFrame();
	SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	int drawX = x;
	int drawY = y;

	// ==========================================
	///DIBUJAR DEBUG DEL PATHFINDING (F9)
// ==========================================
	if (Engine::GetInstance().physics->GetDebug() && pathfinding != nullptr)
	{
		pathfinding->DrawPath();
	}

	if (isKnockedback || isAttackingVisual)
	{
		Uint8 r = 255, g = 25, b = 25;
		Engine::GetInstance().render->SetColorMod(texture, &r, &g, &b, 255, 25, 25);
		Engine::GetInstance().render->DrawRotatedTexture(texture, drawX, drawY, &animFrame, sdlFlip, 1);
		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, 255, 255, 255);
	}
	else
	{
		Engine::GetInstance().render->DrawRotatedTexture(texture, drawX, drawY, &animFrame, sdlFlip, 1);
	}


}

void Dip::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	case ColliderType::PLAYER_ATTACK:
		LOG("Dip hit! Damage = %d", physB->listener->damage);
		Entity::TakeDamage(physB->listener->damage);
		isKnockedback = true;
		anims.SetCurrent("Hit");

		if (currentHealth <= 0) {
			isdead = true;
			currentHealth = 0;
		}
		break;
	case ColliderType::PLAYER:

		playerContacts++;
		isTouchingPlayer = (playerContacts > 0);

		if (isSpecialAttacking)
		{
			Player* p = (Player*)physB->listener;
			if (p && p->pbody)
			{
				float pushForce = (p->position.getX() > position.getX()) ? 25.0f : -25.0f;
				Engine::GetInstance().physics->SetLinearVelocity(p->pbody, { pushForce, -8.0f });
			}
		}
		break;
	default:
		break;
	}
}

void Dip::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	if (physB->ctype == ColliderType::PLAYER)
	{
		playerContacts--;
		if (playerContacts <= 0)
		{
			playerContacts = 0; // Por seguridad, evitamos números negativos
			isTouchingPlayer = false;
		}
	}
}