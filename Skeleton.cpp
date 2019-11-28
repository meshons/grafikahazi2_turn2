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
    mat4 rotationMatrix;

    vec3 position;

    RoughMaterial mat;

    mat4 calculateTranslationMatrix() {
        return TranslateMatrix(position);
    }

    virtual mat4 calculateScaleMatrix() = 0;

    void calculateMatrix() {
        matrix = matrix * calculateScaleMatrix();
        matrix = matrix * rotationMatrix;
        matrix = matrix * calculateTranslationMatrix();
    }

public:
    Shape(RoughMaterial && _mat) : mat{_mat}, position{0, 0, 0}{
        matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        rotationMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }

    virtual void setUniform() = 0;

    void rotateBeforeSet(vec3 dir, float angle) {
        rotationMatrix = rotationMatrix * RotationMatrix(M_PI * angle, dir);
    }

    void rotate(vec3 dir, float angle) {
        matrix = matrix * RotationMatrix(M_PI * angle, dir);
    }

    void translate(vec3 _position) {
        matrix = matrix * TranslateMatrix(_position);
    }

    void upload() {
        setUniform();
    }

    mat4 getMatrix() {
        return matrix;
    }
};

class Cylinder : public Shape
{
    float r1, r2;

    mat4 calculateScaleMatrix() final {
        return ScaleMatrix({r1, r2, 1});
    }

public:
    Cylinder() : Shape(
            {0, {0.1, 0.05, 0.03}, {0.1, 0.1, 0.1}, 100}
    ) {
        r1 = 0;
        r2 = 0;
    }

    void set(vec3 _position, float _r1, float _r2) {
        position = _position;
        r1 = _r1;
        r2 = _r2;
        calculateMatrix();
    }

    void setUniform() final {
        gpuProgram.setUniform(matrix, "cylinder.matrix");
        mat.setUniform();
    }
} cylinder;

class Hyperboloid : public Shape
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

    void set(vec3 _position, float _a, float _b, float _c) {
        position = _position;
        a = _a;
        b = _b;
        c = _c;
        calculateMatrix();
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
        float distance;
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
        cps.emplace_back(cp.y, cp.x);
    }

public:

    void create() {
        AddControlPoint({-1, -6});
        AddControlPoint({0, 17});
        AddControlPoint({1, -6});

        lights.reserve(100);

        int j = 0;
        float average = 0;

        for (int i = 1; i < 200; ++++i) {
            vec2 light = r((float) i / 200);
            vec2 lightBefore = r((float) (i-1) / 200);
            vec2 lightAfter = r((float) (i+1) / 200);
            float distance = length(light-lightBefore) + length(lightAfter-light);
            average = (average + distance) / (float)++j;

            float angle = light.x * (float)M_PI / 4;
            vec4 light4d = {-cosf(angle), sinf(angle), light.y, 1};
            light4d = light4d * cylinder.getMatrix();
            vec3 light3d = {light4d.x, light4d.y, light4d.z};
            lights.push_back({light3d, 1.0, distance});
        }

        for (auto & light : lights) {
            light.intensify = light.distance / average;
        }

        lights.shrink_to_fit();

        whiteLight = {15, 0, 0};
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

    float fov = 45 * M_PI / 180;
    camera.set({30, 0, 0}, {2, 0, 0}, {0, 1, 0}, fov);
    camera.setUniform();

    cylinder.set(
            {1.8125, 0, 0}, 1, 1
    );
    hyperboloid.rotateBeforeSet({1,0,0}, 0.5);
    hyperboloid.set(
            {-2.8125, -0.5, -0.5}, 1.1, 1.1, 1.1
    );

    vec3 rotation = {1,0,0};
    cylinder.rotate(rotation, 0.25);
    hyperboloid.rotate(rotation, 0.25);
    vec3 rotation2 = {0,1,0};
    cylinder.rotate(rotation2, 0.08);
    hyperboloid.rotate(rotation2, 0.08);
    vec3 rotation3 = {0,0,1};
    cylinder.rotate(rotation3, 0.02);
    hyperboloid.rotate(rotation3, 0.02);

    vec3 position = {3, 0, -0.1};
    cylinder.translate(position);
    hyperboloid.translate(position);

    cylinder.upload();
    hyperboloid.upload();

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
    glutPostRedisplay();
}
