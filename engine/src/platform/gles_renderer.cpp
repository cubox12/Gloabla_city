#include "gles_renderer.hpp"
#include <android/log.h>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace global_engine::platform {
namespace {
struct Mat4 { std::array<float,16> v{}; };
Mat4 identity() {
    Mat4 m; m.v[0]=m.v[5]=m.v[10]=m.v[15]=1.0f; return m;
}
Mat4 multiply(const Mat4& a,const Mat4& b) {
    Mat4 o;
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) {
        float s=0; for(int k=0;k<4;++k) s+=a.v[k*4+r]*b.v[c*4+k];
        o.v[c*4+r]=s;
    }
    return o;
}
Mat4 perspective(float fov,float aspect,float n,float f) {
    Mat4 m; const float t=1.0f/std::tan(fov*0.5f);
    m.v[0]=t/aspect; m.v[5]=t; m.v[10]=(f+n)/(n-f);
    m.v[11]=-1; m.v[14]=(2*f*n)/(n-f); return m;
}
Vec3 normalize(const Vec3& v) {
    const float l=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    return l>1e-5f?Vec3{v.x/l,v.y/l,v.z/l}:Vec3{0,0,-1};
}
Vec3 cross(const Vec3&a,const Vec3&b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
float dot(const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Mat4 look_at(const Vec3& eye,const Vec3& target) {
    const Vec3 f=normalize(target-eye),s=normalize(cross(f,{0,1,0})),u=cross(s,f);
    Mat4 m=identity();
    m.v[0]=s.x;m.v[4]=s.y;m.v[8]=s.z;
    m.v[1]=u.x;m.v[5]=u.y;m.v[9]=u.z;
    m.v[2]=-f.x;m.v[6]=-f.y;m.v[10]=-f.z;
    m.v[12]=-dot(s,eye);m.v[13]=-dot(u,eye);m.v[14]=dot(f,eye);
    return m;
}
GLuint compile(GLenum type,const char* source) {
    GLuint sh=glCreateShader(type); glShaderSource(sh,1,&source,nullptr); glCompileShader(sh);
    GLint ok=0;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(ok==GL_TRUE)return sh;
    GLint n=0;glGetShaderiv(sh,GL_INFO_LOG_LENGTH,&n);
    std::string log(static_cast<std::size_t>(n>1?n:1),'\0');
    glGetShaderInfoLog(sh,n,nullptr,log.data());
    __android_log_print(ANDROID_LOG_ERROR,"GlobalEngine","Shader error: %s",log.c_str());
    glDeleteShader(sh);return 0;
}
void append_box(std::vector<float>& out,const Box& b) {
    const Vec3& c=b.center;const Vec3& h=b.half_extent;
    const std::array<Vec3,8> p={{
        {c.x-h.x,c.y-h.y,c.z-h.z},{c.x+h.x,c.y-h.y,c.z-h.z},
        {c.x+h.x,c.y+h.y,c.z-h.z},{c.x-h.x,c.y+h.y,c.z-h.z},
        {c.x-h.x,c.y-h.y,c.z+h.z},{c.x+h.x,c.y-h.y,c.z+h.z},
        {c.x+h.x,c.y+h.y,c.z+h.z},{c.x-h.x,c.y+h.y,c.z+h.z}}};
    constexpr int faces[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},
        {4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
    constexpr float light[6]={0.78f,0.84f,0.72f,0.90f,1.0f,0.62f};
    for(int f=0;f<6;++f)for(int j=0;j<6;++j){
        const Vec3& v=p[faces[f][j]];const float k=light[f];
        out.insert(out.end(),{v.x,v.y,v.z,b.color.r*k,b.color.g*k,b.color.b*k,b.color.a});
    }
}
void prepare_vertex_attributes() {
    constexpr GLsizei stride=static_cast<GLsizei>(7U*sizeof(float));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,nullptr);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,stride,
                          reinterpret_cast<const void*>(3U*sizeof(float)));
}
} // namespace

bool GlesRenderer::initialize() {
    static constexpr char vs_source[]=
        "#version 300 es\n"
        "layout(location=0) in vec3 aPosition;\n"
        "layout(location=1) in vec4 aColor;\n"
        "uniform mat4 uMVP;out vec4 vColor;\n"
        "void main(){gl_Position=uMVP*vec4(aPosition,1.0);vColor=aColor;}\n";
    static constexpr char fs_source[]=
        "#version 300 es\nprecision mediump float;in vec4 vColor;out vec4 outColor;\n"
        "void main(){outColor=vColor;}\n";
    GLuint vs=compile(GL_VERTEX_SHADER,vs_source),fs=compile(GL_FRAGMENT_SHADER,fs_source);
    if(!vs||!fs){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return false;}
    program_=glCreateProgram();glAttachShader(program_,vs);glAttachShader(program_,fs);
    glLinkProgram(program_);glDeleteShader(vs);glDeleteShader(fs);
    GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){glDeleteProgram(program_);program_=0;return false;}
    mvp_location_=glGetUniformLocation(program_,"uMVP");
    glGenBuffers(1,&static_vertex_buffer_);
    glGenBuffers(1,&dynamic_vertex_buffer_);
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glClearColor(0.50f,0.68f,0.78f,1.0f);
    return static_vertex_buffer_!=0&&dynamic_vertex_buffer_!=0&&mvp_location_>=0;
}
void GlesRenderer::resize(int w,int h) {
    width_=w>0?w:1;height_=h>0?h:1;glViewport(0,0,width_,height_);
}
void GlesRenderer::render(const CityWorld& world,const CameraState& camera) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(!program_||!static_vertex_buffer_||!dynamic_vertex_buffer_)return;

    // Static city meshes are converted and uploaded only when the world is rebuilt.
    if(uploaded_static_revision_!=world.static_geometry_revision()) {
        const std::vector<Box>& boxes=world.static_draw_boxes();
        static_vertices_.clear();
        static_vertices_.reserve(boxes.size()*36U*7U);
        for(const Box& box:boxes)append_box(static_vertices_,box);
        glBindBuffer(GL_ARRAY_BUFFER,static_vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(static_vertices_.size()*sizeof(float)),
            static_vertices_.data(),GL_STATIC_DRAW);
        static_vertex_count_=static_cast<GLsizei>(static_vertices_.size()/7U);
        uploaded_static_revision_=world.static_geometry_revision();
    }

    // Reuse CPU-side capacities for moving entities; only this small buffer streams.
    world.gather_dynamic_draw_boxes(dynamic_boxes_);
    dynamic_vertices_.clear();
    const std::size_t required=dynamic_boxes_.size()*36U*7U;
    if(dynamic_vertices_.capacity()<required)dynamic_vertices_.reserve(required);
    for(const Box& box:dynamic_boxes_)append_box(dynamic_vertices_,box);
    glBindBuffer(GL_ARRAY_BUFFER,dynamic_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(dynamic_vertices_.size()*sizeof(float)),
        dynamic_vertices_.data(),GL_STREAM_DRAW);

    const float horizontal=camera.distance*std::cos(camera.pitch);
    const Vec3 eye{std::sin(camera.yaw)*horizontal,
        std::sin(camera.pitch)*camera.distance,std::cos(camera.yaw)*horizontal};
    const Mat4 vp=multiply(perspective(0.92f,static_cast<float>(width_)/height_,0.1f,260.0f),
                           look_at(eye,{0,0,0}));
    glUseProgram(program_);
    glUniformMatrix4fv(mvp_location_,1,GL_FALSE,vp.v.data());

    glBindBuffer(GL_ARRAY_BUFFER,static_vertex_buffer_);
    prepare_vertex_attributes();
    if(static_vertex_count_>0)glDrawArrays(GL_TRIANGLES,0,static_vertex_count_);

    glBindBuffer(GL_ARRAY_BUFFER,dynamic_vertex_buffer_);
    prepare_vertex_attributes();
    const GLsizei dynamic_count=static_cast<GLsizei>(dynamic_vertices_.size()/7U);
    if(dynamic_count>0)glDrawArrays(GL_TRIANGLES,0,dynamic_count);
}
void GlesRenderer::shutdown() {
    if(static_vertex_buffer_)glDeleteBuffers(1,&static_vertex_buffer_);
    if(dynamic_vertex_buffer_)glDeleteBuffers(1,&dynamic_vertex_buffer_);
    if(program_)glDeleteProgram(program_);
    static_vertex_buffer_=0;dynamic_vertex_buffer_=0;program_=0;
    mvp_location_=-1;static_vertex_count_=0;uploaded_static_revision_=0;
    static_vertices_.clear();dynamic_vertices_.clear();dynamic_boxes_.clear();
}
} // namespace global_engine::platform
