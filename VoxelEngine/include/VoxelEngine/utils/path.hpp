#pragma once

std::filesystem::path get_binary_directory();
std::string to_standard_string(const std::filesystem::path::string_type& native_str);
std::string to_escaped_path(const std::filesystem::path& path);
