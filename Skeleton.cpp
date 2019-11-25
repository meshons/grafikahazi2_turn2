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

class Camera {
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
} camera;

class Shape {
protected:
    mat4 matrix;

    vec3 position;
    vec3 direction;

    mat4 calculateTranslationMatrix() {
        return {
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 1, 0,
                position.x, position.y, position.z, 1
        };
    }
    mat4 calculateRotationMatrix() {
        vec3 direction2; // meroleges
        vec3 direction3; // meroleges
        return {
                direction2.x, direction2.y, direction2.z, 0,
                direction3.x, direction3.y, direction3.z, 0,
                direction.x, direction.y, direction.z, 0,
                0, 0, 0, 1
        };
    }
    virtual mat4 calculateScaleMatrix() = 0;
    void calculateMatrix() {
        matrix = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        };
        matrix = matrix * calculateTranslationMatrix();
        matrix = matrix * calculateRotationMatrix();
        matrix = matrix * calculateScaleMatrix();
    }

public:
    virtual void setUniform() = 0;
    void recalculate() {
        calculateMatrix();
        setUniform();
    }
};

class Cylinder : Shape {
    float r1, r2;

    mat4 calculateScaleMatrix() final {
        return {
                r1, 0, 0, 0,
                0, r2, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        };
    }

public:
    void set(vec3 _position, vec3 _direction, float _r1, float _r2) {
        position = _position;
        direction = normalize(_direction);
        r1 = _r1;
        r2 = _r2;
        calculateMatrix();
        setUniform();
    }
    void setUniform() final {
        gpuProgram.setUniform(matrix, "cylinder.matrix");
    }
} cylinder;

class Hyperboloid : Shape {
    float a, b, c;

    mat4 calculateScaleMatrix() final {
        return {
                a, 0, 0, 0,
                0, b, 0, 0,
                0, 0, c, 0,
                0, 0, 0, 1
        };
    }

public:
    void set(vec3 _position, vec3 _direction, float _a, float _b, float _c) {
        position = _position;
        direction = normalize(_direction);
        a = _a;
        b = _b;
        c = _c;
    }
    void setUniform() final {
        gpuProgram.setUniform(matrix, "hyperboloid.matrix");
    }
} hyperboloid;

class FullScreenTexturedQuad {
    unsigned int vao;
public:
    void Create() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        unsigned int vbo;
        glGenBuffers(1, &vbo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };
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

    fullScreenTexturedQuad.Create();

    float fov = 45 * M_PI / 180;
    camera.set({0, 0, 10}, {0, 0, 0}, {0, 1, 0}, fov);
    camera.setUniform();

    std::ifstream t("vertexShader.glsl");
    std::string vertexShader;

    t.seekg(0, std::ios::end);
    vertexShader.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    vertexShader.assign((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());

    std::ifstream t2("vertexShader.glsl");
    std::string fragmentShader;

    t2.seekg(0, std::ios::end);
    fragmentShader.reserve(t2.tellg());
    t2.seekg(0, std::ios::beg);

    fragmentShader.assign((std::istreambuf_iterator<char>(t2)),
                        std::istreambuf_iterator<char>());

    gpuProgram.create(vertexShader.c_str(), fragmentShader.c_str(), "outColor");
    // TODO change in prod
    //gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    fullScreenTexturedQuad.Draw();

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
    float cX = 2.0f * pX / windowWidth - 1;
    float cY = 1.0f - 2.0f * pY / windowHeight;
}

void onIdle() {
    glutPostRedisplay();
}
