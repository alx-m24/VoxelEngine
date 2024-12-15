// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// Imgui
#include "Headers/imgui/imgui.h"
#include "Headers/imgui/imgui_impl_glfw.h"
#include "Headers/imgui/imgui_impl_opengl3.h"
// Other
#include <map>
#include <array>
#include <iostream>
// My headers
#include "Headers/PerlinNoise/PerlinNoise.hpp"
#include "Headers/Shaders/Shader.hpp"
#include "Headers/IO/Input.hpp"
#include "Headers/Camera.hpp"

using namespace IO;

constexpr unsigned int ChunkSize = 16 * 1; // Number of voxels per chunk
constexpr float VoxelSize = 1.0f / 1.0f;
constexpr int VoxelNum = ChunkSize * ChunkSize * ChunkSize;

constexpr int renderRadius = 1;
constexpr int numberOfChunksInAStraightLine = (2 * renderRadius + 1);
constexpr int chunkNum = numberOfChunksInAStraightLine * numberOfChunksInAStraightLine;

static constexpr unsigned int toIdx(glm::vec3 position) {
	return static_cast<int>(position.x) + static_cast<int>(position.y) * ChunkSize + static_cast<int>(position.z) * ChunkSize * ChunkSize;
}
static constexpr glm::vec3 toGridPos(glm::vec3 position) {
	return position / VoxelSize;
}
static bool isValid(glm::vec3 pos) {
	return (pos.x >= 0 && pos.y >= 0 && pos.z >= 0) && (pos.x < ChunkSize && pos.y < ChunkSize && pos.z < ChunkSize);
}

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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

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
	const std::string FragPath = "C:\\Users\\alexa\\OneDrive\\Coding\\VoxelEngine\\Renderer\\Renderer\\src\\Shaders\\Shader.frag";

	Shader shader(VertexPath, FragPath);
#pragma endregion

#pragma region Objects
	std::array<glm::vec4, chunkNum >* chunks = new std::array<glm::vec4, chunkNum>;
	chunks->fill(glm::vec4(0.0f));

	float chunkMaxPos = ChunkSize * VoxelSize;

	(*chunks)[0] = glm::vec4(0.0f);
	(*chunks)[1] = glm::vec4(chunkMaxPos, 0.0f, 0.0f, 0.0f);
	(*chunks)[2] = glm::vec4(-chunkMaxPos, 0.0f, 0.0f, 0.0f);
	(*chunks)[3] = glm::vec4(0.0f, 0.0f, chunkMaxPos, 0.0f);
	(*chunks)[4] = glm::vec4(0.0f, 0.0f, -chunkMaxPos, 0.0f);
	(*chunks)[5] = glm::vec4(chunkMaxPos, 0.0f, -chunkMaxPos, 0.0f);
	(*chunks)[6] = glm::vec4(chunkMaxPos, 0.0f, chunkMaxPos, 0.0f);
	(*chunks)[7] = glm::vec4(-chunkMaxPos, 0.0f, chunkMaxPos, 0.0f);
	(*chunks)[8] = glm::vec4(-chunkMaxPos, 0.0f, -chunkMaxPos, 0.0f);

	std::array<glm::vec4, VoxelNum* chunkNum>* voxels = new std::array<glm::vec4, VoxelNum* chunkNum>;
	voxels->fill(glm::vec4(0.0f));

	const siv::PerlinNoise::seed_type seed = 123456u;
	const siv::PerlinNoise perlin{ seed };
	const float denominator = 0.5f * (ChunkSize * VoxelSize);

	srand(time(0));
	for (int chunkIdx = 0; chunkIdx < chunkNum; ++chunkIdx) {
		glm::vec3 chunkPos = glm::vec3((*chunks)[chunkIdx]);

		for (int i = 0; i < ChunkSize; ++i) {
			for (int j = 0; j < ChunkSize; ++j) {
				for (int k = 0; k < ChunkSize; ++k) {
					glm::vec3 position = (glm::vec3(i, j, k) * VoxelSize) + (VoxelSize / 2.0f) + chunkPos;

					const double noise = perlin.noise3D_01(position.x / denominator, position.y / denominator, position.z / denominator);

					glm::vec3 index = toGridPos(position - chunkPos);

					glm::vec4 color = noise > 0.5f ?
						glm::vec4(
							(float)(rand() % 255) / 255.0f,
							(float)(rand() % 255) / 255.0f,
							(float)(rand() % 255) / 255.0f,
							1.0f)
						: glm::vec4(0.0f);

					//color = glm::vec4(noise, noise, noise, 1.0f);

					(*voxels)[toIdx(index) + chunkIdx * VoxelNum] = color;
				}
			}
		}
	}

	// DEBUG
	(*voxels)[toIdx(glm::vec4(15.0f))] = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

	// Number of bytes following the std140 layout rule
	// N = 4 bytes
	// vec4 = 4N
	// Therefore 1 element is (4 * N) = (4 * 4) = 16 bytes

	unsigned int chunkSSBO;
	glGenBuffers(1, &chunkSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, chunks->size() * 16, chunks->data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunkSSBO);

	unsigned int voxelSSBO;
	glGenBuffers(1, &voxelSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, voxels->size() * 16, voxels->data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelSSBO);

	Camera camera(window, shader);

	camera.Position = glm::vec3(16.0f);

	glm::vec3 lightDir = { 0.0, -1.0, 0.0 };
	float ambient = 0.2f;
	float diffuse = 0.8f;
	float specular = 1.0f;
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

		shader.use();

		shader.use();
		camera.update(window, shader, dt);

		shader.setInt("viewingOptions", viewingOptions);

		shader.setVec3("dirlight.direction", glm::normalize(lightDir));
		shader.setVec3("dirlight.ambient", glm::vec3(ambient));
		shader.setVec3("dirlight.diffuse", glm::vec3(diffuse));
		shader.setVec3("dirlight.specular", glm::vec3(specular));
		shader.setVec3("dirlight.color", color);
		shader.setBool("dirlight.shadows", shadows);

		processInput(window);
#pragma endregion

#pragma region Render
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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
	glDeleteBuffers(1, &voxelSSBO);
	glDeleteBuffers(1, &chunkSSBO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}