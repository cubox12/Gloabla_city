#include "global_engine/world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <array>
#include <utility>

namespace global_engine {
namespace {
constexpr float kChunkSize=64.0f;
constexpr float kWorldHalfSize=2048.0f;
constexpr float kPlayerRadius=0.36f;
constexpr float kPlayerHeight=1.8f;
constexpr float kMoveSpeed=6.5f;
constexpr float kAcceleration=22.0f;
constexpr int kChunkRadius=2;
constexpr int kChunksPerAxis=64;
constexpr float kMissionCompleteRadius=4.25f;
constexpr std::size_t kDynamicBoxCount=52;
float clamp_input(float v) noexcept { return std::max(-1.0f,std::min(1.0f,v)); }
bool overlap(float a0,float a1,float b0,float b1) noexcept { return a0<b1&&a1>b0; }
std::uint32_t hash_cell(int x,int z,int salt) noexcept {
    std::uint32_t h=static_cast<std::uint32_t>(x)*0x8da6b343U;
    h^=static_cast<std::uint32_t>(z)*0xd8163841U;
    h^=static_cast<std::uint32_t>(salt)*0xcb1ab31fU;
    h^=h>>13U;h*=0x85ebca6bU;h^=h>>16U;return h;
}
float unit_random(std::uint32_t h) noexcept { return static_cast<float>(h&0x00ffffffU)/16777215.0f; }

void append_humanoid(std::vector<Box>& out,const Vec3& p,Color shirt,Color pants,
                      const Vec3& forward,float phase,float intensity) {
    const Color skin{0.78f,0.57f,0.43f,1.0f};
    const float amount=std::max(0.0f,std::min(1.0f,intensity));
    const float forward_length=std::sqrt(forward.x*forward.x+forward.z*forward.z);
    const Vec3 f=forward_length>1.0e-4f?
        Vec3{forward.x/forward_length,0.0f,forward.z/forward_length}:Vec3{0.0f,0.0f,-1.0f};
    const Vec3 right{f.z,0.0f,-f.x};
    const float swing=std::sin(phase)*0.22f*amount;
    const float opposite=-swing;
    const float left_lift=std::max(0.0f,std::sin(phase))*0.11f*amount;
    const float right_lift=std::max(0.0f,-std::sin(phase))*0.11f*amount;
    const float bob=0.045f*amount*(0.5f+0.5f*std::sin(phase*2.0f));
    const Vec3 left_leg=p+right*(-0.13f)+f*swing;
    const Vec3 right_leg=p+right*0.13f+f*opposite;
    // The feet lift and shorten slightly, while the pelvis remains stable.
    out.push_back({{left_leg.x,0.39f+left_lift*0.5f,left_leg.z},
        {0.13f,0.39f-left_lift*0.5f,0.14f},pants,false});
    out.push_back({{right_leg.x,0.39f+right_lift*0.5f,right_leg.z},
        {0.13f,0.39f-right_lift*0.5f,0.14f},pants,false});
    out.push_back({{p.x,1.08f+bob,p.z},{0.27f,0.46f,0.18f},shirt,false});
    out.push_back({{p.x,1.75f+bob,p.z},{0.19f,0.19f,0.19f},skin,false});
    const Vec3 left_arm=p+right*(-0.34f)+f*(-swing*0.82f);
    const Vec3 right_arm=p+right*0.34f+f*(swing*0.82f);
    out.push_back({{left_arm.x,1.08f+bob+std::sin(phase+1.5707963f)*0.055f*amount,left_arm.z},
        {0.11f,0.37f,0.12f},shirt,false});
    out.push_back({{right_arm.x,1.08f+bob-std::sin(phase+1.5707963f)*0.055f*amount,right_arm.z},
        {0.11f,0.37f,0.12f},shirt,false});
}
} // namespace

Vec3 Vec3::operator+(const Vec3&r)const noexcept{return{x+r.x,y+r.y,z+r.z};}
Vec3 Vec3::operator-(const Vec3&r)const noexcept{return{x-r.x,y-r.y,z-r.z};}
Vec3 Vec3::operator*(float s)const noexcept{return{x*s,y*s,z*s};}

CityWorld::CityWorld(){reset();}

WorldChunk CityWorld::generate_chunk(int cx,int cz)const{
    WorldChunk chunk;chunk.x=cx;chunk.z=cz;
    // Reserve for the worst facade/window pattern plus road markings and street furniture.
    chunk.high_detail_boxes.reserve(768);
    chunk.low_detail_boxes.reserve(64);
    chunk.collision_boxes.reserve(4);
    const float ox=static_cast<float>(cx)*kChunkSize,oz=static_cast<float>(cz)*kChunkSize;
    chunk.center_x=ox+kChunkSize*0.5f;chunk.center_z=oz+kChunkSize*0.5f;
    auto add=[&](const Box& b,bool low){
        chunk.high_detail_boxes.push_back(b);
        if(low)chunk.low_detail_boxes.push_back(b);
    };

    // Grass lots, asphalt, raised sidewalks and dashed center markings.
    add({{chunk.center_x,-0.18f,chunk.center_z},{32,0.18f,32},{0.30f,0.42f,0.29f,1},false},true);
    for(int lane=0;lane<2;++lane){
        const float x=ox+static_cast<float>(lane)*32.0f;
        const float z=oz+static_cast<float>(lane)*32.0f;
        add({{x,0.015f,chunk.center_z},{4.0f,0.035f,32},{0.16f,0.18f,0.19f,1},false},true);
        add({{chunk.center_x,0.015f,z},{32,0.035f,4.0f},{0.16f,0.18f,0.19f,1},false},true);
        for(float side:{-1.0f,1.0f}){
            add({{x+side*5.2f,0.055f,chunk.center_z},{1.0f,0.07f,32},{0.62f,0.62f,0.57f,1},false},true);
            add({{chunk.center_x,0.055f,z+side*5.2f},{32,0.07f,1.0f},{0.62f,0.62f,0.57f,1},false},true);
        }
        const Color lane_paint{0.84f,0.76f,0.47f,1};
        for(int mark=0;mark<8;++mark){
            const float offset=4.0f+static_cast<float>(mark)*8.0f;
            add({{x,0.075f,oz+offset+1.7f},{0.12f,0.035f,1.7f},lane_paint,false},true);
            add({{ox+offset+1.7f,0.075f,z},{1.7f,0.035f,0.12f},lane_paint,false},true);
        }
    }

    const Color facades[]={{0.73f,0.65f,0.54f,1},{0.48f,0.62f,0.65f,1},
        {0.70f,0.45f,0.35f,1},{0.62f,0.62f,0.68f,1},{0.69f,0.68f,0.59f,1}};
    // Small low-poly street trees make the avenues feel inhabited at a low GPU cost.
    const Color bark{0.31f,0.20f,0.13f,1};
    const Color leaf_a{0.16f,0.36f,0.22f,1};
    const Color leaf_b{0.22f,0.45f,0.27f,1};
    for(const auto&tree:std::array<Vec3,4>{{
        {ox+7.4f,0.0f,oz+24.0f},{ox+24.0f,0.0f,oz+7.4f},
        {ox+56.6f,0.0f,oz+40.0f},{ox+40.0f,0.0f,oz+56.6f}}}){
        add({{tree.x,1.05f,tree.z},{0.18f,1.05f,0.18f},bark,false},true);
        add({{tree.x,2.65f,tree.z},{0.95f,0.92f,0.95f},leaf_a,false},true);
        add({{tree.x+0.34f,2.45f,tree.z-0.22f},{0.58f,0.62f,0.62f},leaf_b,false},true);
    }
    for(int bx=0;bx<2;++bx)for(int bz=0;bz<2;++bz){
        const int gx=cx*2+bx,gz=cz*2+bz;
        const std::uint32_t seed=hash_cell(gx,gz,17);
        const float x=ox+16.0f+static_cast<float>(bx)*32.0f;
        const float z=oz+16.0f+static_cast<float>(bz)*32.0f;
        const float w=7.0f+unit_random(seed)*2.0f;
        const float d=7.0f+unit_random(seed^0x1234U)*2.0f;
        const float h=8.0f+unit_random(seed^0x9abcU)*30.0f;
        const Color facade=facades[seed%5U];
        const Box building{{x,h*0.5f,z},{w,h*0.5f,d},facade,true};
        chunk.collision_boxes.push_back(building);
        add(building,true);
        add({{x,h+0.16f,z},{w+0.2f,0.16f,d+0.2f},{0.34f,0.37f,0.37f,1},false},true);

        // Corner pilasters, roof parapet and subtle floor bands add depth without textures.
        const Color trim{0.82f,0.79f,0.70f,1};
        for(float sx:{-1.0f,1.0f})for(float sz:{-1.0f,1.0f})
            add({{x+sx*(w-0.18f),h*0.5f,z+sz*(d-0.18f)},{0.12f,h*0.5f,0.12f},trim,false},false);
        add({{x,h+0.42f,z-d+0.10f},{w,0.10f,0.12f},trim,false},false);
        add({{x,h+0.42f,z+d-0.10f},{w,0.10f,0.12f},trim,false},false);
        add({{x-w+0.10f,h+0.42f,z},{0.12f,0.10f,d},trim,false},false);
        add({{x+w-0.10f,h+0.42f,z},{0.12f,0.10f,d},trim,false},false);
        for(float band=5.4f;band<h-1.0f;band+=8.1f){
            const Color band_color{facade.r*0.88f,facade.g*0.88f,facade.b*0.88f,1};
            add({{x,band,z-d-0.035f},{w,0.08f,0.06f},band_color,false},false);
            add({{x,band,z+d+0.035f},{w,0.08f,0.06f},band_color,false},false);
            add({{x-w-0.035f,band,z},{0.06f,0.08f,d},band_color,false},false);
            add({{x+w+0.035f,band,z},{0.06f,0.08f,d},band_color,false},false);
        }
        // Two compact rooftop ventilation units.
        const Color rooftop{0.38f,0.42f,0.43f,1};
        add({{x-w*0.48f,h+0.65f,z},{0.72f,0.23f,0.62f},rooftop,false},false);
        add({{x+w*0.43f,h+0.59f,z+d*0.25f},{0.48f,0.17f,0.48f},rooftop,false},false);

        // Recessed-looking glass panes. A few warm panes break up the uniform facade.
        const Color glass{0.12f,0.30f,0.39f,1};
        const Color warm_glass{0.60f,0.48f,0.28f,1};
        const int floors=std::max(1,static_cast<int>(h/3.2f));
        for(int floor=0;floor<floors;++floor){
            const float y=2.1f+static_cast<float>(floor)*2.7f;
            if(y>h-0.8f)continue;
            for(int col=0;col<3;++col){
                const float wx=x+(static_cast<float>(col)-1.0f)*w*0.48f;
                const bool warm=(hash_cell(gx+col,gz+floor,91)&7U)==0U;
                const Color pane=warm?warm_glass:glass;
                add({{wx,y,z-d-0.045f},{0.62f,0.72f,0.05f},pane,false},false);
                add({{wx,y,z+d+0.045f},{0.62f,0.72f,0.05f},pane,false},false);
                add({{x-w-0.045f,y,z+(static_cast<float>(col)-1.0f)*d*0.48f},
                    {0.05f,0.72f,0.62f},pane,false},false);
                add({{x+w+0.045f,y,z+(static_cast<float>(col)-1.0f)*d*0.48f},
                    {0.05f,0.72f,0.62f},pane,false},false);
            }
        }
        add({{x,1.0f,z-d-0.07f},{0.7f,1.0f,0.07f},{0.20f,0.23f,0.24f,1},false},false);

        // Street lamps use three simple meshes each; the warm head is easy to spot at night-like distances.
        for(int side=-1;side<=1;side+=2){
            const float tx=x+static_cast<float>(side)*12.0f;
            const float tz=z+11.0f;
            add({{tx,2.05f,tz},{0.075f,2.05f,0.075f},{0.26f,0.28f,0.29f,1},false},false);
            add({{tx,4.02f,tz-0.66f},{0.075f,0.075f,0.68f},{0.26f,0.28f,0.29f,1},false},false);
            add({{tx,3.92f,tz-1.25f},{0.27f,0.13f,0.31f},{0.91f,0.79f,0.50f,1},false},false);
        }
    }
    return chunk;
}

void CityWorld::reset(){
    player_position_={};player_velocity_={};player_facing_={0.0f,0.0f,-1.0f};
    player_animation_phase_=0.0f;npc_elapsed_=0.0f;
    vehicle_position_={96.0f,0.0f,-6.0f};
    mission_target_=vehicle_position_;
    mission_distance_=std::sqrt(mission_target_.x*mission_target_.x+
        mission_target_.z*mission_target_.z);
    mission_state_=MissionState::InProgress;
    mission_stage_=MissionStage::FindVehicle;
    active_chunks_.clear();active_chunks_.reserve(25);
    stream_center_initialized_=false;stream_center_x_=0;stream_center_z_=0;
    active_building_count_=0;streaming_stats_={};
    streaming_stats_.world_chunks_per_axis=kChunksPerAxis;
    streaming_stats_.active_radius=kChunkRadius;
    sync_chunks();
}
void CityWorld::sync_chunks(){
    const int pcx=static_cast<int>(std::floor(player_position_.x/kChunkSize));
    const int pcz=static_cast<int>(std::floor(player_position_.z/kChunkSize));
    if(stream_center_initialized_&&pcx==stream_center_x_&&pcz==stream_center_z_)return;
    stream_center_x_=pcx;stream_center_z_=pcz;stream_center_initialized_=true;
    bool changed=false;
    auto keep=[&](const WorldChunk& c){
        return std::abs(c.x-pcx)<=kChunkRadius&&std::abs(c.z-pcz)<=kChunkRadius;
    };
    for(auto it=active_chunks_.begin();it!=active_chunks_.end();){
        if(!keep(*it)){active_building_count_-=it->collision_boxes.size();
            streaming_stats_.resident_boxes-=it->high_detail_boxes.size();
            it=active_chunks_.erase(it);++streaming_stats_.evicted_chunks;changed=true;
        }else ++it;
    }
    for(int x=pcx-kChunkRadius;x<=pcx+kChunkRadius;++x){
        for(int z=pcz-kChunkRadius;z<=pcz+kChunkRadius;++z){
            if(x<-(kChunksPerAxis/2)||x>=(kChunksPerAxis/2)||
               z<-(kChunksPerAxis/2)||z>=(kChunksPerAxis/2))continue;
            const bool found=std::any_of(active_chunks_.begin(),active_chunks_.end(),
                [x,z](const WorldChunk& c){return c.x==x&&c.z==z;});
            if(!found){
                WorldChunk chunk=generate_chunk(x,z);
                active_building_count_+=chunk.collision_boxes.size();
                streaming_stats_.resident_boxes+=chunk.high_detail_boxes.size();
                active_chunks_.push_back(std::move(chunk));
                ++streaming_stats_.generated_chunks;changed=true;
            }
        }
    }
    streaming_stats_.active_chunks=active_chunks_.size();
    if(changed){++static_geometry_revision_;if(static_geometry_revision_==0)++static_geometry_revision_;}
}
bool CityWorld::collides_with_building(const Vec3&p)const noexcept{
    for(const WorldChunk& c:active_chunks_)for(const Box& b:c.collision_boxes){
        if(!overlap(p.x-kPlayerRadius,p.x+kPlayerRadius,b.center.x-b.half_extent.x,b.center.x+b.half_extent.x))continue;
        if(!overlap(p.z-kPlayerRadius,p.z+kPlayerRadius,b.center.z-b.half_extent.z,b.center.z+b.half_extent.z))continue;
        if(overlap(p.y,p.y+kPlayerHeight,b.center.y-b.half_extent.y,b.center.y+b.half_extent.y))return true;
    }
    return false;
}
void CityWorld::update(float delta,const InputState&input){
    const float dt=std::max(0.0f,std::min(delta,0.05f));npc_elapsed_+=dt;
    float ix=clamp_input(input.move_x),iz=clamp_input(input.move_z);
    const float len=std::sqrt(ix*ix+iz*iz);if(len>1.0f){ix/=len;iz/=len;}
    const float md=kAcceleration*dt;
    const auto approach=[md](float v,float target){return v+std::max(-md,std::min(md,target-v));};
    player_velocity_.x=approach(player_velocity_.x,ix*kMoveSpeed);
    player_velocity_.z=approach(player_velocity_.z,iz*kMoveSpeed);
    Vec3 next=player_position_;
    next.x=std::max(-kWorldHalfSize+kPlayerRadius,std::min(kWorldHalfSize-kPlayerRadius,next.x+player_velocity_.x*dt));
    if(!collides_with_building(next))player_position_.x=next.x;else player_velocity_.x=0;
    next=player_position_;
    next.z=std::max(-kWorldHalfSize+kPlayerRadius,std::min(kWorldHalfSize-kPlayerRadius,next.z+player_velocity_.z*dt));
    if(!collides_with_building(next))player_position_.z=next.z;else player_velocity_.z=0;
    sync_chunks();
    const float player_speed=std::sqrt(player_velocity_.x*player_velocity_.x+
        player_velocity_.z*player_velocity_.z);
    if(player_speed>0.10f){
        player_facing_={player_velocity_.x/player_speed,0.0f,player_velocity_.z/player_speed};
        player_animation_phase_+=dt*(7.0f+player_speed*0.35f);
    }else{
        player_animation_phase_+=dt*1.25f;
    }
    float mdx=player_position_.x-mission_target_.x;
    float mdz=player_position_.z-mission_target_.z;
    mission_distance_=std::sqrt(mdx*mdx+mdz*mdz);
    if(mission_stage_==MissionStage::FindVehicle&&mission_distance_<=kMissionCompleteRadius){
        mission_stage_=MissionStage::DeliverPackage;
        mission_target_={160.0f,0.0f,-64.0f};
        mdx=player_position_.x-mission_target_.x;
        mdz=player_position_.z-mission_target_.z;
        mission_distance_=std::sqrt(mdx*mdx+mdz*mdz);
    }
    if(mission_stage_==MissionStage::DeliverPackage&&mission_distance_<=kMissionCompleteRadius){
        mission_stage_=MissionStage::Completed;
        mission_state_=MissionState::Completed;
    }
}
void CityWorld::append_dynamic_draw_boxes(std::vector<Box>&out)const{
    const float player_speed=std::sqrt(player_velocity_.x*player_velocity_.x+
        player_velocity_.z*player_velocity_.z);
    append_humanoid(out,player_position_,{0.12f,0.35f,0.62f,1},{0.12f,0.15f,0.19f,1},
        player_facing_,player_animation_phase_,
        std::min(1.0f,player_speed/kMoveSpeed));
    if(mission_stage_==MissionStage::DeliverPackage)
        out.push_back({{player_position_.x+0.30f,1.18f,player_position_.z-0.18f},
            {0.28f,0.25f,0.25f},{0.68f,0.43f,0.20f,1},false});
    for(int i=0;i<5;++i){
        const float phase=npc_elapsed_*0.18f+static_cast<float>(i)*1.25663706f;
        const float radius=23.0f+static_cast<float>(i%2)*3.0f;
        const float x=player_position_.x+std::cos(phase)*radius;
        const float z=player_position_.z+std::sin(phase)*radius;
        const Color shirt=(i%2==0)?Color{0.68f,0.30f,0.18f,1}:Color{0.12f,0.35f,0.55f,1};
        const Color pants=(i%2==0)?Color{0.18f,0.20f,0.22f,1}:Color{0.30f,0.28f,0.23f,1};
        const Vec3 walking_forward{-std::sin(phase),0.0f,std::cos(phase)};
        append_humanoid(out,{x,0,z},shirt,pants,walking_forward,npc_elapsed_*6.2f+i*1.4f,0.82f);
    }

    // Parked blue compact delivery car: body, cabin, front/rear glass and four tyres.
    const float cx=vehicle_position_.x,cz=vehicle_position_.z;
    const Color car_blue{0.06f,0.25f,0.54f,1};
    const Color glass{0.15f,0.42f,0.54f,1};
    const Color rubber{0.045f,0.052f,0.058f,1};
    out.push_back({{cx,0.72f,cz},{2.25f,0.52f,1.02f},car_blue,false});
    out.push_back({{cx-0.10f,1.34f,cz},{1.22f,0.34f,0.88f},car_blue,false});
    out.push_back({{cx+1.13f,1.36f,cz},{0.07f,0.28f,0.72f},glass,false});
    out.push_back({{cx-1.30f,1.36f,cz},{0.07f,0.28f,0.72f},glass,false});
    for(float sx:{-1.35f,1.35f})for(float sz:{-0.98f,0.98f})
        out.push_back({{cx+sx,0.38f,cz+sz},{0.40f,0.34f,0.16f},rubber,false});
    const Color headlights{0.95f,0.84f,0.58f,1};
    out.push_back({{cx+2.23f,0.80f,cz-0.66f},{0.07f,0.14f,0.20f},headlights,false});
    out.push_back({{cx+2.23f,0.80f,cz+0.66f},{0.07f,0.14f,0.20f},headlights,false});

    // The cyan beacon marks the first mission pickup; it turns green when reached.
    const Color beacon=mission_state_==MissionState::Completed?
        Color{0.18f,0.78f,0.34f,1}:Color{0.05f,0.82f,0.96f,1};
    const float marker_x=mission_target_.x,marker_z=mission_target_.z+4.0f;
    const float pulse=0.5f+0.5f*std::sin(npc_elapsed_*3.2f);
    out.push_back({{marker_x,0.18f,marker_z},{2.8f+0.22f*pulse,0.12f,2.8f+0.22f*pulse},beacon,false});
    out.push_back({{marker_x,3.05f+0.28f*pulse,marker_z},{0.16f,2.75f+0.30f*pulse,0.16f},beacon,false});
    out.push_back({{marker_x,6.35f+0.35f*pulse,marker_z},{1.25f+0.10f*pulse,0.13f,0.13f},beacon,false});
    out.push_back({{marker_x,6.35f+0.35f*pulse,marker_z},{0.13f,0.13f,1.25f+0.10f*pulse},beacon,false});
    out.push_back({{marker_x,7.05f+0.55f*pulse,marker_z},{0.42f+0.10f*pulse,0.42f+0.10f*pulse,0.42f+0.10f*pulse},beacon,false});
}
void CityWorld::gather_dynamic_draw_boxes(std::vector<Box>&out)const{
    out.clear();
    if(out.capacity()<kDynamicBoxCount)out.reserve(kDynamicBoxCount);
    append_dynamic_draw_boxes(out);
}
void CityWorld::gather_draw_boxes(std::vector<Box>&out)const{
    out.clear();
    const std::size_t required=streaming_stats_.resident_boxes+kDynamicBoxCount;
    if(out.capacity()<required)out.reserve(required);
    for(const WorldChunk&c:active_chunks_)
        out.insert(out.end(),c.high_detail_boxes.begin(),c.high_detail_boxes.end());
    append_dynamic_draw_boxes(out);
}
bool CityWorld::player_intersects_building()const noexcept{return collides_with_building(player_position_);}
} // namespace global_engine
