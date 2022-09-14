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
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

private:
	// Camera:
	Scene::Camera *camera = nullptr;

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
	Scene::Transform *bird_collider = nullptr;
	float anim_time = 0.0f;

	// Gameplay variables
	uint32_t score = 0;
	uint8_t lives = 3;

	// Spawning
	typedef struct {
		float max_spawn_time;    		// Max time between spawns
		float min_spawn_time;    		// Min time between spawns
		float spawn_time;		 		// Time before next spawn
		float spawn_timer;		 		// Timer until next spawn
		bool spawned;		 	 		// Whether object is spawned
		float max_speed;		 		// Max speed of object
		float min_speed;		 		// Min speed of object
		float speed;		 	 		// Speed of object when spawned
		Scene::Transform *transform;	// Reference to transform to spawn
		Scene::Transform *collider;		// Reference to the collider of the object
	} object_info;

	std::vector<object_info*> plane_infos;
	std::vector<object_info*> coin_infos;
	std::vector<object_info*> cloud_infos;

	const float min_x = -10;
	const float max_x = 30;
	const float min_y = -2.5f;
	const float max_y = 2.5f;
	const float min_z = -1.5f;
	const float max_z = 1.5f;

	/// Functions ///

	// Generates a random float between lo and hi
	float rand_float(float lo, float hi);

	// Checks for collision between the two transforms
	bool collision_check(Scene::Transform *parent1, Scene::Transform *collider1, 
						 Scene::Transform *parent2, Scene::Transform *collider2);

	// Processes the object info for the given elapsed time
	void process_object(object_info *info, float elapsed, bool *collision);

	// Ends the game
	void end_game();
};
