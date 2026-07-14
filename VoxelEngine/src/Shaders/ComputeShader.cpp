#include "pch.h"
#include "VoxelEngine/Shaders/ComputeShader.hpp"

#include "VoxelEngine/utils/error.hpp"

namespace VoxelEngine {
    ComputeShader::ComputeShader(const std::string& src, bool isFromFile)
    {
        std::string shaderCode = src;
        if (isFromFile) {
            std::ifstream ShaderFile;
            // ensure ifstream objects can throw exceptions:
            ShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try
            {
                // open files
                ShaderFile.open(src);
                std::stringstream vShaderStream;
                // read file's buffer contents into streams
                vShaderStream << ShaderFile.rdbuf();
                // close file handlers
                ShaderFile.close();
                // convert stream into string
                shaderCode = vShaderStream.str();
            }
            catch (std::ifstream::failure& e)
            {
                V_LOG_ERROR("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " + std::string(e.what()));
            }
        }
        const char* ShaderCode = shaderCode.c_str();
    
        unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &ShaderCode, NULL);
        glCompileShader(compute);
        checkCompileErrors(compute, "COMPUTE");
    
        ID = glCreateProgram();
        glAttachShader(ID, compute);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
    }

    uint64_t GetMaxThreadsPerDispatch(int localSizeX, int localSizeY, int localSizeZ)
    {
        // Query how many groups you can launch per dimension
        GLint maxGroups[3];
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxGroups[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxGroups[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxGroups[2]);
    
        // Total number of threads = groups * local size
        uint64_t totalThreads =
            static_cast<uint64_t>(maxGroups[0]) *
            static_cast<uint64_t>(maxGroups[1]) *
            static_cast<uint64_t>(maxGroups[2]) *
            static_cast<uint64_t>(localSizeX * localSizeY * localSizeZ);
    
        return totalThreads;
    }
}
