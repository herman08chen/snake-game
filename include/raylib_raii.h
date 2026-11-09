#pragma once

#include "raylib.h"

// Move-only RAII wrappers for raylib resources.
//
// Raylib handles (Sound, Texture2D, Music, ...) are plain C handles that own
// an underlying resource. Copying a handle copies the resource identifier,
// which leads to double-free or resource leaks when both copies are
// unloaded. These wrappers enforce single ownership: they are movable but
// not copyable, and unload the resource exactly once in their destructor.
//
// Usage:
//
//     raii_sound sound(LoadSound("coin.wav"));
//     ...
//     PlaySound(sound.get());   // access the underlying raylib handle
//
// All wrappers are safely default-constructible (empty, not loaded) and can
// be assigned a new resource with reload(), which unloads the previous one.

template <typename T, void (*Unload)(T)>
struct raii_resource {
    T handle{};
    bool loaded = false;

    raii_resource() = default;

    explicit raii_resource(T handle) : handle(handle), loaded(true) {}

    raii_resource(const raii_resource&) = delete;
    raii_resource& operator=(const raii_resource&) = delete;

    raii_resource(raii_resource&& other) noexcept
        : handle(other.release())
        , loaded(true) {}

    raii_resource& operator=(raii_resource&& other) noexcept {
        if (this != &other) {
            reset();
            handle = other.release();
            loaded = true;
        }
        return *this;
    }

    ~raii_resource() { reset(); }

    [[nodiscard]] T get() const noexcept { return handle; }
    [[nodiscard]] explicit operator bool() const noexcept { return loaded; }

    // Unloads the currently held resource, if any.
    void reset() noexcept {
        if (loaded) {
            Unload(handle);
        }
        handle = {};
        loaded = false;
    }

    // Replaces the held resource, unloading the previous one first.
    void reload(T handle) {
        reset();
        this->handle = handle;
        loaded = true;
    }

private:
    [[nodiscard]] T release() noexcept {
        T old = handle;
        handle = {};
        loaded = false;
        return old;
    }
};

using raii_sound = raii_resource<Sound, UnloadSound>;
using raii_texture = raii_resource<Texture2D, UnloadTexture>;
using raii_music = raii_resource<Music, UnloadMusicStream>;
