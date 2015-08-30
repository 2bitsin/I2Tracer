#ifndef __I2CORE_H__
#define __I2CORE_H__

#include "Parser.h"
#include "Common.h"
#include <memory>

namespace i2t {

    struct Core {
        struct Incident {
            double t;
            dvec4 point;
            dvec4 normal;
            SceneData::Material material;
        };

        static const std::uint32_t RGBA32 = 0;

        Core (const SceneData& scene);

        bool sphere_intersect (
            const dvec3& gRo, const dvec3& gRd, 
            const dmat4& invT, const dmat4& T, 
            Core::Incident& ii);

        bool simple_sphere_intersect (const dvec3 & gRo, const dvec3 & gRd, const dmat4 & invT, const dmat4 & T, double & tout);

        bool canonical_sphere_intersect (
            const dvec3& Ro, const dvec3& Rd, 
            double& tout);

        bool polygon_intersect (
            const dvec3& Ro, const dvec3& Rd,
            const dvec3& v0, const dvec3& v1,
            const dvec3& v2, Core::Incident& ii);

        bool canonical_polygon_intersect (
            const dvec3& Ro, const dvec3& Rd, 
            const dvec3& v0, const dvec3& v1, 
            const dvec3& v2, double& tout);
        

        bool intersect (const dvec3& ro, const dvec3& rd, Incident& in);
        bool intersect (const dvec3& ro, const dvec3& rd);
        void store_sample (unsigned x, unsigned y, vec3 sample);
        vec3 render_sample (const dvec4& ro, const dvec4& rd, int bounced);

        void render ();
        Core& snapshot (std::uint32_t, void*, std::uint32_t, std::uint32_t);

    private:
        std::size_t g_width, g_height;
        std::unique_ptr<vec3 []> g_samples;
        SceneData scene;

        int global_x, global_y;
        
        const double EPSILON = 1e-3;

    };
}

#endif
