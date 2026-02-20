#include "settings.hpp"

#define FROM(name) as.name = j.value(#name, as.name)
#define TO(name) {#name, as.name}

namespace settings {
    namespace app_settings {
        void to_json(json& j, const AppSettings& as) {
            j = json{
                TO(vsync),
                TO(window_height),
                TO(window_pos_x),
                TO(window_pos_y),
                TO(window_width),
            };
        }

        void from_json(const json& j, AppSettings& as) {
            FROM(vsync);
            FROM(window_height);
            FROM(window_pos_x);
            FROM(window_pos_y);
            FROM(window_width);
        }
    }
}
