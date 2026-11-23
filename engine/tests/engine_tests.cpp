#include "global_engine/world.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
int main(){
    global_engine::CityWorld world;
    assert(world.streaming_stats().active_chunks==25);
    assert(world.building_count()==100);
    assert(world.static_box_count()>1000);
    assert(!world.player_intersects_building());
    const auto revision=world.static_geometry_revision();
    std::vector<global_engine::Box> dynamic;world.gather_dynamic_draw_boxes(dynamic);
    assert(dynamic.size()==12);
    for(int i=0;i<120;++i)world.update(1.0f/60.0f,{1,0});
    assert(world.player_position().x>5.0f);
    assert(!world.player_intersects_building());
    assert(world.static_geometry_revision()==revision);
    for(int i=0;i<900;++i)world.update(1.0f/60.0f,{1,1});
    assert(world.streaming_stats().active_chunks<=25);
    assert(world.player_position().x<100.0f&&world.player_position().z<100.0f);
    world.reset();world.update(5.0f,{100,100});
    assert(std::isfinite(world.player_position().x));
    assert(std::isfinite(world.player_position().z));
    std::vector<global_engine::Box>all;world.gather_draw_boxes(all);
    assert(all.size()==world.static_box_count()+12);
    std::cout<<"GLOBAL ENGINE CORE: PASS chunks="<<world.streaming_stats().active_chunks
        <<" buildings="<<world.building_count()<<" resident_boxes="<<world.static_box_count()<<'\n';
    return 0;
}
