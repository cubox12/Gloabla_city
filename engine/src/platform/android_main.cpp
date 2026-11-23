#include "gles_renderer.hpp"
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>

#define GE_LOG(...) __android_log_print(ANDROID_LOG_ERROR,"GlobalEngine",__VA_ARGS__)
namespace {
using namespace global_engine;
using namespace global_engine::platform;
struct Touch { float x,y,ox,oy; bool joystick; };
struct State {
    CityWorld world; CameraState camera; GlesRenderer renderer; InputState input;
    EGLDisplay display{EGL_NO_DISPLAY}; EGLSurface surface{EGL_NO_SURFACE};
    EGLContext context{EGL_NO_CONTEXT}; int width{1},height{1};
    bool ready{false}; float pinch{0};
    std::unordered_map<int,Touch> touches;
    std::chrono::steady_clock::time_point last;
};
void destroy_egl(State& s) {
    if(s.ready) s.renderer.shutdown();
    s.ready=false;
    if(s.display!=EGL_NO_DISPLAY){
        eglMakeCurrent(s.display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        if(s.context!=EGL_NO_CONTEXT) eglDestroyContext(s.display,s.context);
        if(s.surface!=EGL_NO_SURFACE) eglDestroySurface(s.display,s.surface);
        eglTerminate(s.display);
    }
    s.display=EGL_NO_DISPLAY;s.surface=EGL_NO_SURFACE;s.context=EGL_NO_CONTEXT;
}
bool init_egl(State& s,ANativeWindow* window) {
    s.display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(s.display==EGL_NO_DISPLAY||eglInitialize(s.display,nullptr,nullptr)!=EGL_TRUE)return false;
#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif
    const EGLint attrs[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
    EGLConfig config{};EGLint count=0;
    if(eglChooseConfig(s.display,attrs,&config,1,&count)!=EGL_TRUE||count==0)return false;
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint visual=0;
    if(eglGetConfigAttrib(s.display,config,EGL_NATIVE_VISUAL_ID,&visual)==EGL_TRUE)
        ANativeWindow_setBuffersGeometry(window,0,0,visual);
    const EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    s.context=eglCreateContext(s.display,config,EGL_NO_CONTEXT,ctx);
    s.surface=eglCreateWindowSurface(s.display,config,window,nullptr);
    if(s.context==EGL_NO_CONTEXT||s.surface==EGL_NO_SURFACE||
       eglMakeCurrent(s.display,s.surface,s.surface,s.context)!=EGL_TRUE)return false;
    s.width=ANativeWindow_getWidth(window);s.height=ANativeWindow_getHeight(window);
    s.renderer.resize(s.width,s.height);s.ready=s.renderer.initialize();
    s.last=std::chrono::steady_clock::now();return s.ready;
}
void command(android_app* app,int32_t cmd) {
    auto* s=static_cast<State*>(app->userData);if(!s)return;
    if(cmd==APP_CMD_INIT_WINDOW&&app->window){
        if(!init_egl(*s,app->window))destroy_egl(*s);
    } else if(cmd==APP_CMD_TERM_WINDOW)destroy_egl(*s);
    else if(cmd==APP_CMD_WINDOW_RESIZED&&app->window&&s->display!=EGL_NO_DISPLAY){
        s->width=ANativeWindow_getWidth(app->window);s->height=ANativeWindow_getHeight(app->window);
        s->renderer.resize(s->width,s->height);
    } else if(cmd==APP_CMD_LOST_FOCUS){
        s->input={};s->touches.clear();s->pinch=0;
    }
}
void key_input(State& s,int key,bool down) {
    if(key==AKEYCODE_A||key==AKEYCODE_DPAD_LEFT)s.input.move_x=down?-1.0f:0.0f;
    if(key==AKEYCODE_D||key==AKEYCODE_DPAD_RIGHT)s.input.move_x=down?1.0f:0.0f;
    if(key==AKEYCODE_W||key==AKEYCODE_DPAD_UP)s.input.move_z=down?1.0f:0.0f;
    if(key==AKEYCODE_S||key==AKEYCODE_DPAD_DOWN)s.input.move_z=down?-1.0f:0.0f;
}
int32_t input(android_app* app,AInputEvent* e) {
    auto* s=static_cast<State*>(app->userData);if(!s)return 0;
    if(AInputEvent_getType(e)==AINPUT_EVENT_TYPE_KEY){
        const int a=AKeyEvent_getAction(e);
        if(a==AKEY_EVENT_ACTION_DOWN||a==AKEY_EVENT_ACTION_UP){
            key_input(*s,AKeyEvent_getKeyCode(e),a==AKEY_EVENT_ACTION_DOWN);return 1;
        }return 0;
    }
    if(AInputEvent_getType(e)!=AINPUT_EVENT_TYPE_MOTION)return 0;
    const int action=AMotionEvent_getAction(e),mask=action&AMOTION_EVENT_ACTION_MASK;
    const std::size_t n=AMotionEvent_getPointerCount(e);
    if(mask==AMOTION_EVENT_ACTION_DOWN||mask==AMOTION_EVENT_ACTION_POINTER_DOWN){
        const std::size_t i=static_cast<std::size_t>((action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>
            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        const int id=AMotionEvent_getPointerId(e,i);
        const float x=AMotionEvent_getX(e,i),y=AMotionEvent_getY(e,i);
        s->touches[id]={x,y,x,y,x<static_cast<float>(s->width)*0.46f};return 1;
    }
    if(mask==AMOTION_EVENT_ACTION_MOVE){
        if(n>=2){
            const float dx=AMotionEvent_getX(e,0)-AMotionEvent_getX(e,1);
            const float dy=AMotionEvent_getY(e,0)-AMotionEvent_getY(e,1);
            const float d=std::sqrt(dx*dx+dy*dy);
            if(s->pinch>1.0f&&d>1.0f)
                s->camera.distance=std::max(24.0f,std::min(125.0f,s->camera.distance*s->pinch/d));
            s->pinch=d;return 1;
        }
        s->pinch=0;
        for(std::size_t i=0;i<n;++i){
            const int id=AMotionEvent_getPointerId(e,i);auto it=s->touches.find(id);
            if(it==s->touches.end())continue;
            Touch& t=it->second;const float x=AMotionEvent_getX(e,i),y=AMotionEvent_getY(e,i);
            const float dx=x-t.x,dy=y-t.y;t.x=x;t.y=y;
            if(t.joystick){
                s->input.move_x=std::max(-1.0f,std::min(1.0f,(x-t.ox)/74.0f));
                s->input.move_z=std::max(-1.0f,std::min(1.0f,(t.oy-y)/74.0f));
            } else {
                s->camera.yaw-=dx*0.005f;
                s->camera.pitch=std::max(0.18f,std::min(1.12f,s->camera.pitch-dy*0.0035f));
            }
        }return 1;
    }
    if(mask==AMOTION_EVENT_ACTION_UP||mask==AMOTION_EVENT_ACTION_POINTER_UP||
       mask==AMOTION_EVENT_ACTION_CANCEL){
        if(mask==AMOTION_EVENT_ACTION_CANCEL){s->touches.clear();s->input={};}
        else {
            const std::size_t i=static_cast<std::size_t>((action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            if(i<n){
                const int id=AMotionEvent_getPointerId(e,i);auto it=s->touches.find(id);
                if(it!=s->touches.end()&&it->second.joystick)s->input={};
                s->touches.erase(id);
            }
        }s->pinch=0;return 1;
    }
    return 0;
}
} // namespace

void android_main(android_app* app) {
    State s;app->userData=&s;app->onAppCmd=command;app->onInputEvent=input;
    s.last=std::chrono::steady_clock::now();
    while(true){
        int events=0;android_poll_source* source=nullptr;
        while(ALooper_pollOnce(s.display==EGL_NO_DISPLAY?-1:0,nullptr,&events,
              reinterpret_cast<void**>(&source))>=0){
            if(source)source->process(app,source);
            if(app->destroyRequested){destroy_egl(s);return;}
        }
        if(s.ready&&s.display!=EGL_NO_DISPLAY){
            const auto now=std::chrono::steady_clock::now();
            const float dt=std::chrono::duration<float>(now-s.last).count();s.last=now;
            s.world.update(dt,s.input);s.renderer.render(s.world,s.camera);
            if(eglSwapBuffers(s.display,s.surface)!=EGL_TRUE){
                const EGLint err=eglGetError();GE_LOG("eglSwapBuffers: 0x%x",err);
                if(err==EGL_BAD_SURFACE||err==EGL_CONTEXT_LOST)destroy_egl(s);
            }
        } else {
            (void)ALooper_pollOnce(16,nullptr,&events,reinterpret_cast<void**>(&source));
            if(source)source->process(app,source);
        }
    }
}
