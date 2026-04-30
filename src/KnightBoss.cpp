#include "KnightBoss.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include "Timer.h"
#include "Physics.h"

KnightBoss::KnightBoss() : Enemy(EntityType::KNIGHT_BOSS)
{
	name = "KnightBoss";
}

KnightBoss::~KnightBoss() {

}

bool KnightBoss::Awake() {
	return true;
}

bool KnightBoss::Start() {
	std::unordered_map<int, std::string> aliases = { {0,"dead"},{16,"defend"},{24,"walk"},{32,"attack"},{48,"idle"},{56,"assault"} };
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Knight/Knight.tsx", aliases);
	anims.SetCurrent("idle");

	// Initialize parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Knight/Knight.png");

	// Añadir físicas al enemigo - hitbox más grande para un boss
	texW = 256;
	texH = 256;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 4, bodyType::DYNAMIC);

	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	// Inicializar pathfinding
	pathfinding = std::make_shared<Pathfinding>(true);
	pathfinding->ResetPath(GetTilePos());
	pathFindingCooldown.Start();

	// Estadísticas del Boss
	vision = 15;
	speed = 1.5f;
	knockbackForce = 1.0f;

	maxHealth = 200;
	currentHealth = maxHealth;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	return true;
}

bool KnightBoss::Update(float dt)
{
	if (!active) return true;

	if (Engine::GetInstance().sceneManager->isGamePaused == false && isdead == false)
	{
		if (pathFindingCooldown.ReadMSec() > 500)
		{
			PerformPathfinding();
			pathFindingCooldown.Start();
		}

		GetPhysicsValues();
		Move();
		Knockback();
		ApplyPhysics();
	}

	if (isdead && anims.GetCurrentName() != "dead")
	{
		Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });
		anims.GetAnim("dead")->SetLoop(false);
		anims.SetCurrent("dead");
		pbody->ctype = ColliderType::UNKNOWN;

		// Crear orbes como recompensa al vencer al boss
		for (int i = 0; i < 3; i++) {
			std::shared_ptr<Entity> healthOrb = Engine::GetInstance().entityManager->CreateEntity(EntityType::HEALTH_ORB);
			healthOrb->position.setX(this->position.getX() + (i * 100));
			healthOrb->position.setY(this->position.getY());
			healthOrb->Start();
		}
	}

	if (anims.GetAnim("dead")->HasFinishedOnce())
	{
		pendingToDelete = true;
	}

	Draw(dt);

	return true;
}

void KnightBoss::PerformPathfinding()
{
	pathfinding->ResetPath(GetTilePos());
	Vector2D pos = GetPosition();
	Player* player = Engine::GetInstance().entityManager->GetPlayer();
	Vector2D playerPos = player->GetPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 128;
	int iter = 0;

	while (pathfinding->pathTiles.empty() && playerTileDist < vision && iter < MaxIterations)
	{
		pathfinding->PropagateAStar();
		iter++;
	}
}

void KnightBoss::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0, velocity.y };
}

void KnightBoss::Move() 
{

	Vector2D tilePos = GetTilePos();

	// 0. Si está descansando (3 segundos)
	if (isResting) {
		velocity.x = 0; // No se mueve
		anims.SetCurrent("idle"); // Puedes poner una animación de "cansado" aquí si la tienes

		if (restTimer.ReadMSec() >= restDuration) {
			isResting = false; // Termina el descanso
			attackStep = 0;    // Reinicia el combo al primer ataque
		}
		return;
	}

	// 1. Si está dando un espadazo
	if (isAttacking) {
		SwordAttack();
		return;
	}

	// 2. Si está en medio de una embestida
	if (isDashing) {
		ShieldDash();
		return;
	}

	// 3. Tomar decisión si estás muy cerca
	if (playerTileDist <= 2 && !isKnockedback) {

		// Mirar hacia el jugador
		Player* player = Engine::GetInstance().entityManager->GetPlayer();
		Vector2D playerPos = player->GetPosition();
		lookingRight = (playerPos.getX() > position.getX());

		// EJECUTAR EL PASO DEL COMBO CORRESPONDIENTE
		if (attackStep == 0 || attackStep == 1) {
			isAttacking = true;
			startAttack.Start();
			SwordAttack();
		}
		else if (attackStep == 2) {
			isDashing = true;
			dashTimer.Start();
			ShieldDash();
		}
		return;
	}

	// 4. Movimiento de persecución normal (si estás lejos)
	if (pathfinding->pathTiles.empty() && isKnockedback == false)
	{
		anims.SetCurrent("idle");
		velocity.x = 0;
		return;
	}
	else if (playerTileDist >= 3 && playerTileDist < vision && isKnockedback == false)
	{
		anims.SetCurrent("walk");

		if (pathfinding->pathTiles.back() == tilePos) {
			pathfinding->pathTiles.pop_back();
			if (pathfinding->pathTiles.empty()) { return; }
		}

		Vector2D nextTile = pathfinding->pathTiles.back();

		if (nextTile.getX() > tilePos.getX()) {
			velocity.x = speed;
			lookingRight = true;
		}
		else if (nextTile.getX() < tilePos.getX()) {
			velocity.x = -speed;
			lookingRight = false;
		}
		else {
			velocity.x = 0;
		}

		if (pathfinding->IsWalkable(nextTile.getX(), nextTile.getY() + 1) && !pathfinding->IsWalkable(tilePos.getX(), tilePos.getY() + 1)) {
			velocity.x *= 5;
		}
	}
}

void KnightBoss::Knockback()
{
	if (isdead) return;

	if (isKnockedback)
	{
		isAttacking = false;
		anims.SetCurrent("hurt");
		if (lookingRight)
		{
			velocity.x = knockbackForce;
		}
		else
		{
			velocity.x = -knockbackForce;
		}
	}

	if (knockbackTime <= 0)
	{
		isKnockedback = false;
		knockbackTime = 300.0f; // Menos tiempo aturdido por ser Boss
	}
	else
	{
		knockbackTime -= Engine::GetInstance().GetDt();
	}
}

void KnightBoss::ApplyPhysics() {
	b2Vec2 currentVel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { velocity.x, currentVel.y });
}

void KnightBoss::Draw(float dt)
{
	if (Engine::GetInstance().sceneManager->isGamePaused == false)
	{
		anims.Update(dt);
	}
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	//SDLFlip
	SDL_FlipMode sdlFlip = SDL_FLIP_NONE;
	if (!lookingRight)
	{
		sdlFlip = SDL_FLIP_HORIZONTAL;
	}

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (Engine::GetInstance().physics->GetDebug())
	{
		pathfinding->DrawPath();
	}

	Engine::GetInstance().render->DrawRotatedTexture(texture, x, y - animFrame.h / 3, &animFrame, sdlFlip, 1.75f);
}

void KnightBoss::SwordAttack()
{
	if (isAttacking) {
		anims.SetCurrent("attack");
		velocity.x = 0;
		damage = 20;

		// 1. CREAR HITBOX: En el momento del impacto visual (ej: a los 500ms)
		if (startAttack.ReadMSec() > 500 && swordHitbox == nullptr) {

			// Calculamos dónde aparece la espada dependiendo de a dónde mire
			int attW = 120,  attH = 160;
			int hX = lookingRight ? position.getX() + texW /2: position.getX() - texW / 2;
			int hY = position.getY();

			// Creamos un rectángulo físico
			swordHitbox = Engine::GetInstance().physics->CreateRectangle(hX, hY, attW, attH, bodyType::KINEMATIC);
			swordHitbox->listener = this;
			swordHitbox->ctype = ColliderType::ENEMY_ATTACK;

			// (Línea de SetSensor eliminada por incompatibilidad con Box2D v3)
		}

		// 2. DESTRUIR HITBOX: Al terminar el ataque
		if (startAttack.ReadMSec() >= attackCooldown) 
		{
			isAttacking = false;
			anims.SetCurrent("idle");
			attackStep++;

			// Si la espada existe, la borramos del mundo de físicas
			if (swordHitbox != nullptr) {
				// SOLUCIÓN BOX2D V3:
				b2DestroyBody(swordHitbox->body);
				swordHitbox = nullptr; // Lo volvemos a poner a null para el próximo ataque
			}
		}
	}
}

void KnightBoss::ShieldDash() // TO DO: MAKE IT WORK (doesnt work because pbody collider)
{
	if (isDashing) {
		anims.SetCurrent("walk");
		damage = 0; // Solo empuja, no hace daño

		// Sale disparado hacia donde mira
		velocity.x = lookingRight ? (speed * 3.5f) : (-speed * 3.5f);

		// Terminar embestida
		if (dashTimer.ReadMSec() >= dashCooldown) {
			isDashing = false;
			damage = 10; // Restaurar el daño normal
			anims.SetCurrent("idle");

			// Al terminar la embestida, se cansa por 3 segundos
			isResting = true;
			restTimer.Start();
		}
	}
}

void KnightBoss::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {
	if (physA == swordHitbox)
	{
		return;
	}


	switch (physB->ctype)
	{
	case ColliderType::MAP:
	case ColliderType::PLAYER:
	case ColliderType::ENEMY:
		// Se puede frenar aquí el ataque si colisiona
		break;

	case ColliderType::PLAYER_ATTACK:
		TakeDamage(physB->listener->damage);
		isKnockedback = true;
		break;

	default:
		break;
	}
}

void KnightBoss::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB)
{
	switch (physB->ctype)
	{
	case ColliderType::MAP:
		break;
	default:
		break;
	}
}

bool KnightBoss::CleanUp() 
{
	active = false;
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	if (swordHitbox != nullptr) {
		b2DestroyBody(swordHitbox->body);
		swordHitbox = nullptr;
	}
	return true;
}
