#include "Motorcycle.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Moto", __VA_ARGS__)

Motorcycle::Motorcycle() {}
Motorcycle::~Motorcycle() { release(); }

// ---- Helper: delete a mesh ----
static void delMesh(Motorcycle::Mesh& m) {
    if(m.vao){glDeleteVertexArrays(1,&m.vao);m.vao=0;}
    if(m.vbo){glDeleteBuffers(1,&m.vbo);m.vbo=0;}
    if(m.ebo){glDeleteBuffers(1,&m.ebo);m.ebo=0;}
    m.count=0;
}

// ---- Build box (centered at origin) ----
Motorcycle::Mesh Motorcycle::buildBox(float sx, float sy, float sz) {
    float hx=sx*0.5f, hy=sy*0.5f, hz=sz*0.5f;
    float verts[]={
        -hx,-hy,-hz,  hx,-hy,-hz,  hx, hy,-hz, -hx, hy,-hz, // front
        -hx,-hy, hz,  hx,-hy, hz,  hx, hy, hz, -hx, hy, hz  // back
    };
    unsigned short idx[]={
        0,1,2, 0,2,3,  // front
        4,5,6, 4,6,7,  // back
        1,5,6, 1,6,2,  // right
        0,4,7, 0,7,3,  // left
        3,2,6, 3,6,7,  // top
        0,1,5, 0,5,4   // bottom
    };
    Mesh m;
    glGenVertexArrays(1,&m.vao); glGenBuffers(1,&m.vbo); glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    glBindVertexArray(0);
    m.count=36;
    return m;
}

Motorcycle::Mesh Motorcycle::buildCylinder(float radius, float height, int seg) {
    std::vector<float> verts;
    std::vector<unsigned short> idx;
    float hy=height*0.5f;
    // Top ring + bottom ring
    for(int i=0;i<=seg;i++){
        float a=6.2832f*i/seg, ca=cosf(a), sa=sinf(a);
        verts.push_back(ca*radius); verts.push_back(hy); verts.push_back(sa*radius); // top
    }
    for(int i=0;i<=seg;i++){
        float a=6.2832f*i/seg, ca=cosf(a), sa=sinf(a);
        verts.push_back(ca*radius); verts.push_back(-hy); verts.push_back(sa*radius); // bottom
    }
    // Side triangles
    for(int i=0;i<seg;i++){
        idx.push_back(i); idx.push_back(i+1); idx.push_back(i+1+seg+1);
        idx.push_back(i); idx.push_back(i+1+seg+1); idx.push_back(i+seg+1);
    }
    Mesh m;
    glGenVertexArrays(1,&m.vao); glGenBuffers(1,&m.vbo); glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned short),idx.data(),GL_STATIC_DRAW);
    glBindVertexArray(0);
    m.count=(int)idx.size();
    return m;
}

Motorcycle::Mesh Motorcycle::buildTorus(float majorR, float minorR, int seg, int rings) {
    std::vector<float> verts;
    std::vector<unsigned short> idx;
    for(int j=0;j<=rings;j++){
        float b=6.2832f*j/rings, cb=cosf(b), sb=sinf(b);
        for(int i=0;i<=seg;i++){
            float a=6.2832f*i/seg, ca=cosf(a), sa=sinf(a);
            float x=(majorR+minorR*ca)*cb;
            float y=minorR*sa;
            float z=(majorR+minorR*ca)*sb;
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
        }
    }
    for(int j=0;j<rings;j++){
        for(int i=0;i<seg;i++){
            int a=j*(seg+1)+i;
            int b=a+1;
            int c=(j+1)*(seg+1)+i;
            int d=c+1;
            idx.push_back(a); idx.push_back(c); idx.push_back(b);
            idx.push_back(b); idx.push_back(c); idx.push_back(d);
        }
    }
    Mesh m;
    glGenVertexArrays(1,&m.vao); glGenBuffers(1,&m.vbo); glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned short),idx.data(),GL_STATIC_DRAW);
    glBindVertexArray(0);
    m.count=(int)idx.size();
    return m;
}

Motorcycle::Mesh Motorcycle::buildSpokes(float radius, int count) {
    std::vector<float> verts;
    std::vector<unsigned short> idx;
    for(int i=0;i<count;i++){
        float a=6.2832f*i/count, ca=cosf(a), sa=sinf(a);
        float r2=radius*0.2f;
        verts.push_back(ca*r2); verts.push_back(sa*r2); verts.push_back(0); // hub
        verts.push_back(ca*radius); verts.push_back(sa*radius); verts.push_back(0); // rim
        idx.push_back(i*2); idx.push_back(i*2+1);
    }
    Mesh m;
    glGenVertexArrays(1,&m.vao); glGenBuffers(1,&m.vbo); glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned short),idx.data(),GL_STATIC_DRAW);
    glBindVertexArray(0);
    m.count=count*2;
    return m;
}

void Motorcycle::buildSharedShader() {
    const char*vsSrc=R"(#version 300 es
layout(location=0)in vec3 aPos;
uniform mat4 uVP;uniform mat4 uModel;out vec3 vPos;
void main(){gl_Position=uVP*uModel*vec4(aPos,1.0);vPos=aPos;})";
    const char*fsSrc=R"(#version 300 es
precision mediump float;uniform vec3 uColor;in vec3 vPos;out vec4 FragColor;
void main(){
    vec3 n=normalize(cross(dFdx(vPos),dFdy(vPos)));
    float l=abs(dot(n,normalize(vec3(0.5,0.8,0.3))))*0.5+0.5;
    FragColor=vec4(uColor*l,1.0);
})";
    GLuint mvs=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(mvs,1,&vsSrc,nullptr); glCompileShader(mvs);
    GLuint mfs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mfs,1,&fsSrc,nullptr); glCompileShader(mfs);
    program_=glCreateProgram();
    glAttachShader(program_,mvs); glAttachShader(program_,mfs); glLinkProgram(program_);
    glDeleteShader(mvs); glDeleteShader(mfs);
    locVP_=glGetUniformLocation(program_,"uVP");
    locModel_=glGetUniformLocation(program_,"uModel");
    locColor_=glGetUniformLocation(program_,"uColor");
}

// ---- 绘制一个部件 ----
static void drawPart(const Motorcycle::Mesh& m, GLuint prog, GLint locModel, const glm::mat4& model, const glm::vec3& color,
                     GLint locVP, const glm::mat4& vp, GLint locColor) {
    if(!m.vao||m.count==0)return;
    glUseProgram(prog);
    glUniformMatrix4fv(locVP,1,GL_FALSE,glm::value_ptr(vp));
    glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(model));
    glUniform3fv(locColor,1,glm::value_ptr(color));
    glBindVertexArray(m.vao);
    if(m.ebo) glDrawElements(GL_TRIANGLES,m.count,GL_UNSIGNED_SHORT,nullptr);
    else glDrawElements(GL_LINES,m.count,GL_UNSIGNED_SHORT,nullptr);
    glBindVertexArray(0);
}

void Motorcycle::init() {
    release();
    buildSharedShader();

    // 车身各部件
    float s=scale;
    bodyMesh_   = buildBox(2.2f*s, 0.3f*s, 0.5f*s);     // 水平主体
    fuelTank_   = buildBox(0.5f*s, 0.35f*s, 0.45f*s);    // 油箱
    seatMesh_   = buildBox(0.4f*s, 0.2f*s, 0.4f*s);      // 坐垫
    headMesh_   = buildBox(0.2f*s, 0.3f*s, 0.2f*s);      // 车头
    handleMesh_ = buildCylinder(0.03f*s, 0.6f*s, 8);      // 把手横杆
    forkMesh_   = buildCylinder(0.025f*s, 0.5f*s, 6);     // 前叉
    exhaustMesh_= buildCylinder(0.04f*s, 0.4f*s, 8);      // 排气管
    wheelMesh_  = buildCylinder(0.25f*s, 0.08f*s, 16);   // 前轮
    wheelMesh2_ = buildCylinder(0.25f*s, 0.08f*s, 16);   // 后轮
    // spokeMesh 不再使用

    wheelAngle_=0.0f;
    LOGI("Motorcycle init done");
}

void Motorcycle::update(float dt) {
    wheelAngle_+=wheelSpeed*dt;
    if(wheelAngle_>6.2832f)wheelAngle_-=6.2832f;
}

void Motorcycle::render(const float* viewMat, const float* projMat) {
    if(!program_)return;
    glm::mat4 vp=glm::make_mat4(projMat)*glm::make_mat4(viewMat);
    float s=scale;

    // 基础变换
    glm::mat4 base=glm::translate(glm::mat4(1.0f),position);
    base=glm::rotate(base,heading,glm::vec3(0,1,0));

    auto T=[&](float x,float y,float z){return glm::translate(glm::mat4(1.0f),glm::vec3(x,y,z));};
    auto R=[&](float a,glm::vec3 ax){return glm::rotate(glm::mat4(1.0f),a,ax);};

    glm::vec3 red(0.8f,0.15f,0.1f);
    glm::vec3 dark(0.15f,0.15f,0.15f);
    glm::vec3 silver(0.6f,0.6f,0.62f);
    glm::vec3 black(0.05f,0.05f,0.05f);
    glm::vec3 brown(0.4f,0.25f,0.1f);

    // ---- 车身主体 ----
    drawPart(bodyMesh_,program_,locModel_,base*T(0,0.3f*s,0),red,locVP_,vp,locColor_);

    // ---- 油箱 (前上方) ----
    drawPart(fuelTank_,program_,locModel_,base*T(0.4f*s,0.55f*s,0),red,locVP_,vp,locColor_);

    // ---- 坐垫 (后上方) ----
    drawPart(seatMesh_,program_,locModel_,base*T(-0.45f*s,0.45f*s,0),brown,locVP_,vp,locColor_);

    // ---- 车头 ----
    drawPart(headMesh_,program_,locModel_,base*T(0.7f*s,0.5f*s,0),silver,locVP_,vp,locColor_);

    // ---- 把手 ----
    drawPart(handleMesh_,program_,locModel_,base*T(0.7f*s,0.7f*s,0)*R(1.57f,glm::vec3(0,0,1)),silver,locVP_,vp,locColor_);

    // ---- 前叉 ----
    drawPart(forkMesh_,program_,locModel_,base*T(0.7f*s,0.1f*s,0)*R(0.1f,glm::vec3(0,0,1)),silver,locVP_,vp,locColor_);

    // ---- 排气管 (右侧) ----
    drawPart(exhaustMesh_,program_,locModel_,base*T(-0.6f*s,0.1f*s,0.4f*s)*R(1.57f,glm::vec3(0,0,1)),silver,locVP_,vp,locColor_);

    // ---- 前轮 ----
    glm::mat4 wheelBase=glm::rotate(glm::mat4(1.0f),wheelAngle_,glm::vec3(0,0,1));
    glm::mat4 wOrient=glm::rotate(glm::mat4(1.0f),1.57f,glm::vec3(1,0,0));
    drawPart(wheelMesh_,program_,locModel_,base*T(0.7f*s,0.1f*s,0)*wheelBase*wOrient,black,locVP_,vp,locColor_);

    // ---- 后轮 ----
    drawPart(wheelMesh2_,program_,locModel_,base*T(-0.7f*s,0.1f*s,0)*wheelBase*wOrient,black,locVP_,vp,locColor_);
}

void Motorcycle::release() {
    auto d=[&](Mesh& m){delMesh(m);};
    d(bodyMesh_); d(fuelTank_); d(seatMesh_); d(headMesh_);
    d(handleMesh_); d(forkMesh_); d(exhaustMesh_);
    d(wheelMesh_); d(wheelMesh2_); d(spokeMesh_);
    if(program_){glDeleteProgram(program_);program_=0;}
}
