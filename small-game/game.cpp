#include <vector>
#include <string>
#include <sstream> 
#include <iostream>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "definitions.h"
#include "SDL_FontCache.h"


static const int32 player_width = 64;
static const int32 player_height = 64;
static const int32 ball_radius = 512;
static const int32 player_radius = 28;
static real32 ppm = 32; // pixels per meter

GameState * state;
static int screen_physical_width = 1280;
static int screen_physical_height = 720;
static bool closing = false;
static uint64 frame_count = 0;
static int player_sprite_x = 0;
static int enemy_sprite_x = 0;
static bool last_pause_press = false;
SDL_GameController *gamepad_handles[MAX_CONTROLLERS];
int32 music_volume = MIX_MAX_VOLUME / 8;

FC_Font* font;
FC_Font* large_font;
static int32 gamepad_index = 0;
static void SDLInitGamepads()
{
	SDL_GameControllerAddMapping("030000001008000001e5000000000000,NEXT SNES Controller,a:b2,b:b1,back:b8,dpdown:+a1,dpleft:-a0,dpright:+a0,dpup:-a1,leftshoulder:b4,rightshoulder:b5,start:b9,x:b3,y:b0,");

	int32 max_joysticks = SDL_NumJoysticks();
	for (int32 joystick_index = 0; joystick_index < max_joysticks; ++joystick_index)
	{
		if (!SDL_IsGameController(joystick_index))
		{
			continue;
		}
		if (gamepad_index >= MAX_CONTROLLERS)
		{
			break;
		}
		gamepad_handles[gamepad_index] = SDL_GameControllerOpen(joystick_index);

		char *mapping;
		mapping = SDL_GameControllerMapping(gamepad_handles[gamepad_index]);
		SDL_Log("Controller %i is mapped as \"%s\".", joystick_index, mapping);

		gamepad_index++;
	}
}


void handleEvents(ControllerInput &controller) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			closing = true;
		}
		else if (event.type == SDL_CONTROLLERDEVICEADDED) {
			LogInfo("Controller added: %d\n", event.cdevice.which);
			int32 device_index = event.cdevice.which;
			if (SDL_IsGameController(device_index)) {
				SDL_GameController* game_controller = SDL_GameControllerOpen(device_index);
			}
		}
		else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
			LogInfo("Controller removed: %d\n", event.cdevice.which);
			int32 instance_id = event.cdevice.which;
		}
		else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && !event.key.repeat) {
			bool is_down = event.type == SDL_KEYDOWN;

			switch (event.key.keysym.sym) {
			case SDLK_w:
			case SDLK_UP:
				controller.dir_up = is_down ? 1.0f : 0;
				break;
			case SDLK_s:
			case SDLK_DOWN:
				controller.dir_down = is_down ? 1.0f : 0;
				break;
			case SDLK_a:
			case SDLK_LEFT:
				controller.dir_left = is_down ? 1.0f : 0;
				break;
			case SDLK_d:
			case SDLK_RIGHT:
				controller.dir_right = is_down ? 1.0f : 0;
				break;
			case SDLK_SPACE:
			case SDLK_p:
				controller.button_start = is_down;
				break;
			case SDLK_ESCAPE:
				controller.button_select = is_down;
				break;
			}
		}
		else if (event.type == SDL_CONTROLLERAXISMOTION) {

			real32 value;
			if (event.caxis.value > 0) {
				value = ((real32)event.caxis.value) / 32767.f;
			}
			else {
				value = ((real32)event.caxis.value) / 32768.f;
			}

			switch (event.caxis.axis) {
			case SDL_CONTROLLER_AXIS_LEFTX:
				if (value < 0) {
					controller.dir_left = -value;
					controller.dir_right = 0;
				}
				else {
					controller.dir_right = value;
					controller.dir_left = 0;
				}
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
				if (value < 0) {
					controller.dir_up = -value;
					controller.dir_down = 0;
				}
				else {
					controller.dir_down = value;
					controller.dir_up = 0;
				}
				break;
			}
		}
		else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {

			bool is_pressed = event.cbutton.state == SDL_PRESSED;

			switch (event.cbutton.button) {
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				controller.dir_up = is_pressed ? 1.0f : 0;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				controller.dir_down = is_pressed ? 1.0f : 0;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				controller.dir_left = is_pressed ? 1.0f : 0;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				controller.dir_right = is_pressed ? 1.0f : 0;
				break;
			case SDL_CONTROLLER_BUTTON_A:
				controller.button_a = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				controller.button_b = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_X:
				controller.button_c = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				controller.button_d = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				controller.button_l = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				controller.button_r = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_START:
				controller.button_start = is_pressed;
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
				controller.button_select = is_pressed;
				break;
			default:
				LogWarn("Unknown controller button pressed");
			}
		}
		else if (event.type == SDL_MOUSEMOTION) {
			controller.mouseMoveX += event.motion.xrel;
			controller.mouseMoveY += event.motion.yrel;

		}
		else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT:
				controller.button_mouse_l = (event.button.state == SDL_PRESSED);
				break;
			case SDL_BUTTON_RIGHT:
				controller.button_mouse_r = (event.button.state == SDL_PRESSED);
				break;
			case SDL_BUTTON_MIDDLE:
				controller.button_mouse_m = (event.button.state == SDL_PRESSED);
				break;
			default:
				break;
			}
		}
		else if (event.type == SDL_MOUSEWHEEL) {
			controller.mouseWheel += event.wheel.y;
		}
	}
}

static real32 SDLGetSecondsElapsed(uint64 old_counter, uint64 current_counter, uint64 perf_frequency)
{
	return ((real32)(current_counter - old_counter) / (real32)(perf_frequency));
}

SDL_Texture * loadTexture(SDL_Renderer * renderer, std::string filepath, SDL_Surface ** surfacePtr = NULL) {
	SDL_Surface * surface = IMG_Load(("assets/" + filepath).c_str());
	SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (surfacePtr == NULL) {
		SDL_FreeSurface(surface);
	}
	else {
		*surfacePtr = surface;
	}
	return texture;
}


SDL_Surface * collision_surface = NULL;
SDL_Surface * player_surface = NULL;

SDL_Texture * overlay_texture = NULL;
SDL_Texture * controls_texture = NULL;
SDL_Texture * bg_texture = NULL;
SDL_Texture * big_circle_texture = NULL;
SDL_Texture * player_texture = NULL;
SDL_Texture * enemy_texture = NULL;
SDL_Texture * ball_texture = NULL;
SDL_Texture * frozen_texture = NULL;
SDL_Texture * ending_texture = NULL;

Mix_Music * title_music = NULL;
Mix_Music * level1_music = NULL;
Mix_Music * level2_music = NULL;
Mix_Music * level3_music = NULL;
Mix_Music * ending_music = NULL;
Mix_Chunk * step = NULL;
Mix_Chunk * lose = NULL;
Mix_Chunk * game_over = NULL;
Mix_Chunk * bounces[4];



void changeCurrentState(State new_state) {
	if (state->current_state == Playing) {
		if (new_state == GameOver) {
			Mix_FadeOutMusic(300);
			Mix_PlayChannel(-1, game_over, 0);
			state->gameover_frames = 0;
		}
		else if (new_state == Paused) {
			Mix_VolumeMusic(music_volume / 2);
		}
		else if (new_state == Shaking) {
			state->shaking_frames = 0;
		}
	}
	else if (state->current_state == Dead) {
		if (new_state == Playing) {
			state->dead_frames = 0;
		}
	}
	else if (state->current_state == Paused) {
		if (new_state == Playing) {
			Mix_VolumeMusic(music_volume);
		}
	}
	else if (state->current_state == MainMenu) {
		if (new_state == Beginning) {
			Mix_PlayMusic(level1_music, -1);
			*state = GameState();
		}
	}
	else if (state->current_state == GameOver) {
		if (new_state == MainMenu) {
			Mix_PlayMusic(title_music, -1);
		}
	}
	else if (state->current_state == Shaking) {
		if (new_state == Dead) {
			state->dead_frames = 0;
		}
	}

	state->current_state = new_state;
}

void playBounceSound(real32 ball_scale) {
	uint32 index = 0;
	if (ball_scale < 0.1f) {
		index = 0;
	}
	else if (ball_scale < 0.2f) {
		index = 1;
	}
	else if (ball_scale < 0.35f) {
		index = 2;
	}
	else {
		index = 3;
	}

	Mix_PlayChannel(3, bounces[index], 0);
}

void bounceEffect() {
	playBounceSound(state->ball_scale);
	if (state->ball_speed > 2000) {
		changeCurrentState(Shaking);
	}
}

void initialize(GameState * state, SDL_Renderer * renderer) {
	frozen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
	overlay_texture = loadTexture(renderer, "overlay.png", &player_surface);
	controls_texture = loadTexture(renderer, "controls.png", &player_surface);
	bg_texture = loadTexture(renderer, "bg.png", &player_surface);
	player_texture = loadTexture(renderer, "crab.png");
	enemy_texture = loadTexture(renderer, "crab_evil.png");
	ball_texture = loadTexture(renderer, "ball.png");
	big_circle_texture = loadTexture(renderer, "big_circle.png");
	ending_texture = loadTexture(renderer, "ending.png");

	//Load music
	title_music = Mix_LoadMUS("assets/title.ogg");
	level1_music = Mix_LoadMUS("assets/level1.ogg");
	level2_music = Mix_LoadMUS("assets/level2.ogg");
	level3_music = Mix_LoadMUS("assets/level3.ogg");
	ending_music = Mix_LoadMUS("assets/ending.ogg");
	if (level1_music == NULL || level2_music == NULL || level3_music == NULL || ending_music == NULL) {
		LogError("Failed to load music! SDL_mixer Error: %s\n", Mix_GetError());
	}

	//Load sound effects
	step = Mix_LoadWAV("assets/step1.wav");
	lose = Mix_LoadWAV("assets/lose.wav");
	game_over = Mix_LoadWAV("assets/gameover.wav");
	for (int i = 0; i < 4; i++) {
		std::ostringstream stream;
		stream << "assets/bounce" << i << ".wav";
		bounces[i] = Mix_LoadWAV(stream.str().c_str());
	}
	if (step == NULL || lose == NULL || game_over == NULL || bounces[0] == NULL || bounces[1] == NULL || bounces[2] == NULL || bounces[3] == NULL) {
		LogError("Failed to load sound effect! SDL_mixer Error: %s\n", Mix_GetError());
	}

	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	Mix_VolumeMusic(music_volume);
	Mix_PlayMusic(title_music, -1);

	state->ball_direction = { 0.5f, 0.4f };
	state->ball_direction.normalize();

	// Text
	font = FC_CreateFont();  
	large_font = FC_CreateFont();
	FC_LoadFont(font, renderer, "assets/8bitOperatorPlus-Regular.ttf", 28, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);
	FC_LoadFont(large_font, renderer, "assets/8bitOperatorPlus-Regular.ttf", 96, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);
}

std::vector<BallPhase> ball_physical_phases = {
	{1, [&] {
			state->ball_moves_physically = true;
			state->ball_moves_linearly = true;
			state->h_reflect = true;
			state->v_reflect = true;
			state->ball_direction = {0.5, 0.4};
			state->ball_direction.normalize(); } },
	{360, [&] {} },
	{180, [&] {state->ball_speed *= MUL_UP_1; }},
	{270, [&] {} },
	{180, [&] {state->ball_speed *= MUL_UP_1; }},
	{360, [&] {} },
	{360, [&] {state->ball_speed *= MUL_DOWN_1;
			   state->ball_scale *= MUL_UP_2L; }},
	{270, [&] {} },
	{180, [&] {state->ball_scale *= MUL_DOWN_2L; }},
	{180, [&] {} },
	{180, [&] {state->ball_scale *= MUL_DOWN_2L; }},
	{90, [&] {}},

	{360, [&] {state->h_reflect = false; state->v_reflect = false;  }},
	{180, [&] {state->ball_speed *= MUL_UP_2; }},
	{180, [&] {} },
	{180, [&] {state->ball_speed *= MUL_DOWN_2;
			   state->ball_scale *= MUL_UP_2L; }},
	{180, [&] {} },
	{180, [&] {state->ball_speed *= MUL_UP_2;}},
	{180, [&] {} },
	{180, [&] {state->ball_speed *= MUL_DOWN_2;
			   state->ball_scale *= MUL_DOWN_2L; }},

	{ 90, [&] {state->h_reflect = false; state->v_reflect = true;  } },
	{ 90, [&] {state->ball_speed *= MUL_UP_2; }},
	{ 180, [&] {} },
	{ 90, [&] {state->ball_speed *= MUL_UP_2; }},
	{ 270, [&] {}},

	{ 1, [&] {state->h_reflect = true; state->v_reflect = false;  } },
	{ 359, [&] {}},
	{ 180, [&] {state->h_reflect = false; state->v_reflect = true;  } },
	{ 180, [&] {state->h_reflect = true; state->v_reflect = false;  } },
	{ 180, [&] {state->h_reflect = true; state->v_reflect = true; }},
	{ 180, [&] {state->ball_speed *= MUL_DOWN_2; }},

	{1, [&] {state->ball_moves_physically = false; }},
	{120, [&] {
		static Vector2f org_pos = state->ball_pos;
		Vector2f dest = { 360, 40 };
		state->next_ball_pos = org_pos + (dest - org_pos) * sin(0.5f*Pi32*(state->ball_phase_frames / 120.f));
	}},
};

real32 b = -4.f;
real32 center_x = SCREEN_WIDTH / 2;
real32 center_y = SCREEN_HEIGHT / 2;

std::vector<BallPhase> ball_geo_phases = {
	{1, [&] {state->ball_moves_physically = false; state->ball_moves_linearly = false; }},
	{240, [&] { // move to center
	Vector2f org_pos = { SCREEN_WIDTH / 2, 90 };
	Vector2f dest = { SCREEN_WIDTH / 2,  SCREEN_HEIGHT / 2 };
	state->next_ball_pos = org_pos + (dest - org_pos) * (sin(0.5f*Pi32*(state->ball_phase_frames / 240.f)));
	}},
	{712, [&] { // spiral
	real32 t = (state->ball_phase_frames / 540.f)*22.f*Pi32;
	state->next_ball_pos.x = SCREEN_WIDTH/2 + (b * t) * cos(t);
	state->next_ball_pos.y = SCREEN_HEIGHT/2 + (b * t) * sin(t);
	}},
	{348, [&] { // 1 fold circle. start x = 720 y = 371
	real32 speed = 15.0f + (state->ball_phase_frames / 540.f) * 25.f;
	real32 t = ((state->ball_phase_frames / 540.f))*speed*Pi32;
	center_x = (SCREEN_WIDTH / 2);
	center_y = (SCREEN_HEIGHT / 2);
	state->next_ball_pos.x = center_x + (SCREEN_WIDTH / 2) * cos(t);
	state->next_ball_pos.y = center_y + (SCREEN_HEIGHT / 2) * sin(t);
	}},
	{458, [&] { // 2 fold circle. start x = 840 y = 360
	real32 t = ((state->ball_phase_frames / 540.f))*40.f*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 3) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_WIDTH / 3) * sin(t / 8);
	state->next_ball_pos.x = center_x + (SCREEN_WIDTH / 3) * cos(t);
	state->next_ball_pos.y = center_y + (SCREEN_HEIGHT / 3) * sin(t);
	}},
	{433, [&] { // 3 fold circle. start x = 788 y = 491
	real32 t = ((state->ball_phase_frames / 540.f))*40.f*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 3) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_WIDTH / 3) * sin(t / 8);
	state->next_ball_pos.x = center_x + (SCREEN_WIDTH / 6) * cos(t) + (SCREEN_WIDTH / 6) * cos(4 * t);
	state->next_ball_pos.y = center_y + (SCREEN_HEIGHT / 6) * sin(t) + (SCREEN_HEIGHT / 6) * sin(4 * t);
	}},
	{500, [&] { // 3 fold circle back x: 788 y: 491
	real32 speed = 30.f - (state->ball_phase_frames / 540.f) * 20.f;
	real32 t = ((state->ball_phase_frames / 540.f))*speed*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 3) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_WIDTH / 3) * sin(t / 8);
	state->next_ball_pos.x = center_x + (SCREEN_WIDTH / 6) * cos(t) + (SCREEN_WIDTH / 6) * cos(4 * t);
	state->next_ball_pos.y = center_y + (SCREEN_HEIGHT / 6) * sin(t) + (SCREEN_HEIGHT / 6) * sin(4 * t);
	}},
		
	{40, [&] {
	Vector2f org_pos = state->ball_pos;
	Vector2f dest = { 519, 459 };
	state->next_ball_pos = org_pos + (dest - org_pos) * (state->ball_phase_frames / 60.f);
	}},

	{1080, [&] { // circle + spiral x: 519 y: 459
	b = -1.8f;
	real32 t = 4.f + ((state->ball_phase_frames / 540.f))*40.f*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 4) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 4) * sin(t / 8);
	state->next_ball_pos.x = center_x + (b * t) * cos(t);
	state->next_ball_pos.y = center_y + (b * t) * sin(t);
	}},
	{540, [&] { // circle + spiral back x: -40 y: 302  
	b = -1.8f;
	real32 t = -(1 - (state->ball_phase_frames / 540.f))*40.f*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 4) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 4) * sin(t / 8);
	state->next_ball_pos.x = center_x + (b * t) * cos(t + Pi32);
	state->next_ball_pos.y = center_y + (b * t) * sin(t + Pi32);
	}},
	
	{1080, [&] { // circle + spiral counter clockwise
	b = -1.8f;
	real32 t = -((state->ball_phase_frames / 540.f))*40.f*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 4) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 4) * sin(t / 8);
	state->next_ball_pos.x = center_x + (b * t) * cos(t);
	state->next_ball_pos.y = center_y + (b * t) * sin(t);
	}},
	{540, [&] { // circle + spiral back counter clockwise
	b = -1.8f;
	real32 t = (1 - (state->ball_phase_frames / 540.f))*40.f*Pi32;
	center_x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 4) * cos(t / 8);
	center_y = (SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 4) * sin(t / 8);
	state->next_ball_pos.x = center_x + (b * t) * cos(t + Pi32);
	state->next_ball_pos.y = center_y + (b * t) * sin(t + Pi32);
	}},

	{120, [&] {
	static Vector2f org_pos = state->ball_pos;
	Vector2f dest = { 360, 40 };
	state->next_ball_pos = org_pos + (dest - org_pos) * sin(0.5f*Pi32*(state->ball_phase_frames / 120.f));
	}},
};

const real32 delta = 0.017f; //fake time delta TEMPORARY
real32 r = SCREEN_HEIGHT / 2;
real32 theta = 0;

real32 x = SCREEN_WIDTH / 2 + r * cos(theta);
real32 y = SCREEN_HEIGHT / 2 + r * sin(theta);

real32 pentagram_dests[] = { 54, 198, 342, 126, 270 };
real32 line_dests[] = { 90, 270 };
real32 * dests;
uint8 num_dests = 2;
uint8 current_dest = 0;

std::vector<BallPhase> ball_final_phases = {
	{1, [&] {
			state->ball_pos = state->prev_ball_pos = state->next_ball_pos = {SCREEN_WIDTH/2, 40};
			state->ball_moves_physically = false;
			state->ball_moves_linearly = true;
			state->h_reflect = true;
			state->v_reflect = true;
			Vector2f dest = {SCREEN_WIDTH, SCREEN_HEIGHT};
			state->ball_direction = (dest - state->ball_pos);
			state->ball_direction.normalize();
			dests = line_dests;
			num_dests = 2;
} },
	// line
	{1440, [&] {
			if (state->ball_phase_frames < 270) {
				state->ball_speed *= MUL_UP_2;
			}
			else if(state->ball_phase_frames < 675) {
				real32 angle = sin(((state->ball_phase_frames - 270) / 405.0)*Pi32);
				dests[0] += angle;
				dests[1] += angle;
			}
			else {
				real32 angle = 2*sin(((state->ball_phase_frames - 675) / 405.0)*Pi32);
				dests[0] -= angle;
				dests[1] -= angle;
				if (state->ball_phase_frames > 1080) {
					state->ball_speed *= MUL_DOWN_1;
				}
			}
			Vector2f car_dest = polarToCar(r, dests[current_dest]);
			Vector2f dir = (car_dest - state->next_ball_pos);
			real32 mag = dir.getMagnitude();
			dir.normalize();
			real32 step = state->ball_speed * delta;
			if (mag <= step) {
				// Going past the destination
				state->next_ball_pos = car_dest;
				current_dest = (current_dest + 1) % num_dests;
				bounceEffect();

				//playBounceSound(state->ball_scale);
			}
			else {
				state->next_ball_pos += dir * step;
			}

			}
	},


	{1, [&] {
			dests = pentagram_dests;
			num_dests = 5;
			state->dests_visible = true;
			state->dests_color = {};
} },
	{540, [&] {
			if (state->ball_phase_frames < 360) {
				state->ball_speed *= MUL_UP_1;
			}
			if (state->ball_phase_frames < 540) {
				//state->ball_speed *= MUL_UP_1;
				state->dests_color.r = (uint8)(255.f * (state->ball_phase_frames / 540.f));
				state->dests_color.a = MAX(0, state->dests_color.r - 128.f);
			}
			Vector2f car_dest = polarToCar(r, dests[current_dest]);
			Vector2f dir = (car_dest - state->next_ball_pos);
			real32 mag = dir.getMagnitude();
			dir.normalize();
			real32 step = state->ball_speed * delta;
			if (mag <= step) {
				// Going past the destination
				state->next_ball_pos = car_dest;
				current_dest = (current_dest + 1) % num_dests;
				bounceEffect();

				//playBounceSound(state->ball_scale);
			}
			else {
				state->next_ball_pos += dir * step;
			}
			}
	},
	{1440, [&] {
			if (state->ball_phase_frames < 360) {
				real32 angle = sin((state->ball_phase_frames / 360.0)*Pi32);
				dests[0] += angle;
				dests[1] += angle;
				dests[2] += angle;
				dests[3] += angle;
				dests[4] += angle;
			}
		else if (state->ball_phase_frames < 540) {
		}
		else if (state->ball_phase_frames < 1260) {
		   real32 angle = 2*sin(((state->ball_phase_frames - 540) / 720.0)*Pi32);
		   dests[0] -= angle;
		   dests[1] -= angle;
		   dests[2] -= angle;
		   dests[3] -= angle;
		   dests[4] -= angle;
		}
		else if (state->ball_phase_frames < 1440) {
		   state->ball_speed *= MUL_DOWN_2;
		}

		Vector2f car_dest = polarToCar(r, dests[current_dest]);
		Vector2f dir = (car_dest - state->next_ball_pos);
		real32 mag = dir.getMagnitude();
		dir.normalize();
		real32 step = state->ball_speed * delta;
		if (mag <= step) {
			// Going past the destination
			state->next_ball_pos = car_dest;
			current_dest = (current_dest + 1) % num_dests;
			bounceEffect();

			//playBounceSound(state->ball_scale);
		}
		else {
			state->next_ball_pos += dir * step;
		}
		}


	},{1440, [&] { //big ball star
		if (state->ball_phase_frames < 360) {
			state->ball_scale *= MUL_UP_1L;
			real32 angle = sin((state->ball_phase_frames / 360.0)*Pi32);
			dests[0] += angle;
			dests[1] += angle;
			dests[2] += angle;
			dests[3] += angle;
			dests[4] += angle;

			state->dests_color.a = MAX(0, 128 - 128*(state->ball_phase_frames/360.0f));
		}
		else if (state->ball_phase_frames < 540) {
			state->dests_visible = false;
			state->ball_scale *= MUL_UP_2L;
			state->ball_speed *= MUL_DOWN_1;
		}
		else if (state->ball_phase_frames < 1260) {
			if (state->ball_phase_frames > 1080) {
				state->dests_visible = false;
				state->ball_scale *= MUL_DOWN_1L;
				state->ball_speed *= MUL_UP_1;
			}
		   real32 angle = sin(((state->ball_phase_frames - 540) / 720.0)*Pi32);
		   dests[0] -= angle;
		   dests[1] -= angle;
		   dests[2] -= angle;
		   dests[3] -= angle;
		   dests[4] -= angle;
		}
		else if (state->ball_phase_frames < 1440) {
		   state->ball_speed *= MUL_DOWN_2;
		}

		Vector2f car_dest = polarToCar(r, dests[current_dest]);
		Vector2f dir = (car_dest - state->next_ball_pos);
		real32 mag = dir.getMagnitude();
		dir.normalize();
		real32 step = state->ball_speed * delta;
		if (mag <= step) {
			// Going past the destination
			state->next_ball_pos = car_dest;
			current_dest = (current_dest + 1) % num_dests;
			bounceEffect();

			//playBounceSound(state->ball_scale);
		}
		else {
			state->next_ball_pos += dir * step;
		}
		}
	},

};

std::vector<BallPhase> * ball_stages[] = { &ball_geo_phases, &ball_physical_phases, &ball_final_phases };

inline static bool collisionCheck(Vector2f a, Vector2f b, real32 limit) {
	return (a - b).getMagnitude() < limit;
}

void deadUpdate(ControllerInput * controller, real32 time_delta) {
	if (state->dead_frames % 20 == 0) {
		state->player_visible = !state->player_visible;
	}
	if (state->dead_frames >= 90) {
		state->player_visible = true;
		changeCurrentState(Playing);
	}
	state->dead_frames++;
}

void playingUpdate(ControllerInput * controller, real32 time_delta) {
	bool pausePress = controller->button_select || controller->button_start;
	if (!last_pause_press && pausePress) {
		changeCurrentState(Paused);
		last_pause_press = pausePress;
		return;
	}
	last_pause_press = pausePress;

	bool player_invul = false;
	if (state->dead_frames < 60) {
		player_invul = true;
		if (state->dead_frames % 20 == 0) {
			state->player_visible = !state->player_visible;
		}
		state->dead_frames++;
	}
	else {
		state->player_visible = true;
	}

	const real32 acc_const = 12000.f;
	Vector2f move_dir = { controller->dir_right - controller->dir_left, controller->dir_down - controller->dir_up };
	move_dir.normalize();
	Vector2f acceleration = move_dir * acc_const;

	state->player_speed += acceleration * time_delta;

	const real32 friction = 0.75f;
	state->player_speed *= friction;
	if (state->player_speed.getMagnitude() < 0.2f) {
		state->player_speed.x = 0;
		state->player_speed.y = 0;
	}

	state->player_pos += state->player_speed * time_delta;

	if (state->ball_stage == 2) {
		Vector2f center = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2};
		real32 dist = (state->player_pos - center).getMagnitude();
		real32 limit = SCREEN_WIDTH / 2 - player_radius;
		if (dist >= limit) {
			Vector2f step = center - state->player_pos;
			step.normalize();
			step *= (dist - limit + 1);
			state->player_pos += step;
		}
	}

	if (state->h_reflect) {
		if (state->player_pos.x + player_radius >= SCREEN_WIDTH) {
			state->player_pos.x = SCREEN_WIDTH - player_radius - 1;
		}
		else if (state->player_pos.x - player_radius <= 0.f) {
			state->player_pos.x = player_radius + 1;
		}
	}
	else {
		if (state->player_pos.x - player_radius >= SCREEN_WIDTH) {
			state->player_pos.x = player_radius + 1;
		}
		else if (state->player_pos.x + player_radius <= 0.f) {
			state->player_pos.x = SCREEN_WIDTH - player_radius - 1;
		}
	}
	if (state->v_reflect) {
		if (state->player_pos.y + player_radius >= SCREEN_HEIGHT) {
			state->player_pos.y = SCREEN_HEIGHT - player_radius - 1;
		}
		else if (state->player_pos.y - player_radius <= 0.f) {
			state->player_pos.y = player_radius + 1;
		}
	}
	else {
		if (state->player_pos.y - player_radius >= SCREEN_HEIGHT) {
			state->player_pos.y = player_radius + 1;
		}
		else if (state->player_pos.y + player_radius <= 0.f) {
			state->player_pos.y = SCREEN_HEIGHT - player_radius - 1;
		}
	}

	if (controller->dir_right - controller->dir_left != 0 || controller->dir_down - controller->dir_up != 0) {
		if (!Mix_Playing(1)) {
			Mix_PlayChannel(1, step, 0);
		}

		if (state->playing_frames % 10 == 0) {
			player_sprite_x = player_sprite_x == 0 ? player_width : 0;
		}
	}

	// Ball movement
	state->prev_ball_pos = state->ball_pos;
	if (state->ball_stage < 3) {
		if (state->ball_phase < ball_stages[state->ball_stage]->size()) {
			(*ball_stages[state->ball_stage])[state->ball_phase].update();
		}
		else {
			state->ball_stage++;
			if (state->ball_stage == 1) {
				Mix_PlayMusic(level2_music, -1);
			}
			if (state->ball_stage == 2) {
				Mix_PlayMusic(level3_music, -1);
			}
			state->ball_phase = 0;
			state->ball_phase_frames = 0;
		}
	}
	else {
		changeCurrentState(Ending);
		Mix_PlayMusic(ending_music, -1);
	}

	if (state->ball_moves_physically) {
		state->next_ball_pos = state->ball_pos + state->ball_direction * state->ball_speed * time_delta;

		if (state->h_reflect) {
			if (state->next_ball_pos.x + state->ball_scale * ball_radius >= SCREEN_WIDTH) {
				state->next_ball_pos.x = SCREEN_WIDTH - state->ball_scale * ball_radius - 1;
				state->ball_direction.x = -state->ball_direction.x;
				bounceEffect();
			}
			else if (state->next_ball_pos.x - state->ball_scale * ball_radius <= 0.f) {
				state->next_ball_pos.x = state->ball_scale * ball_radius + 1;
				state->ball_direction.x = -state->ball_direction.x;
				bounceEffect();
			}
		}
		else {
			if (state->next_ball_pos.x - state->ball_scale * ball_radius >= SCREEN_WIDTH) {
				state->next_ball_pos.x = state->ball_scale * ball_radius + 1;
				state->prev_ball_pos = state->ball_pos = state->next_ball_pos;
			}
			else if (state->next_ball_pos.x + state->ball_scale * ball_radius <= 0.f) {
				state->next_ball_pos.x = SCREEN_WIDTH - state->ball_scale * ball_radius - 1;
				state->prev_ball_pos = state->ball_pos = state->next_ball_pos;
			}
		}
		if (state->v_reflect) {
			if (state->next_ball_pos.y + state->ball_scale * ball_radius >= SCREEN_HEIGHT) {
				state->next_ball_pos.y = SCREEN_HEIGHT - state->ball_scale * ball_radius - 1;
				state->ball_direction.y = -state->ball_direction.y;
				bounceEffect();
			}
			else if (state->next_ball_pos.y - state->ball_scale * ball_radius <= 0.f) {
				state->next_ball_pos.y = state->ball_scale * ball_radius + 1;
				state->ball_direction.y = -state->ball_direction.y;
				bounceEffect();
			}
		}
		else {
			if (state->next_ball_pos.y - state->ball_scale * ball_radius >= SCREEN_HEIGHT) {
				state->next_ball_pos.y = state->ball_scale * ball_radius + 1;
				state->prev_ball_pos = state->ball_pos = state->next_ball_pos;
			}
			else if (state->next_ball_pos.y + state->ball_scale * ball_radius <= 0.f) {
				state->next_ball_pos.y = SCREEN_HEIGHT - state->ball_scale * ball_radius - 1;
				state->prev_ball_pos = state->ball_pos = state->next_ball_pos;
			}
		}
	}

	if (!player_invul) {
		bool collided = false;
		Vector2f ball_dif = state->next_ball_pos - state->ball_pos;
		real32 touch_limit = player_radius + ball_radius * state->ball_scale;
		if (ball_dif.getMagnitude() > ball_radius * state->ball_scale * 2) {
			if (collisionCheck(state->ball_pos + ball_dif * 0.25f, state->player_pos, touch_limit)) {
				collided = true;
				state->ball_pos = state->ball_pos + ball_dif * 0.25f;
			}
			else if (collisionCheck(state->ball_pos + ball_dif * 0.5f, state->player_pos, touch_limit)) {
				collided = true;
				state->ball_pos = state->ball_pos + ball_dif * 0.5f;
			}
			else if (collisionCheck(state->ball_pos + ball_dif * 0.75f, state->player_pos, touch_limit)) {
				collided = true;
				state->ball_pos = state->ball_pos + ball_dif * 0.75f;
			}
		}
		if (!collided) {
			if (collisionCheck(state->next_ball_pos, state->player_pos, touch_limit)) {
				collided = true;
			}
			state->ball_pos = state->next_ball_pos;
		}

		if (collided) {
			LogDebug("Collision!!!");
			Mix_PlayChannel(2, lose, 0);
			if (state->player_lives > 0) {
				state->player_lives--;
				state->shaking_for_dead = true;
				changeCurrentState(Shaking);
			}
			else {
				changeCurrentState(GameOver);
			}
		}
	}
	else {
		state->ball_pos = state->next_ball_pos;
	}

	state->playing_frames++;
}


const int8 shake_xs[] = { -2, 0, 2, 0 };
const int8 shake_ys[] = { 0, -2, 0, 2 };
const int8 death_shake_xs[] = { -6, 3, 5, 2, -3, 2, -2, 0 };
const int8 death_shake_ys[] = { 3, -6, 2, 4, -2, 3, 1, -1 };
const char * enemy_messages[] = { "", "Crabland belongs \nto ME!", "You can't win against \nmy new weapon!", "Bwa ha ha ha" };

void updateAndDraw(SDL_Surface * screen_surface, SDL_Renderer * renderer, SDL_Window * window, ControllerInput * controller, real32 time_delta) {
	SDL_RenderClear(renderer);
	uint8 enemy_message = 0;
	static uint64 non_paused_frame_count = 0;
	bool pausePress = false;
	int32 game_over_y = 0;
	switch (state->current_state)
	{
	case MainMenu:
		if (controller->button_start) {
			last_pause_press = true;
			changeCurrentState(Beginning);
		}
		if (controller->button_select) {
			closing = true;
		}
		break;
	case Beginning:
		if (state->beginning_frames == 0) {
			state->ball_pos.y = -32;
		}
		if (state->beginning_frames <= 60) {
			Vector2f org = { SCREEN_WIDTH / 2,  0 };
			Vector2f dest = { SCREEN_WIDTH / 2,  45 };
			state->enemy_pos = org + (dest - org) * (sin(0.5f*Pi32*(state->beginning_frames / 60.f)));
		}
		else if (state->beginning_frames < 240) {
			enemy_message = 1;
			if (state->beginning_frames % 10 == 0) {
				enemy_sprite_x = enemy_sprite_x == 0 ? player_width : 0;
			}
		}
		else if (state->beginning_frames == 240) {
			enemy_sprite_x = 0;
			enemy_message = 0;
		}
		else if (state->beginning_frames <= 300) {
		}
		else if (state->beginning_frames < 480) {
			enemy_message = 2;
			if (state->beginning_frames % 10 == 0) {
				enemy_sprite_x = enemy_sprite_x == 0 ? player_width : 0;
			}
		}
		else if (state->beginning_frames == 480) {
			enemy_sprite_x = 0;
			enemy_message = 0;
		}
		else if (state->beginning_frames <= 540) {
			Vector2f org = { SCREEN_WIDTH / 2,  60 };
			Vector2f dest = { SCREEN_WIDTH / 2,  90 };
			state->next_ball_pos = org + (dest - org) * (sin(0.5f*Pi32*((state->beginning_frames - 480) / 60.f)));
			state->ball_pos = state->next_ball_pos;
		}
		else if (state->beginning_frames <= 600) {
		}
		else if (state->beginning_frames < 720) {
			enemy_message = 3;
			if (state->beginning_frames % 10 == 0) {
				enemy_sprite_x = enemy_sprite_x == 0 ? player_width : 0;
			}
			Vector2f org = { SCREEN_WIDTH / 2,  45 };
			Vector2f dest = { SCREEN_WIDTH / 2,  -45 };
			state->enemy_pos = org + (dest - org) * (sin(0.5f*Pi32*((state->beginning_frames - 600) / 120.f)));
		}
		else if (state->beginning_frames == 720) {
			enemy_message = 0;
			enemy_sprite_x = 0;
			changeCurrentState(Playing);
		}

		state->beginning_frames++;
		break;
	case Playing:
		playingUpdate(controller, time_delta);
		break;
	case Dead:
		deadUpdate(controller, time_delta);
		break;
	case Paused:
		pausePress = controller->button_start || controller->button_select;
		if (!last_pause_press && pausePress) {
			changeCurrentState(Playing);
		}
		if (!last_pause_press && controller->button_select) {
			closing = true;
		}
		last_pause_press = pausePress;
		break;
	case GameOver:
		game_over_y = 210;
		if (state->gameover_frames <= 95.f) {
			game_over_y = 0 + (state->gameover_frames / 95.f) * 210;
		}
		else {
			if (controller->button_select) {
				changeCurrentState(MainMenu);
			}
		}

		if (state->gameover_frames > 240.f) {
			changeCurrentState(MainMenu);
		}
		state->gameover_frames++;
		break;
	case Shaking:
		if (state->shaking_for_dead) {
			if (state->shaking_frames > 7) {
				changeCurrentState(Dead);
				state->shaking_for_dead = false;
			}
		}
		else if(state->shaking_frames > 1) {
			changeCurrentState(Playing);
		}
		break;
	default:
		break;
	}

	if (state->current_state == Ending) {
		SDL_RenderCopy(renderer, ending_texture, 0, 0);
		SDL_RenderPresent(renderer);

		if (controller->button_start) {
			changeCurrentState(MainMenu);
		}
		if (controller->button_select) {
			closing = true;
		}
		return;
	}

	state->ball_r = (uint8)(MIN(state->ball_speed / 18, 255));
	state->ball_g = 255 - state->ball_r;
	state->ball_b = (uint8)(MIN(state->ball_scale * 400, 255));

	if (state->current_state == Shaking) {
		SDL_SetRenderTarget(renderer, frozen_texture);
	}

	SDL_Rect bg_rect = {0, 1440 - ((non_paused_frame_count *2) % 1440), 720, 720};
	SDL_RenderCopy(renderer, bg_texture, &bg_rect, 0);

	if (state->current_state != MainMenu) {
		if (state->ball_stage == 2) {
			SDL_RenderCopy(renderer, big_circle_texture, 0, 0);
		}

		if (state->player_visible) {
			SDL_Rect player_sprite_rect = { player_sprite_x, 0, player_width, player_height };
			SDL_Rect player_rect = { state->player_pos.x - player_width / 2, state->player_pos.y - player_height / 2, player_width, player_height };
			SDL_RenderCopy(renderer, player_texture, &player_sprite_rect, &player_rect);
		}

		SDL_Rect enemy_sprite_rect = { enemy_sprite_x, 0, player_width, player_height };
		SDL_Rect enemy_rect = { state->enemy_pos.x - player_width / 2, state->enemy_pos.y - player_height / 2, player_width, player_height };
		SDL_RenderCopy(renderer, enemy_texture, &enemy_sprite_rect, &enemy_rect);

		SDL_SetTextureAlphaMod(ball_texture, 255);
		SDL_SetTextureColorMod(ball_texture, state->ball_r, state->ball_g, state->ball_b);

		int32 effective_ball_radius = ball_radius * state->ball_scale;
		SDL_Rect ball_rect = { state->ball_pos.x - effective_ball_radius, state->ball_pos.y - effective_ball_radius, effective_ball_radius * 2, effective_ball_radius * 2 };
		SDL_RenderCopy(renderer, ball_texture, 0, &ball_rect);

		// Speed repeat draw
		if (state->current_state == Playing && state->ball_moves_linearly) {
			const Vector2f diff = state->ball_pos - state->prev_ball_pos;
			const real32 diff_mag = diff.getMagnitude();
			const real32 ball_circ = effective_ball_radius * 2;
			SDL_Rect speed_ball_rect = ball_rect;
			if (diff_mag < ball_circ * 3) {
			}
			else if (diff_mag < ball_circ * 6) {
				speed_ball_rect.x -= diff.x*0.5f;
				speed_ball_rect.y -= diff.y*0.5f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
			}
			else if (diff_mag < ball_circ * 8) {
				speed_ball_rect.x -= diff.x*0.33f;
				speed_ball_rect.y -= diff.y*0.33f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.33f;
				speed_ball_rect.y -= diff.y*0.33f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
			}
			else if (diff_mag < ball_circ * 10) {
				speed_ball_rect.x -= diff.x*0.25f;
				speed_ball_rect.y -= diff.y*0.25f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.25f;
				speed_ball_rect.y -= diff.y*0.25f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.25f;
				speed_ball_rect.y -= diff.y*0.25f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
			}
			else{
				speed_ball_rect.x -= diff.x*0.167f;
				speed_ball_rect.y -= diff.y*0.167f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.167f;
				speed_ball_rect.y -= diff.y*0.167f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.167f;
				speed_ball_rect.y -= diff.y*0.167f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.167f;
				speed_ball_rect.y -= diff.y*0.167f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
				speed_ball_rect.x -= diff.x*0.167f;
				speed_ball_rect.y -= diff.y*0.167f;
				SDL_RenderCopy(renderer, ball_texture, 0, &speed_ball_rect);
			}
		}

		if (state->dests_visible) {
			SDL_SetTextureAlphaMod(ball_texture, state->dests_color.a);
			SDL_SetTextureColorMod(ball_texture, state->dests_color.r, state->dests_color.g, state->dests_color.b);
			for (int i = 0; i < num_dests; i++) {
				Vector2f dest_v = polarToCar(r, dests[i]);
				SDL_Rect dest_rect = { dest_v.x, dest_v.y, 10, 10};
				SDL_RenderCopy(renderer, ball_texture, 0, &dest_rect);
			}
		}


		SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		if (state->h_reflect) {
			SDL_RenderDrawLine(renderer, 0, 0, 0, SCREEN_HEIGHT - 1);
			SDL_RenderDrawLine(renderer, SCREEN_WIDTH - 1, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
		}
		if (state->v_reflect) {
			SDL_RenderDrawLine(renderer, 0, 0, SCREEN_WIDTH - 1, 0);
			SDL_RenderDrawLine(renderer, 0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

		SDL_Rect lives_sprite_rect = { 0, 0, player_width, player_height };
		SDL_Rect lives_rect = { 20, 20, player_width / 2, player_height / 2 };
		SDL_RenderCopy(renderer, player_texture, &lives_sprite_rect, &lives_rect);
		FC_Draw(font, renderer, 60, 15, "x %d", state->player_lives);

		if (state->current_state == GameOver) {
			FC_Draw(large_font, renderer, 120, game_over_y, "Game Over");
		}
		if (state->current_state == Paused) {
			SDL_RenderCopy(renderer, overlay_texture, 0, 0);
			SDL_Rect controls_rect = { 0, 360, 720, 360 };
			SDL_RenderCopy(renderer, controls_texture, 0, &controls_rect);
			SDL_RenderPresent(renderer);
		}

		if (enemy_message > 0) {
			FC_Draw(font, renderer, SCREEN_WIDTH / 2 + 40, 15, enemy_messages[enemy_message]);
		}
	}
	else {
		// Main menu
		FC_Draw(large_font, renderer, 120, 60, "Beware \nthe Orb");

		SDL_Rect controls_rect = {0, 360, 720, 360};
		SDL_RenderCopy(renderer, controls_texture, 0, &controls_rect);
	}

	// Screen Shake
	if (state->current_state == Shaking) {
		SDL_SetRenderTarget(renderer, 0);
		const int8 * xs = state->shaking_for_dead ? death_shake_xs : shake_xs;
		const int8 * ys = state->shaking_for_dead ? death_shake_ys : shake_ys;
		SDL_Rect frame_rect = { xs[state->shaking_frames], ys[state->shaking_frames], SCREEN_WIDTH, SCREEN_HEIGHT};
		SDL_RenderCopy(renderer, frozen_texture, 0, &frame_rect);
		SDL_RenderPresent(renderer);

		state->shaking_frames++;
	}
	else {
		SDL_RenderPresent(renderer);
	}


	if (state->current_state != Paused && state->current_state != Shaking) {
		non_paused_frame_count++;
	}
}


int main(int, char**) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}
	atexit(SDL_Quit);

	SDL_Window* window = NULL;
	SDL_Surface* screen_surface = NULL;
	SDL_Renderer* renderer = NULL;

	uint32 window_properties = 0;

#ifdef DEBUG
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
#else
	SDL_DisplayMode display_mode;
	int32 should_be_zero = SDL_GetCurrentDisplayMode(0, &display_mode);
	window_properties |= SDL_WINDOW_BORDERLESS;
	if (should_be_zero != 0) {
		LogError("Could not get display mode");
	}
	else {
		screen_physical_width = display_mode.w;
		screen_physical_height = display_mode.h;
	}

	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN);
#endif

	window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_physical_width, screen_physical_height, window_properties);
	if (!window) {
		LogError("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}
	LogInfo("Window is created");

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	/* Opening gamepads */
	SDLInitGamepads();

	/* Initialize SDL_image and SDL_mixer */
	if (IMG_Init(IMG_INIT_PNG) == 0) {
		LogError("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
	}
	if (Mix_Init(MIX_INIT_OGG) == 0) {
		LogError("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		LogError("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
	}

	SDL_ShowCursor(SDL_DISABLE);

	real32 game_update_hz = 60;
	real32 target_seconds_per_frame = 1.0f / game_update_hz;
	uint64 perf_frequency = SDL_GetPerformanceFrequency();
	
	ControllerInput controller = {};
	uint64 last_counter = SDL_GetPerformanceCounter();
	uint64 update_counter = last_counter;

	state = new GameState;

	initialize(state, renderer);

	/* Main loop */
	while (1) {
		handleEvents(controller);
		/*if (controller.button_select) {
			closing = true;
		}*/

		uint64 new_update_counter = SDL_GetPerformanceCounter();
		real32 time_delta = SDLGetSecondsElapsed(update_counter, new_update_counter, perf_frequency);
		update_counter = new_update_counter;

		updateAndDraw(screen_surface, renderer, window, &controller, time_delta);

		real32 seconds_elapsed = SDLGetSecondsElapsed(last_counter, SDL_GetPerformanceCounter(), perf_frequency);
		if (seconds_elapsed < target_seconds_per_frame)
		{
			int32 time_to_sleep = (int32)((target_seconds_per_frame - seconds_elapsed) * 1000) - 1;
			if (time_to_sleep > 0)
			{
				SDL_Delay(time_to_sleep);
			}
			//SDL_assert(SDLGetSecondsElapsed(last_counter, SDL_GetPerformanceCounter(), perf_frequency) < target_seconds_per_frame);
			while (SDLGetSecondsElapsed(last_counter, SDL_GetPerformanceCounter(), perf_frequency) < target_seconds_per_frame)
			{
				// Waiting...
			}
		}

		uint64 end_counter = SDL_GetPerformanceCounter();

#ifdef DEBUG
		if (frame_count % 64 == 0) {
			uint64 counter_elapsed = end_counter - last_counter;
			real64 ms_per_frame = (((1000.0f * (real64)counter_elapsed) / (real64)perf_frequency));
			real64 fps = (real64)perf_frequency / (real64)counter_elapsed;
			printf("%.02f ms/f, %.02f f/s \n", ms_per_frame, fps);
		}
#endif
		last_counter = end_counter;


		if (closing) {
			break;
		}
		frame_count++;
	}
	return 0;
}