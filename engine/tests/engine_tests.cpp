#include "global_engine/world.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    global_engine::CityWorld world;
    assert(world.building_count()==16);
    assert(world.static_box_count()>100);
    assert(!world.player_intersects_building());

    const auto initial_revision = world.static_geometry_revision();
    assert(world.static_draw_boxes().size() == world.static_box_count());

    std::vector<global_engine::Box> dynamic_boxes;
    world.gather_dynamic_draw_boxes(dynamic_boxes);
    assert(dynamic_boxes.size() == 12);

    const auto start=world.player_position();
    for (int i=0;i<120;++i) world.update(1.0f/60.0f,{1,0});
    assert(world.player_position().x>start.x+5.0f);
    assert(!world.player_intersects_building());

    world.gather_dynamic_draw_boxes(dynamic_boxes);
    assert(dynamic_boxes.size() == 12);
    assert(world.static_geometry_revision() == initial_revision);

    world.reset();
    assert(world.static_geometry_revision() != initial_revision);
    for (int i=0;i<900;++i) world.update(1.0f/60.0f,{1,1});
    assert(!world.player_intersects_building());
    assert(world.player_position().x<17.0f);
    assert(world.player_position().z<17.0f);

    world.reset();
    world.update(5.0f,{100,100});
    assert(std::isfinite(world.player_position().x));
    assert(std::isfinite(world.player_position().z));

    std::vector<global_engine::Box> boxes;
    world.gather_draw_boxes(boxes);
    assert(boxes.size()==world.static_box_count()+12);
    std::cout << "GLOBAL ENGINE CORE TESTS: PASS; buildings="
              << world.building_count() << "; static_boxes=" << world.static_box_count()
              << "; dynamic_boxes=" << dynamic_boxes.size() << '\n';
    return 0;
}
