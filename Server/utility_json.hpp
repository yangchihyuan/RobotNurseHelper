#pragma once

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

template <typename T>
inline void LoadJSONFile(T &Struct, const std::string& file_path)
{
    std::ifstream file_setting(file_path);
    if (file_setting.is_open())
    {
        try {
            nlohmann::json j;
            file_setting >> j;
            Struct = j.get<T>();
        } catch (nlohmann::json::parse_error& e) {
            std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        }
        file_setting.close();
    } else {
        std::cerr << "Could not open Setting.json for reading!" << std::endl;
    }
}