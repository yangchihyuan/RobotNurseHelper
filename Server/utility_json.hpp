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
            // usage of  nlohmann::json::parse
            // 1. file_setting: input stream
            // 2. nullptr: we don't use a callback function
            // 3. true: exception (allow_exceptions)
            // 4. true: ignore_comments
            nlohmann::json j = nlohmann::json::parse(file_setting, nullptr, true, true);
            
            Struct = j.get<T>();
        } catch (nlohmann::json::parse_error& e) {
            std::cerr << "JSON Parse Error in " << file_path << ": " << e.what() << std::endl;
            throw;
        } catch (nlohmann::json::type_error& e) {
            std::cerr << "JSON Type Error (Mapping to Struct failed): " << e.what() << std::endl;
            throw;
        } catch (nlohmann::json::exception& e) {
            //Catch-all for the rest of nlohmann's exception hierarchy (e.g. out_of_range
            //from a required key missing). Without this, a single incompatible field
            //would throw past LoadJSONFile and crash the whole server at startup.
            std::cerr << "JSON Error while loading " << file_path << ": " << e.what() << std::endl;
            //Raise an exception to indicate that the loading failed, so the caller can handle it appropriately.
            throw;
        }
        file_setting.close();
    } else {
        std::cerr << "Could not open " << file_path << " for reading!" << std::endl;
    }
}