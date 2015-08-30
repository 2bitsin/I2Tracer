#include "Parser.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <utility>
#include <functional>
#include <unordered_map>
#include <stack>
#include <sstream>

template <typename _Ttype> 
inline _Ttype read (std::istream& in) {
    _Ttype t;
    in >> t;
    return t;
}

bool i2t::parse (SceneData& out, const std::string& name) {
    static const auto stack_empty_error = std::runtime_error ("Transformation stack empty");

    using namespace i2t;
    typedef std::function<void (std::istream& args)> command_type;
    
    SceneData scene;
    SceneData::Material material;
    std::vector<dvec4> vertexes;
    std::vector<std::pair<dvec4, dvec4>> vertexes_with_normals;

    vec3 attenuation = vec3 (1.0, 0.0, 0.0);
    dmat4 T = dmat4 (1.0);
    std::stack<dmat4> Tstack;

    static std::unordered_map<std::string, command_type> command_list = {
        /* Configuration */
        {"size", [&] (std::istream& in) {
            scene.$camera.size.x = read<unsigned> (in); 
            scene.$camera.size.y = read<unsigned> (in);
        }},
        {"camera", [&] (std::istream& in) {
            scene.$camera.eye.x = read<double> (in) ; 
            scene.$camera.eye.y = read<double> (in) ;                 
            scene.$camera.eye.z = read<double> (in) ;
            scene.$camera.center.x = read<double> (in) ; 
            scene.$camera.center.y = read<double> (in) ;                 
            scene.$camera.center.z = read<double> (in) ;
            scene.$camera.up.x = read<double> (in) ;
            scene.$camera.up.y = read<double> (in) ;
            scene.$camera.up.z = read<double> (in) ;
            scene.$camera.fov = read<float> (in) ;
        }},
        {"maxdepth", [&] (std::istream& in) {
            scene.$bounces = read<unsigned> (in);
        }},
        {"output", [&] (std::istream& in) {
            scene.$output = read<std::string> (in);
        }},

        /* Geometry */
        {"maxverts", [&] (std::istream& in) {
            vertexes.reserve (read<std::size_t> (in));
        }},
        {"maxvertnorms", [&] (std::istream& in) {
            vertexes_with_normals.reserve (read<std::size_t> (in));
        }},
        {"vertex", [&] (std::istream& in) {
            auto x = read<double> (in);
            auto y = read<double> (in);
            auto z = read<double> (in);
            vertexes.emplace_back (x, y, z, 1.0);
        }},
        {"vertexnormal", [&] (std::istream& in) {
            auto vx = read<double> (in);
            auto vy = read<double> (in);
            auto vz = read<double> (in);
            auto nx = read<double> (in);
            auto ny = read<double> (in);
            auto nz = read<double> (in);
            vertexes_with_normals.emplace_back (
                dvec4 (vx, vy, vz, 1.0),
                dvec4 (nx, ny, nz, 0.0));

        }},
        {"tri", [&] (std::istream& in) {
            auto a = read<unsigned> (in);
            auto b = read<unsigned> (in);
            auto c = read<unsigned> (in);
            scene.$triangles.push_back ({
                material,
                T*vertexes.at (a),
                T*vertexes.at (b),
                T*vertexes.at (c)
            });
        }},
        {"trinormal", [&] (std::istream& in) {
            unsigned a, b, c;
            in >> a >> b >> c;
            __debugbreak ();
        }},
        {"sphere", [&] (std::istream& in) {
            auto x = read<double> (in); 
            auto y = read<double> (in); 
            auto z = read<double> (in);
            auto w = read<double> (in);
            auto R = translate  (dmat4 (1.0), dvec3 (x, y, z));
            auto S = scale      (dmat4 (1.0), dvec3 (w));
            auto M = T*R*S;
            scene.$spheres.push_back ({material, inverse (M), M});
        }},

            /* T    ransformations */
        {"translate", [&] (std::istream& in) {
            auto x = read<double> (in);
            auto y = read<double> (in);
            auto z = read<double> (in);
            T = translate (T, dvec3 (x, y, z));
        }},
        {"rotate", [&] (std::istream& in) {
            auto x = read<double> (in);
            auto y = read<double> (in);
            auto z = read<double> (in);
            auto a = read<double> (in);                        
            T = rotate (T, radians (a), dvec3 (x, y, z));
        }},
        {"scale", [&] (std::istream& in) {
            auto x = read<double> (in);
            auto y = read<double> (in); 
            auto z = read<double> (in);
            T = scale (T, dvec3 (x, y, z));
        }},
        {"pushTransform", [&] (std::istream& in) {
            Tstack.push (T);
        }},
        {"popTransform", [&] (std::istream& in) {
            if (Tstack.empty ())
                throw stack_empty_error;
            T = Tstack.top ();
            Tstack.pop ();
        }},

            /* L    ights */
        {"directional", [&] (std::istream& in) {
            auto x = read<double> (in);
            auto y = read<double> (in);
            auto z = read<double> (in);
            auto r = read<float> (in);
            auto g = read<float> (in);
            auto b = read<float> (in);
            scene.$lights.push_back ({
                T*dvec4 (x,y,z,0.0), 
                vec3 (r,g,b),
                attenuation});
        }},
        {"point", [&] (std::istream& in) {
            auto x = read<double> (in);
            auto y = read<double> (in);
            auto z = read<double> (in);
            auto r = read<float> (in);
            auto g = read<float> (in);
            auto b = read<float> (in);
            scene.$lights.push_back ({
                T*dvec4 (x, y, z, 1.0),
                vec3 (r, g, b),
                attenuation});
        }},
        {"attenuation", [&] (std::istream& in) {          
            attenuation.x = read<float> (in);
            attenuation.y = read<float> (in);
            attenuation.z = read<float> (in);
        }},
        {"ambient", [&] (std::istream& in) {
            material.ambient.x = read<float> (in);
            material.ambient.y = read<float> (in);
            material.ambient.z = read<float> (in);
        }},

        /* Materials */
        {"emission", [&] (std::istream& in) {
            material.emission.r = read<float> (in);
            material.emission.g = read<float> (in);
            material.emission.b = read<float> (in);
        }},
        {"diffuse", [&] (std::istream& in) {
            material.diffuse.r = read<float> (in);
            material.diffuse.g = read<float> (in);
            material.diffuse.b = read<float> (in);            
        }},
        {"specular", [&] (std::istream& in) {
            material.specular.r = read<float> (in);
            material.specular.g = read<float> (in);
            material.specular.b = read<float> (in);            
        }},
        {"shininess", [&] (std::istream& in) {
            material.power = read<float> (in);
        }}
    };

    std::ifstream stream (name);

    if (!stream) throw std::runtime_error ("Couldn't open "+name);
    std::string line;
    while (std::getline (stream, line)) {
        std::istringstream ss (line);
        std::getline (ss, line, '#');
        auto split = std::find (
            line.begin (), line.end (), ' ');
        ss = std::istringstream (std::string (split, line.end ()));
        line = std::string (line.begin (), split);
        if (line.empty ())
            continue;
        auto cmd = command_list.find (line);
        if (cmd != command_list.end ()) {
            cmd->second (ss);
            continue;
        }
        auto rte = "Command not found: " + line;
        throw std::runtime_error (rte);
    }
    
    out = scene;

    return false;
}
