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
    mat4 inverseMatrix;
};

struct Hyperboloid {
    mat4 matrix;
    mat4 inverseMatrix;
};

uniform vec3 wEye;

uniform Cylinder cylinder;
uniform Hyperboloid hyperboloid;

Hit intersectCylinder(const Ray ray) {
    Hit hit;
    hit.t = -1;
    hit.hyperboloid = false;

    Ray transformedRay = (ray.start * cylinder.inverseMatrix, ray.dir * cylinder.inverseMatrix);

    // https://github.com/spinatelli/raytracer/blob/master/Cylinder.cpp
    float a = transformedRay.dir.x * transformedRay.dir.x + transformedRay.dir.z * transformedRay.dir.z;
    float b = transformedRay.dir.x + transformedRay.dir.z;
    float c = -1;

    float delta = b * b - a * c;
    if (delta < 0)
        return hit;

    float t1 = (-b + sqrt(delta)) / a;
    float t2 = (-b - sqrt(delta)) / a;

    if (t1 < 0 && t2 < 0)
        return hit;
    if (t1 > 0 && t2 < 0)
        hit.t = t1;
    else if (t1 < 0 && t2 > 0)
        hit.t = t2;
    else
        hit.t = (t2 < t1) ? t2 : t1;

    hit.position = transformedRay.start + transformedRay.dir * hit.t;
    hit.normal = (hit.position.x, hit.position.y, 0);

    float lengthOfRay = distance(hit.position, transformedRay.start);
    hit.position = hit.position * cylinder.matrix;
    hit.normal = hit.normal * cylinder.matrix;
    float lengthOfRay2 = distance(hit.position, ray.start);
    hit.t = hit.t / lengthOfRay * lengthOfRay2;

    return hit;
}

Hit intersectHyperboloid(const Ray ray) {
    Hit hit;
    hit.t = -1;
    hit.hyperboloid = true;

    Ray transformedRay = (ray.start * hyperboloid.inverseMatrix, ray.dir * hyperboloid.inverseMatrix);
    float x0 = transformedRay.start.x, y0 = transformedRay.start.y, z0 = transformedRay.start.z;
    float normalizeToA = 1 / transformedRay.dir.x;
    float k = transformedRay.dir.y * normalizeToA;
    float l = transformedRay.dir.z * normalizeToA;

    vec4 p1, p2;

    // https://johannesbuchner.github.io/intersection/intersection_line_hyperboloid.html
    float sqrtx1 = -pow(l, 2) + pow(k, 2) + pow(k, 2)*pow(z0, 2) - 2*k*l*y0*z0 + pow(l, 2)*pow(y0, 2) + 1
    + pow(l, 2)*pow(x0, 2) - 2*l*x0*z0 + pow(z0, 2) - pow(k, 2)*pow(x0, 2) + 2*k*x0*y0 - pow(y0, 2);
    if (sqrtx1 >=  0) {
        p1.x = x0 + (l*z0 - k*y0 - x0 + sqrt(sqrtx1))/(-pow(l, 2) + pow(k, 2) + 1);
        p1.y = k * (l*z0 - k*y0 - x0 + sqrt(sqrtx1))/(-pow(l, 2) + pow(k, 2) + 1) + y0;
        p1.z = l * (l*z0 - k*y0 - x0 + sqrt(sqrtx1))/(-pow(l, 2) + pow(k, 2) + 1) + z0;

        p2.x = x0 + (-l*z0 + k*y0 + x0 + sqrt(sqrtx1))/(pow(l, 2) - pow(k, 2) - 1);
        p2.y = k*(-l*z0 + k*y0 + x0 + sqrt(sqrtx1))/(pow(l, 2) - pow(k, 2) - 1) + y0;
        p2.z = l*(-l*z0 + k*y0 + x0 + sqrt(sqrtx1))/(pow(l, 2) - pow(k, 2) - 1) + z0;
    } else
        return hit;

    hit.position = distance(p1, transformedRay.start) < distance(p2, transformedRay.start) ? p1 : p2;
    hit.t = distance(hit.position, transformedRay.start);

    hit.normal = (
        2 * hit.position.x + hit.position.y * hit.position.y - hit.position.z * hit.position.z,
        hit.position.x * hit.position.x + 2 * hit.position.y - hit.position.z * hit.position.z,
        hit.position.x * hit.position.x + hit.position.y * hit.position.y - 2 * hit.position.z
    );

    float lengthOfRay = distance(hit.position, transformedRay.start);
    hit.position = hit.position * hyperboloid.matrix;
    hit.normal = hit.normal * hyperboloid.matrix;
    float lengthOfRay2 = distance(hit.position, ray.start);
    hit.t = hit.t / lengthOfRay * lengthOfRay2;

    return hit;
}

Hit firstIntersect(Ray ray) {
    Hit bestHit;
    bestHit.t = -1;

    Hit hit = intersectHyperboloid(ray);
    if (hit.t > bestHit.t)
        bestHit = hit;
    hit = intersectCylinder(ray);
    if (hit.t > bestHit.t)
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
            return (1, 0, 0);
        else if (hit.hyperboloid)
            return (0, 0, 1);
        else
            return (0, 1, 0);
    }
    return outRadiance;
}

void main() {
    cylinder.inverseMatrix = inverse(cylinder.matrix);
    hyperboloid.inverseMatrix = inverse(hyperboloid.matrix);
    Ray ray;
    ray.start = (wEye, 0);
    ray.dir = (normalize(p - wEye), 0);
    fragmentColor = vec4(trace(ray), 1);
}
