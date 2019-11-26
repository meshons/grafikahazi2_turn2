#version 330
precision highp float;

in  vec3 p;
out vec4 fragmentColor;

struct Hit {
    float t;
    vec4 position, normal;
    bool hyperboloid;
};

struct Ray {
    vec4 start, dir;
};

struct Cylinder {
    mat4 matrix;
};

struct Hyperboloid {
    mat4 matrix;
};

struct Light526 {
    vec3 position;
    float intensify;
};

struct WhiteLight {
    vec3 position;
};

uniform vec3 wEye;

uniform Cylinder cylinder;
uniform Hyperboloid hyperboloid;

uniform Light526[100] lights;
uniform WhiteLight whiteLight;

Hit intersectCylinder(const Ray ray) {
    Hit hit;
    hit.t = -1;
    hit.hyperboloid = false;

    mat4 inverseMatrix = inverse(cylinder.matrix);

    Ray transformedRay = Ray(ray.start * cylinder.matrix, ray.dir * cylinder.matrix);

    float a = transformedRay.dir.x * transformedRay.dir.x + transformedRay.dir.y * transformedRay.dir.y;
    float b = 2 * (transformedRay.start.x * transformedRay.dir.x + transformedRay.start.y * transformedRay.dir.y);
    float c = transformedRay.start.x * transformedRay.start.x + transformedRay.start.y * transformedRay.start.y - 1;

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
    hit.normal = vec4(hit.position.x, hit.position.y, 0, 1);

    float lengthOfRay = distance(hit.position, transformedRay.start);
    hit.position = hit.position * inverseMatrix;
    hit.normal = hit.normal * inverseMatrix;
    float lengthOfRay2 = distance(hit.position, ray.start);
    hit.t = hit.t / lengthOfRay * lengthOfRay2;

    return hit;
}

Hit intersectHyperboloid(const Ray ray) {
    Hit hit;
    hit.t = -1;
    hit.hyperboloid = true;

    mat4 inverseMatrix = inverse(hyperboloid.matrix);

    Ray transformedRay = Ray(ray.start * hyperboloid.matrix, ray.dir * hyperboloid.matrix);

    float a = transformedRay.dir.x * transformedRay.dir.x
                + transformedRay.dir.y * transformedRay.dir.y
                - transformedRay.dir.z * transformedRay.dir.z;
    float b = 2 * (
                transformedRay.start.x * transformedRay.dir.x
                + transformedRay.start.y * transformedRay.dir.y
                - transformedRay.start.z * transformedRay.dir.z
    );
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

    hit.normal = vec4(
        2 * hit.position.x + hit.position.y * hit.position.y - hit.position.z * hit.position.z,
        hit.position.x * hit.position.x + 2 * hit.position.y - hit.position.z * hit.position.z,
        hit.position.x * hit.position.x + hit.position.y * hit.position.y - 2 * hit.position.z,
        1
    );

    float lengthOfRay = distance(hit.position, transformedRay.start);
    hit.position = hit.position * inverseMatrix;
    hit.normal = hit.normal * inverseMatrix;
    float lengthOfRay2 = distance(hit.position, ray.start);
    hit.t = hit.t / lengthOfRay * lengthOfRay2;

    return hit;
}

Hit firstIntersect(Ray ray) {
    Hit bestHit;
    bestHit.t = -1;
    bool find = false;

    Hit hit = intersectCylinder(ray);
    if (hit.t >= 0 && !find){
        bestHit = hit;
        find = true;
    } else if (hit.t >= 0 && find && hit.t < bestHit.t)
        bestHit = hit;

    hit = intersectHyperboloid(ray);
    if (hit.t >= 0 && !find) {
        bestHit = hit;
        find = true;
    } else if (hit.t >= 0 && find && hit.t < bestHit.t)
        bestHit = hit;

    if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
    return bestHit;
}

bool shadowIntersect(Ray ray) {
    if (intersectCylinder(ray).t > 0) return true;
    if (intersectHyperboloid(ray).t > 0) return true;
    return false;
}

vec3 Fresnel(vec3 F0, float cosTheta) {
    return F0 + (vec3(1, 1, 1) - F0) * pow(cosTheta, 5);
}

const float epsilon = 0.0001f;
const int maxdepth = 8;

vec3 trace(Ray ray) {
    vec3 weight = vec3(1, 1, 1);
    vec3 outRadiance = vec3(0, 0, 0);
    for(int d = 0; d < maxdepth; d++) {
        Hit hit = firstIntersect(ray);
        if (hit.t < 0)
            return vec3(1, 0, 0);
        else if (hit.hyperboloid)
            return vec3(0, 0, 1);
        else
            return vec3(0, 1, 0);
    }
    return outRadiance;
}

void main() {
    Ray ray;
    ray.start = vec4(wEye, 1);
    ray.dir = vec4(normalize(p - wEye), 1);
    fragmentColor = vec4(trace(ray), 1);
}
