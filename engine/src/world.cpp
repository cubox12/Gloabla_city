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
constexpr std::size_t kDynamicBoxCount=256;
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
                      const Vec3& forward,float phase,float intensity,bool detailed_face=false,
                      Color skin={0.72f,0.52f,0.39f,1.0f},
                      Color hair={0.075f,0.065f,0.055f,1.0f}) {
    const Color shoes{0.055f,0.062f,0.072f,1.0f};
    const float amount=std::max(0.0f,std::min(1.0f,intensity));
    const float forward_length=std::sqrt(forward.x*forward.x+forward.z*forward.z);
    const Vec3 f=forward_length>1.0e-4f?
        Vec3{forward.x/forward_length,0.0f,forward.z/forward_length}:Vec3{0.0f,0.0f,-1.0f};
    const Vec3 right{f.z,0.0f,-f.x};
    const float swing=std::sin(phase)*0.19f*amount;
    const float opposite=-swing;
    const float left_lift=std::max(0.0f,std::sin(phase))*0.085f*amount;
    const float right_lift=std::max(0.0f,-std::sin(phase))*0.085f*amount;
    const float bob=0.025f*amount*(0.5f+0.5f*std::sin(phase*2.0f));

    auto ellipsoid=[&](const Vec3&center,const Vec3&half,const Color&color,
                       float yaw=0.0f,float pitch=0.0f){
        Box part{center,half,color,false};
        part.shape=GeometryShape::Ellipsoid;
        part.yaw=yaw;
        part.pitch=pitch;
        out.push_back(part);
    };
    auto segment=[&](const Vec3&a,const Vec3&b,float radius,const Color&color){
        const Vec3 delta=b-a;
        const float horizontal=std::sqrt(delta.x*delta.x+delta.z*delta.z);
        const float length=std::sqrt(horizontal*horizontal+delta.y*delta.y);
        if(length<1.0e-4f)return;
        const Vec3 center=(a+b)*0.5f;
        Box part{center,{radius,length*0.5f+radius*0.35f,radius*0.88f},color,false};
        part.shape=GeometryShape::Ellipsoid;
        part.yaw=std::atan2(delta.x,delta.z);
        part.pitch=std::atan2(horizontal,delta.y);
        out.push_back(part);
    };

    // Pelvis, chest, neck, head and hair create a readable human silhouette.
    ellipsoid({p.x,0.99f+bob,p.z},{0.215f,0.16f,0.145f},pants);
    const float body_yaw=std::atan2(f.x,f.z);
    Box torso{{p.x,1.30f+bob,p.z},{0.255f,0.315f,0.165f},shirt,false};
    torso.shape=GeometryShape::TaperedTorso;
    torso.yaw=body_yaw;
    out.push_back(torso);
    // Rounded deltoids soften the gap between chest and upper arms.
    ellipsoid(p+right*(-0.22f)+Vec3{0,1.47f+bob,0},{0.13f,0.13f,0.145f},shirt,body_yaw);
    ellipsoid(p+right*0.22f+Vec3{0,1.47f+bob,0},{0.13f,0.13f,0.145f},shirt,body_yaw);
    ellipsoid({p.x,1.60f+bob,p.z},{0.085f,0.105f,0.085f},skin);
    ellipsoid({p.x,1.78f+bob,p.z},{0.145f,0.18f,0.13f},skin);
    // Rear hair mass and crown create a more natural head silhouette.
    ellipsoid(p-f*0.045f+Vec3{0,1.79f+bob,0},{0.139f,0.145f,0.112f},hair);
    ellipsoid({p.x,1.91f+bob,p.z},{0.148f,0.075f,0.134f},hair);
    ellipsoid({p.x,1.535f+bob,p.z},{0.105f,0.055f,0.105f},shirt);
    if(detailed_face){
        // The protagonist and pedestrians share compact facial detail.
        const Color eye_white{0.91f,0.88f,0.78f,1.0f};
        const Color iris{0.22f,0.15f,0.10f,1.0f};
        const Color pupil{0.025f,0.022f,0.020f,1.0f};
        const float face_yaw=std::atan2(f.x,f.z);
        for(float side:{-1.0f,1.0f}){
            const Vec3 eye_center=p+f*0.112f+right*(side*0.052f)+Vec3{0,1.805f+bob,0};
            ellipsoid(eye_center,{0.031f,0.025f,0.016f},eye_white,face_yaw);
            ellipsoid(eye_center+f*0.014f,{0.015f,0.017f,0.010f},iris,face_yaw);
            ellipsoid(eye_center+f*0.022f,{0.007f,0.010f,0.006f},pupil,face_yaw);
            ellipsoid(p+f*0.108f+right*(side*0.053f)+Vec3{0,1.846f+bob,0},
                {0.039f,0.010f,0.014f},hair,face_yaw);
            ellipsoid(p+right*(side*0.145f)+Vec3{0,1.78f+bob,0},
                {0.035f,0.060f,0.043f},skin,face_yaw);
        }
        ellipsoid(p+f*0.142f+Vec3{0,1.755f+bob,0},
            {0.027f,0.038f,0.033f},skin,face_yaw);
        ellipsoid(p+f*0.126f+Vec3{0,1.704f+bob,0},
            {0.038f,0.009f,0.011f},{0.24f,0.075f,0.065f,1},face_yaw);
    }

    // Arms are two tapered-looking articulated segments with hands.
    const float arm_left=std::sin(phase+3.14159265f)*0.15f*amount;
    const float arm_right=-arm_left;
    const Vec3 left_shoulder=p+right*(-0.275f)+Vec3{0,1.46f+bob,0};
    const Vec3 left_elbow=p+right*(-0.335f)+f*arm_left+Vec3{0,1.12f+bob,0};
    const Vec3 left_wrist=p+right*(-0.36f)+f*(arm_left*1.45f)+Vec3{0,0.89f+bob,0};
    const Vec3 right_shoulder=p+right*0.275f+Vec3{0,1.46f+bob,0};
    const Vec3 right_elbow=p+right*0.335f+f*arm_right+Vec3{0,1.12f+bob,0};
    const Vec3 right_wrist=p+right*0.36f+f*(arm_right*1.45f)+Vec3{0,0.89f+bob,0};
    segment(left_shoulder,left_elbow,0.092f,shirt);
    segment(left_elbow,left_wrist,0.073f,shirt);
    ellipsoid(left_wrist,{0.072f,0.085f,0.068f},skin);
    segment(right_shoulder,right_elbow,0.092f,shirt);
    segment(right_elbow,right_wrist,0.073f,shirt);
    ellipsoid(right_wrist,{0.072f,0.085f,0.068f},skin);

    // Separate thighs and shins animate around the hip and knee, not as rigid blocks.
    const Vec3 left_hip=p+right*(-0.12f)+f*(swing*0.18f)+Vec3{0,0.94f+bob,0};
    const Vec3 left_knee=p+right*(-0.13f)+f*swing+Vec3{0,0.52f+left_lift*0.30f,0};
    const Vec3 left_ankle=p+right*(-0.13f)+f*(swing*0.72f)+Vec3{0,0.17f+left_lift,0};
    const Vec3 right_hip=p+right*0.12f+f*(opposite*0.18f)+Vec3{0,0.94f+bob,0};
    const Vec3 right_knee=p+right*0.13f+f*opposite+Vec3{0,0.52f+right_lift*0.30f,0};
    const Vec3 right_ankle=p+right*0.13f+f*(opposite*0.72f)+Vec3{0,0.17f+right_lift,0};
    segment(left_hip,left_knee,0.125f,pants);
    segment(left_knee,left_ankle,0.088f,pants);
    segment(right_hip,right_knee,0.125f,pants);
    segment(right_knee,right_ankle,0.088f,pants);
    const float foot_yaw=std::atan2(f.x,f.z);
    ellipsoid(left_ankle+f*0.055f+Vec3{0,-0.065f,0},{0.135f,0.072f,0.205f},shoes,foot_yaw);
    ellipsoid(right_ankle+f*0.055f+Vec3{0,-0.065f,0},{0.135f,0.072f,0.205f},shoes,foot_yaw);
}
} // namespace

Vec3 Vec3::operator+(const Vec3&r)const noexcept{return{x+r.x,y+r.y,z+r.z};}
Vec3 Vec3::operator-(const Vec3&r)const noexcept{return{x-r.x,y-r.y,z-r.z};}
Vec3 Vec3::operator*(float s)const noexcept{return{x*s,y*s,z*s};}

CityWorld::CityWorld(){reset();}

WorldChunk CityWorld::generate_chunk(int cx,int cz)const{
    WorldChunk chunk;chunk.x=cx;chunk.z=cz;
    // Reserve for the worst facade/window pattern plus road markings and street furniture.
    chunk.high_detail_boxes.reserve(1024);
    chunk.low_detail_boxes.reserve(256);
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

    // High-contrast zebra crossings at each procedural avenue intersection.
    // They are part of the low-detail mesh too, so road readability survives LOD changes.
    const Color crosswalk_paint{0.88f,0.90f,0.88f,1};
    for(int ix=0;ix<2;++ix)for(int iz=0;iz<2;++iz){
        const float intersection_x=ox+16.0f+static_cast<float>(ix)*32.0f;
        const float intersection_z=oz+16.0f+static_cast<float>(iz)*32.0f;
        for(float side:{-1.0f,1.0f}){
            for(int stripe=0;stripe<5;++stripe){
                const float offset=(static_cast<float>(stripe)-2.0f)*0.72f;
                add({{intersection_x+side*5.2f+offset,0.082f,intersection_z},
                    {0.27f,0.025f,3.25f},crosswalk_paint,false},true);
                add({{intersection_x,0.082f,intersection_z+side*5.2f+offset},
                    {3.25f,0.025f,0.27f},crosswalk_paint,false},true);
            }
        }
    }

    const Color facades[]={{0.66f,0.63f,0.57f,1},{0.53f,0.57f,0.57f,1},
        {0.70f,0.66f,0.59f,1},{0.47f,0.51f,0.53f,1},{0.62f,0.55f,0.47f,1}};
    const Color bark{0.28f,0.18f,0.12f,1};
    const Color leaf_a{0.13f,0.31f,0.18f,1};
    const Color leaf_b{0.19f,0.39f,0.22f,1};

    // Planting strips are aligned with the outer edge of the sidewalk, not the facade.
    // The full canopy envelope stays outside both the carriageway and every building footprint.
    const std::array<Vec3,4> tree_positions{{
        {ox+5.9f,0.0f,oz+24.0f},{ox+24.0f,0.0f,oz+5.9f},
        {ox+58.1f,0.0f,oz+40.0f},{ox+40.0f,0.0f,oz+58.1f}}};
    for(const Vec3&tree:tree_positions){
        Box trunk{{tree.x,1.0f,tree.z},{0.13f,1.0f,0.13f},bark,false};
        trunk.shape=GeometryShape::Cylinder;
        add(trunk,true);
        Box crown{{tree.x,2.68f,tree.z},{0.86f,0.88f,0.86f},leaf_a,false};
        crown.shape=GeometryShape::Ellipsoid;
        add(crown,true);
        for(float side:{-1.0f,1.0f}){
            Box foliage{{tree.x+side*0.34f,2.53f,tree.z+side*0.10f},
                {0.55f,0.58f,0.56f},leaf_b,false};
            foliage.shape=GeometryShape::Ellipsoid;
            add(foliage,true);
        }
    }

    for(int bx=0;bx<2;++bx)for(int bz=0;bz<2;++bz){
        const int gx=cx*2+bx,gz=cz*2+bz;
        const std::uint32_t seed=hash_cell(gx,gz,17);
        const float x=ox+16.0f+static_cast<float>(bx)*32.0f;
        const float z=oz+16.0f+static_cast<float>(bz)*32.0f;
        const float w=6.4f+unit_random(seed)*1.1f;
        const float d=6.3f+unit_random(seed^0x1234U)*1.2f;
        const float h=10.0f+unit_random(seed^0x9abcU)*22.0f;
        const unsigned style=seed&3U;
        const Color facade=facades[seed%5U];
        const Color base_color{facade.r*0.88f,facade.g*0.88f,facade.b*0.88f,1};
        const Color trim{0.82f,0.79f,0.72f,1};
        const Color glass{0.11f,0.25f,0.31f,1};
        const Color warm_glass{0.46f,0.36f,0.22f,1};
        const Color window_frame{0.105f,0.125f,0.135f,1};

        // Collision is a single footprint; visible geometry is split into a podium
        // and a slightly inset tower so every building has a more natural skyline.
        const Box building{{x,h*0.5f,z},{w,h*0.5f,d},facade,true};
        chunk.collision_boxes.push_back(building);
        const float podium_height=3.35f;
        Box podium{{x,podium_height*0.5f,z},{w,podium_height*0.5f,d},base_color,false};
        podium.shape=GeometryShape::SoftBox;
        add(podium,true);

        const float upper_height=std::max(2.0f,h-podium_height);
        const float upper_w=w-((style&1U)?0.72f:0.42f);
        const float upper_d=d-((style&2U)?0.72f:0.42f);
        const float upper_x=x+((style&1U)?0.22f:-0.10f);
        const float upper_z=z+((style&2U)?0.22f:-0.10f);
        Box tower{{upper_x,podium_height+upper_height*0.5f,upper_z},
            {upper_w,upper_height*0.5f,upper_d},facade,false};
        tower.shape=GeometryShape::SoftBox;
        add(tower,true);
        Box roof_cap{{upper_x,h+0.14f,upper_z},{upper_w+0.16f,0.14f,upper_d+0.16f},
            {0.34f,0.37f,0.37f,1},false};
        roof_cap.shape=GeometryShape::SoftBox;
        add(roof_cap,true);

        // Clean corner lines and a restrained floor belt break up the large facade planes.
        for(float sx:{-1.0f,1.0f})for(float sz:{-1.0f,1.0f})
            add({{upper_x+sx*(upper_w-0.12f),podium_height+upper_height*0.5f,
                    upper_z+sz*(upper_d-0.12f)},
                {0.075f,upper_height*0.5f,0.075f},trim,false},false);
        for(float band=6.4f;band<h-1.0f;band+=7.2f){
            add({{upper_x,band,upper_z-upper_d-0.035f},{upper_w,0.055f,0.045f},
                base_color,false},false);
            add({{upper_x,band,upper_z+upper_d+0.035f},{upper_w,0.055f,0.045f},
                base_color,false},false);
        }

        // Storefront glazing, recessed door and quiet signage make the ground floor readable.
        add({{x,1.55f,z-d-0.055f},{0.62f,0.86f,0.055f},glass,false},false);
        add({{x-1.38f,1.62f,z-d-0.055f},{0.62f,0.76f,0.055f},glass,false},false);
        add({{x+1.38f,1.62f,z-d-0.055f},{0.62f,0.76f,0.055f},glass,false},false);
        const Color sign_colors[]={{0.48f,0.19f,0.14f,1},{0.12f,0.29f,0.34f,1},
            {0.56f,0.39f,0.20f,1},{0.22f,0.34f,0.25f,1}};
        const Color shop_color=sign_colors[(seed>>3U)%4U];
        add({{x,3.30f,z-d-0.10f},{1.14f,0.19f,0.09f},shop_color,false},false);
        add({{x,2.82f,z-d-0.42f},{1.48f,0.09f,0.43f},shop_color,false},false);
        for(int stripe=0;stripe<4;++stripe){
            const float stripe_x=x+(static_cast<float>(stripe)-1.5f)*0.72f;
            add({{stripe_x,2.83f,z-d-0.44f},{0.18f,0.095f,0.44f},
                {0.82f,0.78f,0.68f,1},false},false);
        }

        // Recessed windows use one compact pane per opening to keep the mobile mesh budget small.
        const int floors=std::max(2,std::min(8,static_cast<int>((h-podium_height)/3.05f)));
        for(int floor=0;floor<floors;++floor){
            const float y=4.9f+static_cast<float>(floor)*3.05f;
            if(y>h-0.8f)continue;
            for(int col=0;col<2;++col){
                const float wx=upper_x+(static_cast<float>(col)-0.5f)*upper_w*0.92f;
                const bool warm=(hash_cell(gx+col,gz+floor,91)&7U)==0U;
                const Color pane=warm?warm_glass:glass;
                // A dark, slightly oversized surround sits behind each pane. The glass
                // is offset toward the exterior so the frame remains visible without z-fighting.
                const float front_z=upper_z-upper_d-0.045f;
                const float back_z=upper_z+upper_d+0.045f;
                add({{wx,y,front_z},{0.77f,0.86f,0.035f},window_frame,false},false);
                add({{wx,y,back_z},{0.77f,0.86f,0.035f},window_frame,false},false);
                add({{wx,y,front_z-0.055f},{0.68f,0.77f,0.045f},pane,false},false);
                add({{wx,y,back_z+0.055f},{0.68f,0.77f,0.045f},pane,false},false);
                const float wz=upper_z+(static_cast<float>(col)-0.5f)*upper_d*0.92f;
                const float left_x=upper_x-upper_w-0.045f;
                const float right_x=upper_x+upper_w+0.045f;
                add({{left_x,y,wz},{0.035f,0.86f,0.77f},window_frame,false},false);
                add({{right_x,y,wz},{0.035f,0.86f,0.77f},window_frame,false},false);
                add({{left_x-0.055f,y,wz},{0.045f,0.77f,0.68f},pane,false},false);
                add({{right_x+0.055f,y,wz},{0.045f,0.77f,0.68f},pane,false},false);
            }
        }

        // Selected floors get projecting balconies with a slab, glass rail and metal cap.
        for(int floor=1;floor<floors;floor+=2){
            const float y=4.25f+static_cast<float>(floor)*3.05f;
            if(y>h-1.3f)continue;
            const float balcony_x=upper_x+((floor&2)?0.62f:-0.62f);
            add({{balcony_x,y,upper_z-upper_d-0.25f},{0.88f,0.075f,0.40f},
                {0.58f,0.58f,0.55f,1},false},false);
            add({{balcony_x,y+0.36f,upper_z-upper_d-0.59f},{0.82f,0.24f,0.045f},
                {0.30f,0.43f,0.47f,1},false},false);
            add({{balcony_x,y+0.62f,upper_z-upper_d-0.59f},{0.86f,0.035f,0.055f},
                trim,false},false);
            for(float side:{-1.0f,1.0f})
                add({{balcony_x+side*0.84f,y+0.34f,upper_z-upper_d-0.39f},
                    {0.035f,0.23f,0.22f},trim,false},false);
        }

        // Roof equipment is smaller and less decorative: HVAC and an occasional water tank.
        const Color rooftop{0.34f,0.38f,0.39f,1};
        add({{upper_x-upper_w*0.45f,h+0.46f,upper_z},{0.58f,0.18f,0.48f},rooftop,false},false);
        if((seed&1U)==0U){
            Box tank{{upper_x+upper_w*0.34f,h+0.76f,upper_z-upper_d*0.28f},
                {0.47f,0.59f,0.47f},{0.43f,0.49f,0.49f,1},false};
            tank.shape=GeometryShape::Ellipsoid;
            add(tank,false);
        }

        // Street lights are cylindrical and attached to the pedestrian verge.
        for(int side=-1;side<=1;side+=2){
            const float tx=x+static_cast<float>(side)*10.8f;
            const float tz=z;
            Box pole{{tx,2.05f,tz},{0.075f,2.05f,0.075f},{0.26f,0.28f,0.29f,1},false};
            pole.shape=GeometryShape::Cylinder;
            add(pole,false);
            Box arm{{tx+static_cast<float>(side)*0.66f,4.02f,tz},
                {0.075f,0.68f,0.075f},{0.26f,0.28f,0.29f,1},false};
            arm.shape=GeometryShape::Cylinder;
            arm.yaw=1.5707963f;
            arm.pitch=1.5707963f;
            add(arm,false);
            add({{tx+static_cast<float>(side)*1.25f,3.92f,tz},
                {0.13f,0.27f,0.31f},{0.91f,0.82f,0.62f,1},false},false);
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
    // Soft contact shadows ground the characters and parked vehicle without a shadow-map pass.
    // These translucent flattened ellipsoids are cheap enough for the Adreno 610 target.
    auto contact_shadow=[&](const Vec3&position,float radius_x,float radius_z,float alpha){
        Box shadow{{position.x,0.045f,position.z},{radius_x,0.022f,radius_z},
            {0.018f,0.025f,0.030f,alpha},false};
        shadow.shape=GeometryShape::Ellipsoid;
        out.push_back(shadow);
    };
    contact_shadow(player_position_,0.43f,0.34f,0.34f);
    append_humanoid(out,player_position_,{0.12f,0.35f,0.62f,1},{0.12f,0.15f,0.19f,1},
        player_facing_,player_animation_phase_,
        std::min(1.0f,player_speed/kMoveSpeed),true);
    if(mission_stage_==MissionStage::DeliverPackage)
        out.push_back({{player_position_.x+0.30f,1.18f,player_position_.z-0.18f},
            {0.28f,0.25f,0.25f},{0.68f,0.43f,0.20f,1},false});
    const std::array<Color,5> npc_skin{{
        {0.72f,0.52f,0.39f,1},{0.48f,0.32f,0.23f,1},{0.83f,0.65f,0.51f,1},
        {0.61f,0.42f,0.31f,1},{0.76f,0.57f,0.43f,1}}};
    const std::array<Color,5> npc_hair{{
        {0.055f,0.045f,0.040f,1},{0.16f,0.085f,0.035f,1},{0.10f,0.075f,0.055f,1},
        {0.22f,0.17f,0.12f,1},{0.035f,0.030f,0.028f,1}}};
    const std::array<Color,5> npc_shirts{{
        {0.42f,0.20f,0.14f,1},{0.14f,0.25f,0.34f,1},{0.38f,0.40f,0.32f,1},
        {0.42f,0.39f,0.34f,1},{0.22f,0.25f,0.29f,1}}};
    const std::array<Color,5> npc_pants{{
        {0.14f,0.16f,0.18f,1},{0.26f,0.24f,0.20f,1},{0.20f,0.23f,0.25f,1},
        {0.32f,0.27f,0.22f,1},{0.16f,0.16f,0.17f,1}}};
    for(int i=0;i<5;++i){
        const float phase=npc_elapsed_*0.18f+static_cast<float>(i)*1.25663706f;
        const float radius=23.0f+static_cast<float>(i%2)*3.0f;
        const float x=player_position_.x+std::cos(phase)*radius;
        const float z=player_position_.z+std::sin(phase)*radius;
        const Color shirt=npc_shirts[static_cast<std::size_t>(i)];
        const Color pants=npc_pants[static_cast<std::size_t>(i)];
        const Vec3 walking_forward{-std::sin(phase),0.0f,std::cos(phase)};
        contact_shadow({x,0.0f,z},0.42f,0.33f,0.27f);
        append_humanoid(out,{x,0,z},shirt,pants,walking_forward,
            npc_elapsed_*6.2f+static_cast<float>(i)*1.4f,0.82f,true,
            npc_skin[static_cast<std::size_t>(i)],npc_hair[static_cast<std::size_t>(i)]);
    }

    // Parked blue compact delivery car: body, cabin, front/rear glass and four tyres.
    const float cx=vehicle_position_.x,cz=vehicle_position_.z;
    contact_shadow(vehicle_position_,2.34f,1.12f,0.30f);
    const Color car_blue{0.06f,0.25f,0.54f,1};
    const Color glass{0.15f,0.42f,0.54f,1};
    const Color rubber{0.045f,0.052f,0.058f,1};
    Box car_body{{cx,0.72f,cz},{2.25f,0.52f,1.02f},car_blue,false};
    car_body.shape=GeometryShape::SoftBox;
    out.push_back(car_body);
    Box car_cabin{{cx-0.10f,1.34f,cz},{1.22f,0.34f,0.88f},car_blue,false};
    car_cabin.shape=GeometryShape::SoftBox;
    out.push_back(car_cabin);
    out.push_back({{cx+1.13f,1.36f,cz},{0.07f,0.28f,0.72f},glass,false});
    out.push_back({{cx-1.30f,1.36f,cz},{0.07f,0.28f,0.72f},glass,false});
    for(float sx:{-1.35f,1.35f})for(float sz:{-0.98f,0.98f}){
        Box tyre{{cx+sx,0.38f,cz+sz},{0.37f,0.16f,0.37f},rubber,false};
        tyre.shape=GeometryShape::Cylinder;
        tyre.pitch=1.5707963f;
        out.push_back(tyre);
    }
    const Color headlights{0.95f,0.84f,0.58f,1};
    out.push_back({{cx+2.23f,0.80f,cz-0.66f},{0.07f,0.14f,0.20f},headlights,false});
    out.push_back({{cx+2.23f,0.80f,cz+0.66f},{0.07f,0.14f,0.20f},headlights,false});

    // Side glazing, door handles, front grille, bumpers, tail lamps and wheel hubs.
    for(float side:{-1.0f,1.0f}){
        out.push_back({{cx-0.10f,1.35f,cz+side*0.91f},{0.73f,0.22f,0.045f},glass,false});
        out.push_back({{cx+0.18f,1.04f,cz+side*1.045f},{0.20f,0.035f,0.035f},
            {0.76f,0.79f,0.78f,1},false});
    }
    const Color grille{0.035f,0.055f,0.068f,1};
    const Color bumper{0.12f,0.15f,0.18f,1};
    const Color tail_lamps{0.72f,0.08f,0.055f,1};
    const Color wheel_hubs{0.48f,0.53f,0.56f,1};
    out.push_back({{cx+2.30f,0.76f,cz},{0.055f,0.22f,0.44f},grille,false});
    out.push_back({{cx+2.28f,0.45f,cz},{0.15f,0.12f,0.94f},bumper,false});
    out.push_back({{cx-2.28f,0.45f,cz},{0.15f,0.12f,0.94f},bumper,false});
    out.push_back({{cx-2.27f,0.82f,cz-0.70f},{0.07f,0.15f,0.18f},tail_lamps,false});
    out.push_back({{cx-2.27f,0.82f,cz+0.70f},{0.07f,0.15f,0.18f},tail_lamps,false});
    for(float sx:{-1.35f,1.35f})for(float sz:{-1.0f,1.0f}){
        Box hub{{cx+sx,0.38f,cz+sz*1.145f},{0.17f,0.045f,0.17f},wheel_hubs,false};
        hub.shape=GeometryShape::Cylinder;
        hub.pitch=1.5707963f;
        out.push_back(hub);
    }

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
