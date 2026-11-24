#include "gles_renderer.hpp"
#include "global_engine/touch_controls.hpp"
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <unordered_map>

#define GE_LOG(...) __android_log_print(ANDROID_LOG_ERROR,"GlobalEngine",__VA_ARGS__)
#define GE_INFO(...) __android_log_print(ANDROID_LOG_INFO,"GlobalEngine",__VA_ARGS__)
namespace {
using namespace global_engine;
using namespace global_engine::platform;
struct Touch { float x{0.0f},y{0.0f}; bool joystick{false},camera{false}; };
struct AudioFeedback {
    SLObjectItf engine_object{nullptr},output_mix_object{nullptr},player_object{nullptr};
    SLEngineItf engine{nullptr};
    SLPlayItf play_interface{nullptr};
    SLAndroidSimpleBufferQueueItf queue{nullptr};
    std::vector<SLint16> samples;
    std::atomic<bool> busy{false};

    static void queue_callback(SLAndroidSimpleBufferQueueItf,void* context){
        static_cast<AudioFeedback*>(context)->busy.store(false);
    }
    bool initialize(){
        if(slCreateEngine(&engine_object,0,nullptr,0,nullptr,nullptr)!=SL_RESULT_SUCCESS)return false;
        if((*engine_object)->Realize(engine_object,SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS)return false;
        if((*engine_object)->GetInterface(engine_object,SL_IID_ENGINE,&engine)!=SL_RESULT_SUCCESS)return false;
        if((*engine)->CreateOutputMix(engine,&output_mix_object,0,nullptr,nullptr)!=SL_RESULT_SUCCESS)return false;
        if((*output_mix_object)->Realize(output_mix_object,SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS)return false;
        SLDataLocator_AndroidSimpleBufferQueue locator{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
        SLDataFormat_PCM format{SL_DATAFORMAT_PCM,2,SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,SL_BYTEORDER_LITTLEENDIAN};
        SLDataSource source{&locator,&format};
        SLDataLocator_OutputMix sink_locator{SL_DATALOCATOR_OUTPUTMIX,output_mix_object};
        SLDataSink sink{&sink_locator,nullptr};
        const SLInterfaceID ids[]={SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
        const SLboolean required[]={SL_BOOLEAN_TRUE};
        if((*engine)->CreateAudioPlayer(engine,&player_object,&source,&sink,1,ids,required)!=SL_RESULT_SUCCESS)return false;
        if((*player_object)->Realize(player_object,SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS)return false;
        if((*player_object)->GetInterface(player_object,SL_IID_PLAY,&play_interface)!=SL_RESULT_SUCCESS)return false;
        if((*player_object)->GetInterface(player_object,SL_IID_ANDROIDSIMPLEBUFFERQUEUE,&queue)!=SL_RESULT_SUCCESS)return false;
        if((*queue)->RegisterCallback(queue,queue_callback,this)!=SL_RESULT_SUCCESS)return false;
        return true;
    }
    void play(int effect){
        if(!queue||!play_interface||busy.exchange(true))return;
        const int frames=effect==2?5292:(effect==1?22050:(effect==0?13230:3528));
        samples.resize(static_cast<std::size_t>(frames)*2U);
        constexpr float two_pi=6.28318530718f;
        float phase=0.0f;
        constexpr float notes[4]={523.25f,659.25f,783.99f,1046.50f};
        std::uint32_t noise_state=0x8f31a2d5U;
        for(int i=0;i<frames;++i){
            const float t=static_cast<float>(i)/44100.0f;
            const float progress=static_cast<float>(i)/static_cast<float>(frames);
            float frequency=520.0f;
            float envelope=0.0f;
            float wave=0.0f;
            if(effect==0){
                // Package pickup: smooth rising two-tone chime.
                frequency=430.0f+520.0f*progress;
                envelope=std::min(1.0f,t/0.025f)*std::max(0.0f,1.0f-progress);
                phase+=two_pi*frequency/44100.0f;
                wave=std::sin(phase)+0.24f*std::sin(phase*2.01f);
            }else if(effect==1){
                // Delivery complete: four-note arpeggio with a soft harmonic tail.
                const int note=std::min(3,static_cast<int>(t/0.125f));
                frequency=notes[note];
                envelope=std::min(1.0f,t/0.018f)*std::max(0.0f,1.0f-progress*0.92f);
                phase+=two_pi*frequency/44100.0f;
                wave=std::sin(phase)+0.18f*std::sin(phase*2.0f);
            }else if(effect==2){
                // Footstep: short low thump with a tiny deterministic noise transient.
                envelope=std::exp(-t*25.0f);
                phase+=two_pi*(112.0f-26.0f*progress)/44100.0f;
                noise_state=noise_state*1664525U+1013904223U;
                const float noise=(static_cast<float>((noise_state>>8U)&0xffffU)/32767.5f)-1.0f;
                wave=0.82f*std::sin(phase)+0.18f*std::sin(phase*2.7f)+noise*0.12f*std::exp(-t*75.0f);
            }else{
                // Light UI tick reserved for future menu and interaction feedback.
                frequency=1120.0f-370.0f*progress;
                envelope=std::exp(-t*34.0f);
                phase+=two_pi*frequency/44100.0f;
                wave=std::sin(phase)+0.12f*std::sin(phase*2.0f);
            }
            const float gain=effect==2?0.12f:(effect==1?0.16f:0.15f);
            const SLint16 sample=static_cast<SLint16>(std::max(-1.0f,std::min(1.0f,wave*envelope*gain))*32767.0f);
            samples[static_cast<std::size_t>(i)*2U]=sample;
            samples[static_cast<std::size_t>(i)*2U+1U]=sample;
        }
        (*queue)->Clear(queue);
        if((*queue)->Enqueue(queue,samples.data(),static_cast<SLuint32>(samples.size()*sizeof(SLint16)))!=SL_RESULT_SUCCESS){
            busy.store(false);return;
        }
        (*play_interface)->SetPlayState(play_interface,SL_PLAYSTATE_PLAYING);
    }
    void shutdown(){
        if(play_interface)(*play_interface)->SetPlayState(play_interface,SL_PLAYSTATE_STOPPED);
        if(player_object)(*player_object)->Destroy(player_object);
        if(output_mix_object)(*output_mix_object)->Destroy(output_mix_object);
        if(engine_object)(*engine_object)->Destroy(engine_object);
        player_object=nullptr;output_mix_object=nullptr;engine_object=nullptr;
        engine=nullptr;play_interface=nullptr;queue=nullptr;samples.clear();busy.store(false);
    }
};
struct State {
    CityWorld world; CameraState camera; GlesRenderer renderer; InputState input; AudioFeedback audio;
    EGLDisplay display{EGL_NO_DISPLAY}; EGLSurface surface{EGL_NO_SURFACE};
    EGLContext context{EGL_NO_CONTEXT}; int width{1},height{1};
    bool ready{false};
    float step_elapsed{0.0f};
    int joystick_pointer{-1};
    std::unordered_map<int,Touch> touches;
    std::chrono::steady_clock::time_point last;
};
void draw_native_fallback(ANativeWindow* window) {
    if(!window)return;
    ANativeWindow_Buffer buffer{};
    if(ANativeWindow_lock(window,&buffer,nullptr)!=0)return;
    if(buffer.format==WINDOW_FORMAT_RGB_565){
        auto* pixels=static_cast<std::uint16_t*>(buffer.bits);
        const std::uint16_t color=static_cast<std::uint16_t>(((42U>>3U)<<11U)|((76U>>2U)<<5U)|(104U>>3U));
        for(int y=0;y<buffer.height;++y)
            std::fill_n(pixels+static_cast<std::ptrdiff_t>(y)*buffer.stride,buffer.width,color);
    }else{
        auto* pixels=static_cast<std::uint32_t*>(buffer.bits);
        // Little-endian RGBA/RGBX byte order: muted blue-gray startup diagnostic.
        constexpr std::uint32_t color=0xFF684C2AU;
        for(int y=0;y<buffer.height;++y)
            std::fill_n(pixels+static_cast<std::ptrdiff_t>(y)*buffer.stride,buffer.width,color);
    }
    ANativeWindow_unlockAndPost(window);
}
void destroy_egl(State& s) {
    if(s.ready) s.renderer.shutdown();
    s.audio.shutdown();
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
    GE_INFO("Initializing EGL for native window");
    s.display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(s.display==EGL_NO_DISPLAY){GE_LOG("eglGetDisplay failed: 0x%x",eglGetError());return false;}
    EGLint egl_major=0,egl_minor=0;
    if(eglInitialize(s.display,&egl_major,&egl_minor)!=EGL_TRUE){
        GE_LOG("eglInitialize failed: 0x%x",eglGetError());return false;
    }
    GE_INFO("EGL initialized: %d.%d",egl_major,egl_minor);
#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif
    const EGLint attrs[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
    EGLConfig config{};EGLint count=0;
    if(eglChooseConfig(s.display,attrs,&config,1,&count)!=EGL_TRUE||count==0){
        GE_LOG("No EGL config supports ES3 + window + depth16: 0x%x",eglGetError());
        const EGLint relaxed_attrs[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,
            EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
        if(eglChooseConfig(s.display,relaxed_attrs,&config,1,&count)!=EGL_TRUE||count==0){
            GE_LOG("No compatible EGL ES3 window config: 0x%x",eglGetError());return false;
        }
        GE_INFO("Using relaxed EGL ES3 config without depth-buffer requirement");
    }
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint visual=0;
    if(eglGetConfigAttrib(s.display,config,EGL_NATIVE_VISUAL_ID,&visual)==EGL_TRUE)
        ANativeWindow_setBuffersGeometry(window,0,0,visual);
    const EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
        s.context=eglCreateContext(s.display,config,EGL_NO_CONTEXT,ctx);
    if(s.context==EGL_NO_CONTEXT){GE_LOG("eglCreateContext ES3 failed: 0x%x",eglGetError());return false;}
    s.surface=eglCreateWindowSurface(s.display,config,window,nullptr);
    if(s.surface==EGL_NO_SURFACE){GE_LOG("eglCreateWindowSurface failed: 0x%x",eglGetError());return false;}
    if(eglMakeCurrent(s.display,s.surface,s.surface,s.context)!=EGL_TRUE){
        GE_LOG("eglMakeCurrent failed: 0x%x",eglGetError());return false;
    }
    (void)eglSwapInterval(s.display,1);
    s.width=ANativeWindow_getWidth(window);s.height=ANativeWindow_getHeight(window);
    s.renderer.resize(s.width,s.height);
    s.ready=s.renderer.initialize();
    if(!s.ready){
        s.renderer.shutdown();
        GE_LOG("Renderer initialization failed; keeping EGL alive for visible fallback diagnostics");
        glViewport(0,0,s.width,s.height);
        glClearColor(0.30f,0.43f,0.52f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        (void)eglSwapBuffers(s.display,s.surface);
    }else{
        const char* vendor=reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* renderer=reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* version=reinterpret_cast<const char*>(glGetString(GL_VERSION));
        GE_INFO("GLES ready: vendor=%s renderer=%s version=%s",
            vendor?vendor:"?",renderer?renderer:"?",version?version:"?");
        (void)s.audio.initialize();
    }
    s.last=std::chrono::steady_clock::now();
    return true;
}
void reset_joystick(State& s) {
    s.joystick_pointer=-1;
    s.input.move_x=0.0f;s.input.move_z=0.0f;
    s.camera.joystick_active=false;
    s.camera.move_x=0.0f;s.camera.move_y=0.0f;
    s.camera.stick_knob_x=0.0f;s.camera.stick_knob_y=0.0f;s.camera.stick_magnitude=0.0f;
}
void apply_joystick(State& s,float x,float y) {
    const VirtualStickLayout layout=virtual_stick_layout(s.width,s.height);
    const VirtualStickOutput value=map_virtual_stick(x,y,layout);
    s.camera.joystick_active=true;
    s.camera.joystick_x=layout.center_x;s.camera.joystick_y=layout.center_y;
    s.camera.move_x=value.move_x;s.camera.move_y=value.move_z;
    s.camera.stick_knob_x=value.knob_x;s.camera.stick_knob_y=value.knob_y;
    s.camera.stick_magnitude=value.magnitude;
    s.input.move_x=value.move_x;s.input.move_z=value.move_z;
}
void command(android_app* app,int32_t cmd) {
    auto* s=static_cast<State*>(app->userData);if(!s)return;
    if(cmd==APP_CMD_INIT_WINDOW&&app->window){
        if(!init_egl(*s,app->window)){GE_LOG("EGL initialization failed; showing CPU window fallback");destroy_egl(*s);draw_native_fallback(app->window);}
    } else if(cmd==APP_CMD_TERM_WINDOW)destroy_egl(*s);
    else if(cmd==APP_CMD_WINDOW_RESIZED&&app->window&&s->display!=EGL_NO_DISPLAY){
        s->width=ANativeWindow_getWidth(app->window);s->height=ANativeWindow_getHeight(app->window);
        s->renderer.resize(s->width,s->height);
    } else if(cmd==APP_CMD_LOST_FOCUS||cmd==APP_CMD_PAUSE){
        s->touches.clear();reset_joystick(*s);s->input={};
        s->last=std::chrono::steady_clock::now();
    } else if(cmd==APP_CMD_RESUME){
        s->last=std::chrono::steady_clock::now();
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
        const bool joystick=s->joystick_pointer<0&&is_virtual_stick_touch(x,y,s->width,s->height);
        const bool camera=!joystick&&x>=static_cast<float>(s->width)*0.44f;
        s->touches[id]={x,y,joystick,camera};
        if(joystick){
            s->joystick_pointer=id;
            apply_joystick(*s,x,y);
        }
        return 1;
    }
    if(mask==AMOTION_EVENT_ACTION_MOVE){
        for(std::size_t i=0;i<n;++i){
            const int id=AMotionEvent_getPointerId(e,i);auto it=s->touches.find(id);
            if(it==s->touches.end())continue;
            Touch& t=it->second;const float x=AMotionEvent_getX(e,i),y=AMotionEvent_getY(e,i);
            const float dx=x-t.x,dy=y-t.y;t.x=x;t.y=y;
            if(t.joystick&&id==s->joystick_pointer){
                apply_joystick(*s,x,y);
            }else if(t.camera){
                s->camera.yaw-=dx*0.0044f;
                s->camera.pitch=std::max(0.20f,std::min(1.10f,s->camera.pitch-dy*0.0032f));
            }
        }return 1;
    }
    if(mask==AMOTION_EVENT_ACTION_UP||mask==AMOTION_EVENT_ACTION_POINTER_UP||
       mask==AMOTION_EVENT_ACTION_CANCEL){
        if(mask==AMOTION_EVENT_ACTION_CANCEL){
            s->touches.clear();reset_joystick(*s);s->input={};
        }else{
            const std::size_t i=static_cast<std::size_t>((action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            if(i<n){
                const int id=AMotionEvent_getPointerId(e,i);auto it=s->touches.find(id);
                if(it!=s->touches.end()&&it->second.joystick&&id==s->joystick_pointer)
                    reset_joystick(*s);
                s->touches.erase(id);
            }
        }return 1;
    }
    return 0;
}
} // namespace

extern "C" void android_main(android_app* app) {
    GE_INFO("GLOBAL CITY-BETA 0.1 native entry point reached");
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
            const float dt=std::max(0.0f,std::min(0.05f,std::chrono::duration<float>(now-s.last).count()));s.last=now;
            const MissionStage previous_stage=s.world.mission_stage();
            s.world.update(dt,s.input);
            const MissionStage current_stage=s.world.mission_stage();
            if(previous_stage!=current_stage){
                if(current_stage==MissionStage::DeliverPackage)s.audio.play(0);
                else if(current_stage==MissionStage::Completed)s.audio.play(1);
                s.step_elapsed=0.0f;
            }else if(std::abs(s.input.move_x)+std::abs(s.input.move_z)>0.25f){
                s.step_elapsed+=dt;
                if(s.step_elapsed>=0.46f){s.audio.play(2);s.step_elapsed=0.0f;}
            }else s.step_elapsed=0.0f;
            s.renderer.render(s.world,s.camera);
            if(eglSwapBuffers(s.display,s.surface)!=EGL_TRUE){
                const EGLint err=eglGetError();GE_LOG("eglSwapBuffers failed: 0x%x",err);
                if((err==EGL_BAD_SURFACE||err==EGL_CONTEXT_LOST)&&app->window){
                    destroy_egl(s);
                    if(!init_egl(s,app->window))GE_LOG("EGL recovery failed");
                }
            }
        } else {
            // Keep the native window visibly alive when shader/GPU initialization fails.
            // A black screen used to hide the actual initialization error from users.
            if(s.display!=EGL_NO_DISPLAY&&s.surface!=EGL_NO_SURFACE&&s.context!=EGL_NO_CONTEXT){
                glViewport(0,0,s.width,s.height);
                glClearColor(0.30f,0.43f,0.52f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                (void)eglSwapBuffers(s.display,s.surface);
            }
            (void)ALooper_pollOnce(16,nullptr,&events,reinterpret_cast<void**>(&source));
            if(source)source->process(app,source);
        }
    }
}
