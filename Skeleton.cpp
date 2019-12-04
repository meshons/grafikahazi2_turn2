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

const char * const vertexSource = R"(
	#version 330
    precision highp float;

    uniform vec3 wLookAt, wRight, wUp;

    layout(location = 0) in vec2 cCamWindowVertex;
    out vec3 p;

    void main() {
        gl_Position = vec4(cCamWindowVertex, 0, 1);
        p = wLookAt + wRight * cCamWindowVertex.x + wUp * cCamWindowVertex.y;
    }
)";

const char * const fragmentSource = R"(
	#version 330
    precision highp float;

    #define M_PI 3.1415926535897932384626433832795

    in  vec3 p;
    out vec4 fragmentColor;

    struct Material {
        vec3 ka, kd, ks;
        float  shininess;
    };

    struct Hit {
        float t;
        vec3 position, normal;
        bool hyperboloid;
    };

    struct Ray {
        vec3 start, dir;
    };

    struct Hyperboloid {
        mat4 matrix;
    };

    const vec3 light526nm = vec3(0.305882352941, 1, 0);
    const float epsilon = 1;

    struct Light526 {
        // rgb(78,255, 0)
        // Hex: #4eff00
        vec3 position;
        float intensify;
    };

    struct WhiteLight {
        vec3 position;
    };

    uniform vec3 wEye;

    uniform Hyperboloid hyperboloid;

    const int nMaxLights = 100;

    uniform Light526 lights[nMaxLights];
    uniform WhiteLight whiteLight;

    uniform Material materials[2];

    Hit intersectHyperboloid(const Ray ray) {
        Hit hit;
        hit.t = -1;
        hit.hyperboloid = true;

        mat4 inverseMatrix = inverse(hyperboloid.matrix);

        Ray transformedRay = Ray(
            vec3(vec4(ray.start, 1) * inverseMatrix),
            vec3(vec4(ray.dir, 1) * inverseMatrix)
        );
        float a = transformedRay.dir.x * transformedRay.dir.x
                    + transformedRay.dir.y * transformedRay.dir.y
                    - transformedRay.dir.z * transformedRay.dir.z;
        float b = 2 * (
                    transformedRay.start.x * transformedRay.dir.x
                    + transformedRay.start.y * transformedRay.dir.y
                    - transformedRay.start.z * transformedRay.dir.z);
        float c = transformedRay.start.x * transformedRay.start.x
                    + transformedRay.start.y * transformedRay.start.y
                    - transformedRay.start.z * transformedRay.start.z
                    - 1;

        float delta = b * b - 4 * a * c;
        if (delta < 0)
        return hit;

        float t1 = (-b + sqrt(delta)) / 2 / a;
        float t2 = (-b - sqrt(delta)) / 2 / a;

        if (t1 < 0 && t2 < 0)
        return hit;
        if (t1 >= 0 && t2 < 0)
        hit.t = t1;
        else if (t1 < 0 && t2 >= 0)
        hit.t = t2;
        else
        hit.t = (t2 < t1) ? t2 : t1;

        hit.position = transformedRay.start + transformedRay.dir * hit.t;

        hit.normal = normalize(vec3(
            2 * hit.position.x,
            2 * hit.position.y,
            -2 * hit.position.z
        ));

        float lengthOfRay = distance(hit.position, transformedRay.start);
        hit.position = vec3(vec4(hit.position, 1) * hyperboloid.matrix);
        hit.normal = normalize(vec3(vec4(hit.normal, 1) * hyperboloid.matrix));
        float lengthOfRay2 = distance(hit.position, ray.start);
        hit.t = hit.t / lengthOfRay * lengthOfRay2;

        return hit;
    }

    Hit firstIntersect(Ray ray) {
        Hit bestHit;
        bestHit.t = -1;
        bool find = false;

        Hit hit = intersectHyperboloid(ray);
        if (hit.t >= 0 && !find){
            bestHit = hit;
            find = true;
        } else if (hit.t >= 0 && find && hit.t < bestHit.t)
            bestHit = hit;


        if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
        return bestHit;
    }

    const float conversion = 500;
    const float lambda = 526;
    const float c = 299792458;
    const float f = c / lambda;
    const float omega = f * 2 * M_PI;
    const float alpha = conversion * lambda * 2 * M_PI * pow(10, -9);
    const float k = 2 * M_PI / lambda;

    vec3 Fresnel(vec3 F0, float cosTheta) {
        return F0 + (vec3(1, 1, 1) - F0) * pow(cosTheta, 5);
    }

    vec3 trace(Ray ray) {
        vec3 weight = vec3(1, 1, 1);
        vec3 outRadiance = vec3(0, 0, 0);

        Hit hit = firstIntersect(ray);
        if (hit.t < 0) return outRadiance;
        int mat = hit.hyperboloid ? 1 : 0;
        outRadiance += weight * 1.2 * materials[mat].ka;
        vec3 lightDirection = normalize(whiteLight.position - hit.position);
        float cosTheta = dot(hit.normal, lightDirection);
        if (cosTheta > 0) {
            outRadiance += weight * materials[mat].kd * cosTheta;
            vec3 halfway = normalize(-ray.dir + lightDirection);
            float cosDelta = dot(hit.normal, halfway);
            if (cosDelta > 0) outRadiance += weight * materials[mat].ks * pow(cosDelta, materials[mat].shininess);
        }

        if (hit.hyperboloid) {
            float intensify = 0;
            for (int i = 0; i < nMaxLights; i++) {
                float d = distance(hit.position, lights[i].position) * conversion;
                intensify += sqrt(lights[i].intensify) * 15 / d *
                    cos(lambda * d * 2 * M_PI * pow(10, -18) - k * d + alpha);
            }
            outRadiance += intensify * intensify * light526nm;
        }

        return outRadiance;
    }

    void main() {
        Ray ray;
        ray.start = wEye;
        ray.dir = normalize(p - wEye);
        fragmentColor = vec4(trace(ray), 1);
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

    gpuProgram.create(vertexSource, fragmentSource, "outColor");

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
