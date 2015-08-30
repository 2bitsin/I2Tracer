#ifndef __PARSER_H__
#define __PARSER_H__

#include "Common.h"
#include <vector>
#include <string>

namespace i2t {

    struct SceneData {

        struct Material {
            vec3 emission;
            vec3 diffuse;
            vec3 specular;
            double power;
        };

        struct Triangle {
            Material material;
            dvec4 v0;
            dvec4 v1;
            dvec4 v2;
        };

        struct Sphere {
            Material material;            
            dmat4 inverseT;
            dmat4 T;
        };

        struct Light {
            dvec4 position;
            vec3 color;
            vec3 attenuation;
        };

        struct Camera {
            uvec2 size;
            dvec4 up;
            dvec4 eye;
            dvec4 center;
            float fov;
        };

        friend bool parse (SceneData& out, const std::string& name);

        auto&& camera    () const { return $camera; }
        auto&& ambient   () const { return $ambient; }
        auto&& lights    () const { return $lights; }
        auto&& triangles () const { return $triangles; }
        auto&& spheres   () const { return $spheres; }   
        auto&& bounces   () const { return $bounces; }
        auto&& output    () const { return $output ; }
    private:
        unsigned                $bounces = 5u;
        std::string             $output  = "default"; 

        Camera $camera = {
            uvec2 (640u, 480u),
            dvec4 (+0.0, +1.0, +0.0, +0.0),
            dvec4 (+0.0, +0.0, +0.0, +1.0),
            dvec4 (+0.0, +0.0, -1.0, +1.0),
            45.0f
        };

        vec3                    $ambient = vec3 (0.0);
        std::vector<Light>      $lights;
        std::vector<Triangle>   $triangles;
        std::vector<Sphere>     $spheres;      
    
    };

    

}

#endif
