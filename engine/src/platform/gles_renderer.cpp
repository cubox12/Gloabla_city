#include "gles_renderer.hpp"
#include "global_engine/touch_controls.hpp"
#include <android/log.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <string>
#include <utility>
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
Vec3 rotate_shape_vector(const Vec3&v,float yaw,float pitch){
    const float cp=std::cos(pitch),sp=std::sin(pitch);
    const float cy=std::cos(yaw),sy=std::sin(yaw);
    const float py=cp*v.y-sp*v.z;
    const float pz=sp*v.y+cp*v.z;
    return {cy*v.x+sy*pz,py,-sy*v.x+cy*pz};
}
Vec3 normalized_or(const Vec3&v,const Vec3&fallback){
    const float length=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    return length>1.0e-6f?Vec3{v.x/length,v.y/length,v.z/length}:fallback;
}
void append_lit_vertex(std::vector<float>&out,const Vec3&position,const Vec3&normal,const Color&color){
    const Vec3 light=normalized_or({-0.42f,0.84f,0.34f},{0,1,0});
    const float diffuse=std::max(0.0f,dot(normal,light));
    const float shade=0.58f+0.42f*diffuse;
    out.insert(out.end(),{position.x,position.y,position.z,
        color.r*shade,color.g*shade,color.b*shade,color.a});
}
void append_ellipsoid(std::vector<float>&out,const Box&b){
    constexpr int rings=5;
    constexpr int segments=8;
    const float pi=3.14159265358979323846f;
    auto sample=[&](float theta,float phi){
        const Vec3 n{std::sin(theta)*std::cos(phi),std::cos(theta),std::sin(theta)*std::sin(phi)};
        const Vec3 local{n.x*b.half_extent.x,n.y*b.half_extent.y,n.z*b.half_extent.z};
        const Vec3 position=b.center+rotate_shape_vector(local,b.yaw,b.pitch);
        const Vec3 normal=normalized_or(rotate_shape_vector({
            n.x/std::max(b.half_extent.x,1.0e-5f),
            n.y/std::max(b.half_extent.y,1.0e-5f),
            n.z/std::max(b.half_extent.z,1.0e-5f)},b.yaw,b.pitch),{0,1,0});
        return std::pair<Vec3,Vec3>{position,normal};
    };
    for(int ring=0;ring<rings;++ring){
        const float t0=pi*static_cast<float>(ring)/static_cast<float>(rings);
        const float t1=pi*static_cast<float>(ring+1)/static_cast<float>(rings);
        for(int segment=0;segment<segments;++segment){
            const float p0=2.0f*pi*static_cast<float>(segment)/static_cast<float>(segments);
            const float p1=2.0f*pi*static_cast<float>(segment+1)/static_cast<float>(segments);
            const auto a=sample(t0,p0),b0=sample(t1,p0),c=sample(t1,p1),d=sample(t0,p1);
            append_lit_vertex(out,a.first,a.second,b.color);
            append_lit_vertex(out,b0.first,b0.second,b.color);
            append_lit_vertex(out,c.first,c.second,b.color);
            append_lit_vertex(out,a.first,a.second,b.color);
            append_lit_vertex(out,c.first,c.second,b.color);
            append_lit_vertex(out,d.first,d.second,b.color);
        }
    }
}
void append_cylinder(std::vector<float>&out,const Box&b){
    constexpr int segments=8;
    const float pi=3.14159265358979323846f;
    auto point=[&](float angle,float y){
        const Vec3 local{std::cos(angle)*b.half_extent.x,y*b.half_extent.y,
            std::sin(angle)*b.half_extent.z};
        return b.center+rotate_shape_vector(local,b.yaw,b.pitch);
    };
    auto normal=[&](float angle,float ny){
        return normalized_or(rotate_shape_vector({std::cos(angle),ny,std::sin(angle)},
            b.yaw,b.pitch),{0,1,0});
    };
    const Vec3 top=b.center+rotate_shape_vector({0,b.half_extent.y,0},b.yaw,b.pitch);
    const Vec3 bottom=b.center+rotate_shape_vector({0,-b.half_extent.y,0},b.yaw,b.pitch);
    for(int i=0;i<segments;++i){
        const float a0=2.0f*pi*static_cast<float>(i)/static_cast<float>(segments);
        const float a1=2.0f*pi*static_cast<float>(i+1)/static_cast<float>(segments);
        const Vec3 p00=point(a0,-1),p01=point(a1,-1),p10=point(a0,1),p11=point(a1,1);
        const Vec3 n0=normal((a0+a1)*0.5f,0);
        append_lit_vertex(out,p00,n0,b.color);append_lit_vertex(out,p01,n0,b.color);
        append_lit_vertex(out,p11,n0,b.color);append_lit_vertex(out,p00,n0,b.color);
        append_lit_vertex(out,p11,n0,b.color);append_lit_vertex(out,p10,n0,b.color);
        const Vec3 nt=normal(0,1),nb=normal(0,-1);
        const Vec3 ct=top,cb=bottom;
        append_lit_vertex(out,ct,nt,b.color);append_lit_vertex(out,p10,nt,b.color);
        append_lit_vertex(out,p11,nt,b.color);
        append_lit_vertex(out,cb,nb,b.color);append_lit_vertex(out,p01,nb,b.color);
        append_lit_vertex(out,p00,nb,b.color);
    }
}
void append_soft_box(std::vector<float>&out,const Box&b){
    const Vec3&h=b.half_extent;
    const float bevel=std::min({h.x,h.y,h.z})*0.085f;
    const Vec3 inner{std::max(0.001f,h.x-bevel),std::max(0.001f,h.y-bevel),
        std::max(0.001f,h.z-bevel)};
    // Two subdivisions per face retain rounded edges at one quarter of the
    // previous vertex count, which matters when 25 sectors keep two LODs.
    const std::array<float,3> gx{{-1.0f,0.0f,1.0f}};
    const std::array<float,3> gy{{-1.0f,0.0f,1.0f}};
    const std::array<float,3> gz{{-1.0f,0.0f,1.0f}};
    auto face_point=[&](int face,int row,int col){
        switch(face){
            case 0:return Vec3{ h.x,gy[row]*h.y,gz[col]*h.z};
            case 1:return Vec3{-h.x,gy[row]*h.y,-gz[col]*h.z};
            case 2:return Vec3{gx[col]*h.x,h.y,gz[row]*h.z};
            case 3:return Vec3{-gx[col]*h.x,-h.y,gz[row]*h.z};
            case 4:return Vec3{gx[col]*h.x,gy[row]*h.y,h.z};
            default:return Vec3{-gx[col]*h.x,gy[row]*h.y,-h.z};
        }
    };
    auto rounded=[&](const Vec3&p,const Vec3&fallback){
        const Vec3 q{std::max(-inner.x,std::min(inner.x,p.x)),
            std::max(-inner.y,std::min(inner.y,p.y)),
            std::max(-inner.z,std::min(inner.z,p.z))};
        const Vec3 delta=p-q;
        const Vec3 n=normalized_or(delta,fallback);
        return std::pair<Vec3,Vec3>{b.center+q+n*bevel,n};
    };
    const std::array<Vec3,6> normals{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    for(int face=0;face<6;++face){
        for(int row=0;row<2;++row)for(int col=0;col<2;++col){
            const auto p00=rounded(face_point(face,row,col),normals[face]);
            const auto p10=rounded(face_point(face,row,col+1),normals[face]);
            const auto p11=rounded(face_point(face,row+1,col+1),normals[face]);
            const auto p01=rounded(face_point(face,row+1,col),normals[face]);
            append_lit_vertex(out,p00.first,p00.second,b.color);
            append_lit_vertex(out,p10.first,p10.second,b.color);
            append_lit_vertex(out,p11.first,p11.second,b.color);
            append_lit_vertex(out,p00.first,p00.second,b.color);
            append_lit_vertex(out,p11.first,p11.second,b.color);
            append_lit_vertex(out,p01.first,p01.second,b.color);
        }
    }
}
void append_box(std::vector<float>&out,const Box&b){
    if(b.shape==GeometryShape::Ellipsoid){append_ellipsoid(out,b);return;}
    if(b.shape==GeometryShape::Cylinder){append_cylinder(out,b);return;}
    if(b.shape==GeometryShape::SoftBox){append_soft_box(out,b);return;}
    const Vec3&c=b.center;const Vec3&h=b.half_extent;
    const std::array<Vec3,8>p={{{c.x-h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y+h.y,c.z+h.z},{c.x-h.x,c.y+h.y,c.z+h.z}}};
    constexpr int f[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
    constexpr float light[6]={.78f,.84f,.72f,.90f,1,.62f};
    for(int face=0;face<6;++face)for(int j=0;j<6;++j){
        const Vec3&v=p[f[face][j]];float k=light[face];
        out.insert(out.end(),{v.x,v.y,v.z,b.color.r*k,b.color.g*k,b.color.b*k,b.color.a});
    }
}
std::size_t geometry_vertex_count(const Box&box){
    switch(box.shape){
        case GeometryShape::Ellipsoid:return 240U;
        case GeometryShape::Cylinder:return 96U;
        case GeometryShape::SoftBox:return 144U;
        case GeometryShape::Box:return 36U;
    }
    return 36U;
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
        "uniform mat4 uMVP;uniform mat4 uView;out vec4 vColor;out float vFogDepth;out float vWorldHeight;out float vViewHeight;"
        "void main(){vec4 p=vec4(aPosition,1.0);vec4 viewPosition=uView*p;gl_Position=uMVP*p;"
        "vColor=aColor;vFogDepth=length(viewPosition.xyz);vWorldHeight=aPosition.y;vViewHeight=viewPosition.y;}\n";
    static constexpr char fs[]=
        "#version 300 es\nprecision mediump float;in vec4 vColor;in float vFogDepth;in float vWorldHeight;in float vViewHeight;"
        "out vec4 outColor;void main(){float fog=smoothstep(158.0,278.0,vFogDepth);"
        "float sun=0.96+0.07*smoothstep(0.0,48.0,vWorldHeight);"
        "vec3 lit=pow(max(vColor.rgb,vec3(0.0)),vec3(0.91))*sun;"
        "vec3 warm=lit*vec3(1.055,1.025,0.985);"
        "vec3 grade=(mix(lit,warm,0.42)-vec3(0.5))*1.12+vec3(0.5);"
        "float elevation=clamp(0.52+vViewHeight/max(vFogDepth,1.0)*1.65,0.0,1.0);"
        "vec3 horizon=vec3(0.78,0.84,0.87);vec3 zenith=vec3(0.27,0.49,0.70);"
        "vec3 sky=mix(horizon,zenith,smoothstep(0.05,0.94,elevation));"
        "float haze=mix(0.92,1.04,smoothstep(0.0,0.65,elevation));"
        "sky*=haze;float luminance=dot(grade,vec3(0.2126,0.7152,0.0722));"
        "grade=mix(vec3(luminance),grade,1.08);"
        "outColor=vec4(mix(grade,sky,fog),vColor.a);}\n";
    glClearColor(0.55f,0.72f,0.82f,1.0f);
    gpu_chunks_.reserve(25);
    dynamic_boxes_.reserve(132);
    dynamic_vertices_.reserve(132U*240U*7U);
    hud_vertices_.reserve(36000U);
    control_vertices_.reserve(16000U);
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
void GlesRenderer::upload_chunk(const WorldChunk&src,GpuChunk&dst,bool high_detail){
    dst.x=src.x;dst.z=src.z;
    const std::vector<Box>&boxes=high_detail?src.high_detail_boxes:src.low_detail_boxes;
    GLuint&buffer=high_detail?dst.high_buffer:dst.low_buffer;
    GLsizei&count=high_detail?dst.high_count:dst.low_count;
    std::vector<float>vertices;
    std::size_t required_vertices=0;
    for(const Box&box:boxes)required_vertices+=geometry_vertex_count(box);
    vertices.reserve(required_vertices*7U);
    for(const Box&b:boxes)append_box(vertices,b);
    if(!buffer)glGenBuffers(1,&buffer);
    if(!buffer){count=0;return;}
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(float)),
        vertices.data(),GL_STATIC_DRAW);
    count=static_cast<GLsizei>(vertices.size()/7U);
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
        draw_text("JOYSTICK IZQ  CAMARA DER",27,105,muted);
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
    control_vertices_.reserve(16000U);
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
    auto control_rect=[&](float x,float y,float w,float h,const Color&color){
        add_control_vertex(x,y,color);add_control_vertex(x+w,y,color);add_control_vertex(x+w,y+h,color);
        add_control_vertex(x,y,color);add_control_vertex(x+w,y+h,color);add_control_vertex(x,y+h,color);
    };
    // Compact top-down minimap: road grid, building blocks, player and current objective.
    if(width_>=700&&height_>=360){
        const float map_size=std::max(86.0f,std::min(118.0f,static_cast<float>(height_)*0.22f));
        const float map_x=static_cast<float>(width_)-map_size-14.0f,map_y=14.0f;
        const float inset=5.0f,map_left=map_x+inset,map_top=map_y+inset;
        const float map_inner=map_size-inset*2.0f,map_cx=map_x+map_size*0.5f,map_cy=map_y+map_size*0.5f;
        const float map_scale=map_inner/96.0f;
        const Vec3&position=world.player_position();
        control_rect(map_x-2.0f,map_y-2.0f,map_size+4.0f,map_size+4.0f,{0.72f,0.84f,0.84f,0.26f});
        control_rect(map_x,map_y,map_size,map_size,{0.025f,0.055f,0.065f,0.92f});
        control_rect(map_left,map_top,map_inner,map_inner,{0.16f,0.25f,0.18f,0.98f});
        auto clipped_map_rect=[&](float x,float y,float w,float h,const Color&color){
            const float left=std::max(map_left,x),top=std::max(map_top,y);
            const float right=std::min(map_left+map_inner,x+w),bottom=std::min(map_top+map_inner,y+h);
            if(right>left&&bottom>top)control_rect(left,top,right-left,bottom-top,color);
        };
        const int base_x=static_cast<int>(std::floor(position.x/32.0f));
        const int base_z=static_cast<int>(std::floor(position.z/32.0f));
        for(int ix=-3;ix<=3;++ix)for(int iz=-3;iz<=3;++iz){
            const float building_x=(static_cast<float>(base_x+ix)*32.0f)+16.0f;
            const float building_z=(static_cast<float>(base_z+iz)*32.0f)+16.0f;
            const float px=map_cx+(building_x-position.x)*map_scale;
            const float py=map_cy+(building_z-position.z)*map_scale;
            const Color block=((ix+iz)&1)==0?Color{0.31f,0.38f,0.27f,1}:Color{0.36f,0.39f,0.28f,1};
            clipped_map_rect(px-4.5f*map_scale,py-4.5f*map_scale,
                9.0f*map_scale,9.0f*map_scale,block);
        }
        const Color road_color{0.23f,0.27f,0.29f,1};
        for(int i=-3;i<=3;++i){
            const float road_x=static_cast<float>(base_x+i)*32.0f;
            const float road_z=static_cast<float>(base_z+i)*32.0f;
            const float px=map_cx+(road_x-position.x)*map_scale;
            const float py=map_cy+(road_z-position.z)*map_scale;
            clipped_map_rect(px-4.0f*map_scale,map_top,8.0f*map_scale,map_inner,road_color);
            clipped_map_rect(map_left,py-4.0f*map_scale,map_inner,8.0f*map_scale,road_color);
        }
        const Vec3&objective=world.mission_target();
        float target_dx=(objective.x-position.x)*map_scale;
        float target_dz=(objective.z-position.z)*map_scale;
        const float edge=map_inner*0.5f-6.0f;
        const float max_component=std::max(std::abs(target_dx),std::abs(target_dz));
        if(max_component>edge){const float factor=edge/max_component;target_dx*=factor;target_dz*=factor;}
        const Color objective_color=world.mission_completed()?Color{0.22f,0.92f,0.40f,1}:Color{0.08f,0.88f,1.0f,1};
        disk(map_cx+target_dx,map_cy+target_dz,4.1f,objective_color);
        disk(map_cx,map_cy,4.2f,{0.94f,0.97f,0.98f,1});
        disk(map_cx,map_cy,2.3f,{0.12f,0.42f,0.95f,1});
    }
    const VirtualStickLayout stick=virtual_stick_layout(width_,height_);
    const float base_x=stick.center_x;
    const float base_y=stick.center_y;
    const float radius=stick.radius;
    const float active=camera.joystick_active?1.0f:0.0f;
    // Large fixed-base pad with a bright rim, recessed track and raised thumb cap.
    disk(base_x,base_y+radius*0.055f,radius*1.14f,{0.005f,0.018f,0.026f,0.25f});
    disk(base_x,base_y,radius*1.06f,{0.53f,0.76f,0.82f,0.16f+0.12f*active});
    disk(base_x,base_y,radius*0.96f,{0.012f,0.045f,0.060f,0.64f});
    disk(base_x,base_y,radius*0.77f,{0.14f,0.31f,0.37f,0.40f});
    disk(base_x,base_y,radius*0.58f,{0.055f,0.13f,0.17f,0.22f});
    const float knob_x=base_x+camera.stick_knob_x*radius*0.66f;
    const float knob_y=base_y+camera.stick_knob_y*radius*0.66f;
    disk(knob_x,knob_y+radius*0.045f,radius*0.45f,{0.0f,0.015f,0.022f,0.36f});
    disk(knob_x,knob_y,radius*0.405f,{0.70f,0.88f,0.92f,0.74f+0.18f*active});
    disk(knob_x,knob_y-radius*0.035f,radius*0.31f,{0.24f,0.47f,0.53f,0.72f});
    disk(knob_x-radius*0.06f,knob_y-radius*0.095f,radius*0.075f,{0.88f,0.96f,0.98f,0.52f});
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
            gpu_chunks_.push_back({});it=std::prev(gpu_chunks_.end());
            it->x=chunk.x;it->z=chunk.z;
        }
        const bool high_lod=distance_sq<105.0f*105.0f;
        GLuint&buffer=high_lod?it->high_buffer:it->low_buffer;
        GLsizei&count=high_lod?it->high_count:it->low_count;
        // Upload only the active LOD; the other detail level stays off GPU until needed.
        if(buffer==0)upload_chunk(chunk,*it,high_lod);
        glBindBuffer(GL_ARRAY_BUFFER,buffer);attributes();
        if(count>0)glDrawArrays(GL_TRIANGLES,0,count);
    }

    world.gather_dynamic_draw_boxes(dynamic_boxes_);dynamic_vertices_.clear();
    std::size_t required_vertices=0;
    for(const Box&box:dynamic_boxes_)required_vertices+=geometry_vertex_count(box);
    const std::size_t required=required_vertices*7U;
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
