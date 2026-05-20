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
	// Initialize enemy parameters
	std::unordered_map<int, std::string> aliases = { {0,"Attack"},{22,"Dead"},{44,"Hit"},{66,"Still"},{88,"Idle"},{110,"Move"},{132,"Jump"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Dip/SS_Dip.tsx", aliases);
	anims.SetCurrent("Idle");

	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Dip/SS_Dip.png");

	texW = 64;
	texH = 64;
	// Parámetros: (X centro, Y centro, Ancho, Alto, Tipo)
	pbody = Engine::GetInstance().physics->CreateRectangle(
		(int)position.getX() + texW / 2,
		(int)position.getY() + texH / 2, // Centrado perfectamente en Y
		50, // Ancho (Width): Más largo para cubrir hocico y cola
		30, // Alto (Height): Más bajito para el lomo del lobo
		bodyType::DYNAMIC
	);	
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	vision = 15;
	speed = 8.0f;
	knockbackForce = 5.0f;
	maxHealth = 20;
	currentHealth = 20;

	attackDamage = 5;
	startAttack.Start();

	
	attackVisualTimer.Start();
	isAttackingVisual = false;

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

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
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

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void Dip::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
}
//void Dip::ExecuteSpecialAttack(Vector2D playerPos)
//{
//	Vector2D pos = GetPosition();
//
//	if (specialPhase == 1)
//	{
//		// Fase 1: Saltar lejos del jugador
//		if (phaseTimer.ReadMSec() < 400)
//		{
//			anims.SetCurrent("Jump");
//			velocity.x = (pos.getX() > playerPos.getX()) ? speed * 4.0f : -speed * 4.0f;
//			lookingRight = (playerPos.getX() > pos.getX()); // Mirar al jugador mientras retrocede
//		}
//		else
//		{
//			// Fin de la fase 1, grabar posiciones y saltar hacia el jugador
//			leapStartPos = GetPosition();
//			lockedPlayerPos = playerPos;
//			specialPhase = 2;
//			phaseTimer.Start();
//			Engine::GetInstance().physics->SetYVelocity(pbody, -8.0f); // Salto vertical
//		}
//	}
//	else if (specialPhase == 2)
//	{
//		// Fase 2: Saltar hacia la posición bloqueada del jugador
//		if (phaseTimer.ReadMSec() < 500)
//		{
//			anims.SetCurrent("Jump");
//			velocity.x = (lockedPlayerPos.getX() > pos.getX()) ? speed * 7.0f : -speed * 7.0f;
//			lookingRight = (velocity.x > 0);
//		}
//		else
//		{
//			// Fin de la fase 2, saltar de vuelta al inicio
//			specialPhase = 3;
//			phaseTimer.Start();
//			Engine::GetInstance().physics->SetYVelocity(pbody, -8.0f);
//		}
//	}
//	else if (specialPhase == 3)
//	{
//		// Fase 3: Saltar de vuelta a la posición inicial (leapStartPos)
//		if (phaseTimer.ReadMSec() < 500)
//		{
//			anims.SetCurrent("Jump");
//			velocity.x = (leapStartPos.getX() > pos.getX()) ? speed * 7.0f : -speed * 7.0f;
//			lookingRight = (playerPos.getX() > pos.getX());
//		}
//		else
//		{
//			// Fin del ataque especial. Restaurar el comportamiento normal
//			isSpecialAttacking = false;
//			specialPhase = 0;
//			specialAttackTimer.Start();
//
//			this->damage = attackDamage; // Restaurar el daño normal
//			pbody->ctype = ColliderType::ENEMY;
//			velocity.x = 0;
//		}
//	}
//}
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
	//if (isKnockedback || isdead) return;
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
	// [Corrección principal 1: Detección rectangular] Dividir la distancia en valores absolutos X e Y
	float distX = std::abs(pos.getX() - playerPos.getX());
	float distY = std::abs(pos.getY() - playerPos.getY());

	// Si la distancia X (ej. 75) y la Y (ej. 85) son cercanas, ¡es un golpe cuerpo a cuerpo perfecto! 
	// Ya no le afectan las pequeñas diferencias de altura
	if (distX <= 90.0f && distY <= 85.0f)
	{
		velocity.x = 0; // Detenerse frente al jugador
		lookingRight = (playerPos.getX() > pos.getX()); // Mirar fijamente al jugador
		AttackPlayer(); // ¡Atacar con la garra!
		return;
	}

	// 2. Manejo cuando el array de pathfinding está vacío
	if (pathfinding->pathTiles.empty())
	{
		// Cuando llega al mismo tile que el jugador, el pathfinding termina (se vacía).
		// Pero si aún no está en rango de ataque (el if de arriba no saltó), 
		// ignora el tile y se acerca a la fuerza en el eje X.
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
			velocity.x = 0; // Se queda quieto solo si está realmente lejos y no hay ruta
			anims.SetCurrent("Idle");
	
			lookingRight = (playerPos.getX() > pos.getX());
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
	anims.Update(dt);

	SDL_Rect animFrame = anims.GetCurrentFrame();

	SDL_FlipMode sdlFlip = SDL_FLIP_NONE;
	if (lookingRight)
	{
		sdlFlip = SDL_FLIP_HORIZONTAL;
	}

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Calculamos el centro correcto
	int drawX = x - (animFrame.w / 2);
	int drawY = y - (animFrame.h / 2);

	if (isKnockedback || isAttackingVisual)
	{
		Uint8 r = 255, g = 25, b = 25;
		Engine::GetInstance().render->SetColorMod(texture, &r, &g, &b, 255, 25, 25);

		
		Engine::GetInstance().render->DrawRotatedTexture(texture, drawX, drawY, &animFrame, sdlFlip, 1);

		Uint8 defR = 255, defG = 255, defB = 255;
		Engine::GetInstance().render->SetColorMod(texture, nullptr, nullptr, nullptr, defR, defG, defB);
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
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
		anims.SetCurrent("Hit");
		break;
	case ColliderType::PLAYER:
		// [Nuevo] Si toca al jugador MIENTRAS hace el ataque especial
		if (isSpecialAttacking)
		{
			Player* p = (Player*)physB->listener;
			if (p && p->pbody)
			{
				// Aplicar un empuje FUERTE (aproximadamente 2 casillas de distancia)
				float pushForce = (p->position.getX() > position.getX()) ? 25.0f : -25.0f;
				Engine::GetInstance().physics->SetLinearVelocity(p->pbody, { pushForce, -8.0f });

				// Nota: El daño de 1 se aplica automáticamente porque 
				// configuramos pbody->ctype = ColliderType::ENEMY_ATTACK
				// y this->damage = 1 al inicio del ataque especial.
			}
		}
		break;
	default:
		break;
	}
}

void Dip::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
}