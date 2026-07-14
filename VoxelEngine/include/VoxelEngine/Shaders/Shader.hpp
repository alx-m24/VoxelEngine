#pragma once

#include <string>

#include "VoxelEngine/Maths/units.hpp"
#include "VoxelEngine/Bindable/Bindable.hpp"

namespace VoxelEngine {

    class Shader : public Bindable {
    public:
        unsigned int ID = 0;

    protected:
        void checkCompileErrors(unsigned int shader, std::string type);

    public:
        Shader() = default;
        Shader(std::string vertexSrc, std::string fragmentSrc, std::string geometrySrc = "", bool isFromFile = true);

		void Bind() const override;
		void Unbind() const override;
    public:
        void setBool(const std::string& name, bool value) const;
        void setInt(const std::string& name, int value) const;
        void setUint(const std::string& name, unsigned int value) const;
        void setFloat(const std::string& name, float value) const;
        void setVec2(const std::string& name, const vec2& value) const;
        void setVec2(const std::string& name, float x, float y) const;
        void setiVec2(const std::string& name, const ivec2& value) const;
        void setVec3(const std::string& name, const vec3& value) const;
        void setiVec3(const std::string& name, const ivec3& value) const;
        void setVec3(const std::string& name, float x, float y, float z) const;
        void setVec4(const std::string& name, const vec4& value) const;
        void setVec4(const std::string& name, float x, float y, float z, float w) const;
        void setMat2(const std::string& name, const mat2& mat) const;
        void setMat3(const std::string& name, const mat3& mat) const;
        void setMat4(const std::string& name, const mat4& mat) const;
    };
}
