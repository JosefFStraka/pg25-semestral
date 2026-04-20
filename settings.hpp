#pragma once 

#include <string>
#include <map>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace settings {
    namespace app_settings {
        struct AppSettings {
            int window_width = 800;
            int window_height = 600;
            int window_pos_x = -1;
            int window_pos_y = -1;
            int window_maximized = 0;
            bool fullscreen = false;
            bool gui_always_enabled = false;
            bool gui_axis_display_enabled = false;
            bool gui_enabled = false;
            bool vsync = true;
            bool msaa_enabled = true;

            //not saved
            bool debug_draw_aabb = false;
            bool debug_freeze_frustum = false;
        };

        void to_json(json& j, const AppSettings& as);
        void from_json(const json& j, AppSettings& as);
    }

    template <typename T>
    int load(std::string path, T& data) {
        if (!std::filesystem::exists(path)) {
            return 1;
        }
        std::ifstream i(path);
        data = json::parse(i);
        return 0;
    }
    template <typename T>
    int save(std::string path, T& data) {
        json j = data;
        std::ofstream o(path);
        o << j.dump(4) << std::endl;
        return 0;
    }
}
