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

KnightBoss::KnightBoss() : Enemy(EntityType::KNIGHT_BOSS) // ASIGNAR TU TIPO DE ENTIDAD PARA EL BOSS SI LO TIENES EN EL ENUM
{
	name = "KnightBoss";
}

KnightBoss::~KnightBoss() {

}

bool KnightBoss::Awake() {
	return true;
}

bool KnightBoss::Start() {
	// Mapeamos los nombres que necesita el Boss a las filas REALES que tiene la Ninfa
	std::unordered_map<int, std::string> aliases = {
		{0,  "idle"},   // Fila 0: Idle de la Ninfa
		{3,  "walk"},   // Fila 3: Animación de vuelo de la Ninfa (lo usamos para caminar)
		{8,  "attack"}, // Fila 8: Ataque de la Ninfa
		{0,  "hurt"},   // La Ninfa no tiene animación de daño, repetimos idle para que no desaparezca
		{16, "dead"}    // Fila 16: Muerte de la Ninfa
	};

	// CAMBIO: Apuntamos al .tsx de la Ninfa
	anims.LoadFromTSX("Assets/Textures/Entities/Enemies/Ninfa/Ninfa.tsx", aliases);
	anims.SetCurrent("idle");

	// CAMBIO: Inicializar la textura usando el .png de la Ninfa
	texture = Engine::GetInstance().textures->Load("Assets/Textures/Entities/Enemies/Ninfa/Ninfa.png");

	// Añadir físicas al enemigo - hitbox más grande para un boss
	texW = 64;
	texH = 64;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);

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

	maxHealth = 100;
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
			healthOrb->position.setX(this->position.getX() + (i * 20));
			healthOrb->position.setY(this->position.getY() - 100);
			healthOrb->Start();
		}
	}

	Draw(dt);

	return true;
}

void KnightBoss::PerformPathfinding()
{
	pathfinding->ResetPath(GetTilePos());
	Vector2D pos = GetPosition();
	Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();

	playerTileDist = sqrt(pos.distanceSquared(playerPos)) / 32;
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

void KnightBoss::Move() {

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
	if (playerTileDist <= 3 && !isKnockedback) {

		// Mirar hacia el jugador
		Vector2D playerPos = Engine::GetInstance().sceneManager->GetPlayerPosition();
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
			lookingRight = !true;
		}
		else if (nextTile.getX() < tilePos.getX()) {
			velocity.x = -speed;
			lookingRight = !false;
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

	// MODIFICADO: Invertimos el flip para que coincida con el asset de la Ninfa
	SDL_FlipMode sdlFlip = lookingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	if (Engine::GetInstance().physics->GetDebug())
	{
		pathfinding->DrawPath();
	}

	Engine::GetInstance().render->DrawRotatedTexture(texture, x - texW / 2, y - animFrame.h / 6, &animFrame, sdlFlip, 1.0);
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
			int hX = lookingRight ? position.getX() + 70 : position.getX() - 70;
			int hY = position.getY();

			// Creamos un rectángulo físico
			swordHitbox = Engine::GetInstance().physics->CreateRectangle(hX, hY, 80, 40, bodyType::KINEMATIC);
			swordHitbox->listener = this;
			swordHitbox->ctype = ColliderType::ENEMY;

			// (Línea de SetSensor eliminada por incompatibilidad con Box2D v3)
		}

		// 2. DESTRUIR HITBOX: Al terminar el ataque
		if (startAttack.ReadMSec() >= attackCooldown) {
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

void KnightBoss::ShieldDash()
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

			// NUEVO: Al terminar la embestida, se cansa por 3 segundos
			isResting = true;
			restTimer.Start();
		}
	}
}

void KnightBoss::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	case ColliderType::WALL:
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

void KnightBoss::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::WALL:
		break;
	default:
		break;
	}
}

bool KnightBoss::CleanUp() {
	if (swordHitbox != nullptr) {
		b2DestroyBody(swordHitbox->body);
		swordHitbox = nullptr;
	}
	return true;
}
