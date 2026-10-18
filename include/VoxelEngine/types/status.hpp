#pragma once

#include <string>

namespace VoxelEngine::Type {
    struct Status {
        bool success = true;
        std::string error{};

        Status() = default;
        Status(std::string&& error) : success(false), error(std::move(error)) {}

        bool ok() const {
            return success;
        }

        const std::string& getError() const {
            return error;
        }

        operator bool() const {
            return ok();
        }
    };
}
