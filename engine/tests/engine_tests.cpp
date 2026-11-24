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
    assert(world.mission_state()==global_engine::MissionState::InProgress);
    assert(world.mission_stage()==global_engine::MissionStage::FindVehicle);
    assert(world.mission_distance()>96.0f&&world.mission_distance()<97.0f);
    const auto revision=world.static_geometry_revision();

    std::vector<global_engine::Box> dynamic;
    world.gather_dynamic_draw_boxes(dynamic);
    assert(dynamic.size()==51);
    for(int i=0;i<120;++i)world.update(1.0f/60.0f,{1,0});
    assert(world.player_position().x>5.0f);
    assert(!world.player_intersects_building());
    assert(world.static_geometry_revision()==revision);

    for(int i=0;i<900;++i)world.update(1.0f/60.0f,{1,1});
    assert(world.streaming_stats().active_chunks<=25);
    assert(world.player_position().x<100.0f&&world.player_position().z<100.0f);

    // Reach the blue vehicle, collect the parcel, then follow the road to the delivery point.
    world.reset();
    for(int i=0;i<900&&world.mission_stage()==global_engine::MissionStage::FindVehicle;++i)
        world.update(1.0f/60.0f,{1.0f,-0.0625f});
    assert(world.mission_stage()==global_engine::MissionStage::DeliverPackage);
    assert(!world.mission_completed());
    assert(world.mission_target().x==160.0f&&world.mission_target().z==-64.0f);
    std::vector<global_engine::Box> carrying;
    world.gather_dynamic_draw_boxes(carrying);
    assert(carrying.size()==52);
    for(int i=0;i<5000&&!world.mission_completed();++i){
        const auto&p=world.player_position();
        if(p.z>-64.0f+1.0f)world.update(1.0f/60.0f,{0.0f,-1.0f});
        else world.update(1.0f/60.0f,{1.0f,0.0f});
    }
    assert(world.mission_completed());
    assert(world.mission_stage()==global_engine::MissionStage::Completed);
    assert(world.mission_distance()<=4.25f);
    assert(!world.player_intersects_building());

    std::vector<global_engine::Box> all;
    world.gather_draw_boxes(all);
    assert(all.size()==world.static_box_count()+51);
    std::cout<<"GLOBAL ENGINE CORE: PASS chunks="<<world.streaming_stats().active_chunks
        <<" buildings="<<world.building_count()<<" resident_boxes="<<world.static_box_count()
        <<" mission=delivered dynamic_boxes="<<dynamic.size()<<'\n';
    return 0;
}
