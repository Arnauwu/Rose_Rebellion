#pragma once
#include "Enemy.h"
#include "Animation.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include "Pathfinding.h"
#include "Timer.h"

enum class NinfaMareState {
    SPAWNING,    // Secuencia inicial de burbujas
    IDLE,        // Respiración/Flotar
    CHASE,       // Perseguir al jugador
    ATTACK_SHOT, // Disparo normal (Homing)
    ATTACK_WAVE, // Ataque de ola (manos adelante)
    ATTACK_RAIN, // Ataque de lluvia (manos arriba)
    COOLDOWN,    // Descanso entre ataques
    DEAD,        // Estado de muerte
    HURT
};

class NinfaMare : public Enemy {
public:
    NinfaMare();
    virtual ~NinfaMare();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) override;

protected:
    void GetPhysicsValues() override;
    void Move() override;
    void Knockback() override;
    void ApplyPhysics() override;
    void Draw(float dt);

private:
    void PerformPathfinding();
    void ShootHomingProjectile();
    void LaunchWaterWave();
    void StartRainAttack();
    void HandleSpawningLogic();

public:
    NinfaMareState currentState;

    // Ajustes de Boss (Más grandes que la ninfa joven)
    float targetOffsetX = 220.0f;
    float targetOffsetY = 120.0f;
    float attackRange = 1000.0f;

    Timer rainTimer; // Temporizador para el ritmo de caída de la lluvia
    float rainDurationMs = 10000.0f;

    Timer stateTimer;
    float spawnDurationMs = 2000.0f;
    float shotCooldownMs = 800.0f;
    float specialAttackCooldownMs = 600.0f;

    float hurtDurationMs = 400.0f;
    float damageCooldown = 0.0f;
    NinfaMareState previousState;

    int shotsFiredInCombo = 0;       // Cuenta cuántos disparos lleva
    bool nextSpecialIsWave = true;

    bool hasAppeared = false;

    // Sonidos
    int volarFx, atacarFx, morirFx, gritoFx, waveFx;

    // Control de ataques
    bool canDoSpecial = true;
    Timer specialTimer;
};
