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
    DEAD         // Estado de muerte
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
    float targetOffsetX = 180.0f;
    float targetOffsetY = 120.0f;
    float attackRange = 500.0f;

    Timer stateTimer;
    float spawnDurationMs = 3000.0f;
    float shotCooldownMs = 2000.0f;
    float specialAttackCooldownMs = 6000.0f;

    // Sonidos
    int volarFx, atacarFx, morirFx, gritoFx, waveFx;

    // Control de ataques
    bool canDoSpecial = true;
    Timer specialTimer;
};
