#include "settings.hpp"

#define FROM(name) as.name = j.value(#name, as.name)
#define TO(name) {#name, as.name}

namespace settings {
    namespace app_settings {
        void to_json(json& j, const AppSettings& as) {
            j = json{
                TO(fullscreen),
                TO(gui_always_enabled),
                TO(gui_axis_display_enabled),
                TO(gui_enabled),
                TO(vsync),
                TO(window_height),
                TO(window_pos_x),
                TO(window_pos_y),
                TO(window_width),
                TO(msaa_enabled)
            };
        }

        void from_json(const json& j, AppSettings& as) {
            FROM(fullscreen);
            FROM(gui_always_enabled);
            FROM(gui_axis_display_enabled);
            FROM(gui_enabled);
            FROM(vsync);
            FROM(window_height);
            FROM(window_pos_x);
            FROM(window_pos_y);
            FROM(window_width);
            FROM(msaa_enabled);
        }
    }
}
