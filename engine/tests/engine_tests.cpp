#include "global_engine/world.hpp"
#include "global_engine/touch_controls.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main(){
    using namespace global_engine;
    const VirtualStickLayout stick=virtual_stick_layout(2400,1080);
    assert(stick.radius>58.0f);
    assert(is_virtual_stick_touch(stick.center_x,stick.center_y,2400,1080));
    assert(!is_virtual_stick_touch(stick.center_x+stick.radius*2.0f,stick.center_y,2400,1080));
    const VirtualStickOutput idle=map_virtual_stick(stick.center_x,stick.center_y,stick);
    assert(std::abs(idle.move_x)<1.0e-6f&&std::abs(idle.move_z)<1.0e-6f);
    const VirtualStickOutput right=map_virtual_stick(stick.center_x+stick.radius,stick.center_y,stick);
    assert(right.move_x>0.99f&&std::abs(right.move_z)<1.0e-5f);
    const VirtualStickOutput up=map_virtual_stick(stick.center_x,stick.center_y-stick.radius,stick);
    assert(up.move_z< -0.99f&&std::abs(up.move_x)<1.0e-5f);
    const VirtualStickOutput down=map_virtual_stick(stick.center_x,stick.center_y+stick.radius,stick);
    assert(down.move_z>0.99f&&std::abs(down.move_x)<1.0e-5f);
    constexpr float camera_yaw=-1.45f;
    const WorldMovement forward=camera_relative_movement(0.0f,-1.0f,camera_yaw);
    const WorldMovement backward=camera_relative_movement(0.0f,1.0f,camera_yaw);
    const WorldMovement rightward=camera_relative_movement(1.0f,0.0f,camera_yaw);
    const WorldMovement leftward=camera_relative_movement(-1.0f,0.0f,camera_yaw);
    assert(std::abs(forward.move_x+std::sin(camera_yaw))<1.0e-5f);
    assert(std::abs(forward.move_z+std::cos(camera_yaw))<1.0e-5f);
    assert(std::abs(backward.move_x+forward.move_x)<1.0e-5f);
    assert(std::abs(backward.move_z+forward.move_z)<1.0e-5f);
    assert(std::abs(rightward.move_x-std::cos(camera_yaw))<1.0e-5f);
    assert(std::abs(rightward.move_z+std::sin(camera_yaw))<1.0e-5f);
    assert(std::abs(leftward.move_x+rightward.move_x)<1.0e-5f);
    assert(std::abs(leftward.move_z+rightward.move_z)<1.0e-5f);
    const WorldMovement default_forward=camera_relative_movement(0.0f,-1.0f,0.0f);
    const WorldMovement default_right=camera_relative_movement(1.0f,0.0f,0.0f);
    assert(std::abs(default_forward.move_x)<1.0e-6f&&default_forward.move_z< -0.99f);
    assert(default_right.move_x>0.99f&&std::abs(default_right.move_z)<1.0e-6f);
    const VirtualStickOutput diagonal=map_virtual_stick(stick.center_x+stick.radius*2.0f,
        stick.center_y-stick.radius*2.0f,stick);
    assert(diagonal.magnitude<=1.0f&&std::abs(diagonal.move_x)<=1.0f&&std::abs(diagonal.move_z)<=1.0f);
    const VirtualStickOutput deadzone=map_virtual_stick(stick.center_x+stick.radius*0.05f,stick.center_y,stick);
    assert(std::abs(deadzone.move_x)<1.0e-6f);

    CityWorld world;
    assert(world.streaming_stats().active_chunks==25);
    assert(world.building_count()==100);
    assert(world.static_box_count()>1000);
    std::size_t crosswalk_count=0;
    for(const WorldChunk&chunk:world.active_chunks()){
        for(const Box&box:chunk.high_detail_boxes){
            if(std::abs(box.color.r-0.88f)<1.0e-5f&&
               std::abs(box.color.g-0.90f)<1.0e-5f&&
               std::abs(box.color.b-0.88f)<1.0e-5f)++crosswalk_count;
        }
    }
    assert(crosswalk_count==world.active_chunks().size()*80U);
    std::size_t streetlamp_poles=0;
    for(const WorldChunk&chunk:world.active_chunks()){
        for(const Box&box:chunk.high_detail_boxes){
            const bool lamp_pole=std::abs(box.center.y-2.05f)<1.0e-5f&&
                std::abs(box.half_extent.x-0.075f)<1.0e-5f&&
                std::abs(box.half_extent.z-0.075f)<1.0e-5f&&
                std::abs(box.color.r-0.26f)<1.0e-5f&&
                std::abs(box.color.g-0.28f)<1.0e-5f;
            if(!lamp_pole)continue;
            ++streetlamp_poles;
            const float road_x=std::round(box.center.x/32.0f)*32.0f;
            const float road_z=std::round(box.center.z/32.0f)*32.0f;
            assert(std::abs(box.center.x-road_x)>4.0f);
            assert(std::abs(box.center.z-road_z)>4.0f);
        }
    }
    assert(streetlamp_poles==world.active_chunks().size()*8U);
    std::size_t tree_trunks=0,tree_crowns=0;
    for(const WorldChunk&chunk:world.active_chunks()){
        for(const Box&box:chunk.high_detail_boxes){
            const bool tree_trunk=box.shape==GeometryShape::Cylinder&&
                std::abs(box.center.y-1.0f)<1.0e-5f&&
                std::abs(box.color.r-0.28f)<1.0e-5f&&
                std::abs(box.color.g-0.18f)<1.0e-5f;
            const bool leaf_a=box.shape==GeometryShape::Ellipsoid&&
                std::abs(box.color.r-0.13f)<1.0e-5f&&
                std::abs(box.color.g-0.31f)<1.0e-5f;
            const bool leaf_b=box.shape==GeometryShape::Ellipsoid&&
                std::abs(box.color.r-0.19f)<1.0e-5f&&
                std::abs(box.color.g-0.39f)<1.0e-5f;
            if(tree_trunk)++tree_trunks;
            if(leaf_a)++tree_crowns;
            if(!tree_trunk&&!leaf_a&&!leaf_b)continue;
            for(const Box&building:chunk.collision_boxes){
                const bool overlaps_x=std::abs(box.center.x-building.center.x)<
                    box.half_extent.x+building.half_extent.x;
                const bool overlaps_z=std::abs(box.center.z-building.center.z)<
                    box.half_extent.z+building.half_extent.z;
                const bool overlaps_y=box.center.y+box.half_extent.y>
                    building.center.y-building.half_extent.y;
                assert(!(overlaps_x&&overlaps_z&&overlaps_y));
            }
            const float road_x=std::round(box.center.x/32.0f)*32.0f;
            const float road_z=std::round(box.center.z/32.0f)*32.0f;
            const bool clear_vertical_road=std::abs(box.center.x-road_x)-box.half_extent.x>4.0f;
            const bool clear_horizontal_road=std::abs(box.center.z-road_z)-box.half_extent.z>4.0f;
            assert(clear_vertical_road&&clear_horizontal_road);
        }
    }
    assert(tree_trunks==world.active_chunks().size()*4U);
    assert(tree_crowns==world.active_chunks().size()*4U);
    assert(!world.player_intersects_building());
    assert(world.mission_state()==global_engine::MissionState::InProgress);
    assert(world.mission_stage()==global_engine::MissionStage::FindVehicle);
    assert(world.mission_distance()>96.0f&&world.mission_distance()<97.0f);
    const auto revision=world.static_geometry_revision();

    std::vector<global_engine::Box> dynamic;
    world.gather_dynamic_draw_boxes(dynamic);
    assert(dynamic.size()==130);
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
    assert(carrying.size()==131);
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
    assert(all.size()==world.static_box_count()+130);
    std::cout<<"GLOBAL ENGINE CORE: PASS chunks="<<world.streaming_stats().active_chunks
        <<" buildings="<<world.building_count()<<" resident_boxes="<<world.static_box_count()
        <<" mission=delivered dynamic_boxes="<<dynamic.size()<<'\n';
    return 0;
}
