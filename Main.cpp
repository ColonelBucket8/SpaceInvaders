#include "OurShader.hpp"

#include <cstdint>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

const int buffer_width = 224;
const int buffer_height = 256;

bool game_running = false;
int move_dir = 0;

struct Alien
{
	size_t x, y;
	uint8_t type;
};

struct Player
{
	size_t x, y;
	size_t life;
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
	Alien* aliens;
	Player player;
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
	buffer.width = buffer_width;
	buffer.height = buffer_height;
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
	Sprite alien_sprite0;
	alien_sprite0.width = 11;
	alien_sprite0.height = 8;
	alien_sprite0.data = new uint8_t[11 * 8]
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

	Sprite alien_sprite1;
	alien_sprite1.width = 11;
	alien_sprite1.height = 8;
	alien_sprite1.data = new uint8_t[88]
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

	SpriteAnimation* alien_animation = new SpriteAnimation;
	alien_animation->loop = true;
	alien_animation->num_frames = 2;
	alien_animation->frame_duration = 10;
	alien_animation->time = 0;

	alien_animation->frames = new Sprite * [2];
	alien_animation->frames[0] = &alien_sprite0;
	alien_animation->frames[1] = &alien_sprite1;


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

	Game game;
	game.width = buffer_width;
	game.height = buffer_height;
	game.num_aliens = 55;
	game.aliens = new Alien[game.num_aliens];

	game.player.x = (buffer_width / 2) - (player_sprite.width / 2);
	game.player.y = 32;

	game.player.life = 3;

	// Set alien positions
	for (size_t yi = 0; yi < 5; yi++)
	{
		for (size_t xi = 0; xi < 11; xi++)
		{
			game.aliens[yi * 11 + xi].x = 16 * xi + 20;
			game.aliens[yi * 11 + xi].y = 17 * yi + 128;
		}
	}

	int player_move_dir = 1;

	game_running = true;
	while (!glfwWindowShouldClose(window) && game_running)
	{
		buffer_clear(&buffer, clear_color);

		// Draw
		for (size_t ai = 0; ai < game.num_aliens; ai++)
		{
			const Alien& alien = game.aliens[ai];
			size_t current_frame = alien_animation->time / alien_animation->frame_duration;
			const Sprite& sprite = *alien_animation->frames[current_frame];
			buffer_draw_sprite(&buffer, sprite,
				alien.x, alien.y, rgb_to_uint32(128, 0, 0));
		}

		buffer_draw_sprite(&buffer, player_sprite,
			game.player.x, game.player.y, rgb_to_uint32(128, 0, 0));

		// Input
		int player_move_dir = 2 * move_dir;
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
		++alien_animation->time;
		if (alien_animation->time == alien_animation->num_frames * alien_animation->frame_duration)
		{
			if (alien_animation->loop)
			{
				alien_animation->time = 0;
			}
			else
			{
				delete alien_animation;
				alien_animation = nullptr;
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLint>(buffer.width),
			static_cast<GLint>(buffer.height), GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);

	delete[] alien_sprite0.data;
	delete[] alien_sprite1.data;
	delete[] alien_animation->frames;
	delete[] buffer.data;
	delete[] game.aliens;

	delete alien_animation;

	return 0;
}