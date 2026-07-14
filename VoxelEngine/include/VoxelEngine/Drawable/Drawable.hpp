// Drawable.hpp
#pragma once
namespace VoxelEngine {
    class Drawable {
        public:
            virtual ~Drawable() = default;
            virtual void Draw() const = 0;
    };
}
