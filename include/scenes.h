#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>

// Base class for every scene in the game.
//
// Scenes are looked up by name, so a scene can request a switch to any other
// registered scene through request_scene(). scene_table performs the switch
// before the next update of the target scene.
//
// Scene lifetime is managed by the creator (main()); scene_table only stores
// non-owning pointers, so every scene must outlive the scene_table that uses
// it. Scenes are not copyable or movable because they own raylib resources.
class scene_base {
public:
    explicit scene_base(std::string name) : _name(std::move(name)) {}

    virtual ~scene_base() = default;

    scene_base(const scene_base&) = delete;
    scene_base& operator=(const scene_base&) = delete;
    scene_base(scene_base&&) = delete;
    scene_base& operator=(scene_base&&) = delete;

    [[nodiscard]] const std::string& name() const noexcept { return _name; }

    // Requests a switch to the scene with the given name.
    void request_scene(std::string name) { _requested_scene = std::move(name); }

    [[nodiscard]] const std::string& pending_request() const noexcept {
        return _requested_scene;
    }

    // Takes ownership of the pending scene-name request, leaving none pending.
    [[nodiscard]] std::string take_request() noexcept {
        return std::exchange(_requested_scene, {});
    }

    void clear_request() noexcept { _requested_scene.clear(); }

    virtual void update() = 0;
    virtual void draw() = 0;

private:
    std::string _name;
    std::string _requested_scene;
};

// Concrete no-op scene used only as a lookup key when finding a registered
// scene by name.
class stripped_scene : public scene_base {
public:
    explicit stripped_scene(std::string name) : scene_base(std::move(name)) {}

    void update() override {}
    void draw() override {}
};

// Holds all game scenes and dispatches update/draw to the current one.
//
// Scenes are hashed and compared by name, so a lookup key only needs to
// carry the right name.
class scene_table {
public:
    struct scene_hash {
        [[nodiscard]] std::size_t operator()(const scene_base* scene) const noexcept {
            return std::hash<std::string>{}(scene->name());
        }
    };

    struct scene_eq {
        [[nodiscard]] bool operator()(const scene_base* lhs,
                                      const scene_base* rhs) const noexcept {
            return lhs->name() == rhs->name();
        }
    };

    // Constructs the table from the starting scene followed by any number of
    // additional scenes. The game always starts on `start`.
    template <typename... Scenes>
    explicit scene_table(scene_base& start, Scenes&... rest)
        : current_scene(&start) {
        (table.insert(&rest), ...);
        table.insert(&start);
    }

    scene_table(const scene_table&) = delete;
    scene_table& operator=(const scene_table&) = delete;

    void update();
    void draw();

private:
    std::unordered_set<scene_base*, scene_hash, scene_eq> table;
    scene_base* current_scene = nullptr;
};
