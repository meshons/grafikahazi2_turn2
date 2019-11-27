//=============================================================================================
// Mintaprogram: Zold haromszog. Ervenyes 2018. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Stork Gabor
// Neptun : NO047V
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// TODO delete on prod
#include <string>
#include <fstream>
#include <streambuf>

const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
    uniform float scale;        // scale
    uniform float translateX;   // x translate
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x * scale - translateX, vp.y * scale, 0, 1) * MVP;
	}
)";

const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram;

class Camera
{
    vec3 eye, lookat, right, up;
    float fov;
public:
    void set(vec3 _eye, vec3 _lookat, vec3 vup, float _fov) {
        eye = _eye;
        lookat = _lookat;
        vec3 w = eye - lookat;
        fov = _fov;
        float f = length(w);
        right = normalize(cross(vup, w)) * f * tan(fov / 2);
        up = normalize(cross(w, right)) * f * tan(fov / 2);
    }

    void setUniform() {
        gpuProgram.setUniform(eye, "wEye");
        gpuProgram.setUniform(lookat, "wLookAt");
        gpuProgram.setUniform(right, "wRight");
        gpuProgram.setUniform(up, "wUp");
    }

    void Animate(float dt) {
        eye = vec3((eye.x - lookat.x) * cos(dt) + (eye.z - lookat.z) * sin(dt) + lookat.x, eye.y,
                   -(eye.x - lookat.x) * sin(dt) + (eye.z - lookat.z) * cos(dt) + lookat.z
        );
        set(eye, lookat, up, fov);
    }
} camera;

class Material
{
protected:
    vec3 ka, kd, ks;
    float shininess;
    int mat;
public:
    void setUniform() {
        char buffer[256];
        sprintf(buffer, "materials[%d].ka", mat);
        gpuProgram.setUniform(ka, buffer);
        sprintf(buffer, "materials[%d].kd", mat);
        gpuProgram.setUniform(kd, buffer);
        sprintf(buffer, "materials[%d].ks", mat);
        gpuProgram.setUniform(ks, buffer);
        sprintf(buffer, "materials[%d].shininess", mat);
        gpuProgram.setUniform(shininess, buffer);
        sprintf(buffer, "materials[%d].F0", mat);
    }
};

class RoughMaterial : public Material
{
public:
    RoughMaterial(int _mat, vec3 _kd, vec3 _ks, float _shininess) {
        mat = _mat;
        ka = _kd;
        kd = _kd;
        ks = _ks;
        shininess = _shininess;
    }
};

class Shape
{
protected:
    mat4 matrix;

    vec3 position;
    float angle;
    vec3 direction;

    RoughMaterial mat;

    mat4 calculateTranslationMatrix() {
        return TranslateMatrix(position);
    }

    mat4 calculateRotationMatrix() {
        return RotationMatrix(M_PI * angle, direction);
    }

    virtual mat4 calculateScaleMatrix() = 0;

    void calculateMatrix() {
        matrix = matrix * calculateScaleMatrix();
        matrix = matrix * calculateRotationMatrix();
        matrix = matrix * calculateTranslationMatrix();
    }

public:
    Shape(RoughMaterial && _mat) : mat{_mat}, position{0, 0, 0}, direction{1, 0, 0} {
        matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }

    virtual void setUniform() = 0;

    mat4 getMatrix() {
        return matrix;
    }
};

class Cylinder : Shape
{
    float r1, r2;

    mat4 calculateScaleMatrix() final {
        return ScaleMatrix({r1, r2, 1});
    }

public:
    Cylinder() : Shape(
            {0, {0.5, 0, 0}, {0.1, 0.1, 0.1}, 2}
    ) {
        r1 = 0;
        r2 = 0;
    }

    void set(vec3 _position, vec3 _direction, float _r1, float _r2, float _angle) {
        position = _position;
        direction = normalize(_direction);
        r1 = _r1;
        r2 = _r2;
        angle = _angle;
        calculateMatrix();
        setUniform();
    }

    void setUniform() final {
        gpuProgram.setUniform(matrix, "cylinder.matrix");
        mat.setUniform();
    }
} cylinder;

class Hyperboloid : Shape
{
    float a, b, c;

    mat4 calculateScaleMatrix() final {
        return ScaleMatrix({a, b, c});
    }

public:
    Hyperboloid() : Shape(
            {1, {0.15625, 0.04296875, 0.2109375}, {0.1, 0.1, 0.1}, 2}
    ) {
        a = 0;
        b = 0;
        c = 0;
    }

    void set(vec3 _position, vec3 _direction, float _a, float _b, float _c, float _angle) {
        position = _position;
        direction = normalize(_direction);
        a = _a;
        b = _b;
        c = _c;
        angle = _angle;
        calculateMatrix();
        setUniform();
    }

    void setUniform() final {
        gpuProgram.setUniform(matrix, "hyperboloid.matrix");
        mat.setUniform();
    }
} hyperboloid;

class LightMaker
{
    struct light526nm
    {
        vec3 light;
        float intensify;
    };

    std::vector<vec2> cps;
    std::vector<light526nm> lights;
    vec3 whiteLight;

    float B(int i, float t) {
        int n = (int) cps.size() - 1;
        float choose = 1;
        for (int j = 1; j <= i; ++j)
            choose *= (float) (n - j + 1) / (float) j;
        return choose * (float) pow(t, i) * (float) pow(1 - t, n - i);
    }

    vec2 r(float t) {
        vec2 rr{0, 0};
        for (int i = 0; i < cps.size(); ++i)
            rr = rr + cps[i] * B(i, t);
        return rr;
    }

    void AddControlPoint(vec2 cp) {
        cps.push_back(cp);
    }

public:

    void create() {
        AddControlPoint({-0.1, -0.5});
        AddControlPoint({-0.2, 0.5});
        AddControlPoint({0.2, 0.5});
        AddControlPoint({0.1, -0.5});

        lights.reserve(100);

        for (int i = 1; i < 200; ++ ++i) {
            vec2 light = r((float) i / 100);
            float angle = light.x * (float) M_PI * 2;
            vec3 light3d = {sinf(angle), cosf(angle), light.y};
            lights.push_back({light3d, 1.0});
        }

        lights.shrink_to_fit();

        whiteLight = {5, 0, -1};
        setUniform();
    }

    void setUniform() {
        for (int i = 0; i < lights.size(); ++i) {
            gpuProgram.setUniform(
                    lights[i].light,
                    std::string() + "lights[" + std::to_string(i) + "].position"
            );
            gpuProgram.setUniform(
                    lights[i].intensify, std::string() + "lights[" + std::to_string(i) + "].intensify"
            );
        }
        gpuProgram.setUniform(
                whiteLight, "whiteLight.position"
        );
    }

} lightMaker;

class FullScreenTexturedQuad
{
    unsigned int vao;
public:
    void Create() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        unsigned int vbo;
        glGenBuffers(1, &vbo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        float vertexCoords[] = {-1, -1, 1, -1, 1, 1, -1, 1};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    void Draw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
} fullScreenTexturedQuad;

void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);

    std::ifstream t("../vertexShader.glsl");
    std::string vertexShader;

    t.seekg(0, std::ios::end);
    vertexShader.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    vertexShader.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    std::ifstream t2("../fragmentShader.glsl");
    std::string fragmentShader;

    t2.seekg(0, std::ios::end);
    fragmentShader.reserve(t2.tellg());
    t2.seekg(0, std::ios::beg);

    fragmentShader.assign((std::istreambuf_iterator<char>(t2)), std::istreambuf_iterator<char>());

    gpuProgram.create(vertexShader.c_str(), fragmentShader.c_str(), "fragmentColor");
    // TODO change in prod
    //gpuProgram.create(vertexSource, fragmentSource, "outColor");

    fullScreenTexturedQuad.Create();

    float fov = 70 * M_PI / 180;
    camera.set({5, 0, 0}, {0, 0, 0}, {0, 1, 0}, fov);
    camera.setUniform();

    cylinder.set(
            {1.3, -0.3, 0}, {1, 1, 1}, 0.1, 0.1, 0.25
    );
    hyperboloid.set(
            {-0.2, 0.1, 0}, {1, 1, 1}, 2.5, 2, 3, 1.25
    );

    lightMaker.create();
}

void onDisplay() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    fullScreenTexturedQuad.Draw();
    camera.setUniform();

    glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onMouseMotion(
        int pX, int pY
) {
}

void onMouse(
        int button, int state, int pX, int pY
) {
}

void onIdle() {
    //camera.Animate(0.0001f);
    glutPostRedisplay();
}
