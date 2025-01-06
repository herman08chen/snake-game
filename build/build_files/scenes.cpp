#include <string>
#include <unordered_set>

struct scene_base {
	virtual const std::string& name() const = 0;
	virtual void update() = 0;

	friend bool operator==(const scene_base lhs, const scene_base rhs) {
		return lhs.name() == rhs.name();
	}
	std::size_t hash() const {
		return std::hash<std::string>()(name());
	}
};

class scene_table {
	std::unordered_set < scene_base*, 
		decltype([](const scene_base* i) { return i->hash(); }), 
		decltype([](const scene_base* lhs, const scene_base* rhs) {
			return lhs->name() == rhs->name();
		})
	> table;
	
};