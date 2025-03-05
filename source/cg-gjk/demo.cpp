#include <algorithm>
#include <bitset>
#include <vector>
#include <chrono>

#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS

#define RAYGUI_IMPLEMENTATION

#if   defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-compare-conditional"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER) 
#pragma warning(push)
#pragma warning(disable: 4389)
#endif

extern "C"
{
    #include "raylib.h"
    #include "rayext/raygizmo.h"
}

#include "raymath.h"
#include "raygui.h"

#include "demo_util.hpp"

#if   defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER) 
#pragma warning(pop)
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include "cg_gjk.hpp"

using namespace s2cpp;

std::vector<mxlib::xfloat3> minkowski_diff(gjk::mesh_object* a, gjk::mesh_object* b)
{
    std::vector<mxlib::xfloat3> wverts_a = {}; 
    std::vector<mxlib::xfloat3> wverts_b = {};
    std::vector<mxlib::xfloat3> minkowski_diff = {};

    wverts_a.reserve(a->_vertex_count);
    wverts_b.reserve(b->_vertex_count);
    minkowski_diff.reserve(a->_vertex_count * b->_vertex_count);

    for(int i = 0; i < (int)a->_vertex_count; ++i) {
        wverts_a.push_back(mxlib::transform(a->_vertices[i], a->_model_mtx));
    }

    for(int i = 0; i < (int)b->_vertex_count; ++i) {
        wverts_b.push_back(mxlib::transform(b->_vertices[i], b->_model_mtx));
    } 

    for(int i = 0; i < (int)a->_vertex_count; ++i) {   
        for(int j = 0; j < (int)b->_vertex_count; ++j) {
            const auto pos = wverts_b[j] - wverts_a[i];        
            minkowski_diff.push_back(pos);
        } 
    }

    return minkowski_diff;
}

void draw_simplex(fixed_list<xfloat3, 4> simplex, Color color)
{
    mxlib::fixed_list<mxlib::xfloat3, 12> simplex_triangles{};
    
    
    switch(simplex.size())
    {
        case 0: return;
        case 1: {
            auto position = mxlib::reinterpret<Vector3>(simplex[0]);
            DrawPoint3D(position, color);
            return;
        }
        case 2: {
            auto arr = reinterpret_cast<Vector3*>(simplex.data());
            DrawLine3D(arr[0],   arr[1], color);
            return;
        } 
        case 3: {
            simplex_triangles.add(simplex[0]);
            simplex_triangles.add(simplex[1]);
            simplex_triangles.add(simplex[2]);
            break;
        } 
        case 4: {
            simplex_triangles.add(simplex[0]);
            simplex_triangles.add(simplex[1]);
            simplex_triangles.add(simplex[2]);
    
            simplex_triangles.add(simplex[3]);
            simplex_triangles.add(simplex[2]);
            simplex_triangles.add(simplex[1]);
    
            simplex_triangles.add(simplex[3]);
            simplex_triangles.add(simplex[1]);
            simplex_triangles.add(simplex[0]);
    
            simplex_triangles.add(simplex[3]);
            simplex_triangles.add(simplex[0]);
            simplex_triangles.add(simplex[2]);
            break;
        } 
        default:break;
    }

    const auto size_ = (int)simplex_triangles.size();
    auto arr = reinterpret_cast<Vector3*>(simplex_triangles.data());
    for(int k = 0; k < size_; k+=3) {
        DrawLine3D(arr[k],   arr[k+1], color);
        DrawLine3D(arr[k+1], arr[k+2], color);
        DrawLine3D(arr[k+2], arr[k],   color);
    }
}

int main()
{
    SetTargetFPS(60); 

    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    
    InitWindow(screenWidth, screenHeight, "GJK - Demo");

    Camera3D camera = { 0 };
    camera.position = { 12.5f, 15.0f, 12.5f };  
    camera.target = { 0.0f, 2.5f, 0.0f };     
    camera.up = { 0.0f, 1.0f, 0.0f };         
    camera.fovy = 50.0f;                                
    camera.projection = CAMERA_PERSPECTIVE;             
    
    constexpr uint32_t PRIMITIVE_COUNT = 3;

    GuiLoadStyle("style_dark.rgs");
    
    Model models[PRIMITIVE_COUNT];
    models[0] = LoadModelFromMesh(GenMeshCube(2, 2, 2));
    models[1] = LoadModelFromMesh(GenMeshCone(2, 2.5f, 4));
    models[2] = LoadModelFromMesh(s2::demo_util::gen_mesh_icosphere(1.5f));
   
    std::vector<mxlib::xfloat3> vertices[PRIMITIVE_COUNT]{};
    for(int i = 0; i < PRIMITIVE_COUNT; ++i) { 
        auto& mesh_ = models[i].meshes[0];
        vertices[i].resize(mesh_.vertexCount);
        for(int j = 0; j < mesh_.vertexCount; ++j) {   
            mxlib::xfloat3 vertex{mesh_.vertices + j*3};
            vertices[i][j] = vertex;
        }
    }

    // materials
    const auto common_color = ColorNormalize(CLITERAL(Color){ 245, 245, 245, 128 });
    Vector4 colors[PRIMITIVE_COUNT];
    colors[0] = common_color;
    colors[1] = common_color;
    colors[2] = common_color;
    
    // gizmos
    Transform gizmo_transforms[PRIMITIVE_COUNT];
    gizmo_transforms[0] = {{3, 5,-3}, QuaternionIdentity(), Vector3One()};
    gizmo_transforms[1] = {{0,2.5f,3}, QuaternionIdentity(), Vector3One()};
    gizmo_transforms[2] = {{-3,5,-3}, QuaternionIdentity(), Vector3One()};
 
    // shader
    Shader polyviz_shader = LoadShader(
        "polyviz_simple.vs", 
        "polyviz_simple.fs");

    // shader uniform locations
    int color_idx = GetShaderLocation(polyviz_shader, "color");

    gjk::mesh_object       objects[PRIMITIVE_COUNT]{};
    gjk::by_products_data  results[PRIMITIVE_COUNT]{};
    
    static bool gui_ui_enabled       = true;
    static bool gizmo_move_enabled   = true;
    static bool gizmo_rotate_enabled = false;
    static bool gizmo_scale_enabled  = false;
    static int  simplex_stages_screenshot_viz = 0;

    GuiSetIconScale(1);

    bool  viz_mode = false, last_viz_mode = false;
    Model viz_points_model{};

    static bool  viz_iteration_enabled = false;
    static int   viz_iteration = 0;
    // seconds to change.
    static float viz_iteration_change_playback_speed = 0.1f;
    static float viz_iteration_change_timer  = 0.0f;

    while (!WindowShouldClose())  
    {   
        const auto delta_time = GetFrameTime();

        if(IsKeyPressed(KEY_RIGHT)){
            viz_iteration++;
            viz_iteration_change_timer = 0;
        }
        if(IsKeyPressed(KEY_LEFT)){
            viz_iteration--;
            viz_iteration_change_timer = 0;
        }

        if(IsKeyDown(KEY_RIGHT)){
            viz_iteration_change_timer += delta_time;
            if(viz_iteration_change_timer > viz_iteration_change_playback_speed){
                viz_iteration++;
                viz_iteration_change_timer = 0;
            }
        }
        else if(IsKeyDown(KEY_LEFT)) {
            viz_iteration_change_timer += delta_time;
            if(viz_iteration_change_timer > viz_iteration_change_playback_speed){
                viz_iteration--;
                viz_iteration_change_timer = 0;
            }
        } else {
            viz_iteration_change_timer = 0;
        }
        viz_iteration = std::clamp(viz_iteration, 0, 100);

        if(IsMouseButtonDown(1)){
            UpdateCamera(&camera, CAMERA_FREE);
        }
        
        if(IsKeyPressed(KEY_KP_1)){
            simplex_stages_screenshot_viz = 0;
        }

        if(IsKeyPressed(KEY_KP_2)){
            simplex_stages_screenshot_viz = 1;
        }

        if(IsKeyPressed(KEY_KP_3)){
            simplex_stages_screenshot_viz = 2;
        }

        if(IsKeyPressed(KEY_KP_4)){
            simplex_stages_screenshot_viz = 3;
        }
        
        if(IsKeyPressed(KEY_Z)) { 
            camera.target = { 0.0f, 0.0f, 0.0f };
        }

        if(IsKeyPressed(KEY_I)){
            gui_ui_enabled = !gui_ui_enabled; 
        }

        BeginDrawing();
        ClearBackground(EXAMPLE_BACKGROUND);

        // GUI 
        if(gui_ui_enabled)
        {
            GuiCheckBox({24,48,16,16}, "#51# Move",   &gizmo_move_enabled);
            GuiCheckBox({24,72,16,16}, "#60# Rotate", &gizmo_rotate_enabled);
            GuiCheckBox({24,96,16,16}, "#53# Scale",  &gizmo_scale_enabled);
            GuiCheckBox({24,120,16,16}, "Viz iteration",  &viz_iteration_enabled);

            auto iter_str = (std::string("iter: ") + std::to_string(viz_iteration));
            int  iter_str_font_size = 20; 
            auto iter_str_spacing = 0.0f;
            auto iter_str_measured = MeasureTextEx(GuiGetFont(), iter_str.c_str(), (float)iter_str_font_size, iter_str_spacing);
            DrawText(iter_str.c_str(), GetScreenWidth() -72 -((int) iter_str_measured.x), 48, iter_str_font_size, RAYWHITE);

            if(!viz_mode) {
                if(GuiButton({24,150,96,32}, "#44# Capture")) {
                    viz_mode = true;
                }
            } else {
                if(GuiButton({24,150,96,32}, "#119# Sandbox")) {
                    viz_mode = false;
                }
            }

            DrawFPS(10, 10);
        }
        
        int gizmo_flags = 0;
        gizmo_flags |= GIZMO_TRANSLATE * (int)gizmo_move_enabled;
        gizmo_flags |= GIZMO_ROTATE    * (int)gizmo_rotate_enabled;
        gizmo_flags |= GIZMO_SCALE     * (int)gizmo_scale_enabled;

        BeginMode3D(camera);

        // update primitive model matrices
        for(int i = 0; i < PRIMITIVE_COUNT; ++i) {
            models[i].transform = GizmoToMatrix(gizmo_transforms[i]);

            objects[i] = gjk::mesh_object {
                mxlib::xfloat4x4(reinterpret_cast<float*>(&models[i].transform.m0)),
                reinterpret_cast<mxlib::xfloat3*>(&models[i].meshes[0].vertices[0]),
                static_cast<uint32_t>(models[i].meshes[0].vertexCount)
            };
        }

        if(viz_mode != last_viz_mode) {
            if(viz_mode) {
                uint32_t viz_points_size = 0;
                for(int i = 0; i < PRIMITIVE_COUNT; ++i){
                    for(int j = 0; j < PRIMITIVE_COUNT; ++j){
                        if(i != j){
                            viz_points_size += 
                                objects[i]._vertex_count * 
                                objects[j]._vertex_count; 
                        }
                    }   
                }
                // pre-alloc once
                Mesh mesh_{};
                mesh_.vertices = new float[viz_points_size * 3];
                mesh_.vertexCount = viz_points_size;
                uint32_t head = 0;
                for(int i = 0; i < PRIMITIVE_COUNT; ++i){
                    for(int j = 0; j < PRIMITIVE_COUNT; ++j){
                        if(i == j) continue;
                        auto vertices = minkowski_diff(&objects[i], &objects[j]);
                        std::memcpy(mesh_.vertices + head, vertices.data(), vertices.size() * sizeof(mxlib::xfloat3));
                        head += (uint32_t)vertices.size() * 3u; // floats 
                    }   
                }
                UploadMesh(&mesh_, false);
                viz_points_model = LoadModelFromMesh(mesh_);
            } else {
                UnloadModel(viz_points_model);
            }
            last_viz_mode = viz_mode;
        }

        // draw primitive objects
        for(int i = 0; i < PRIMITIVE_COUNT; ++i) 
        {   
            bool is_intersecting = false;
           
            // intersection tests
            for(int j = 0; j < PRIMITIVE_COUNT; ++j){
                if(i == j) continue;

                const auto result_bits = gjk::intersects(&objects[i], &objects[j], 100, &results[j]);
                const auto intersecting = mxlib::contains(result_bits, gjk::GJK_INTERSECTING_BIT);
                const auto invalid      = mxlib::contains(result_bits, gjk::GJK_INVALID_BIT);

                auto& result = results[j];

                if(intersecting)
                {
                    is_intersecting = true;

                    auto simplex = results[j]._simplex_points;

                    if(viz_mode) { 
                        if(!viz_iteration_enabled){
                            draw_simplex(simplex, ORANGE);
                        } else {
                            auto max_idx  = (int)result._simplex_construction_buffer.size()-1; 
                            if(max_idx >= 0) {
                                int iter_ = std::clamp(viz_iteration, 0, max_idx);
                                draw_simplex(result._simplex_construction_buffer[iter_], ORANGE);
                            }
                        }
                    }
                }
            }
            
            const auto overlap_color = CLITERAL(Color){ 43, 255, 156, 128 };
            const auto primitive_color = is_intersecting ? ColorNormalize(overlap_color) : colors[i];
         
            if(!viz_mode)
            {
                SetShaderValue(polyviz_shader, color_idx, &primitive_color, RL_SHADER_UNIFORM_VEC4);
                auto shader_original = models[i].materials[0].shader;
                models[i].materials[0].shader = polyviz_shader;
                DrawModel(models[i], {}, 1.0f, RAYWHITE);
                models[i].materials[0].shader = shader_original;
                DrawModelWires(models[i], {}, 1.0f, DARKGRAY);
            } else {
                const auto viz_primitive_color = is_intersecting ? overlap_color : PURPLE;
                DrawModelPoints(viz_points_model, {}, 1.0f, RED);
                DrawModelWires(models[i], {}, 1.0f, viz_primitive_color);
                // show origin
                DrawSphere(Vector3Zero(), 0.1f, RAYWHITE);
            }
            
            if(gizmo_flags > 0 && !viz_mode) {
                DrawGizmo3D(gizmo_flags, &gizmo_transforms[i]);
            }
        }

        Color grid0 = CLITERAL(Color){100,100,100};
        Color grid1 = CLITERAL(Color){100,100,100};
        s2::demo_util::draw_grid(10, 1.0f, grid0, grid1);

        EndMode3D();
    
        EndDrawing();

        if(IsKeyPressed(KEY_H)){
            auto now_ = std::chrono::system_clock::now();
            auto utc_ = std::chrono::duration_cast<std::chrono::milliseconds>(now_.time_since_epoch()).count();
            std::stringstream ss_{};
            ss_ << "screenshot_"
            << utc_
            << ".png";
            TakeScreenshot(ss_.str().c_str());
            printf("screenshot saved\n");
        }
    }
    CloseWindow();

    return 0;
}