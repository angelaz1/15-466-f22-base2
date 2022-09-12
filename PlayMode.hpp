#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

private:
	// Bird animation references:
	Scene::Transform *left_leg = nullptr;
	Scene::Transform *right_leg = nullptr;
	Scene::Transform *left_wing = nullptr;
	Scene::Transform *right_wing = nullptr;
	Scene::Transform *beak = nullptr;
	glm::quat left_leg_base_rotation;
	glm::quat right_leg_base_rotation;
	glm::quat left_wing_base_rotation;
	glm::quat right_wing_base_rotation;

	// Main gameplay transforms to reference
	Scene::Transform *bird = nullptr;
	Scene::Transform *plane = nullptr;
	Scene::Transform *coin = nullptr;
	Scene::Transform *cloud = nullptr;
	float anim_time = 0.0f;

	// Gameplay variables
	// uint32_t score = 0;
	// uint8_t lives = 3;

	// const float max_plane_spawn_time = 2.0f;
	// const float min_plane_spawn_time = 5.0f;
	// const float max_coin_spawn_time = 2.0f;
	// const float min_coin_spawn_time = 5.0f;

	// float plane_spawn_timer;
	// float coin_spawn_timer;

	// Smth smth handle collisions
	
	//camera:
	Scene::Camera *camera = nullptr;
};
