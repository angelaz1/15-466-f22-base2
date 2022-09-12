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
	// Get pointers to animated components:
	Scene::Transform *hemi_light_pos;
	for (auto &transform : scene.transforms) {
		if (transform.name == "leftFoot") left_leg = &transform;
		else if (transform.name == "rightFoot") right_leg = &transform;
		else if (transform.name == "leftWing") left_wing = &transform;
		else if (transform.name == "rightWing") right_wing = &transform;
		else if (transform.name == "beak") beak = &transform;
		else if (transform.name == "bird") bird = &transform;
		else if (transform.name == "plane") plane = &transform;
		else if (transform.name == "coin") coin = &transform;
		else if (transform.name == "cloud") cloud = &transform;
		else if (transform.name == "hemi_light") hemi_light_pos = &transform;
	}
	if (left_leg == nullptr) throw std::runtime_error("Left Leg not found.");
	if (right_leg == nullptr) throw std::runtime_error("Right Leg not found.");
	if (left_wing == nullptr) throw std::runtime_error("Left Wing not found.");
	if (right_wing == nullptr) throw std::runtime_error("Right Wing not found.");
	if (beak == nullptr) throw std::runtime_error("Beak not found.");
	if (bird == nullptr) throw std::runtime_error("Bird not found.");
	if (plane == nullptr) throw std::runtime_error("Plane not found.");
	if (coin == nullptr) throw std::runtime_error("Coin not found.");
	if (cloud == nullptr) throw std::runtime_error("Cloud not found.");

	left_leg_base_rotation = left_leg->rotation;
	right_leg_base_rotation = right_leg->rotation;
	left_wing_base_rotation = left_wing->rotation;
	right_wing_base_rotation = right_wing->rotation;

	// Beak rotation ends up being off due to export issues - correct it here
	beak->rotation = glm::quat(0.646011, -0.700926, -0.1464357, 0.131397);

	// Get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// Set bird position + rotation
	bird->position = camera->transform->position + glm::vec3(7, 0, 0);
	bird->rotation = glm::quat(0, 0, 0.707f, 0.707f);

	// Add lighting
	Scene::Light light = Scene::Light(hemi_light_pos);
	light.type = light.Hemisphere;
	light.energy = glm::vec3(0.95f, 0.9f, 0.9f);
	scene.lights.push_back(light);
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
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	// Slowly rotates through [0,1):
	anim_time += elapsed / 2.0f;
	anim_time -= std::floor(anim_time);

	left_wing->rotation = left_wing_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(anim_time * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
	right_wing->rotation = right_wing_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(anim_time * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(-1.0f, 0.0f, 0.0f)
	);
	left_leg->rotation = left_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(anim_time * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	right_leg->rotation = right_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(anim_time * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, -1.0f)
	);

	// Move player:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 10.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;
		glm::mat4x3 frame = bird->make_local_to_parent();
		glm::vec3 frame_up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		bird->position += move.x * frame_forward + move.y * frame_up;
	}

	// Reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
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

		constexpr float H = 0.09f;
		lines.draw_text("WASD to move;",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("WASD to move;",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
