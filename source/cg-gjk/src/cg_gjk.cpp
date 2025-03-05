///////////////////////////////////////////////////////////////////
// Gilbert–Johnson–Keerthi (GJK) distance algorithm implemetation
///////////////////////////////////////////////////////////////////

#include "cg_gjk.hpp"
#include <vector>
#include <cassert>

using namespace s2cpp;
using namespace mxlib;

struct support_point 
{
    xfloat3 
    _support_a{};
    
    xfloat3 
    _support_b{};

    xfloat3 
    _position{};
};

static std::vector<xfloat3> transform_mesh_object_vertices_to_ws(const gjk::mesh_object* mesh_object_)
{   
    const auto& model_mtx = mesh_object_->_model_mtx;
    const auto  vertex_count = mesh_object_->_vertex_count;

    auto ws_vertices = std::vector<xfloat3>();
    ws_vertices.resize(vertex_count);
    for(uint32_t i = 0; i < vertex_count; ++i) {
        xfloat3 vertex_ = mxlib::transform(mesh_object_->_vertices[i], model_mtx);
        ws_vertices[i] = vertex_;
    }

    return ws_vertices;
};

static xfloat3 find_support_point (
    const xfloat3& search_direction, 
    const xfloat3* verts,
    const uint32_t count) 
{
    if(count == 0) {
        return {0,0,0};
    }
    // finding a vertex with the biggest magnitude in the specified direction,
    // this is easily tested by calculating the dot product of direction argument and vertex.
    auto best_index = 0;
    auto best_match = dot_product(search_direction, verts[0]);
    for(uint32_t i = 1; i < count; ++i){
        const auto dot = dot_product(search_direction, verts[i]);
        if(dot > best_match) {
            best_index = i;
            best_match   = dot;
        }
    }
    return verts[best_index];
}

static support_point find_minkowski_support (
    const xfloat3& search_direction, 
    const xfloat3* verts_a, 
    const xfloat3* verts_b,
    const uint32_t count_a,
    const uint32_t count_b) 
{
    support_point point = {};

    // find and store support points of the objects
    point._support_a = find_support_point(negate(search_direction), verts_a, count_a);
    point._support_b = find_support_point(search_direction, verts_b, count_b);

    // calculate minkowski sum (or "difference" in our case) by subtracting A and B support points,
    point._position = point._support_b - point._support_a;

    return point;
}

static bool test_simplex(fixed_list<support_point, 4>& simplex, xfloat3& direction)
{    
    // case 1D - line
    if(simplex.size() == 2)
    {
        // direction of the line, from 0 to 1
        xfloat3 d01 = simplex[1]._position - simplex[0]._position;

        // direction towards origin, as vertex position (or translation from the origin) is 
        // always pointing away from the origin, we can simply negate it to get 
        // the reversed direction towards the origin.
        xfloat3 dto = negate(simplex[0]._position);

        // is the line pointing towards the origin?
        if(dot_product(d01, dto) > 0) {

            // perpendicular explained:
            // two geometric objects are perpendicular
            // when they intersect exactly at an angle of 90 degrees (or π/2 radians),
            // Good characteristic to remember is that dot product of a perpendicular 
            // lines is always zero.
            //
            //           line1
            //             |   
            //    line0 ___|___
            //
            // looking at the crude visualization above, we can say 
            // line0 and line1 are perpendicular.

            // we calculate perpendicular of the line (towards the origin) to use 
            // as the next search direction to find suitable support point
            // to form a triangle 
            xfloat3 perp_01 = triple_product(d01, dto, d01);
            direction = perp_01;
        } else {
            // line was not pointing towards the origin, so we will use the negated position(A) 
            // of the simplex as the next search direction, we can be certain its pointing
            // towards the origin
            direction = dto;
        }
        return false;
    }

    // case 2D - triangle
    if(simplex.size() == 3)
    {
        // 1 _____ 0
        //   \   /
        //    \ /              
        //     2

        // triangle forming lines 2 => 0 and 2 => 1,
        
        // we call these lines "right" and "left"  
        // to make this part easier to conceptualize. 
    
        // "right" line
        xfloat3 d20 = simplex[0]._position - simplex[2]._position;
        // "left" line
        xfloat3 d21 = simplex[1]._position - simplex[2]._position;

        // direction towards the origin
        xfloat3 dto = negate(simplex[2]._position);

        // we calculate a perpendicular (or normal) of the triangle surface by calculating
        // the cross product of the triangle forming lines.   
        xfloat3 perp = cross_product(d21, d20); 
            
        // is the origin on the "right" side of the triangle?
        if(mixed_product(perp, d20, dto) > 0) {
            // is the "right" line facing the origin? 
            if(dot_product(d20, dto) > 0) {
                // set the next search direction towards the origin.
                direction = triple_product(d20, dto, d20);
                // remove the point that is not part of the "right" line.
                simplex.remove(1);
                return false; 
            }
        } 
        // is the origin on the right side of the "left" line? 
        else if(mixed_product(d21, perp, dto) <= 0) {
            if(dot_product(perp, dto) > 0) {
                // the origin is above 
                direction = perp;
            } else {   
                // the origin is below,
                //
                // swap points 0 and 1 to maintain
                // the correct order of the triangle,
                // or in other words, to maintain 
                // the correct side of the 
                // surface normal.
                simplex.swap(0, 1);
                direction = negate(perp);
            }
            return false;
        }

        // is the "left" line facing the origin?
        if(dot_product(d21, dto) > 0){
            // set the next search direction towards the origin.
            direction  = triple_product(d21, dto, d21);
            // remove the point that is not part of the "left" line.
            simplex.remove(0);
        } 
        else {
            // if we got here, we can't refine the simplex
            // in any meaningful way based on the current 
            // set of points, we will just reset the simplex,
            // set the next search direction to DTO
            // and start over.
            direction = dto;
            simplex.reset();
        }

        return false;
    }

    // case 3D - tetrahedron
    if(simplex.size() == 4)
    {
        //       3
        //      /|\
        //     / | \
        //    /  |  \
        //   1'-.|.-'0
        //       2

        // tetrahedron forming lines 3 => 2, 3 => 1, 3 => 0
        xfloat3 d32 = simplex[2]._position - simplex[3]._position;
        xfloat3 d31 = simplex[1]._position - simplex[3]._position;
        xfloat3 d30 = simplex[0]._position - simplex[3]._position;
        
        // direction towards the origin
        xfloat3 dto = negate(simplex[3]._position);

        // perpendicular vector for the each newly formed triangle surface,
        // notice the clockwise order.
        xfloat3 perp321 = cross_product(d32, d31);
        xfloat3 perp310 = cross_product(d31, d30);
        xfloat3 perp302 = cross_product(d30, d32);

        // there are only three tests left  
        // to determine if the formed tetrahedron contains  
        // the origin.  
        //  
        // for each newly formed triangular surface and  
        // the direction towards the origin, we calculate  
        // the plane distance to determine whether the  
        // origin is contained within the surface's  
        // hyperplane.  
        //  
        // if the origin is not contained within the triangle's plane,  
        // we know it lies in front of the triangle.  
        // In this case, we remove the only support point  
        // that is not part of the triangle from the simplex,  
        // set the next search direction to normal  
        // of the triangle surface, and then exit to form a new tetrahedron.  
        //  
        // if the origin is contained within all the triangle planes,  
        // we return 'true' from this function, indicating that  
        // the origin is enclosed within the simplex.
        if(dot_product(perp321, dto) > 0) {
            direction = perp321;
            simplex.remove(0);
        }
        else if(dot_product(perp310, dto) > 0) {
            direction = perp310;
            simplex.remove(2);
        }
        else if(dot_product(perp302, dto) > 0) {
            direction = perp302;
            simplex.remove(1);
        }
        else {
            // We have encapsulated the origin! 
            // intersection between objects is happening.
            return true;
        }
    }

    return false;
}

template<auto MASK>
inline constexpr auto mask_if_false(const bool cond) {
    return !cond * MASK;
}

gjk::result_bits gjk::intersects(const mesh_object* alpha_, const mesh_object* beta_, uint32_t max_iter_, by_products_data* by_products)
{
    // argument validation checks
    std::underlying_type<gjk::result_bits>::type validation_error_bits = 
        mask_if_false<GJK_ERROR_SAME_OBJECT_BIT>(alpha_ != beta_) |
        mask_if_false<GJK_ERROR_NULL_OBJECT_BIT>(alpha_ && beta_);

    if(validation_error_bits == GJK_EMPTY_MASK) {
        // alpha_ and beta_ pointers can be dereferenced, add rest of the checks
        validation_error_bits |= 
        mask_if_false<GJK_ERROR_SAME_VERTEX_ARRAY_BIT>(alpha_->_vertices != beta_->_vertices)  |
        mask_if_false<GJK_ERROR_NULL_VERTEX_ARRAY_BIT>(alpha_->_vertices && beta_ ->_vertices) |
        mask_if_false<GJK_ERROR_NOT_ENOUGH_VERTICES_BIT>(alpha_->_vertex_count >= 3 && beta_->_vertex_count >= 3);
    }

    if(validation_error_bits  != GJK_EMPTY_MASK) {
        // add the invalid bit '0000 0001' as validation was not successful
        validation_error_bits |= GJK_INVALID_BIT;
        return static_cast<gjk::result_bits>(validation_error_bits);
    }

    if(by_products) { *by_products = {}; }

    // transform vertices to common space (world space)
    auto wverts_a = transform_mesh_object_vertices_to_ws(alpha_);
    auto wverts_b = transform_mesh_object_vertices_to_ws(beta_);

    fixed_list<support_point, 4> simplex{};

    auto wpos_a = xfloat3(alpha_->_model_mtx[3], alpha_->_model_mtx[7], alpha_->_model_mtx[11]);
    auto wpos_b = xfloat3(beta_->_model_mtx[3], beta_->_model_mtx[7], beta_->_model_mtx[11]);

    // we can use any direction to find a initial support point for simplex construction
    auto search_direction = wpos_b - wpos_a;

    // add starting point to simplex
    const auto initial_support_point = 
        find_minkowski_support(
            search_direction,
            wverts_a.data(), 
            wverts_b.data(), 
            (uint32_t)wverts_a.size(), 
            (uint32_t)wverts_b.size());
    
    simplex.add(initial_support_point);
    
    // store the current state of the simplex to buffer, 
    // this to visualize the construction steps.
    if(by_products) {
        fixed_list<xfloat3, 4> store_simplex{};
        for(uint32_t i = 0; i < simplex.size(); ++i){
            store_simplex.add(simplex[i]._position);   
        }
        by_products->_simplex_construction_buffer.push_back(store_simplex);
    }

    // we use the initial support point to calculate our next search direction, next search direction must 
    // always be directed towards the origin, so we negate.
    search_direction = negate(initial_support_point._position); 

    uint32_t iter = 0;
    while(1) 
    {
        // find next support point
        auto support_point = 
            find_minkowski_support(
                search_direction,
                wverts_a.data(), 
                wverts_b.data(), 
                (uint32_t)wverts_a.size(), 
                (uint32_t)wverts_b.size());

        // we are beyond the origin, early exit
        if(dot_product(support_point._position, search_direction) < 0) {
            // no collision, we will return GJK_EMPTY value
            return GJK_EMPTY_MASK;
        }
        
        // add support point to simplex
        simplex.add(support_point);
        
        // store the current state of the simplex to buffer, 
        // this to visualize the construction steps.
        if(by_products) {
            fixed_list<xfloat3, 4> store_simplex{};
            for(uint32_t i = 0; i < simplex.size(); ++i){
                store_simplex.add(simplex[i]._position);   
            }
            by_products->_simplex_construction_buffer.push_back(store_simplex);
        }

        if(test_simplex(simplex, search_direction)) 
        {   
            // if simplex is tetrahedron containing the origin
            // intersection is happening
            break;
        }
         
        ++iter;
        if(iter > max_iter_){
            // if 'iter' reaches 'max_iter_' we can assume we have fallen into infinite loop scenario
            // caused by a floating point precission error and objects are in very close proximity of each
            // other or intersecting, purpose of this implementation is not to provide 
            // solution for highly accurate physics simulation (32 bit floating point scalar type makes this extremely challening already), 
            // instead we aim for high-perfomance reasonable accurate approximation for general purposes, we can assume 
            // intersection is happening.  
            break;
        }
    }

    if(by_products) {
        for(uint32_t i = 0; i < simplex.size(); ++i){
            by_products->_simplex_points.add(simplex[i]._position);
        }
    }

    // intersection is happening, we will return GJK_INTERSECTING_BIT
    return GJK_INTERSECTING_BIT;
}

