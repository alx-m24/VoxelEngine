#pragma once

#include "Bindable.hpp"

namespace VoxelEngine {
    class SmartBinder {
        private:
            const Bindable& m_bindable;

        public:
            SmartBinder(const Bindable& bindable) : m_bindable(bindable) { m_bindable.Bind(); }
            ~SmartBinder() { m_bindable.Unbind(); }
    };
}
