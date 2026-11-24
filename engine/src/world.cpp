#include "global_engine/world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
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
float clamp_input(float v) noexcept { return std::max(-1.0f,std::min(1.0f,v)); }
bool overlap(float a0,float a1,float b0,float b1) noexcept { return a0<b1&&a1>b0; }
std::uint32_t hash_cell(int x,int z,int salt) noexcept {
    std::uint32_t h=static_cast<std::uint32_t>(x)*0x8da6b343U;
    h^=static_cast<std::uint32_t>(z)*0xd8163841U;
    h^=static_cast<std::uint32_t>(salt)*0xcb1ab31fU;
    h^=h>>13U;h*=0x85ebca6bU;h^=h>>16U;return h;
}
float unit_random(std::uint32_t h) noexcept { return static_cast<float>(h&0x00ffffffU)/16777215.0f; }
} // namespace
Vec3 Vec3::operator+(const Vec3&r)const noexcept{return{x+r.x,y+r.y,z+r.z};}
Vec3 Vec3::operator-(const Vec3&r)const noexcept{return{x-r.x,y-r.y,z-r.z};}
Vec3 Vec3::operator*(float s)const noexcept{return{x*s,y*s,z*s};}

CityWorld::CityWorld(){reset();}
WorldChunk CityWorld::generate_chunk(int cx,int cz)const{
    WorldChunk chunk;chunk.x=cx;chunk.z=cz;
    const float ox=static_cast<float>(cx)*kChunkSize,oz=static_cast<float>(cz)*kChunkSize;
    chunk.center_x=ox+kChunkSize*0.5f;chunk.center_z=oz+kChunkSize*0.5f;
    auto add=[&](const Box& b,bool low){
        chunk.high_detail_boxes.push_back(b);
        if(low)chunk.low_detail_boxes.push_back(b);
    };
    // Every chunk owns its terrain and road segments; adjacent edges line up exactly.
    add({{chunk.center_x,-0.18f,chunk.center_z},{32,0.18f,32},{0.34f,0.48f,0.30f,1},false},true);
    for(int lane=0;lane<2;++lane){
        const float x=ox+static_cast<float>(lane)*32.0f;
        const float z=oz+static_cast<float>(lane)*32.0f;
        add({{x,0.015f,chunk.center_z},{4.0f,0.035f,32},{0.19f,0.21f,0.22f,1},false},true);
        add({{chunk.center_x,0.015f,z},{32,0.035f,4.0f},{0.19f,0.21f,0.22f,1},false},true);
        for(float side:{-1.0f,1.0f}){
            add({{x+side*5.2f,0.055f,chunk.center_z},{1.0f,0.07f,32},{0.65f,0.65f,0.60f,1},false},true);
            add({{chunk.center_x,0.055f,z+side*5.2f},{32,0.07f,1.0f},{0.65f,0.65f,0.60f,1},false},true);
        }
    }
    const Color facades[]={{0.78f,0.68f,0.53f,1},{0.56f,0.68f,0.70f,1},
        {0.75f,0.49f,0.37f,1},{0.66f,0.65f,0.72f,1},{0.72f,0.72f,0.62f,1}};
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
        add({{x,h+0.16f,z},{w+0.2f,0.16f,d+0.2f},{0.36f,0.39f,0.39f,1},false},true);
        // Windows are high LOD only; distant buildings retain clean silhouettes.
        const Color glass{0.18f,0.39f,0.50f,1};
        const int floors=std::max(1,static_cast<int>(h/3.2f));
        for(int floor=0;floor<floors;++floor){
            const float y=2.1f+static_cast<float>(floor)*2.7f;
            if(y>h-0.8f)continue;
            for(int col=0;col<3;++col){
                const float wx=x+(static_cast<float>(col)-1.0f)*w*0.48f;
                add({{wx,y,z-d-0.045f},{0.62f,0.72f,0.05f},glass,false},false);
                add({{wx,y,z+d+0.045f},{0.62f,0.72f,0.05f},glass,false},false);
                add({{x-w-0.045f,y,z+(static_cast<float>(col)-1.0f)*d*0.48f},
                    {0.05f,0.72f,0.62f},glass,false},false);
                add({{x+w+0.045f,y,z+(static_cast<float>(col)-1.0f)*d*0.48f},
                    {0.05f,0.72f,0.62f},glass,false},false);
            }
        }
        add({{x,1.0f,z-d-0.07f},{0.7f,1.0f,0.07f},{0.22f,0.27f,0.28f,1},false},false);
        // A few low-cost street props per lot.
        for(int side=-1;side<=1;side+=2){
            const float tx=x+static_cast<float>(side)*12.0f;
            add({{tx,1.1f,z+11.0f},{0.22f,1.1f,0.22f},{0.32f,0.23f,0.15f,1},false},false);
            add({{tx,2.35f,z+11.0f},{1.0f,1.0f,1.0f},{0.20f,0.42f,0.20f,1},false},false);
        }
    }
    return chunk;
}
void CityWorld::reset(){
    player_position_={};player_velocity_={};npc_elapsed_=0.0f;
    active_chunks_.clear();active_building_count_=0;streaming_stats_={};
    streaming_stats_.world_chunks_per_axis=kChunksPerAxis;
    streaming_stats_.active_radius=kChunkRadius;
    sync_chunks();
}
void CityWorld::sync_chunks(){
    const int pcx=static_cast<int>(std::floor(player_position_.x/kChunkSize));
    const int pcz=static_cast<int>(std::floor(player_position_.z/kChunkSize));
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
}
void CityWorld::gather_dynamic_draw_boxes(std::vector<Box>&out)const{
    out.clear();out.reserve(12);
    out.push_back({{player_position_.x,1.02f,player_position_.z},{0.34f,0.56f,0.24f},{0.15f,0.42f,0.67f,1},false});
    out.push_back({{player_position_.x,1.78f,player_position_.z},{0.22f,0.22f,0.22f},{0.78f,0.57f,0.42f,1},false});
    for(int i=0;i<5;++i){
        const float phase=npc_elapsed_*0.18f+static_cast<float>(i)*1.25663706f;
        const float radius=23.0f+static_cast<float>(i%2)*3.0f;
        const float x=player_position_.x+std::cos(phase)*radius;
        const float z=player_position_.z+std::sin(phase)*radius;
        const Color shirt=(i%2==0)?Color{0.70f,0.35f,0.22f,1}:Color{0.15f,0.42f,0.67f,1};
        out.push_back({{x,0.92f,z},{0.24f,0.46f,0.24f},shirt,false});
        out.push_back({{x,1.55f,z},{0.18f,0.18f,0.18f},{0.78f,0.57f,0.42f,1},false});
    }
}
void CityWorld::gather_draw_boxes(std::vector<Box>&out)const{
    out.clear();out.reserve(streaming_stats_.resident_boxes+12);
    for(const WorldChunk&c:active_chunks_)out.insert(out.end(),c.high_detail_boxes.begin(),c.high_detail_boxes.end());
    std::vector<Box> dynamic;gather_dynamic_draw_boxes(dynamic);out.insert(out.end(),dynamic.begin(),dynamic.end());
}
bool CityWorld::player_intersects_building()const noexcept{return collides_with_building(player_position_);}
} // namespace global_engine
