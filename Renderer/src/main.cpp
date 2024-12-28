// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// Imgui
#include "Headers/imgui/imgui.h"
#include "Headers/imgui/imgui_impl_glfw.h"
#include "Headers/imgui/imgui_impl_opengl3.h"
// Other
#include <array>
#include <thread>
#include <iostream>
// My headers
#include "Headers/PerlinNoise/PerlinNoise.hpp"
#include "Headers/Shaders/Shader.hpp"
#include "Headers/ChunkSystem.hpp"
#include "Headers/IO/Input.hpp"
#include "Headers/Camera.hpp"
#include "Headers/Terrain.hpp"
#include "Headers/Model.hpp"

using namespace IO;

int main() {
#pragma region init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#pragma endregion

#pragma region Window and Context
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel Engine", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "Failed to create window" << std::endl;
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	stbi_set_flip_vertically_on_load(true);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
#pragma endregion

#pragma region GUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
#pragma endregion

#pragma region Shader
	const std::string VertexPath = "C:\\Users\\alexa\\OneDrive\\Coding\\VoxelEngine\\Renderer\\Renderer\\src\\Shaders\\Shader.vert";
	const std::string rasterVertexPath = "C:\\Users\\alexa\\OneDrive\\Coding\\VoxelEngine\\Renderer\\Renderer\\src\\Shaders\\Rasterized.vert";
	const std::string FragPath = "C:\\Users\\alexa\\OneDrive\\Coding\\VoxelEngine\\Renderer\\Renderer\\src\\Shaders\\Shader.frag";
	const std::string rasterFragPath = "C:\\Users\\alexa\\OneDrive\\Coding\\VoxelEngine\\Renderer\\Renderer\\src\\Shaders\\Rasterized.frag";

	Shader voxelShader(VertexPath, FragPath);
	Shader rasterized(rasterVertexPath, rasterFragPath);
#pragma endregion

#pragma region Models
	Model backbag("C:\\Users\\alexa\\OneDrive\\Coding\\VoxelEngine\\Renderer\\Renderer\\src\\Models\\BackBag\\backpack.obj");
#pragma endregion

#pragma region Objects
	Camera camera(window, voxelShader);

	camera.Position = glm::vec3(16.0f);

	std::array<glm::vec4, chunkNum>* chunks = new std::array<glm::vec4, chunkNum>;

	// Number of bytes following the std140 layout rule
	// N = 4 bytes
	// vec4 = 4N
	// Therefore 1 element is (4 * N) = (4 * 4) = 16 bytes
	unsigned int voxelSSBO;
	glGenBuffers(1, &voxelSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, VoxelNum * chunkNum * 16, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelSSBO);

	unsigned int chunkSSBO;
	glGenBuffers(1, &chunkSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, chunks->size() * 16, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunkSSBO);

	ChunkSystem chunkSys(camera, chunks, voxelSSBO, chunkSSBO, 1587343);

	glm::vec3 lightDir = { 0.0, -1.0, 0.0 };
	float ambient = 0.2f;
	float diffuse = 0.8f;
	float specular = 0.25f;
	glm::vec3 color = glm::vec3(1.0f);
	bool shadows = true;

	int viewingOptions = 0;
#pragma endregion

#pragma region Quad
	float size = 1.0f;
	float vertices[] = {
		-size, -size, 0.0f, // Bottom right
		size, -size, 0.0f, // Bottom left
		size,  size, 0.0f, // Top left
		-size,  size, 0.0f // Top Right
	};

	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
#pragma endregion

#pragma region Time Variables
	float myTime = 0.0f;
	float lastTime = 0.0f;
	float dt = 0.0f;
#pragma endregion

#pragma region Main Loop
	while (!glfwWindowShouldClose(window)) {
#pragma region Time
		myTime = static_cast<float>(glfwGetTime());
		dt = myTime - lastTime;
		lastTime = myTime;
#pragma endregion

#pragma region Update
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		camera.update(window, voxelShader, dt);

		chunkSys.update();

#pragma region Rasterized
		rasterized.use();

		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, glm::vec3(15.0f, 15.0f, 20.0f));
		model = glm::scale(model, glm::vec3(0.5f));

		rasterized.setMat4("model", model);
		rasterized.setMat4("view", camera.viewMatrix);
		rasterized.setMat4("projection", camera.projectionMatrix);
		rasterized.setFloat("material.shininess", 30);

		rasterized.setVec3("viewPos", camera.Position);

		rasterized.setVec3("dirLight.direction", glm::normalize(lightDir));
		rasterized.setVec3("dirLight.ambient", glm::vec3(ambient));
		rasterized.setVec3("dirLight.diffuse", glm::vec3(diffuse));
		rasterized.setVec3("dirLight.specular", glm::vec3(specular));
		rasterized.setVec3("dirLight.color", color);
#pragma endregion

#pragma region Voxels
		voxelShader.use();

		voxelShader.setInt("viewingOptions", viewingOptions);

		voxelShader.setVec3("dirlight.direction", glm::normalize(lightDir));
		voxelShader.setVec3("dirlight.ambient", glm::vec3(ambient));
		voxelShader.setVec3("dirlight.diffuse", glm::vec3(diffuse));
		voxelShader.setVec3("dirlight.specular", glm::vec3(specular));
		voxelShader.setVec3("dirlight.color", color);
		voxelShader.setBool("dirlight.shadows", shadows);
#pragma endregion

		processInput(window);
#pragma endregion

#pragma region Render
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#pragma region Rasterized
		backbag.draw(rasterized);
#pragma endregion

#pragma region Voxels
		voxelShader.use();

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#pragma endregion
		
#pragma region GUI

		ImGui::ShowMetricsWindow();

		{
			ImGui::Begin("Light");

			ImGui::DragFloat3("Direction", &lightDir[0], 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Ambient", &ambient, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Diffuse", &diffuse, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Specular", &specular, 0.01f, 0.0f, 1.0f);
			ImGui::Checkbox("Shadows", &shadows);
			ImGui::ColorEdit3("Color", &color[0]);

			ImGui::End();
		}
		{
			ImGui::Begin("Camera");

			ImGui::DragFloat3("Position", &camera.Position[0], 0.01f);

			ImGui::DragFloat("FOV", &camera.FOV, 1.0f, 0.0f);

			ImGui::RadioButton("Lighting", &viewingOptions, 0);
			ImGui::RadioButton("Colors", &viewingOptions, 1);
			ImGui::RadioButton("Normals", &viewingOptions, 2);
			ImGui::RadioButton("Distance", &viewingOptions, 3);

			ImGui::End();
		}
		

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

		glfwSwapBuffers(window);
#pragma endregion
	}
#pragma endregion

#pragma region Terminate
	glDeleteBuffers(1, &voxelSSBO);
	glDeleteBuffers(1, &chunkSSBO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
#pragma endregion
}

// TODO: Add 3D-rasterized models to voxels convertion
// TODO: Add colored shadows
// TODO: Add point lights