#include "gles_renderer.hpp"
#include <android/log.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iterator>
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
GLuint compile(GLenum type,const char*source){
    GLuint sh=glCreateShader(type);glShaderSource(sh,1,&source,nullptr);glCompileShader(sh);
    GLint ok=0;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(ok==GL_TRUE)return sh;
    GLint n=0;glGetShaderiv(sh,GL_INFO_LOG_LENGTH,&n);
    std::string log(static_cast<std::size_t>(n>1?n:1),'\0');
    glGetShaderInfoLog(sh,n,nullptr,log.data());
    __android_log_print(ANDROID_LOG_ERROR,"GlobalEngine","Shader error: %s",log.c_str());
    glDeleteShader(sh);return 0;
}
void append_box(std::vector<float>&out,const Box&b){
    const Vec3&c=b.center;const Vec3&h=b.half_extent;
    const std::array<Vec3,8>p={{{c.x-h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y+h.y,c.z+h.z},{c.x-h.x,c.y+h.y,c.z+h.z}}};
    constexpr int f[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
    constexpr float light[6]={.78f,.84f,.72f,.90f,1,.62f};
    for(int face=0;face<6;++face)for(int j=0;j<6;++j){
        const Vec3&v=p[f[face][j]];float k=light[face];
        out.insert(out.end(),{v.x,v.y,v.z,b.color.r*k,b.color.g*k,b.color.b*k,b.color.a});
    }
}
void attributes(){
    constexpr GLsizei stride=static_cast<GLsizei>(7U*sizeof(float));
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,nullptr);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void*>(3U*sizeof(float)));
}

// Compact 5x7 bitmap font: 26 capitals followed by digits 0-9.
constexpr std::uint8_t kFont[36][7]={
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};
} // namespace

bool GlesRenderer::initialize(){
    static constexpr char vs[]=
        "#version 300 es\n"
        "layout(location=0) in vec3 aPosition;layout(location=1) in vec4 aColor;"
        "uniform mat4 uMVP;uniform mat4 uView;out vec4 vColor;out float vFogDepth;"
        "void main(){vec4 p=vec4(aPosition,1.0);gl_Position=uMVP*p;"
        "vColor=aColor;vFogDepth=length((uView*p).xyz); }\n";
    static constexpr char fs[]=
        "#version 300 es\nprecision mediump float;in vec4 vColor;in float vFogDepth;"
        "out vec4 outColor;void main(){float fog=smoothstep(145.0,265.0,vFogDepth);"
        "vec3 lit=pow(max(vColor.rgb,vec3(0.0)),vec3(0.94));"
        "float contrast=(lit-vec3(0.5))*1.08+vec3(0.5);"
        "vec3 grade=mix(contrast,contrast*vec3(1.04,1.02,0.96),0.28);"
        "vec3 sky=vec3(0.55,0.72,0.82);"
        "outColor=vec4(mix(grade,sky,fog),vColor.a);}\n";
    glClearColor(0.55f,0.72f,0.82f,1.0f);
    GLuint v=compile(GL_VERTEX_SHADER,vs);
    if(!v){__android_log_print(ANDROID_LOG_ERROR,"GlobalEngine","Vertex shader compilation failed");return false;}
    GLuint f=compile(GL_FRAGMENT_SHADER,fs);
    if(!f){
        // Keep a deliberately simple GLES 3 fallback for drivers that reject optional fog/color grading.
        static constexpr char fallback_fs[]=
            "#version 300 es\nprecision mediump float;in vec4 vColor;"
            "out vec4 outColor;void main(){outColor=vColor;}\n";
        __android_log_print(ANDROID_LOG_WARN,"GlobalEngine","Using simple fragment shader fallback");
        f=compile(GL_FRAGMENT_SHADER,fallback_fs);
    }
    if(!f){glDeleteShader(v);__android_log_print(ANDROID_LOG_ERROR,"GlobalEngine","No usable fragment shader");return false;}
    program_=glCreateProgram();
    if(!program_){glDeleteShader(v);glDeleteShader(f);return false;}
    glAttachShader(program_,v);glAttachShader(program_,f);
    glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
    GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){
        GLint log_size=0;glGetProgramiv(program_,GL_INFO_LOG_LENGTH,&log_size);
        std::string link_log(static_cast<std::size_t>(std::max(1,log_size)),'\0');
        glGetProgramInfoLog(program_,log_size,nullptr,link_log.data());
        __android_log_print(ANDROID_LOG_ERROR,"GlobalEngine","Program link failed: %s",link_log.c_str());
        glDeleteProgram(program_);program_=0;return false;
    }
    mvp_location_=glGetUniformLocation(program_,"uMVP");
    view_location_=glGetUniformLocation(program_,"uView");
    glGenBuffers(1,&dynamic_vertex_buffer_);glGenBuffers(1,&hud_vertex_buffer_);glGenBuffers(1,&control_vertex_buffer_);
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const GLenum error=glGetError();
    if(dynamic_vertex_buffer_==0||hud_vertex_buffer_==0||control_vertex_buffer_==0||
       mvp_location_<0||view_location_<0||error!=GL_NO_ERROR){
        __android_log_print(ANDROID_LOG_ERROR,"GlobalEngine",
            "Renderer resource setup failed: buffers=%u/%u/%u uniforms=%d/%d glError=0x%x",
            dynamic_vertex_buffer_,hud_vertex_buffer_,control_vertex_buffer_,
            mvp_location_,view_location_,static_cast<unsigned>(error));
        return false;
    }
    __android_log_print(ANDROID_LOG_INFO,"GlobalEngine","GLES renderer initialized successfully");
    return true;
}
void GlesRenderer::resize(int w,int h){
    width_=w>0?w:1;height_=h>0?h:1;glViewport(0,0,width_,height_);
    hud_cached_width_=0;hud_cached_height_=0;
}
void GlesRenderer::upload_chunk(const WorldChunk&src,GpuChunk&dst){
    dst.x=src.x;dst.z=src.z;
    auto upload=[](const std::vector<Box>&boxes,GLuint&buffer,GLsizei&count){
        std::vector<float>vertices;std::size_t n=0;for(const Box&b:boxes)n+=36U*7U;
        vertices.reserve(n);for(const Box&b:boxes)append_box(vertices,b);
        if(!buffer)glGenBuffers(1,&buffer);
        glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(float)),vertices.data(),GL_STATIC_DRAW);
        count=static_cast<GLsizei>(vertices.size()/7U);
    };
    upload(src.high_detail_boxes,dst.high_buffer,dst.high_count);
    upload(src.low_detail_boxes,dst.low_buffer,dst.low_count);
}
void GlesRenderer::draw_hud(const CityWorld&world,const CameraState&camera){
    const int distance=std::max(0,static_cast<int>(world.mission_distance()+0.5f));
    const bool complete=world.mission_completed();
    const int stage=static_cast<int>(world.mission_stage());
    if(hud_cached_width_!=width_||hud_cached_height_!=height_||
       hud_cached_distance_!=distance||hud_cached_complete_!=complete||hud_cached_stage_!=stage){
        hud_vertices_.clear();
        hud_vertices_.reserve(36000);
        auto add_vertex=[&](float px,float py,const Color&color){
            const float x=2.0f*px/static_cast<float>(width_)-1.0f;
            const float y=1.0f-2.0f*py/static_cast<float>(height_);
            hud_vertices_.insert(hud_vertices_.end(),{x,y,0.0f,color.r,color.g,color.b,color.a});
        };
        auto rect=[&](float x,float y,float w,float h,const Color&color){
            add_vertex(x,y,color);add_vertex(x+w,y,color);add_vertex(x+w,y+h,color);
            add_vertex(x,y,color);add_vertex(x+w,y+h,color);add_vertex(x,y+h,color);
        };
        const float panel_width=std::min(static_cast<float>(width_-24),520.0f);
        const float panel_height=137.0f;
        const float scale=std::max(1.15f,std::min(2.5f,
            std::min(static_cast<float>(width_)/500.0f,static_cast<float>(height_)/260.0f)));
        rect(12,12,panel_width,panel_height,{0.025f,0.055f,0.075f,0.90f});
        rect(12,12,5,panel_height,complete?Color{0.20f,0.86f,0.36f,1}:Color{0.05f,0.76f,0.92f,1});
        const Color white{0.91f,0.95f,0.96f,1};
        const Color muted{0.60f,0.76f,0.81f,1};
        auto draw_text=[&](const char*text,float x,float y,const Color&color){
            for(const char*ch=text;*ch!='\0';++ch){
                if(*ch==' '){x+=6.0f*scale;continue;}
                int index=-1;
                if(*ch>='A'&&*ch<='Z')index=static_cast<int>(*ch-'A');
                else if(*ch>='0'&&*ch<='9')index=26+static_cast<int>(*ch-'0');
                if(index>=0){
                    for(int row=0;row<7;++row)for(int col=0;col<5;++col){
                        if((kFont[index][row]&(1U<<static_cast<unsigned>(4-col)))!=0U)
                            rect(x+static_cast<float>(col)*scale,y+static_cast<float>(row)*scale,
                                scale*0.82f,scale*0.82f,color);
                    }
                }
                x+=6.0f*scale;
            }
        };
        char distance_line[32]{};
        std::snprintf(distance_line,sizeof(distance_line),"DISTANCIA %03d M",distance);
        draw_text("GLOBAL CITY MISION 01",27,24,white);
        if(stage==0){
            draw_text("BUSCA COCHE AZUL",27,51,white);
            draw_text(distance_line,27,78,white);
        }else if(stage==1){
            draw_text("PAQUETE RECOGIDO",27,51,white);
            draw_text("VE A LA ENTREGA",27,78,white);
        }else{
            draw_text("ENTREGA COMPLETADA",27,51,white);
            draw_text("PRIMER REPARTO LISTO",27,78,white);
        }
        draw_text("IZQ JOYSTICK  DER CAMARA",27,105,muted);
        hud_cached_width_=width_;hud_cached_height_=height_;
        hud_cached_distance_=distance;hud_cached_complete_=complete;hud_cached_stage_=stage;
        hud_vertex_count_=static_cast<GLsizei>(hud_vertices_.size()/7U);
        glBindBuffer(GL_ARRAY_BUFFER,hud_vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(hud_vertices_.size()*sizeof(float)),
            hud_vertices_.data(),GL_STATIC_DRAW);
    }
    const Mat4 hud_matrix=identity();
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
    glUseProgram(program_);
    glUniformMatrix4fv(mvp_location_,1,GL_FALSE,hud_matrix.v.data());
    glUniformMatrix4fv(view_location_,1,GL_FALSE,hud_matrix.v.data());
    glBindBuffer(GL_ARRAY_BUFFER,hud_vertex_buffer_);attributes();
    if(hud_vertex_count_>0)glDrawArrays(GL_TRIANGLES,0,hud_vertex_count_);

    // Fixed-position virtual stick on the left; right half remains a free-look drag area.
    control_vertices_.clear();
    control_vertices_.reserve(32U*3U*7U*2U+32U*6U*7U);
    auto add_control_vertex=[&](float px,float py,const Color&color){
        const float x=2.0f*px/static_cast<float>(width_)-1.0f;
        const float y=1.0f-2.0f*py/static_cast<float>(height_);
        control_vertices_.insert(control_vertices_.end(),{x,y,0.0f,color.r,color.g,color.b,color.a});
    };
    auto disk=[&](float cx,float cy,float radius,const Color&color){
        constexpr int segments=32;
        for(int i=0;i<segments;++i){
            const float a0=6.28318530718f*static_cast<float>(i)/segments;
            const float a1=6.28318530718f*static_cast<float>(i+1)/segments;
            add_control_vertex(cx,cy,color);
            add_control_vertex(cx+std::cos(a0)*radius,cy+std::sin(a0)*radius,color);
            add_control_vertex(cx+std::cos(a1)*radius,cy+std::sin(a1)*radius,color);
        }
    };
    const float base_x=camera.joystick_active?camera.joystick_x:static_cast<float>(width_)*0.18f;
    const float base_y=camera.joystick_active?camera.joystick_y:static_cast<float>(height_)*0.79f;
    const float radius=std::max(34.0f,std::min(58.0f,static_cast<float>(std::min(width_,height_))*0.105f));
    disk(base_x,base_y,radius,{0.025f,0.07f,0.09f,0.34f});
    disk(base_x,base_y,radius*0.82f,{0.24f,0.48f,0.54f,0.18f});
    const float knob_x=base_x+camera.move_x*radius*0.66f;
    const float knob_y=base_y-camera.move_y*radius*0.66f;
    disk(knob_x,knob_y,radius*0.39f,{0.68f,0.88f,0.91f,0.72f});
    // A small camera hint on the right: drag anywhere on that half to orbit.
    const float cam_x=static_cast<float>(width_)*0.84f,cam_y=static_cast<float>(height_)*0.79f;
    disk(cam_x,cam_y,13.0f,{0.025f,0.07f,0.09f,0.20f});
    disk(cam_x,cam_y,5.0f,{0.78f,0.91f,0.94f,0.38f});
    glBindBuffer(GL_ARRAY_BUFFER,control_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(control_vertices_.size()*sizeof(float)),
        control_vertices_.data(),GL_STREAM_DRAW);
    attributes();
    if(!control_vertices_.empty())
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(control_vertices_.size()/7U));
    glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);
}
void GlesRenderer::render(const CityWorld&world,const CameraState&camera){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(!program_||!dynamic_vertex_buffer_||!hud_vertex_buffer_)return;
    const Vec3 player=world.player_position();const Vec3 target{player.x,1.0f,player.z};
    const float horizontal=camera.distance*std::cos(camera.pitch);
    const Vec3 eye{target.x+std::sin(camera.yaw)*horizontal,
        target.y+std::sin(camera.pitch)*camera.distance,
        target.z+std::cos(camera.yaw)*horizontal};
    const Mat4 view=look_at(eye,target);
    const Mat4 vp=multiply(perspective(.92f,static_cast<float>(width_)/static_cast<float>(height_),.1f,360.0f),view);
    const auto planes=extract_planes(vp);
    const auto&chunks=world.active_chunks();
    // Only GPU-resident sectors are kept; leaving the stream radius releases their buffers.
    for(auto it=gpu_chunks_.begin();it!=gpu_chunks_.end();){
        const bool exists=std::any_of(chunks.begin(),chunks.end(),
            [&](const WorldChunk&c){return c.x==it->x&&c.z==it->z;});
        if(!exists){
            if(it->high_buffer)glDeleteBuffers(1,&it->high_buffer);
            if(it->low_buffer)glDeleteBuffers(1,&it->low_buffer);
            it=gpu_chunks_.erase(it);
        }else ++it;
    }
    glUseProgram(program_);
    glUniformMatrix4fv(mvp_location_,1,GL_FALSE,vp.v.data());
    glUniformMatrix4fv(view_location_,1,GL_FALSE,view.v.data());
    for(const WorldChunk&chunk:chunks){
        const float dx=chunk.center_x-player.x,dz=chunk.center_z-player.z;
        const float distance_sq=dx*dx+dz*dz;
        if(distance_sq>205.0f*205.0f||
           !sphere_visible(planes,chunk.center_x,8.0f,chunk.center_z,chunk.bounds_radius))continue;
        auto it=std::find_if(gpu_chunks_.begin(),gpu_chunks_.end(),
            [&](const GpuChunk&g){return g.x==chunk.x&&g.z==chunk.z;});
        if(it==gpu_chunks_.end()){
            gpu_chunks_.push_back({});it=std::prev(gpu_chunks_.end());upload_chunk(chunk,*it);
        }
        const bool high_lod=distance_sq<105.0f*105.0f;
        const GLuint buffer=high_lod?it->high_buffer:it->low_buffer;
        const GLsizei count=high_lod?it->high_count:it->low_count;
        glBindBuffer(GL_ARRAY_BUFFER,buffer);attributes();
        if(count>0)glDrawArrays(GL_TRIANGLES,0,count);
    }

    world.gather_dynamic_draw_boxes(dynamic_boxes_);dynamic_vertices_.clear();
    const std::size_t required=dynamic_boxes_.size()*36U*7U;
    if(dynamic_vertices_.capacity()<required)dynamic_vertices_.reserve(required);
    for(const Box&b:dynamic_boxes_)append_box(dynamic_vertices_,b);
    glBindBuffer(GL_ARRAY_BUFFER,dynamic_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(dynamic_vertices_.size()*sizeof(float)),
        dynamic_vertices_.data(),GL_STREAM_DRAW);
    attributes();
    const GLsizei dynamic_count=static_cast<GLsizei>(dynamic_vertices_.size()/7U);
    if(dynamic_count>0)glDrawArrays(GL_TRIANGLES,0,dynamic_count);
    draw_hud(world,camera);
}
void GlesRenderer::shutdown(){
    for(GpuChunk&c:gpu_chunks_){
        if(c.high_buffer)glDeleteBuffers(1,&c.high_buffer);
        if(c.low_buffer)glDeleteBuffers(1,&c.low_buffer);
    }
    gpu_chunks_.clear();
    if(dynamic_vertex_buffer_)glDeleteBuffers(1,&dynamic_vertex_buffer_);
    if(hud_vertex_buffer_)glDeleteBuffers(1,&hud_vertex_buffer_);
    if(control_vertex_buffer_)glDeleteBuffers(1,&control_vertex_buffer_);
    if(program_)glDeleteProgram(program_);
    dynamic_vertex_buffer_=0;hud_vertex_buffer_=0;control_vertex_buffer_=0;program_=0;
    mvp_location_=-1;view_location_=-1;dynamic_boxes_.clear();
    dynamic_vertices_.clear();hud_vertices_.clear();control_vertices_.clear();hud_vertex_count_=0;
    hud_cached_width_=0;hud_cached_height_=0;hud_cached_distance_=-1;hud_cached_stage_=-1;
}
} // namespace global_engine::platform
