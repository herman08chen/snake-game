#include <string>
#include <unordered_set>

struct scene_base {
	std::string _requested_scene{};
	void request_scene(std::string _name) {
		_requested_scene = std::move(_name);
	}

	virtual const std::string& name() const = 0;
	virtual void update() = 0;
	virtual void draw() = 0;

	virtual std::size_t hash() const {
		return std::hash<std::string>()(name());
	}
};
struct stripped_scene : scene_base {
	std::string _name;
	stripped_scene(std::string&& _name) : _name(std::move(_name)) {}
	const std::string& name() const override {
		return _name;
	}
	void update() override {}
	void draw() override {}
};

struct scene_table {
	std::unordered_set < scene_base*,
		decltype([](const scene_base* i) { return i->hash(); }),
		decltype([](const scene_base* lhs, const scene_base* rhs) {
		return lhs->name() == rhs->name();
		})
	> table;
	scene_base* current_scene = *(table.begin());

	scene_table(auto&& arg1, auto&&... args) : table{ static_cast<scene_base*>(&arg1), static_cast<scene_base*>(&args)...}, current_scene(static_cast<scene_base*>(&arg1)){}

	void update() {
		if (current_scene->_requested_scene.size()) [[unlikely]] {
			stripped_scene key = std::move(current_scene->_requested_scene);
			current_scene->_requested_scene.clear();
			current_scene = *table.find(static_cast<scene_base*>(&key));
		}
		current_scene->update();
	}
	void draw() {
		current_scene->draw();
	}
};