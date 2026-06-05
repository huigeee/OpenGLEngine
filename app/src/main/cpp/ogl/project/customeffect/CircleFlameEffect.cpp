#include "CircleFlameEffect.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "CircleFlame", __VA_ARGS__)

CircleFlameEffect::CircleFlameEffect() {}
CircleFlameEffect::~CircleFlameEffect() { release(); }

static GLuint compileProg(const char* vsSrc, const char* fsSrc, GLint& locVP, GLint& locSize) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr); glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    locVP = glGetUniformLocation(p, "uVP");
    locSize = glGetUniformLocation(p, "uSizeScale");
    return p;
}

void CircleFlameEffect::init() {
    release();

    // ---- Shared glow texture ----
    const int SZ = 32;
    unsigned char td[SZ*SZ];
    float ctr = (SZ-1)*0.5f;
    for (int y=0; y<SZ; y++) for (int x=0; x<SZ; x++) {
        float d = sqrtf((x-ctr)*(x-ctr)+(y-ctr)*(y-ctr))/ctr;
        float v = 1.0f-fminf(d,1.0f); v=v*v;
        td[y*SZ+x]=(unsigned char)(v*255);
    }
    glGenTextures(1,&texId_);
    glBindTexture(GL_TEXTURE_2D,texId_);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,SZ,SZ,0,GL_RED,GL_UNSIGNED_BYTE,td);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,0);

    // ---- Ring VAO ----
    glGenVertexArrays(1,&ringVAO_);
    glGenBuffers(1,&ringVBO_);
    rebuildRingVBO();
    glBindVertexArray(ringVAO_);
    glBindBuffer(GL_ARRAY_BUFFER,ringVBO_);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // ---- Ring shader ----
    {
const char* ringVSsrc=R"(#version 300 es
layout(location=0)in vec3 aPos;layout(location=1)in vec3 aColor;layout(location=2)in vec2 aUV;
uniform mat4 uVP;out vec3 vColor;out vec2 vUV;
void main(){gl_Position=uVP*vec4(aPos,1.0);vColor=aColor;vUV=aUV;})";
const char* ringFSsrc=R"(#version 300 es
precision mediump float;in vec3 vColor;in vec2 vUV;out vec4 FragColor;
void main(){
    float d=distance(vUV,vec2(0.5));
    float a=1.0-smoothstep(0.0,0.7,d);
    a=a*a*(1.0-a*0.3);
    FragColor=vec4(vColor*1.2,a*0.9);
})";
        GLuint rvs=glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(rvs,1,&ringVSsrc,nullptr);glCompileShader(rvs);
        GLuint rfs=glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(rfs,1,&ringFSsrc,nullptr);glCompileShader(rfs);
        ringProg_=glCreateProgram();
        glAttachShader(ringProg_,rvs);glAttachShader(ringProg_,rfs);glLinkProgram(ringProg_);
        glDeleteShader(rvs);glDeleteShader(rfs);
        ringProj_=glGetUniformLocation(ringProg_,"uVP");
    }

    // ---- Flame programs ----
    const char* pvs=R"(#version 300 es
layout(location=0)in vec3 aPos;layout(location=1)in vec4 aColor;layout(location=2)in float aSize;
uniform mat4 uVP;uniform float uSizeScale;out vec4 vColor;
void main(){vColor=aColor;gl_Position=uVP*vec4(aPos,1.0);gl_PointSize=aSize/gl_Position.w*uSizeScale;})";
    const char* pfs=R"(#version 300 es
precision mediump float;in vec4 vColor;uniform sampler2D uTex;out vec4 FragColor;
void main(){float a=texture(uTex,gl_PointCoord).r;FragColor=vec4(vColor.rgb,vColor.a*a);})";
    flameProg_=compileProg(pvs,pfs,flameVP_,flameSize_);
    ffProg_=compileProg(pvs,pfs,ffVP_,ffSize_);

    // ---- Flame VAO ----
    flames_.resize(maxFlames_);
    glGenVertexArrays(1,&flameVAO_);
    glGenBuffers(1,&flameVBO_);
    glBindVertexArray(flameVAO_);
    glBindBuffer(GL_ARRAY_BUFFER,flameVBO_);
    glBufferData(GL_ARRAY_BUFFER,maxFlames_*8*sizeof(float),nullptr,GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(7*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // ---- Firefly VAO ----
    fireflies_.resize(maxFireflies_);
    glGenVertexArrays(1,&ffVAO_);
    glGenBuffers(1,&ffVBO_);
    glBindVertexArray(ffVAO_);
    glBindBuffer(GL_ARRAY_BUFFER,ffVBO_);
    glBufferData(GL_ARRAY_BUFFER,maxFireflies_*8*sizeof(float),nullptr,GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(7*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    circleAlpha_=0.0f;
    burnAngle_=0.0f;
    emitAccum_=0.0f;
    ffAccum_=0.0f;
    LOGI("CircleFlameEffect init done");
}

// ---- Ring: two concentric circles with UVs for texture fade ----
void CircleFlameEffect::rebuildRingVBO() {
    int seg=ringSegments_;
    std::vector<float> v;
    v.reserve((seg+1)*2*8);
    float rOut=ringRadius+ringWidth*0.5f;
    float rIn=ringRadius-ringWidth*0.5f;
    if(rIn<0.01f)rIn=0.01f;
    // Color: warm orange
    float cr=1.0f,cg=0.4f,cb=0.1f;
    // Darker where burned
    float burnFrac=burnAngle_/(6.2832f);
    for(int i=0;i<=seg;i++){
        float a=6.2832f*i/seg;
        float ca=cosf(a),sa=sinf(a);
        // Vertex color: full brightness before burn, dim after
        float bright=1.0f;
        // Determine if this vertex is in the burned area
        float angle=i*6.2832f/seg;
        float diff=fabsf(angle-burnAngle_);
        if(diff>6.2832f-diff)diff=6.2832f-diff;
        if(diff<0.8f)bright=1.0f; // near burn front = bright
        else if(i*6.2832f/seg<burnAngle_)bright=0.8f; // burned = slightly dim
        else bright=0.3f; // unburned = dim

        // Outer
        v.push_back(ca*rOut); v.push_back(groundY); v.push_back(sa*rOut);
        v.push_back(cr*bright); v.push_back(cg*bright); v.push_back(cb*bright);
        v.push_back(0.0f); v.push_back(0.0f); // UV for fade (not used with color)
        // Inner
        v.push_back(ca*rIn); v.push_back(groundY); v.push_back(sa*rIn);
        v.push_back(cr*bright); v.push_back(cg*bright); v.push_back(cb*bright);
        v.push_back(1.0f); v.push_back(1.0f);
    }
    glBindBuffer(GL_ARRAY_BUFFER,ringVBO_);
    glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(float),v.data(),GL_DYNAMIC_DRAW);
}

void CircleFlameEffect::update(float dt) {
    // ---- Ring fade in ----
    if(circleAlpha_<1.0f){
        circleAlpha_+=dt*0.5f;
        if(circleAlpha_>1.0f)circleAlpha_=1.0f;
    }

    // ---- Flame spread along ring ----
    if(burnAngle_<6.2832f){
        burnAngle_+=burnSpeed*dt;
        if(burnAngle_>6.2832f)burnAngle_=6.2832f;
    }
    rebuildRingVBO();

    // ---- Emit flame particles from the burned portion of the ring ----
    if(burnAngle_>0.1f){
        emitAccum_+=200.0f*dt;
        while(emitAccum_>=1.0f){
            emitAccum_-=1.0f;
            for(auto& fp:flames_){
                if(!fp.active){
                    fp.active=true;
                    float a=randF(0,burnAngle_);
                    float r=ringRadius+randF(-ringWidth*0.4f,ringWidth*0.4f);
                    fp.pos=center+glm::vec3(cosf(a)*r,groundY,sinf(a)*r);
                    fp.vel=glm::vec3(randF(-0.15f,0.15f),randF(0.8f,1.8f),randF(-0.15f,0.15f));
                    fp.maxLife=randF(0.3f,0.8f);
                    fp.life=fp.maxLife;
                    fp.size=randF(4.0f,10.0f);
                    break;
                }
            }
        }
    }

    // ---- Update flames ----
    for(auto& fp:flames_){
        if(!fp.active)continue;
        fp.life-=dt;
        if(fp.life<=0){fp.active=false;continue;}
        fp.pos+=fp.vel*dt;
        fp.vel.y+=0.8f*dt;
        fp.vel.x+=randF(-0.3f,0.3f)*dt;
        fp.vel.z+=randF(-0.3f,0.3f)*dt;
        fp.size*=0.985f;
    }
    rebuildFlameVBO();

    // ---- Emit fireflies from burned area ----
    if(burnAngle_>0.5f){
        ffAccum_+=8.0f*dt;
        while(ffAccum_>=1.0f){
            ffAccum_-=1.0f;
            for(auto& ff:fireflies_){
                if(!ff.active){
                    ff.active=true;
                    float a=randF(0,burnAngle_);
                    float r=ringRadius+randF(-ringWidth*0.8f,ringWidth*0.8f);
                    ff.pos=center+glm::vec3(cosf(a)*r,groundY+randF(0.2f,0.5f),sinf(a)*r);
                    ff.vel=glm::vec3(randF(-0.03f,0.03f),randF(0.05f,0.15f),randF(-0.03f,0.03f));
                    ff.maxLife=randF(1.5f,3.0f);
                    ff.life=ff.maxLife;
                    ff.size=randF(1.5f,3.5f);
                    ff.phase=randF(0,6.2832f);
                    break;
                }
            }
        }
    }

    // ---- Update fireflies ----
    for(auto& ff:fireflies_){
        if(!ff.active)continue;
        ff.life-=dt;
        if(ff.life<=0){ff.active=false;continue;}
        ff.pos+=ff.vel*dt;
        ff.phase+=dt*3.0f;
        ff.pos.x+=sinf(ff.phase)*0.005f;
        ff.pos.z+=cosf(ff.phase*0.7f)*0.005f;
    }
    rebuildFireflyVBO();
}

void CircleFlameEffect::rebuildFlameVBO() {
    std::vector<float> d; d.reserve(maxFlames_*8);
    for(auto& fp:flames_){
        if(!fp.active)continue;
        float t=1.0f-fp.life/fp.maxLife;
        float r=1.0f,g=0.5f-t*0.4f,b=0.1f-t*0.1f; if(b<0)b=0;
        float a=1.0f-t;
        d.push_back(fp.pos.x);d.push_back(fp.pos.y);d.push_back(fp.pos.z);
        d.push_back(r);d.push_back(g);d.push_back(b);d.push_back(a);
        d.push_back(fp.size);
    }
    if(!d.empty()){
        glBindBuffer(GL_ARRAY_BUFFER,flameVBO_);
        glBufferSubData(GL_ARRAY_BUFFER,0,d.size()*sizeof(float),d.data());
    }
}

void CircleFlameEffect::rebuildFireflyVBO() {
    std::vector<float> d; d.reserve(maxFireflies_*8);
    for(auto& ff:fireflies_){
        if(!ff.active)continue;
        float t=1.0f-ff.life/ff.maxLife;
        float tw=0.5f+0.5f*sinf(ff.phase);
        float a=(1.0f-t)*tw;
        d.push_back(ff.pos.x);d.push_back(ff.pos.y);d.push_back(ff.pos.z);
        d.push_back(0.2f);d.push_back(0.8f);d.push_back(0.15f);d.push_back(a);
        d.push_back(ff.size);
    }
    if(!d.empty()){
        glBindBuffer(GL_ARRAY_BUFFER,ffVBO_);
        glBufferSubData(GL_ARRAY_BUFFER,0,d.size()*sizeof(float),d.data());
    }
}

void CircleFlameEffect::render(const float* viewMat, const float* projMat) {
    glm::mat4 vp=glm::make_mat4(projMat)*glm::make_mat4(viewMat);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texId_);

    // ---- 1. Ring ----
    if(circleAlpha_>0.01f){
        glUseProgram(ringProg_);
        glUniformMatrix4fv(ringProj_,1,GL_FALSE,glm::value_ptr(vp));
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(ringVAO_);
        glDrawArrays(GL_TRIANGLE_STRIP,0,(ringSegments_+1)*2);
        glBindVertexArray(0);
    }

    // ---- 2. Flames (additive) ----
    int fc=0; for(auto& fp:flames_)if(fp.active)fc++;
    if(fc>0){
        glUseProgram(flameProg_);
        glUniformMatrix4fv(flameVP_,1,GL_FALSE,glm::value_ptr(vp));
        glUniform1f(flameSize_,200.0f);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glBindVertexArray(flameVAO_);
        glDrawArrays(GL_POINTS,0,fc);
        glBindVertexArray(0);
    }

    // ---- 3. Fireflies (additive) ----
    int ffc=0; for(auto& ff:fireflies_)if(ff.active)ffc++;
    if(ffc>0){
        glUseProgram(ffProg_);
        glUniformMatrix4fv(ffVP_,1,GL_FALSE,glm::value_ptr(vp));
        glUniform1f(ffSize_,120.0f);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glBindVertexArray(ffVAO_);
        glDrawArrays(GL_POINTS,0,ffc);
        glBindVertexArray(0);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void CircleFlameEffect::release() {
    auto del=[&](GLuint& p){if(p){glDeleteProgram(p);p=0;}};
    del(ringProg_);del(flameProg_);del(ffProg_);
    if(ringVAO_){glDeleteVertexArrays(1,&ringVAO_);ringVAO_=0;}
    if(ringVBO_){glDeleteBuffers(1,&ringVBO_);ringVBO_=0;}
    if(flameVAO_){glDeleteVertexArrays(1,&flameVAO_);flameVAO_=0;}
    if(flameVBO_){glDeleteBuffers(1,&flameVBO_);flameVBO_=0;}
    if(ffVAO_){glDeleteVertexArrays(1,&ffVAO_);ffVAO_=0;}
    if(ffVBO_){glDeleteBuffers(1,&ffVBO_);ffVBO_=0;}
    if(texId_){glDeleteTextures(1,&texId_);texId_=0;}
    flames_.clear();fireflies_.clear();
}
