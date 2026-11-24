#include "global_engine/world.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
struct Agent { float x,z,vx,vz; };
}
int main(){
    using clock=std::chrono::steady_clock;
    global_engine::CityWorld world;
    constexpr int frames=20000;
    constexpr int synthetic_agents=2048;
    constexpr float dt=1.0f/30.0f;
    std::vector<Agent> agents;agents.reserve(synthetic_agents);
    for(int i=0;i<synthetic_agents;++i){
        const float f=static_cast<float>(i);
        agents.push_back({f*0.37f,f*0.19f,std::sin(f)*1.4f,std::cos(f)*1.4f});
    }
    std::vector<double> frame_ms;frame_ms.reserve(frames);
    std::size_t peak_chunks=0,peak_boxes=0;
    for(int frame=0;frame<frames;++frame){
        const auto begin=clock::now();
        world.update(dt,{1.0f,0.0f});
        for(Agent&a:agents){
            a.x+=a.vx*dt;a.z+=a.vz*dt;
            if(a.x>64.0f)a.x-=64.0f;if(a.x<0.0f)a.x+=64.0f;
            if(a.z>64.0f)a.z-=64.0f;if(a.z<0.0f)a.z+=64.0f;
        }
        const auto end=clock::now();
        frame_ms.push_back(std::chrono::duration<double,std::milli>(end-begin).count());
        const auto&s=world.streaming_stats();
        peak_chunks=std::max(peak_chunks,s.active_chunks);
        peak_boxes=std::max(peak_boxes,s.resident_boxes);
        assert(s.active_chunks<=25);
        assert(s.resident_boxes<15000);
        assert(!world.player_intersects_building());
    }
    std::sort(frame_ms.begin(),frame_ms.end());
    const double p50=frame_ms[frame_ms.size()/2];
    const double p95=frame_ms[frame_ms.size()*95/100];
    const double worst=frame_ms.back();
    const auto&s=world.streaming_stats();
    assert(s.generated_chunks>100);
    assert(s.evicted_chunks>0);
    assert(peak_chunks==25);
    const std::size_t estimated_gpu_bytes=peak_boxes*36U*7U*sizeof(float);
    assert(estimated_gpu_bytes<32U*1024U*1024U);
    assert(world.building_count()<=100);
    for(const Agent&a:agents)assert(std::isfinite(a.x)&&std::isfinite(a.z));
    std::cout<<"LOW-END CPU STRESS PASS\n"
             <<"profile: 30 FPS target; synthetic crowd: "<<synthetic_agents<<" agents\n"
             <<"frames: "<<frames<<"\n"
             <<"chunk generated/evicted: "<<s.generated_chunks<<"/"<<s.evicted_chunks<<"\n"
             <<"peak resident chunks/boxes: "<<peak_chunks<<"/"<<peak_boxes<<"\n"
             <<"CPU frame ms p50/p95/max: "<<p50<<"/"<<p95<<"/"<<worst<<"\n"
             <<"30 FPS CPU budget: 33.33 ms (host result; not Android GPU performance)\n";
    return 0;
}
