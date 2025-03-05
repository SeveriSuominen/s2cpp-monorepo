#include <sstream>
#include <string>
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

#include <mxlib.hpp>

int main()
{
    SetTargetFPS(60); 

    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    
    InitWindow(screenWidth, screenHeight, "Dot and Cross - Demo");

    // style
    GuiLoadStyle("style_dark.rgs");

    // font
    Font font_otf = LoadFontEx("fonts/DepartureMono-Regular.otf", 20, 0, 250);
    GuiSetFont(font_otf);

    Image right_hand_rule_image = LoadImage("images/right_hand_rule2.png");
    Texture2D right_hand_rule_texture = LoadTextureFromImage(right_hand_rule_image);
    
    Camera3D camera = { 0 };
    camera.position = { 3.5f, 2.5f, 2.5f };  
    camera.target = { -0.15f, 1.5f, 0.0f };     
    camera.up = { 0.0f, 1.0f, 0.0f };         
    camera.fovy = 60.0f;                                
    camera.projection = CAMERA_PERSPECTIVE;             
 
    static bool gui_ui_enabled       = true;
    static int  simplex_stages_screenshot_viz = 0;

    GuiSetIconScale(1);

    bool  viz_mode = false, last_viz_mode = false;
    Model viz_points_model{};

    Color arc_color = CLITERAL(Color){70,70,70,255};
    Color grid0 = CLITERAL(Color){40,40,40,255};
    Color grid1 = CLITERAL(Color){40,40,40,255};

    while (!WindowShouldClose())  
    {   
        if(IsMouseButtonDown(1)){
            UpdateCamera(&camera, CAMERA_FREE);
        }

        if(IsKeyPressed(KEY_I)){
            gui_ui_enabled = !gui_ui_enabled; 
        }

        BeginDrawing();
        ClearBackground(EXAMPLE_BACKGROUND);

        static float degrees_value_ = 0;
        static float dot_product = 0.0f;
        static Vector3 cross_product_value_normalized_ = {0,0,0};
        static Vector3 cross_product_value_ = {0,0,0};

        Vector3 l0[2]{Vector3(0.f, 0.f, 0.f), Vector3(-1.f, 0.f, 0.f)};
        Vector3 l1[2]{Vector3(0.f, 0.f, 0.f), Vector3( 0.f, 0.f, 1.f)};
        auto rot = QuaternionFromEuler(0, degrees_value_ * PI, 0);
        l0[1] = Vector3RotateByQuaternion(l0[1], rot);
        const Vector3 offset{0.f,1.f,0.f}; 

        BeginMode3D(camera);
        DrawLine3D(Vector3(-1.5f, 0.f, 0.f)+offset, Vector3(1.5f, 0.f, 0.f)+offset, arc_color);

        s2::demo_util::draw_arrow_3d(l0[1]+offset+(l0[1]*0.05f), l0[1]+offset+(l0[1]*0.10f), 1.f, 0.1f, BLUE); 
        s2::demo_util::draw_arrow_3d(l1[1]+offset+(l1[1]*0.05f), l1[1]+offset+(l1[1]*0.10f), 1.f, 0.1f, RED); 
       
        DrawLine3D(l0[0]+offset, l0[1]+offset, BLUE);
        DrawLine3D(l1[0]+offset, l1[1]+offset, RED);

        auto dir0 = Vector3Normalize(l0[1] - l0[0]);
        auto dir1 = Vector3Normalize(l1[1] - l1[0]);

        s2::demo_util::draw_arc_3d(offset + Vector3{0.f,-0.f, 0.f}, 1.5f, {1.f, 0.f, 0.f}, 90.f, 1.0f, arc_color);
   
        cross_product_value_normalized_ = Vector3CrossProduct(dir0, dir1);
        cross_product_value_ = Vector3CrossProduct(l0[1] - l0[0], l1[1] - l1[0]);
        DrawLine3D(offset, cross_product_value_normalized_+offset, YELLOW);

        auto cp_dir = Vector3Normalize(cross_product_value_normalized_);
        s2::demo_util::draw_arrow_3d(cross_product_value_normalized_+offset+(cp_dir*0.05f), cross_product_value_normalized_+offset+(cp_dir*0.10f), 1.f, 0.1f, YELLOW); 

        s2::demo_util::draw_grid(10, 1.0f, grid0, grid1);

        EndMode3D();

        // GUI 
        if(gui_ui_enabled)
        {
            DrawRectangleRec({24,26,24,24}, BLUE);
            DrawRectangleRec({24,62,24,24}, RED);
            DrawRectangleRec({24,98,24,24}, YELLOW);

            constexpr float spacing = 0.0f;
            DrawTextEx(font_otf, "Vector A      = Index finger",Vector2( 58, 30), 20, spacing, RAYWHITE);
            DrawTextEx(font_otf, "Vector B      = Middle finger",Vector2( 58,66), 20, spacing, RAYWHITE);
            DrawTextEx(font_otf, "Cross product = Thumb", Vector2(58, 104), 20,spacing, RAYWHITE);

            const char* txt_left = "-1";
            const char* txt_right = "1";
            
            auto slider_rect = Rectangle{static_cast<float>(GetScreenWidth()) * 0.5f -128,static_cast<float>(GetScreenHeight()) -48, 256, 24};
            auto slider_rect2 = Rectangle{static_cast<float>(GetScreenWidth()) * 0.5f -128,static_cast<float>(GetScreenHeight()) -88, 256, 24};
            
            std::stringstream txt_slider_value{};
            txt_slider_value << std::fixed;
            txt_slider_value << "Degrees: ";
            txt_slider_value.precision(2);
            txt_slider_value << degrees_value_;
            
            std::stringstream txt_cross_product_value{};
            txt_cross_product_value << std::fixed;
            txt_cross_product_value << "Cross product (A, B) value: (";
            txt_cross_product_value.precision(2);
            txt_cross_product_value << cross_product_value_.x;
            txt_cross_product_value << ", ";
            txt_cross_product_value << cross_product_value_.y;
            txt_cross_product_value << ", ";
            txt_cross_product_value << cross_product_value_.z;
            txt_cross_product_value << ")";

            DrawTextEx(font_otf, txt_cross_product_value.str().c_str(), Vector2(58, 140), 20, spacing, RAYWHITE);

            DrawTextureEx(right_hand_rule_texture, {static_cast<float>(GetScreenWidth()) - 200, static_cast<float>(GetScreenHeight()) * 0.05f}, 0, 0.12f, WHITE);
            DrawTextEx(font_otf, txt_slider_value.str().c_str(), Vector2(slider_rect.x, slider_rect.y-40), 20, spacing, RAYWHITE);
            GuiSlider(slider_rect, txt_left, txt_right, &degrees_value_, -1.0f, 1.0f);
            GuiSlider(slider_rect, txt_left, txt_right, &degrees_value_, -1.0f, 1.0f);

            const auto text_offset = Vector3{0,0.25f, 0};
            const auto text_offset_cross = Vector3{0,0.4f * copysignf(1.f, cross_product_value_.y) , 0};
            DrawTextEx(font_otf, "A", GetWorldToScreen(l0[1]+offset+text_offset, camera), 20, 0, RAYWHITE);
            DrawTextEx(font_otf, "B", GetWorldToScreen(l1[1]+offset+text_offset, camera), 20, 0, RAYWHITE);
            const char* cross_txt = "Cross product";
            const char* dot_txt   = "Dot product";

            const auto dot_value = Vector3DotProduct(dir0, dir1);
            std::stringstream txt_info_value_dot{};
            txt_info_value_dot << std::fixed;
            txt_info_value_dot << dot_txt;
            txt_info_value_dot << "\ndot:";
            //txt_info_value_dot.precision(2);
            txt_info_value_dot << dot_value;
            txt_info_value_dot << "\ncos:";
            txt_info_value_dot << cosf((dot_value + 1.0f) * 0.5f * PI);

            const auto cross_mag = cross_product_value_.y;
            std::stringstream txt_info_value_cross{};
            txt_info_value_cross << std::fixed;
            txt_info_value_cross << cross_txt;
            txt_info_value_cross << "\ncross (magnitude):";
            //txt_info_value_cross.precision(2);
            txt_info_value_cross << cross_mag;
            txt_info_value_cross << "\nsin:";
            txt_info_value_cross << sinf((dot_value + 1.0f) * 0.5f * PI);

            auto center_offset_1 = Vector2{-MeasureTextEx(font_otf, txt_info_value_cross.str().c_str(), 20, 0).x * 0.5f, 0.f};
            auto center_offset_2 = Vector2{-MeasureTextEx(font_otf, txt_info_value_dot.str().c_str(), 20, 0).x * 0.5f, 0.f};

            auto dot_pos = l0[1]+offset+(l0[1]*0.85f);
            DrawTextEx(font_otf, txt_info_value_cross.str().c_str(), GetWorldToScreen(cross_product_value_normalized_+offset+text_offset_cross, camera)+center_offset_1, 20, 0, RAYWHITE);
            DrawTextEx(font_otf, txt_info_value_dot.str().c_str(), GetWorldToScreen(dot_pos+text_offset, camera)+center_offset_2, 20, 0, RAYWHITE);            
        }
 
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
    UnloadFont(font_otf); 
    return 0;
}