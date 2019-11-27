#version 330
precision highp float;

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

const int nMaxLights = 100;

uniform Light526 lights[nMaxLights];
uniform WhiteLight whiteLight;

uniform Material materials[2];

Hit intersectCylinder(const Ray ray) {
    Hit hit;
    hit.t = -1;
    hit.hyperboloid = false;

    mat4 inverseMatrix = inverse(cylinder.matrix);

    Ray transformedRay = Ray(
        vec3(vec4(ray.start, 1) * inverseMatrix),
        vec3(vec4(ray.dir, 1) * inverseMatrix)
    );

    float a = transformedRay.dir.x * transformedRay.dir.x
                + transformedRay.dir.y * transformedRay.dir.y;
    float b = 2 * (transformedRay.start.x * transformedRay.dir.x
                + transformedRay.start.y * transformedRay.dir.y);
    float c = transformedRay.start.x * transformedRay.start.x
                + transformedRay.start.y * transformedRay.start.y - 1;

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
        0
    ));
    float lengthOfRay = distance(hit.position, transformedRay.start);
    hit.position = vec3(vec4(hit.position, 1) * cylinder.matrix);
    hit.normal = normalize(vec3(vec4(hit.normal, 1) * cylinder.matrix));
    float lengthOfRay2 = distance(hit.position, ray.start);
    hit.t = hit.t / lengthOfRay * lengthOfRay2;

    return hit;
}

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

    Hit hit2 = intersectCylinder(ray);
    if (hit2.t >= 0 && !find) {
        bestHit = hit2;
        find = true;
    } else if (hit2.t >= 0 && find && hit2.t < bestHit.t)
        bestHit = hit2;

    if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
    return bestHit;
}

const float epsilon = 0.0001f;

bool shadowIntersect(Ray ray, int mat) {
    if (mat == 1 && intersectCylinder(ray).t > 0 + epsilon * 5) return true;
    if (mat == 0 && intersectHyperboloid(ray).t > 0 + epsilon * 5) return true;
    return false;
}

vec3 Fresnel(vec3 F0, float cosTheta) {
    return F0 + (vec3(1, 1, 1) - F0) * pow(cosTheta, 5);
}

vec3 trace(Ray ray) {
    vec3 weight = vec3(0.3, 0.3, 0.3);
    vec3 outRadiance = vec3(0, 0, 0);

    Hit hit = firstIntersect(ray);
    if (hit.t < 0) return outRadiance;
    int mat = hit.hyperboloid ? 1 : 0;
    outRadiance += weight * materials[mat].ka;
    Ray shadowRay;
    shadowRay.start = hit.position + hit.normal * epsilon;
    vec3 lightDirection = normalize(whiteLight.position - hit.position);
    shadowRay.dir = lightDirection;
    float cosTheta = dot(hit.normal, lightDirection);
    if (cosTheta > 0 && !shadowIntersect(shadowRay, mat)) {
        outRadiance += weight * materials[mat].kd * cosTheta;
        vec3 halfway = normalize(-ray.dir + lightDirection);
        float cosDelta = dot(hit.normal, halfway);
        if (cosDelta > 0) outRadiance += weight * materials[mat].ks * pow(cosDelta, materials[mat].shininess);
    }

    if (hit.hyperboloid) {
        for (int i = 0; i < nMaxLights; i++) {

        }
    }

    return outRadiance;
}

void main() {
    Ray ray;
    ray.start = wEye;
    ray.dir = normalize(p - wEye);
    fragmentColor = vec4(trace(ray), 1);
}
