#include <cstdint>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OurShader.hpp"

const int buffer_width = 224;
const int buffer_height = 256;

struct Buffer
{
	size_t width, height;
	uint32_t* data;
};

struct Sprite
{
	size_t width, height;
	uint8_t* data;
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

void buffer_sprite_draw(
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

	glBindVertexArray(fullscreen_triangle_vao);

	Sprite alien_sprite;
	alien_sprite.width = 11;
	alien_sprite.height = 8;
	alien_sprite.data = new uint8_t[11 * 8]
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

	while (!glfwWindowShouldClose(window))
	{
		buffer_clear(&buffer, clear_color);

		buffer_sprite_draw(&buffer, alien_sprite, 112, 128, rgb_to_uint32(128, 0, 0));

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLint>(buffer.width),
			static_cast<GLint>(buffer.height), GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);

	delete[] alien_sprite.data;
	delete[] buffer.data;

	return 0;
}