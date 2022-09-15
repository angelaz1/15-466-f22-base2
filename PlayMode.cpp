#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint game_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > bird_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("bird.pnct"));
	game_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > bird_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("bird.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = bird_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = game_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	});
});

PlayMode::PlayMode() : scene(*bird_scene) {
	// Get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	std::map<Scene::Transform*, object_info*> mapping;

	// Get pointers to meshes:
	Scene::Transform *hemi_light_pos = nullptr;
	for (auto &transform : scene.transforms) {
		if (transform.name == "leftFoot") left_leg = &transform;
		else if (transform.name == "rightFoot") right_leg = &transform;
		else if (transform.name == "leftWing") left_wing = &transform;
		else if (transform.name == "rightWing") right_wing = &transform;
		else if (transform.name == "beak") beak = &transform;
		else if (transform.name == "bird") bird = &transform;
		else if (transform.name == "birdCollider") bird_collider = &transform;
		else if (transform.name == "hemi_light") hemi_light_pos = &transform;
		else if (transform.name.find("_plane") != std::string::npos) {
			// Create a plane
			Scene::Transform *plane = &transform;
			plane->position = camera->transform->position + glm::vec3(max_x, 0, 0);
			plane->scale = glm::vec3(0.5f, 0.5f, 0.5f);

			// Initializing fields in info
			object_info *plane_info = new object_info;
			plane_info->max_spawn_time = 7;
			plane_info->min_spawn_time = 1;
			plane_info->spawn_time = rand_float(plane_info->min_spawn_time, plane_info->max_spawn_time);
			plane_info->spawn_timer = 0;
			plane_info->spawned = false;
			plane_info->max_speed = 12;
			plane_info->min_speed = 5;
			plane_info->transform = plane;
			plane_info->collider = nullptr;

			plane_infos.push_back(plane_info);
			mapping[plane] = plane_info;
		} else if (transform.name.find("_coin") != std::string::npos) {
			// Create a coin
			Scene::Transform *coin = &transform;
			coin->position = camera->transform->position + glm::vec3(max_x, 0, 0);
			coin->scale = glm::vec3(0.5f, 0.5f, 0.5f);

			// Initializing fields in info
			object_info *coin_info = new object_info;
			coin_info->max_spawn_time = 5;
			coin_info->min_spawn_time = 1;
			coin_info->spawn_time = rand_float(coin_info->min_spawn_time, coin_info->max_spawn_time);
			coin_info->spawn_timer = 0;
			coin_info->spawned = false;
			coin_info->max_speed = 9;
			coin_info->min_speed = 6;
			coin_info->transform = coin;
			coin_info->collider = nullptr;

			coin_infos.push_back(coin_info);
			mapping[coin] = coin_info;
		} else if (transform.name.find("_cloud") != std::string::npos) {
			// Create a cloud
			Scene::Transform *cloud = &transform;
			cloud->position = camera->transform->position + glm::vec3(max_x, 0, 0);
			cloud->scale = glm::vec3(0.5f, 0.5f, 0.5f);

			// Initializing fields in info
			object_info *cloud_info = new object_info;
			cloud_info->max_spawn_time = 10;
			cloud_info->min_spawn_time = 2;
			cloud_info->spawn_time = rand_float(cloud_info->min_spawn_time, cloud_info->max_spawn_time);
			cloud_info->spawn_timer = 0;
			cloud_info->spawned = false;
			cloud_info->max_speed = 5;
			cloud_info->min_speed = 3;
			cloud_info->transform = cloud;
			cloud_info->collider = nullptr;

			cloud_infos.push_back(cloud_info);
			mapping[cloud] = cloud_info;
		}
	}

	// Second pass for colliders
	for (auto &transform : scene.transforms) {
		if (transform.name.find("planeCollider") != std::string::npos) {
			Scene::Transform *plane_collider = &transform;
			mapping[plane_collider->parent]->collider = plane_collider;
		} else if (transform.name.find("coinCollider") != std::string::npos) {
			Scene::Transform *coin_collider = &transform;
			mapping[coin_collider->parent]->collider = coin_collider;
		}
	}

	if (left_leg == nullptr) throw std::runtime_error("Left Leg not found.");
	if (right_leg == nullptr) throw std::runtime_error("Right Leg not found.");
	if (left_wing == nullptr) throw std::runtime_error("Left Wing not found.");
	if (right_wing == nullptr) throw std::runtime_error("Right Wing not found.");
	if (beak == nullptr) throw std::runtime_error("Beak not found.");
	if (bird == nullptr) throw std::runtime_error("Bird not found.");
	if (bird_collider == nullptr) throw std::runtime_error("Bird collider not found.");

	left_leg_base_rotation = left_leg->rotation;
	right_leg_base_rotation = right_leg->rotation;
	left_wing_base_rotation = left_wing->rotation;
	right_wing_base_rotation = right_wing->rotation;

	// Beak rotation ends up being off due to export issues - correct it here
	beak->rotation = glm::quat(0.646011f, -0.700926f, -0.1464357f, 0.131397f);

	// Set bird position + rotation
	bird->position = camera->transform->position + glm::vec3(7, 0, 0);
	bird->scale = glm::vec3(0.8f, 0.8f, 0.8f);

	// Add lighting
	if (hemi_light_pos != nullptr) {
		Scene::Light light = Scene::Light(hemi_light_pos);
		light.type = light.Hemisphere;
		light.energy = glm::vec3(0.95f, 0.9f, 0.9f);
		scene.lights.push_back(light);
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed += 1;
			return true;
		}
	}

	return false;
}

float PlayMode::rand_float(float lo, float hi) {
	// Referenced StackOverflow https://stackoverflow.com/questions/5289613/generate-random-float-between-two-floats
	float rand_val = ((float) rand()) / (float) RAND_MAX; // Generates random between 0 and 1
	float diff = hi - lo;
	return lo + diff * rand_val;
}

bool PlayMode::collision_check(Scene::Transform *parent1, Scene::Transform *collider1, 
							   Scene::Transform *parent2, Scene::Transform *collider2) {
	glm::vec3 pos1 = parent1->position + collider1->position;
	glm::vec3 scale1 = parent1->scale * collider1->scale;
	glm::vec3 pos2 = parent2->position + collider2->position;
	glm::vec3 scale2 = parent2->scale * collider2->scale;

	if (pos1.x - scale1.x/2 > pos2.x + scale2.x/2 || pos1.x + scale1.x/2 < pos2.x - scale2.x/2) {
		return false;
	}

	if (pos1.y - scale1.y/2 > pos2.y + scale2.y/2 || pos1.y + scale1.y/2 < pos2.y - scale2.y/2) {
		return false;
	}

	if (pos1.z - scale1.z/2 > pos2.z + scale2.z/2 || pos1.z + scale1.z/2 < pos2.z - scale2.z/2) {
		return false;
	}

	return true;
}

void PlayMode::process_object(object_info *info, float elapsed, bool *collision) {
	if (info->spawned) {
		glm::vec3 forward = glm::vec3(-1, 0, 0);
		info->transform->position += info->speed * elapsed * forward;

		if (info->transform->position.x <= min_x) {
			info->transform->position.x = max_x;
			info->spawned = false;
			info->spawn_timer = 0;
			info->spawn_time = rand_float(info->min_spawn_time, info->max_spawn_time);
		}

		if (info->collider == nullptr) return;
		if (collision_check(info->transform, info->collider, bird, bird_collider)) {
			if (collision != nullptr) *collision = true;
			info->transform->position.x = max_x;
			info->spawned = false;
			info->spawn_timer = 0;
			info->spawn_time = rand_float(info->min_spawn_time, info->max_spawn_time);
		}
	}
	else {
		info->spawn_timer += elapsed;
		if (info->spawn_timer >= info->spawn_time) {
			// Spawn object
			info->transform->position.y = camera->transform->position.y + rand_float(min_y, max_y);
			info->transform->position.z = camera->transform->position.z + rand_float(min_z, max_z);
			info->speed = rand_float(info->min_speed, info->max_speed);
			info->spawned = true;
		}
	}
}

void PlayMode::end_game() {
	std::cout << "\n------------------------------\n";
	std::cout << "Thanks for playing Flappy Goose!\n";
	std::cout << "Your score was: " << score << "\n";
	std::cout << "------------------------------\n\n";
	Mode::set_current(nullptr);
}

void PlayMode::update(float elapsed) {
	// Slowly rotates through [0,1):
	if (bird_vel_y > 0) {
		wing_anim_time += elapsed / 2.0f;
		wing_anim_time -= std::floor(wing_anim_time);

		left_wing->rotation = left_wing_base_rotation * glm::angleAxis(
			glm::radians(13.0f * std::sin(wing_anim_time * 8.0f * 2.0f * float(M_PI))),
			glm::vec3(1.0f, 0.0f, 0.0f)
		);
		right_wing->rotation = right_wing_base_rotation * glm::angleAxis(
			glm::radians(13.0f * std::sin(wing_anim_time * 8.0f * 2.0f * float(M_PI))),
			glm::vec3(-1.0f, 0.0f, 0.0f)
		);
	}

	feet_anim_time += elapsed / 2.0f;
	feet_anim_time -= std::floor(feet_anim_time);

	left_leg->rotation = left_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(feet_anim_time * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	right_leg->rotation = right_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(feet_anim_time * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, -1.0f)
	);

	// Move player:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-PlayerSpeed;
		if (!left.pressed && right.pressed) move.x = PlayerSpeed;
		if (space.downs > 0) bird_vel_y = jump_vel;
		move.y = bird_vel_y;
		bird_vel_y -= gravity * elapsed;
		bird_vel_y = std::clamp (bird_vel_y, -max_bird_vel_y, max_bird_vel_y);

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = move * elapsed;
		glm::mat4x3 frame = bird->make_local_to_parent();
		glm::vec3 frame_right = -frame[1];
		glm::vec3 frame_up = frame[2];

		bird->position += move.x * frame_right + move.y * frame_up;

		// Wrap y_pos around
		float min_bird_y_pos = camera->transform->position.y + min_y;
		float max_bird_y_pos = camera->transform->position.y + max_y;
		float y_range = max_bird_y_pos - min_bird_y_pos;
		float norm_bird_y_pos = bird->position.y - min_bird_y_pos;

		while (norm_bird_y_pos < 0) {
			norm_bird_y_pos += y_range;
		}
		while (norm_bird_y_pos > y_range) {
			norm_bird_y_pos -= y_range;
		}

		bird->position.y = min_bird_y_pos + norm_bird_y_pos;

		// Wrap z_pos around
		float min_bird_z_pos = camera->transform->position.z + min_z;
		float max_bird_z_pos = camera->transform->position.z + max_z;
		float z_range = max_bird_z_pos - min_bird_z_pos;
		float norm_bird_z_pos = bird->position.z - min_bird_z_pos;

		while (norm_bird_z_pos < 0) {
			norm_bird_z_pos += z_range;
		}
		while (norm_bird_z_pos > z_range) {
			norm_bird_z_pos -= z_range;
		}

		bird->position.z = min_bird_z_pos + norm_bird_z_pos;
	}

	// Handle plane movement
	{
		for (object_info *plane_info : plane_infos) {
			bool plane_collision = false;
			process_object(plane_info, elapsed, &plane_collision);
			if (plane_collision && lives > 0) {
				lives--;
				if (lives == 0) {
					end_game();
				}
			}
		}
	}

	// Handle coin movement
	{
		for (object_info *coin_info : coin_infos) {
			bool coin_collision = false;
			process_object(coin_info, elapsed, &coin_collision);
			glm::quat rotate = glm::angleAxis(
				glm::radians(20.0f * std::sin(elapsed * 2.0f * float(M_PI))), glm::vec3(1.0f, 0.0f, 0.0f));
			coin_info->transform->rotation = coin_info->transform->rotation * rotate;
			if (coin_collision) {
				score += 10;
			}
		}
	}

	// Handle cloud movement
	{
		for (object_info *cloud_info : cloud_infos) {
			process_object(cloud_info, elapsed, nullptr);
		}
	}

	// Reset button press counters:
	left.downs = 0;
	right.downs = 0;
	space.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);

	Scene::Light light = scene.lights.front();
	switch (light.type) {
		case light.Point: {
			glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
			break;
		}
		case light.Hemisphere: {
			glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
			break;
		}
		case light.Spot: {
			glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 2);
			break;
		}
		case light.Directional: {
			glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 3);
			break;
		}
		default: break;
	}

	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(light.transform->rotation * glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(light.energy));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.085f;

		std::string display_text = 
			"AD to move + Space to fly; Lives: " + std::to_string(lives) + 
			"; Score: " + std::to_string(score);
		lines.draw_text(display_text,
			glm::vec3(-aspect + 0.6f * H, -1.0 + 0.6f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(display_text,
			glm::vec3(-aspect + 0.6f * H + ofs, -1.0 + + 0.6f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
