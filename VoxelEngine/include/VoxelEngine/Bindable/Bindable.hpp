#pragma once

namespace VoxelEngine {
    class Bindable {
        public:
            Bindable() = default;
            virtual ~Bindable() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;
    };
}
