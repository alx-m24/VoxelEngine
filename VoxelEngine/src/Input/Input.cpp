#include "pch.h"
#include "VoxelEngine/Input/Input.hpp"

namespace VoxelEngine {
	namespace Input {
		namespace {
            [[maybe_unused]] constexpr size_t KeysNum = 120;
    		constexpr size_t MaxGLFWKeys = 348 + 1;

    		[[maybe_unused]] constexpr size_t MouseButtonNum = 8;
    		constexpr size_t MaxGLFWMouseButtons = 7 + 1;

			std::bitset<MaxGLFWKeys> keysDown{};
            std::bitset<MaxGLFWKeys> keysDownLastFrame{};
			std::bitset<MaxGLFWKeys> keysRepeat{};
			std::bitset<MaxGLFWMouseButtons> mouseButtons{};
			    
			glm::vec2 mousePos{};
			glm::vec2 lastMousePos{};
			glm::vec2 mouseDelta{};

            float mouseScroll = 0.0f;

			bool firstMouseCallback = true;
		}


		void keyCallback(GLFWwindow*, int key, int, int action, int) {
			// We will ignore MODS and SCANCODE
			//
			// key == GLFW_KEY_~
			// Action == GLFW_PRESS || GLFW_RELEASE || GLFW_REPEAT
			
			if (key < 0 || static_cast<size_t>(key) >= MaxGLFWKeys) return;

			size_t keyIndex = static_cast<size_t>(key);
			if (action == GLFW_RELEASE) {
				keysDown.reset(keyIndex);
				keysRepeat.reset(keyIndex);
			}
			else if (action == GLFW_REPEAT) {
				keysRepeat.set(keyIndex);
			}
			else if (action == GLFW_PRESS) {
				keysDown.set(keyIndex);
			}
		}

		void MousePositionCallback(GLFWwindow*, double xpos, double ypos) {
			mousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

			if (firstMouseCallback) {
				firstMouseCallback = false;
				lastMousePos = mousePos;
			}
		}

		void MouseButtonCallback(GLFWwindow*, int button, int action, int) {
            if (button < 0 || static_cast<size_t>(button) >= MaxGLFWMouseButtons) return;

            size_t buttonIndex = static_cast<size_t>(button);
            if (action == GLFW_RELEASE) {
                mouseButtons.reset(buttonIndex);
            }
            else if (action == GLFW_PRESS) {
                mouseButtons.set(buttonIndex);
            }
		}
		
		void MouseScrollCallback(GLFWwindow*, double, double yoffset) { 
            mouseScroll += static_cast<float>(yoffset);
		}

        bool isKeyDown(Key key) {
            return keysDown.test(static_cast<size_t>(key));
        }
        
        bool wasKeyReleased(Key key) {
            size_t k = static_cast<size_t>(key);
            return keysDownLastFrame.test(k) && !keysDown.test(k);
        }

        bool wasKeyPressed(Key key) {
            size_t k = static_cast<size_t>(key);
            return !keysDownLastFrame.test(k) && keysDown.test(k);
        }

        bool isKeyRepeat(Key key) {
            return keysRepeat.test(static_cast<size_t>(key));
        }

        bool isMouseButtonDown(MouseButton mouse) {
            return mouseButtons.test(static_cast<size_t>(mouse));
        }

        float getScrollDelta() {
            return mouseScroll;
        }

        glm::vec2 getMousePosition() {
            return mousePos;
        }
        
        glm::vec2 getMouseDelta() {
            return mouseDelta;
        }

        void CalculateDeltas() {
            mouseDelta = (firstMouseCallback) ? glm::vec2(0.0f) : mousePos - lastMousePos;
            lastMousePos = mousePos;
        }

        void ClearFrameData() {
			keysRepeat.reset();
            keysDownLastFrame = keysDown;
            mouseScroll = 0.0f;
        }
	}
}
