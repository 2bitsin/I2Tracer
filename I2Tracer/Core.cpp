#include "Core.h"
#include <omp.h>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace i2t;
static const double M_PI = 3.14159265359;


i2t::Core::Core (const SceneData& s):
    scene (s),
    g_width (s.camera ().size.x),
    g_height (s.camera ().size.y),
    g_samples (std::make_unique<vec3 []>(g_width*g_height))
{}

bool i2t::Core::sphere_intersect (
    const dvec3& gRo, const dvec3& gRd,
    const dmat4& invT, const dmat4& T, 
    Core::Incident& ii) 
{
    auto Ro = dvec3 ((invT*dvec4 (gRo, 1.0)).xyz);
    auto Rd = dvec3 ((invT*dvec4 (gRd, 0.0)).xyz);
    double t;
    if (canonical_sphere_intersect (Ro, Rd, t)) {
        auto p = dvec4 (Ro+Rd*t, 1.0);
        auto n = normalize (p - dvec4 (0.0, 0.0, 0.0, 1.0));
        n = (transpose (invT)*n);
        ii.normal = dvec4 (normalize (dvec3 (n.xyz)),1.0);        
        ii.point = T*p;
        ii.t = t;
        return true;
    }
    return false;
}

bool i2t::Core::simple_sphere_intersect (
    const dvec3& gRo, const dvec3& gRd,
    const dmat4& invT, const dmat4& T,
    double& tout) 
{
    auto Ro = dvec3 ((invT*dvec4 (gRo, 1.0)).xyz);
    auto Rd = dvec3 ((invT*dvec4 (gRd, 0.0)).xyz);    
    return canonical_sphere_intersect (Ro, Rd, tout);
}

bool i2t::Core::canonical_sphere_intersect (
    const dvec3& Ro, const dvec3& Rd, double& tout) 
{
    auto a = dot (Rd, Rd);
    auto b = dot (Rd, Ro)*2.0;
    auto c = dot (Ro, Ro) - 1.0;
    auto disc = b*b - 4.0*a*c;
    if (disc < 0.0)
        return false;
    auto q = 0.5*(std::copysign (std::sqrt (disc), b) - b);
    auto t0 = q/a;
    auto t1 = c/q;
    if (t0 > t1)
        std::swap (t0, t1);
    if (t1 < 0.0)
        return false;
    tout = t0 < 0.0 ? t1 : t0;
    return true;
}


bool i2t::Core::polygon_intersect (
    const dvec3& Ro, const dvec3& Rd,
    const dvec3& v0, const dvec3& v1,
    const dvec3& v2, Core::Incident& ii) 
{
    double t;    
    if (canonical_polygon_intersect (Ro, Rd, v0, v1, v2, t)) {
        auto e1 = v1 - v0;
        auto e2 = v2 - v0;
        ii.normal = dvec4 (normalize (cross (e1, e2)), 0.0);
        ii.point = dvec4 (Ro+Rd*t, 1.0);
        ii.t = t;
        return true;
    }
    return false;
}

bool i2t::Core::canonical_polygon_intersect (
    const dvec3& Ro, const dvec3& Rd, 
    const dvec3& v0, const dvec3& v1, 
    const dvec3& v2, double& tout)  
{
    auto e1 = v1 - v0;
    auto e2 = v2 - v0;    
    auto normal = normalize (cross (e1, e2));
    auto b = dot (normal, Rd);
    auto w0 = Ro - v0;
    auto a = -dot (normal, w0);
    auto t = a / b;
    auto p = Ro + t * Rd;
    auto uu = dot (e1, e1);
    auto uv = dot (e1, e2);
    auto vv = dot (e2, e2);
    auto w = p - v0;
    auto wu = dot (w, e1);
    auto wv = dot (w, e2);
    auto inverseD = uv * uv - uu * vv;
    inverseD = 1.0f / inverseD;
    auto u = (uv * wv - vv * wu) * inverseD;
    if (u < 0.0f || u > 1.0f)
        return false;
    auto v = (uv * wu - uu * wv) * inverseD;
    if (v < 0.0f || (u + v) > 1.0f)
        return false;
    //auto UV = dvec2 (u, v);
    tout = t;
    return true;
}

bool i2t::Core::intersect (const dvec3& Ro, const dvec3& Rd, Incident& in) {    
    auto mint = 1e9;
    auto hit = false;
    Incident ti;

    for (const auto& obj: scene.triangles ()) {        
        if (!polygon_intersect (Ro, Rd, obj.v0.xyz, obj.v1.xyz, obj.v2.xyz, ti))
            continue;
        if (ti.t < mint) {
            mint = ti.t;
            ti.material = obj.material;
            in = ti;
            hit = true;
        }
    }

    for (const auto& obj: scene.spheres ()) {
        Incident ti;
        if (!sphere_intersect (Ro, Rd, obj.inverseT, obj.T, ti))
            continue;
        if (ti.t < mint) {
            mint = ti.t;
            ti.material = obj.material;
            in = ti;
            hit = true;
        }
    }

    return hit;
}

bool i2t::Core::intersect (const dvec3& Ro, const dvec3& Rd) {
    double t;
    for (const auto& obj: scene.triangles ()) {        
        if (canonical_polygon_intersect (Ro, Rd, obj.v0.xyz, obj.v1.xyz, obj.v2.xyz, t))
            if (t > EPSILON) return true;
    }
    for (const auto& obj: scene.spheres ()) {
        if (simple_sphere_intersect (Ro, Rd, obj.inverseT, obj.T, t))
            if (t > EPSILON) return true;
    }
    return false;
}

void i2t::Core::store_sample (unsigned x, unsigned y, vec3 sample) {
    g_samples [x + y*scene.camera ().size.x] = sample;
}

vec3 i2t::Core::render_sample (const dvec4& Ro, const dvec4& Rd, unsigned bounces) {
    if (bounces <= 0)
        return vec3 (1.0);
    Incident ti;
    if (!intersect (Ro.xyz, Rd.xyz, ti))
        return vec3 (0.0);
    
    
    const auto& A = scene.ambient ();
    const auto& E = ti.material.emission;
    const auto& D = ti.material.diffuse;
    const auto& S = ti.material.specular;
    const auto& s = ti.material.power;
    const auto& N = ti.normal;
    auto ED = scene.camera ().eye-ti.point;
    auto I = A + E;
    
    for (const auto& light: scene.lights ()) {        
        auto is_directional = std::abs (light.position.w) < EPSILON;
        auto Li = light.color;
        auto L = is_directional ? light.position : 
            normalize (light.position - ti.point);
        auto r = float (is_directional ? 0.0f : 
            length (light.position - ti.point));
        auto H = normalize (L + ED) ;
        auto V = intersect (ti.point.xyz, dvec3 (L.xyz)) ? 0.0f : 1.0f;
        const auto c = dot (light.attenuation, vec3 (1.0f, r, r*r));
        auto spec = S*float (std::pow (std::max (dot (N, H), 0.0), s));
        auto diff = D*float (std::max (dot (N, L), 0.0));
        I += (V*Li/c)*(diff + spec);        
    }
    auto rRd = dvec4 (normalize (reflect (dvec3 (Rd.xyz), dvec3 (N.xyz))), 0.0);
    auto rRo = ti.point;
    return I + S*render_sample (rRo, rRd, bounces-1);
}

void i2t::Core::render () {    

    auto width  = int (scene.camera ().size.x);
    auto height = int (scene.camera ().size.y);
    auto fov    = 0.5*radians (scene.camera ().fov); 
    auto aspect = (1.0*width)/height;

    auto halfw  = 0.5*width;
    auto halfh  = 0.5*height;

    auto tanfx  = (std::tan (fov)/halfw)*aspect;
    auto tanfy  = (std::tan (fov)/halfh);

    auto ro = scene.camera ().eye;

    auto w = normalize (dvec3 (ro - scene.camera ().center));
    auto u = normalize (cross (dvec3 (scene.camera ().up), w));
    auto v = normalize (cross (w, u));

    #pragma omp parallel for
    for (auto y = 0; y < height; ++y)         
    for (auto x = 0; x < width;  ++x) {
        auto alfa = +tanfx*(x + 0.5 - halfw);
        auto beta = -tanfy*(y + 0.5 - halfh);
        auto rd = dvec4 (normalize (alfa*u + beta*v - w), 0.0);        
        store_sample (x, y, render_sample (ro, rd, scene.bounces ()));
    }
}

i2t::Core& i2t::Core::snapshot (std::uint32_t type, void* buff, std::uint32_t w, std::uint32_t h) {
    if (type != RGBA32)
        return *this;

    auto buffer = reinterpret_cast<std::uint32_t*> (buff);

    static auto const rgb32 = [] (vec3 v) {
        auto bv = u8vec3 (glm::clamp (v, vec3 (0.0), vec3 (1.0))*255.0f);
        return (bv [0] << 16)| (bv [1] << 8) | bv [2];
    };

    for (auto y = 0u; y < g_height; ++y) 
    for (auto x = 0u; x < g_width; ++x) {
        if (x > w || y > h) continue;
        buffer [x + y*w] = rgb32 (g_samples [x + y*g_width]);
    }
    
    return *this; 
}


