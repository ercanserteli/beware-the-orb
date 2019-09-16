#pragma once

#include <functional>
#include <stdint.h>

// The logical screen width and height being rendered to
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 720

#define Pi32 3.14159265358979f

#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define degToRad(angleInDegrees) ((angleInDegrees) * Pi32 / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / Pi32)
#define SIGN(a) (((a) > 0) - ((a) < 0))

#define LEN(x)  (sizeof(x) / sizeof((x)[0]))

#define polarToCar(r, theta) {SCREEN_WIDTH / 2 + r * cos(degToRad(theta)), SCREEN_HEIGHT / 2 + r * sin(degToRad(theta))}

#define MAX_CONTROLLERS 4

#ifdef DEBUG
	#define LogDebug(...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);
	#define LogInfo(...) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);
#else
	#define LogDebug(...)
	#define LogInfo(...)
#endif

#define LogWarn(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);
#define LogError(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);

#define MUL_UP_1L 1.005f
#define MUL_UP_2L 1.01f
#define MUL_DOWN_1L 0.995025f
#define MUL_DOWN_2L 0.990099f

#define MUL_UP_1M 1.004f
#define MUL_UP_2M 1.008f
#define MUL_DOWN_1M 0.9960159f
#define MUL_DOWN_2M 0.99206349f

#define MUL_UP_1 1.0045f
#define MUL_UP_2 1.009f
#define MUL_DOWN_1 0.995520159f
#define MUL_DOWN_2 0.9910802775f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

struct Vector2f {
	real32 x;
	real32 y;

	real32 getMagnitude() const {
		real32 magnitude = sqrtf(x * x + y * y);
		return magnitude;
	}

	void normalize() {
		real32 magnitude = sqrtf(x*x + y * y);
		if (magnitude > 0) {
			x = x / magnitude;
			y = y / magnitude;
		}
	}

	Vector2f getNormalized() {
		Vector2f normalized = *this;
		normalized.normalize();
		return normalized;
	}

	// With vectors

	Vector2f& operator+=(Vector2f b) {
		this->x += b.x;
		this->y += b.y;
		return *this;
	}
	Vector2f& operator-=(Vector2f b) {
		this->x -= b.x;
		this->y -= b.y;
		return *this;
	}
	// Hadamard product
	Vector2f& operator*=(Vector2f b) {
		this->x *= b.x;
		this->y *= b.y;
		return *this;
	}
	Vector2f& operator/=(Vector2f b) {
		this->x /= b.x;
		this->y /= b.y;
		return *this;
	}

	// Scaling
	Vector2f& operator*=(real32 b) {
		this->x = this->x*b;
		this->y = this->y*b;
		return *this;
	}
	Vector2f& operator/=(real32 b) {
		this->x = this->x / b;
		this->y = this->y / b;
		return *this;
	}
};


// With vectors

inline Vector2f operator+(Vector2f a, Vector2f b) {
	Vector2f result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}
inline Vector2f operator-(Vector2f a, Vector2f b) {
	Vector2f result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}
// Hadamard product
inline Vector2f operator*(Vector2f a, Vector2f b) {
	Vector2f result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return result;
}
inline Vector2f operator/(Vector2f a, Vector2f b) {
	Vector2f result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	return result;
}
// Dot product
inline real32 dot(Vector2f a, Vector2f b) {
	return a.x*b.x + a.y*b.y;
}

// Scaling
inline Vector2f operator*(Vector2f a, real32 b) {
	Vector2f result;
	result.x = a.x * b;
	result.y = a.y * b;
	return result;
}
inline Vector2f operator/(Vector2f a, real32 b) {
	Vector2f result;
	result.x = a.x / b;
	result.y = a.y / b;
	return result;
}


struct ControllerInput {
	real32 dir_left;
	real32 dir_right;
	real32 dir_up;
	real32 dir_down;

	int32 mouseMoveX;
	int32 mouseMoveY;
	int32 mouseWheel;
	bool button_mouse_l;
	bool button_mouse_r;
	bool button_mouse_m;

	bool button_a;
	bool button_b;
	bool button_c;
	bool button_d;
	bool button_l;
	bool button_r;
	bool button_l2;
	bool button_r2;
	bool button_select;
	bool button_start;
};

enum State {
	MainMenu, Beginning, Playing, Dead, Paused, GameOver, Shaking, Ending
};

struct GameState {
	Vector2f player_pos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT /2 };
	Vector2f player_speed = {};
	bool player_visible = true;
	uint32 player_lives = 5;

	Vector2f ball_pos = { SCREEN_WIDTH / 2, 40 };
	Vector2f next_ball_pos;
	Vector2f prev_ball_pos = { SCREEN_WIDTH / 2, 90 };
	Vector2f ball_direction;
	real32 ball_speed = 500;
	real32 ball_scale = 0.01f;
	bool h_reflect = true;
	bool v_reflect = true;
	bool ball_moves_physically = true;
	bool ball_moves_linearly = false;
	uint8 ball_r = 255;
	uint8 ball_g = 255;
	uint8 ball_b = 255;
	uint8 ball_phase = 0;
	uint32 ball_phase_frames = 0;
	uint32 ball_stage = 0;

	Vector2f enemy_pos = { SCREEN_WIDTH / 2, -32 };

	State current_state = MainMenu;
	uint32 dead_frames = 0;
	uint32 playing_frames = 0;
	uint32 gameover_frames = 0;
	uint32 shaking_frames = 0;
	uint32 beginning_frames = 0;
	bool shaking_for_dead = false;

	bool dests_visible = false;
	SDL_Color dests_color = {};
};

extern GameState * state;

struct BallPhase {
	uint32 total_frames;
	std::function<void()> execute;

	void update() {
		if (ballPhaseCheck(total_frames)) {
			execute();
			state->ball_phase_frames++;
		}
	}

	static bool ballPhaseCheck(uint32 frame_limit) {
		if (state->ball_phase_frames < frame_limit) {
			return true;
		}
		else {
			state->ball_phase++;
			state->ball_phase_frames = 0;
			return false;
		}
	}
};