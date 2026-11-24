#include "gles_renderer.hpp"
#include <android/log.h>
#include <algorithm>
#include <array>
#include <iterator>
#include <cmath>
#include <string>
#include <vector>
namespace global_engine::platform {
namespace {
struct Mat4{std::array<float,16>v{};};
struct Plane{float x,y,z,w;};
Mat4 identity(){Mat4 m;m.v[0]=m.v[5]=m.v[10]=m.v[15]=1;return m;}
Mat4 multiply(const Mat4&a,const Mat4&b){Mat4 o;for(int c=0;c<4;++c)for(int r=0;r<4;++r){float s=0;for(int k=0;k<4;++k)s+=a.v[k*4+r]*b.v[c*4+k];o.v[c*4+r]=s;}return o;}
Mat4 perspective(float fov,float aspect,float n,float f){Mat4 m;float t=1/std::tan(fov*.5f);m.v[0]=t/aspect;m.v[5]=t;m.v[10]=(f+n)/(n-f);m.v[11]=-1;m.v[14]=(2*f*n)/(n-f);return m;}
Vec3 normalize(const Vec3&v){float l=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);return l>1e-5f?Vec3{v.x/l,v.y/l,v.z/l}:Vec3{0,0,-1};}
Vec3 cross(const Vec3&a,const Vec3&b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
float dot(const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Mat4 look_at(const Vec3&eye,const Vec3&target){Vec3 f=normalize(target-eye),s=normalize(cross(f,{0,1,0})),u=cross(s,f);Mat4 m=identity();m.v[0]=s.x;m.v[4]=s.y;m.v[8]=s.z;m.v[1]=u.x;m.v[5]=u.y;m.v[9]=u.z;m.v[2]=-f.x;m.v[6]=-f.y;m.v[10]=-f.z;m.v[12]=-dot(s,eye);m.v[13]=-dot(u,eye);m.v[14]=dot(f,eye);return m;}
std::array<Plane,6> extract_planes(const Mat4&m){
    std::array<Plane,6>p{{
        {m.v[3]+m.v[0],m.v[7]+m.v[4],m.v[11]+m.v[8],m.v[15]+m.v[12]},
        {m.v[3]-m.v[0],m.v[7]-m.v[4],m.v[11]-m.v[8],m.v[15]-m.v[12]},
        {m.v[3]+m.v[1],m.v[7]+m.v[5],m.v[11]+m.v[9],m.v[15]+m.v[13]},
        {m.v[3]-m.v[1],m.v[7]-m.v[5],m.v[11]-m.v[9],m.v[15]-m.v[13]},
        {m.v[3]+m.v[2],m.v[7]+m.v[6],m.v[11]+m.v[10],m.v[15]+m.v[14]},
        {m.v[3]-m.v[2],m.v[7]-m.v[6],m.v[11]-m.v[10],m.v[15]-m.v[14]}}};
    for(Plane&v:p){float l=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);if(l>1e-6f){v.x/=l;v.y/=l;v.z/=l;v.w/=l;}}
    return p;
}
bool sphere_visible(const std::array<Plane,6>&p,float x,float y,float z,float radius){
    for(const Plane&v:p)if(v.x*x+v.y*y+v.z*z+v.w < -radius)return false;
    return true;
}
GLuint compile(GLenum type,const char*source){GLuint sh=glCreateShader(type);glShaderSource(sh,1,&source,nullptr);glCompileShader(sh);GLint ok=0;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(ok==GL_TRUE)return sh;GLint n=0;glGetShaderiv(sh,GL_INFO_LOG_LENGTH,&n);std::string log(static_cast<std::size_t>(n>1?n:1),'\0');glGetShaderInfoLog(sh,n,nullptr,log.data());__android_log_print(ANDROID_LOG_ERROR,"GlobalEngine","Shader error: %s",log.c_str());glDeleteShader(sh);return 0;}
void append_box(std::vector<float>&out,const Box&b){
    const Vec3&c=b.center;const Vec3&h=b.half_extent;
    const std::array<Vec3,8>p={{{c.x-h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y+h.y,c.z+h.z},{c.x-h.x,c.y+h.y,c.z+h.z}}};
    constexpr int f[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
    constexpr float light[6]={.78f,.84f,.72f,.90f,1,.62f};
    for(int face=0;face<6;++face)for(int j=0;j<6;++j){const Vec3&v=p[f[face][j]];float k=light[face];out.insert(out.end(),{v.x,v.y,v.z,b.color.r*k,b.color.g*k,b.color.b*k,b.color.a});}
}
void attributes(){constexpr GLsizei stride=static_cast<GLsizei>(7U*sizeof(float));glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,nullptr);glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void*>(3U*sizeof(float)));}
} // namespace
bool GlesRenderer::initialize(){
    static constexpr char vs[]="#version 300 es\nlayout(location=0) in vec3 aPosition;layout(location=1) in vec4 aColor;uniform mat4 uMVP;out vec4 vColor;void main(){gl_Position=uMVP*vec4(aPosition,1.0);vColor=aColor;}\n";
    static constexpr char fs[]="#version 300 es\nprecision mediump float;in vec4 vColor;out vec4 outColor;void main(){outColor=vColor;}\n";
    GLuint v=compile(GL_VERTEX_SHADER,vs),f=compile(GL_FRAGMENT_SHADER,fs);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return false;}
    program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
    GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);if(linked!=GL_TRUE){glDeleteProgram(program_);program_=0;return false;}
    mvp_location_=glGetUniformLocation(program_,"uMVP");glGenBuffers(1,&dynamic_vertex_buffer_);
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glClearColor(.50f,.68f,.78f,1);
    return dynamic_vertex_buffer_!=0&&mvp_location_>=0;
}
void GlesRenderer::resize(int w,int h){width_=w>0?w:1;height_=h>0?h:1;glViewport(0,0,width_,height_);}
void GlesRenderer::upload_chunk(const WorldChunk&src,GpuChunk&dst){
    dst.x=src.x;dst.z=src.z;
    auto upload=[](const std::vector<Box>&boxes,GLuint&buffer,GLsizei&count){
        std::vector<float>vertices;std::size_t n=0;for(const Box&b:boxes)n+=36U*7U;
        vertices.reserve(n);for(const Box&b:boxes)append_box(vertices,b);
        if(!buffer)glGenBuffers(1,&buffer);glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(float)),vertices.data(),GL_STATIC_DRAW);
        count=static_cast<GLsizei>(vertices.size()/7U);
    };
    upload(src.high_detail_boxes,dst.high_buffer,dst.high_count);
    upload(src.low_detail_boxes,dst.low_buffer,dst.low_count);
}
void GlesRenderer::render(const CityWorld&world,const CameraState&camera){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);if(!program_||!dynamic_vertex_buffer_)return;
    const Vec3 player=world.player_position();const Vec3 target{player.x,1.0f,player.z};
    const float horizontal=camera.distance*std::cos(camera.pitch);
    const Vec3 eye{target.x+std::sin(camera.yaw)*horizontal,target.y+std::sin(camera.pitch)*camera.distance,target.z+std::cos(camera.yaw)*horizontal};
    const Mat4 vp=multiply(perspective(.92f,static_cast<float>(width_)/static_cast<float>(height_),.1f,360.0f),look_at(eye,target));
    const auto planes=extract_planes(vp);
    const auto&chunks=world.active_chunks();
    // Keep GPU buffers only for chunks currently resident in the CPU streaming window.
    for(auto it=gpu_chunks_.begin();it!=gpu_chunks_.end();){
        const bool exists=std::any_of(chunks.begin(),chunks.end(),[&](const WorldChunk&c){return c.x==it->x&&c.z==it->z;});
        if(!exists){if(it->high_buffer)glDeleteBuffers(1,&it->high_buffer);if(it->low_buffer)glDeleteBuffers(1,&it->low_buffer);it=gpu_chunks_.erase(it);}else ++it;
    }
    glUseProgram(program_);glUniformMatrix4fv(mvp_location_,1,GL_FALSE,vp.v.data());
    for(const WorldChunk&chunk:chunks){
        const float dx=chunk.center_x-player.x,dz=chunk.center_z-player.z;
        const float distance_sq=dx*dx+dz*dz;
        if(distance_sq>205.0f*205.0f||!sphere_visible(planes,chunk.center_x,8.0f,chunk.center_z,chunk.bounds_radius))continue;
        auto it=std::find_if(gpu_chunks_.begin(),gpu_chunks_.end(),[&](const GpuChunk&g){return g.x==chunk.x&&g.z==chunk.z;});
        if(it==gpu_chunks_.end()){gpu_chunks_.push_back({});it=std::prev(gpu_chunks_.end());upload_chunk(chunk,*it);}
        const bool high_lod=distance_sq<105.0f*105.0f;
        const GLuint buffer=high_lod?it->high_buffer:it->low_buffer;
        const GLsizei count=high_lod?it->high_count:it->low_count;
        glBindBuffer(GL_ARRAY_BUFFER,buffer);attributes();if(count>0)glDrawArrays(GL_TRIANGLES,0,count);
    }
    world.gather_dynamic_draw_boxes(dynamic_boxes_);dynamic_vertices_.clear();
    const std::size_t required=dynamic_boxes_.size()*36U*7U;if(dynamic_vertices_.capacity()<required)dynamic_vertices_.reserve(required);
    for(const Box&b:dynamic_boxes_)append_box(dynamic_vertices_,b);
    glBindBuffer(GL_ARRAY_BUFFER,dynamic_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(dynamic_vertices_.size()*sizeof(float)),dynamic_vertices_.data(),GL_STREAM_DRAW);
    attributes();const GLsizei dynamic_count=static_cast<GLsizei>(dynamic_vertices_.size()/7U);if(dynamic_count>0)glDrawArrays(GL_TRIANGLES,0,dynamic_count);
}
void GlesRenderer::shutdown(){
    for(GpuChunk&c:gpu_chunks_){if(c.high_buffer)glDeleteBuffers(1,&c.high_buffer);if(c.low_buffer)glDeleteBuffers(1,&c.low_buffer);}
    gpu_chunks_.clear();if(dynamic_vertex_buffer_)glDeleteBuffers(1,&dynamic_vertex_buffer_);if(program_)glDeleteProgram(program_);
    dynamic_vertex_buffer_=0;program_=0;mvp_location_=-1;dynamic_boxes_.clear();dynamic_vertices_.clear();
}
} // namespace global_engine::platform
