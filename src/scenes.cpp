#include "raylib.h"
#include "scenes.h"

void scene_table::update() {
    scene_base* current = current_scene;

    if (!current->pending_request().empty()) [[unlikely]] {
        stripped_scene key(current->take_request());
        current->clear_request();

        scene_base* target = nullptr;
        if (const auto it = table.find(&key); it != table.end()) {
            target = *it;
        }

        if (target != nullptr && target != current) {
            TraceLog(LOG_INFO, "scene: switching from '%s' to '%s'",
                     current->name().c_str(), target->name().c_str());
            current_scene = target;
        }
        // If the requested scene is not registered, stay on the current scene.
    }

    current_scene->update();
}

void scene_table::draw() {
    current_scene->draw();
}
