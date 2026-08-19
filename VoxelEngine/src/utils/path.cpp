#include "pch.h"
#include "VoxelEngine/utils/path.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

std::filesystem::path get_binary_directory() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();

#elif defined(__linux__)
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();

#elif defined(__APPLE__)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return std::filesystem::weakly_canonical(buffer).parent_path();
    }
#endif
    return std::filesystem::current_path(); // Fallback
}

std::string to_standard_string(const fs::path::string_type& native_str) {
#if defined(_WIN32)
    // Windows: Convert std::wstring to std::string safely
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(native_str);
#else
    // Linux/macOS: string_type is already std::string
    return native_str;
#endif
}

std::string to_escaped_path(const fs::path& path) {
    std::string str = path.string();
    std::string result;
    for (char c : str) {
        if (c == '\\') {
            result += "\\\\"; // Add two backslashes
        } else {
            result += c;
        }
    }
    return result;
}
