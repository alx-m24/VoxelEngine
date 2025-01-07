#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Inputs and outputs
namespace IO {
	extern unsigned int SCR_WIDTH;
	extern unsigned int SCR_HEIGHT;
	extern float xoffset;
	extern float yoffset;

	extern bool firstMouse;
	extern bool lastEsc;
	extern bool useCam;

	extern float lastX;
	extern float lastY;

	extern unsigned int depthTex;
}

using namespace IO;

void inline framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	SCR_WIDTH = width;
	SCR_HEIGHT = height;

	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void inline mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	xoffset = xpos - lastX;
	yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;
}

void inline processInput(GLFWwindow* window)
{
	bool Esc = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	if (Esc && !lastEsc) {
		useCam = !useCam;

		if (useCam) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	lastEsc = Esc;

	xoffset = 0.0f;
	yoffset = 0.0f;
}
