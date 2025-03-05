///////////////////////////////////////////////////////////////////
// Gilbert–Johnson–Keerthi (GJK) distance algorithm
///////////////////////////////////////////////////////////////////

#pragma once

#include "mxlib.hpp"

#include <vector>

using namespace mxlib;

namespace s2cpp::gjk
{
    struct mesh_object
    {
        xfloat4x4
        _model_mtx{};

        xfloat3*   
        _vertices{};

        uint32_t
        _vertex_count{};
    };

    // for visualization
    struct by_products_data
    {   
        fixed_list<xfloat3, 4> 
        _simplex_points{};

        std::vector<fixed_list<mxlib::xfloat3, 4>>
        _simplex_construction_buffer{};
    };

    typedef enum result_bits : uint8_t {
        GJK_EMPTY_MASK                    = 0,    // 0000 0000
        
        // validation bits
        GJK_INVALID_BIT                   = 0x1,  // 0000 0001
        GJK_ERROR_SAME_OBJECT_BIT         = 0x2,  // 0000 0010
        GJK_ERROR_NULL_OBJECT_BIT         = 0x4,  // 0000 0100
        GJK_ERROR_SAME_VERTEX_ARRAY_BIT   = 0x8,  // 0000 1000
        GJK_ERROR_NULL_VERTEX_ARRAY_BIT   = 0x10, // 0001 0000
        GJK_ERROR_NOT_ENOUGH_VERTICES_BIT = 0x20, // 0010 0000
        
        // result bits
        GJK_INTERSECTING_BIT              = 0x40, // 0100 0000
        // lets leave the last bit for a later use
        GJK_EXTRA_BIT                     = 0x80, // 1000 0000

    } result_bits;

    gjk::result_bits intersects (
        const mesh_object* alpha_, 
        const mesh_object* beta_, 
        const uint32_t     max_iter_ = 100, 
        by_products_data*  by_products = nullptr);
};