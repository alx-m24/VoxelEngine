#include "Input.hpp"

unsigned int IO::SCR_WIDTH = 1200;
unsigned int IO::SCR_HEIGHT = 750;
float IO::xoffset = 0.0f;
float IO::yoffset = 0.0f;
bool IO::firstMouse = true;
float IO::lastX = IO::SCR_WIDTH / 2.0f;
float IO::lastY = IO::SCR_HEIGHT / 2.0f;
bool IO::lastEsc = false;
bool IO::useCam = false;
