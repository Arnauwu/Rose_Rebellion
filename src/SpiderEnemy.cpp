#include "SpiderEnemy.h"
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


SpiderEnemy::SpiderEnemy() : Enemy(EntityType::ENEMY) {
    name = "spider";
}

SpiderEnemy::~SpiderEnemy() {}

bool SpiderEnemy::Awake() {
    return true;
}

bool SpiderEnemy::Start() {

    std::unordered_map<int, std::string> spiderr = { {0,"idle"},{11,"move"},{22,"jump"} };
    anims.LoadFromTSX("Assets/Textures/PLayer2_Spritesheet.tsx", spiderr);
    anims.SetCurrent("idle");

    // Initialize Player parameters
    texture = Engine::GetInstance().textures->Load("Assets/Textures/player2_spritesheet.png");

    //Load Audio


   //Add physics to the enemy - initialize physics body
    texW = 32; texH = 32;
    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX() + texW / 2, (int)position.getY() + texH / 2, (texW * 2) / 5, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::ENEMY;
    b2Body_SetGravityScale(pbody->body, 0.0f);

    //Stats
    speed = 2.0f;

    return true;
}

bool SpiderEnemy::Update(float dt) {
    
    if (!active) return true;

    if (Engine::GetInstance().sceneManager->isGamePaused == false)
    {
        GetPhysicsValues();
        Move();
		Knockback();
        ApplyPhysics();
    }

    Draw(dt);

    if (Engine::GetInstance().sceneManager->isGamePaused == false)
    {
        if (isStuck) {
            time += dt;
            if (time > 100) {
                isStuck = false;
                time = 0;
            }
	    }
    }

    return true;
}

void SpiderEnemy::GetPhysicsValues() {
    // Read current velocity
    velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
}

void SpiderEnemy::Move() {
    
    float stickyForce = 3.5f;

    switch (currentFacing) {
    case Facing::DOWN:  // Walking on the floor
        velocity.x = speed;
        velocity.y = stickyForce;
        angle = 0.0f;
        break;
    case Facing::LEFT:  // Walking on the left wall
        velocity.x = stickyForce;
        velocity.y = -speed;
        angle = -90.0f;
        break;
    case Facing::UP:    // Walking on the ceiling
        velocity.x = -speed;
        velocity.y = -stickyForce;
        angle = 180.0f;
        break;
    case Facing::RIGHT: // Walking on the right wall
        velocity.x = -stickyForce;
        velocity.y = speed;
        angle = 90.0f;
        break;
    }
}

void SpiderEnemy::Knockback()
{
    if (isdead) return;

    if (isKnockedback)
    {
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
        knockbackTime = 500.0f;
    }
    else
    {
        knockbackTime -= Engine::GetInstance().GetDt();
    }
}

void SpiderEnemy::RotateFacing() {
    // Ciclo de rotación: DOWN -> LEFT -> UP -> RIGHT -> DOWN
    int next = (int)currentFacing + 1;
    if (next > 3) next = 0;
    currentFacing = (Facing)next;
    LOG("Outer corner detected! New facing: %d", currentFacing);
}

void SpiderEnemy::ApplyPhysics() {

    // Apply velocity via helper
    Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void SpiderEnemy::Draw(float dt)
{
    if (Engine::GetInstance().sceneManager->isGamePaused == false)
    {
        anims.Update(dt);
    }
    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    // Update render position using your PhysBody helper
    int x, y;
    pbody->GetPosition(x, y);

    // Draw pathfinding debug
    if (Engine::GetInstance().physics->GetDebug())
    {
        b2Vec2 pos = b2Body_GetPosition(pbody->body);
        float rayLength = 0.8f;
        float aheadOffset = 0.6f;
        b2Vec2 rayEnd;

        // Raycast
       /* switch (currentFacing) {
        case Facing::DOWN:  rayEnd = { pos.x + aheadOffset, pos.y - rayLength }; break;
        case Facing::LEFT:  rayEnd = { pos.x + rayLength, pos.y + aheadOffset }; break;
        case Facing::UP:    rayEnd = { pos.x - aheadOffset, pos.y + rayLength }; break;
        case Facing::RIGHT: rayEnd = { pos.x - rayLength, pos.y - aheadOffset }; break;
        }

        Engine::GetInstance().render->DrawLine(
            METERS_TO_PIXELS(pos.x), METERS_TO_PIXELS(pos.y),
            METERS_TO_PIXELS(rayEnd.x), METERS_TO_PIXELS(rayEnd.y),
            255, 0, 0
        );*/
    }

    //Draw the player using the texture and the current animation frame
    Engine::GetInstance().render->DrawRotatedTexture(texture, x, y, &animFrame, SDL_FLIP_NONE, 1.0f, angle, 16, 16);
}

Vector2D SpiderEnemy::GetTilePos()
{
    //Get the position of the enemy
    Vector2D pos = GetPosition();
    //Convert to tile coordinates
    Vector2D tilePos = Engine::GetInstance().map->WorldToMap((int)pos.getX(), (int)pos.getY());
    return tilePos;
}

void SpiderEnemy::OnCollision(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {

	
    switch (physB->ctype)
    {
   // case ColliderType::MAP: //TO DO FIX
   //     if (velocity.y >= 0 && rule == 1 && !isStuck) {
   //         currentFacing = Facing::RIGHT;
			//isStuck = true;
   //         LOG("Spider rotating to WALL_RIGHT %d", rule);
   //         rule = 2;
   //     }
   //     if (velocity.y <= 0 && rule == 3 && !isStuck) {
   //         currentFacing = Facing::LEFT;
   //         isStuck = true;
   //         LOG("Spider rotating to WALL_LEFT %d", rule);
   //         rule = 0;
   //     }
   //     break;
   // case ColliderType::MAP:
   //     if (rule == 0 && !isStuck) {
   //     currentFacing = Facing::DOWN;
   //     isStuck = true;
   //     LOG("Spider rotating to GROUND %d", rule);
   //     rule = 1;
   //     }
   //     break;
   // case ColliderType::MAP:
   //     if (rule == 2 && !isStuck) {
   //     currentFacing = Facing::UP;
   //     isStuck = true;
   //     LOG("Spider rotating to CEILING %d", rule);
   //     rule = 3;
   //     }
   //     break;
    default:
        //currentFacing = Facing::DOWN;
        break;
    }
}

void SpiderEnemy::OnCollisionEnd(PhysBody* physA, PhysBody* physB, b2ShapeId shapeA, b2ShapeId shapeB) {}
