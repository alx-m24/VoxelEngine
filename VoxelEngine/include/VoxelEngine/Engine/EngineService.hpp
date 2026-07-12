#pragma once

namespace VoxelEngine {
    class EngineService {
        public:
            EngineService() = default;
            virtual ~EngineService() = default;
            virtual void RequestClose() = 0;

            enum class CURSOR_MODE {
                LOCKED,
                FREE
            };
            virtual void setCursorMode(EngineService::CURSOR_MODE mode) = 0;

            virtual void setVSync(bool on) = 0;
    };
}
