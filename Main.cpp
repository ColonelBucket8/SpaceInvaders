#include "OurShader.hpp"

#include <cstdint>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

const int BUFFER_WIDTH = 224;
const int BUFFER_HEIGHT = 256;
const int GAME_MAX_BULLETS = 200;

bool game_running = false;
int player_move_dir = 1;
int move_dir = 0;
bool fire_pressed = 0;

enum AlienType : uint8_t
{
	ALIEN_DEAD = 0,
	ALIEN_TYPE_A = 1,
	ALIEN_TYPE_B = 2,
	ALIEN_TYPE_C = 3,
};

struct Alien
{
	size_t x, y;
	AlienType type;
};

struct Player
{
	size_t x, y;
	size_t life;
};

struct Bullet
{
	size_t x, y;
	int dir;
};

struct Sprite
{
	size_t width, height;
	uint8_t* data;
};

struct SpriteAnimation
{
	bool loop;
	size_t num_frames;
	size_t frame_duration;
	size_t time;
	Sprite** frames;
};

struct Game
{
	size_t width, height;
	size_t num_aliens;
	size_t num_bullets;
	Alien* aliens;
	Player player;
	Bullet bullets[GAME_MAX_BULLETS];
};

struct Buffer
{
	size_t width, height;
	uint32_t* data;
};


uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b)
{
	return (r << 24) | (g << 16) | (b << 8) | 255;
}

void buffer_clear(Buffer* buffer, uint32_t color)
{
	for (size_t i = 0; i < buffer->width * buffer->height; i++)
	{
		buffer->data[i] = color;
	}
}

void buffer_draw_sprite(
	Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color
)
{
	for (size_t xi = 0; xi < sprite.width; xi++)
	{
		for (size_t yi = 0; yi < sprite.height; yi++)
		{
			size_t sy = sprite.height - 1 + y - yi;
			size_t sx = x + xi;
			if (sprite.data[yi * sprite.width + xi] && sy < buffer->height && sx < buffer->width)
			{
				buffer->data[sy * buffer->width + sx] = color;
			}
		}
	}
}

bool sprite_overlap_check(
	const Sprite& sp_a, size_t x_a, size_t y_a,
	const Sprite& sp_b, size_t x_b, size_t y_b
)
{
	if (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
		y_a < y_b + sp_b.height && y_a + sp_a.height > y_b)
	{
		return true;
	}

	return false;
}

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %d: %s\n", error, description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS) game_running = false;
		break;
	case GLFW_KEY_RIGHT:
		if (action == GLFW_PRESS) move_dir += 1;
		else if (action == GLFW_RELEASE) move_dir -= 1;
		break;
	case GLFW_KEY_LEFT:
		if (action == GLFW_PRESS) move_dir -= 1;
		else if (action == GLFW_RELEASE) move_dir += 1;
		break;
	case GLFW_KEY_SPACE:
		if (action == GLFW_PRESS) fire_pressed = true;
		break;
	default:
		break;
	}
}

int main()
{
	GLFWerrorfun glfwSetErrorCallback(GLFWerrorfun cbfun);
	glfwSetErrorCallback(error_callback);

	GLFWwindow* window;

	if (!glfwInit())
	{
		return -1;
	}

	window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);
	glfwSetKeyCallback(window, key_callback);

	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD\n");
		return -1;
	}

	int glVersion[2] = { -1, 1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);

	glClearColor(1.0, 0.0, 0.0, 1.0);

	uint32_t clear_color = rgb_to_uint32(0, 128, 0);
	Buffer buffer;
	buffer.width = BUFFER_WIDTH;
	buffer.height = BUFFER_HEIGHT;
	buffer.data = new uint32_t[buffer.width * buffer.height];
	buffer_clear(&buffer, clear_color);

	GLuint fullscreen_triangle_vao;
	glGenVertexArrays(1, &fullscreen_triangle_vao);
	glBindVertexArray(fullscreen_triangle_vao);

	OurShader ourShader("shader.vs.glsl", "shader.fs.glsl");

	GLuint buffer_texture;
	glGenTextures(1, &buffer_texture);

	glBindTexture(GL_TEXTURE_2D, buffer_texture);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGB8,
		static_cast<GLint>(buffer.width), static_cast<GLint>(buffer.height), 0,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	ourShader.use();
	ourShader.setInt("buffer", 0);

	// OpenGL setup
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glfwSwapInterval(1);

	glBindVertexArray(fullscreen_triangle_vao);

	// Alien sprite
	const size_t ALIEN_SPRITES_MAX = 6;
	Sprite alien_sprites[ALIEN_SPRITES_MAX];

	alien_sprites[0].width = 8;
	alien_sprites[0].height = 8;
	alien_sprites[0].data = new uint8_t[64]
	{
		0,0,0,1,1,0,0,0, // ...@@...
		0,0,1,1,1,1,0,0, // ..@@@@..
		0,1,1,1,1,1,1,0, // .@@@@@@.
		1,1,0,1,1,0,1,1, // @@.@@.@@
		1,1,1,1,1,1,1,1, // @@@@@@@@
		0,1,0,1,1,0,1,0, // .@.@@.@.
		1,0,0,0,0,0,0,1, // @......@
		0,1,0,0,0,0,1,0  // .@....@.
	};

	alien_sprites[1].width = 8;
	alien_sprites[1].height = 8;
	alien_sprites[1].data = new uint8_t[64]
	{
		0,0,0,1,1,0,0,0, // ...@@...
		0,0,1,1,1,1,0,0, // ..@@@@..
		0,1,1,1,1,1,1,0, // .@@@@@@.
		1,1,0,1,1,0,1,1, // @@.@@.@@
		1,1,1,1,1,1,1,1, // @@@@@@@@
		0,0,1,0,0,1,0,0, // ..@..@..
		0,1,0,1,1,0,1,0, // .@.@@.@.
		1,0,1,0,0,1,0,1  // @.@..@.@
	};

	alien_sprites[2].width = 11;
	alien_sprites[2].height = 8;
	alien_sprites[2].data = new uint8_t[88]
	{
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
		0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
		0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
		1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
		0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
	};

	alien_sprites[3].width = 11;
	alien_sprites[3].height = 8;
	alien_sprites[3].data = new uint8_t[88]
	{
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
		1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
		1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
	};

	alien_sprites[4].width = 12;
	alien_sprites[4].height = 8;
	alien_sprites[4].data = new uint8_t[96]
	{
		0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
		0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
		0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
		1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
	};


	alien_sprites[5].width = 12;
	alien_sprites[5].height = 8;
	alien_sprites[5].data = new uint8_t[96]
	{
		0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
		0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
		1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
		0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
		0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
		0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
	};

	Sprite alien_death_sprite;
	alien_death_sprite.width = 13;
	alien_death_sprite.height = 7;
	alien_death_sprite.data = new uint8_t[91]
	{
		0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
		0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
		0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
		1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
		0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
		0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
		0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
	};

	const size_t ALIEN_ANIMATION_MAX = 3;
	SpriteAnimation alien_animation[ALIEN_ANIMATION_MAX];

	for (size_t i = 0; i < ALIEN_ANIMATION_MAX; i++)
	{
		alien_animation[i].loop = true;
		alien_animation[i].num_frames = 2;
		alien_animation[i].frame_duration = 10;
		alien_animation[i].time = 0;

		alien_animation[i].frames = new Sprite * [2];
		alien_animation[i].frames[0] = &alien_sprites[2 * i];
		alien_animation[i].frames[1] = &alien_sprites[2 * i + 1];

	}



	// Player sprite
	Sprite player_sprite;
	player_sprite.width = 11;
	player_sprite.height = 7;
	player_sprite.data = new uint8_t[77]
	{
		0,0,0,0,0,1,0,0,0,0,0, // .....@.....
		0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
		0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
		0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	};

	// Bullet sprite
	Sprite bullet_sprite;
	bullet_sprite.width = 1;
	bullet_sprite.height = 3;
	bullet_sprite.data = new uint8_t[3]
	{
		1, // @
		1, // @
		1  // @
	};


	Game game;
	game.width = BUFFER_WIDTH;
	game.height = BUFFER_HEIGHT;
	game.num_aliens = 55;
	game.num_bullets = GAME_MAX_BULLETS;
	game.aliens = new Alien[game.num_aliens];

	game.player.x = (BUFFER_WIDTH / 2) - (player_sprite.width / 2);
	game.player.y = 32;

	game.player.life = 3;

	// Set alien positions and types
	for (size_t yi = 0; yi < 5; yi++)
	{
		for (size_t xi = 0; xi < 11; xi++)
		{
			Alien& alien = game.aliens[yi * 11 + xi];
			alien.type = static_cast<AlienType>((5 - yi) / 2 + 1);

			const Sprite& sprite = alien_sprites[2 * (alien.type - 1)];

			alien.x = 16 * xi + 20 + (alien_death_sprite.width - sprite.width) / 2;
			alien.y = 17 * yi + 128;
		}
	}


	// Initialize death counter for aliens
	uint8_t* death_counters = new uint8_t[game.num_aliens];
	for (size_t i = 0; i < game.num_aliens; i++)
	{
		death_counters[i] = 10;
	}

	game_running = true;


	while (!glfwWindowShouldClose(window) && game_running)
	{
		buffer_clear(&buffer, clear_color);

		// Draw
		for (size_t ai = 0; ai < game.num_aliens; ai++)
		{
			if (!death_counters[ai]) continue;

			const Alien& alien = game.aliens[ai];
			if (alien.type == ALIEN_DEAD)
			{
				buffer_draw_sprite(&buffer, alien_death_sprite,
					alien.x, alien.y, rgb_to_uint32(128, 0, 0));
			}
			else
			{
				const SpriteAnimation& animation = alien_animation[alien.type - 1];
				size_t current_frame = animation.time / animation.frame_duration;
				const Sprite& sprite = *animation.frames[current_frame];
				buffer_draw_sprite(&buffer, sprite,
					alien.x, alien.y, rgb_to_uint32(128, 0, 0));
			}
		}

		// Draw bullet
		for (size_t bi = 0; bi < game.num_bullets; bi++)
		{
			const Bullet& bullet = game.bullets[bi];
			const Sprite& sprite = bullet_sprite;
			buffer_draw_sprite(&buffer, sprite,
				bullet.x, bullet.y, rgb_to_uint32(128, 0, 0));
		}

		buffer_draw_sprite(&buffer, player_sprite,
			game.player.x, game.player.y, rgb_to_uint32(128, 0, 0));

		// Input
		player_move_dir = 2 * move_dir;
		if (player_move_dir != 0)
		{
			if (game.player.x + player_sprite.width + player_move_dir >= game.width)
			{
				game.player.x = game.width - player_sprite.width;
			}
			else if (static_cast<int>(game.player.x) + player_move_dir <= 0)
			{
				game.player.x = 0;
			}
			else
			{
				game.player.x += player_move_dir;
			}
		}

		// Update animation
		for (size_t i = 0; i < ALIEN_ANIMATION_MAX; i++)
		{
			++alien_animation[i].time;
			if (alien_animation[i].time == alien_animation[i].num_frames * alien_animation[i].frame_duration)
			{
				alien_animation[i].time = 0;
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLint>(buffer.width),
			static_cast<GLint>(buffer.height), GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glfwSwapBuffers(window);

		// Simulate Alien
		for (size_t ai = 0; ai < game.num_aliens; ai++)
		{
			const Alien& alien = game.aliens[ai];
			if (alien.type == ALIEN_DEAD && death_counters[ai] != 0)
			{
				--death_counters[ai];
			}

		}

		// Simulate bullets
		for (size_t bi = 0; bi < game.num_bullets; bi++)
		{
			game.bullets[bi].y += game.bullets[bi].dir;
			if (game.bullets[bi].y >= game.height ||
				game.bullets[bi].y < bullet_sprite.height)
			{
				game.bullets[bi] = game.bullets[game.num_bullets - 1];
				--game.num_bullets;
				continue;
			}

			// Check hit
			for (size_t ai = 0; ai < game.num_aliens; ai++)
			{
				const Alien& alien = game.aliens[ai];
				if (alien.type == ALIEN_DEAD) continue;

				const SpriteAnimation& animation = alien_animation[alien.type - 1];
				size_t current_frame = animation.time / animation.frame_duration;
				const Sprite& alien_sprite = *animation.frames[current_frame];
				bool overlap = sprite_overlap_check(
					bullet_sprite, game.bullets[bi].x, game.bullets[bi].y,
					alien_sprite, game.aliens[ai].x, game.aliens[ai].y
				);

				if (overlap)
				{
					game.aliens[ai].type = ALIEN_DEAD;
					// NOTE: Hack to recenter death sprite
					game.aliens[ai].x -= (alien_death_sprite.width - alien_sprite.width) / 2;
					game.bullets[bi] = game.bullets[game.num_bullets - 1];
					--game.num_bullets;
					continue;
				}
			}

			++bi;
		}




		// Fire
		if (fire_pressed && game.num_bullets < GAME_MAX_BULLETS)
		{
			game.bullets[game.num_bullets].x = game.player.x + player_sprite.width / 2;
			game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
			game.bullets[game.num_bullets].dir = 2;
			++game.num_bullets;
		}
		fire_pressed = false;

		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);
	for (size_t i = 0; i < ALIEN_ANIMATION_MAX; i++)
	{
		delete[] alien_animation[i].frames;
	}

	delete alien_death_sprite.data;

	for (size_t i = 0; i < ALIEN_SPRITES_MAX; i++)
	{
		delete[] alien_sprites[i].data;
	}

	delete[] buffer.data;
	delete[] game.aliens;
	delete[] death_counters;


	return 0;
}