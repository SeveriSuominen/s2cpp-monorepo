#pragma once 

#include "external/par_shapes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


extern "C"
{
    #include "raylib.h"
}
#include "raymath.h"
#include "rlgl.h"           // OpenGL abstraction layer to OpenGL 1.1, 2.1, 3.3+ or ES2

#define EXAMPLE_BACKGROUND CLITERAL(Color){ 15, 15, 15, 255 }

namespace s2::demo_util
{

    static void take_screenshot(const char *fileName, Vector3* size)
    {
        // Security check to (partially) avoid malicious code
        if (strchr(fileName, '\'') != NULL) { TRACELOG(LOG_WARNING, "SYSTEM: Provided fileName could be potentially malicious, avoid [\'] character"); return; }

        Vector2 scale = GetWindowScaleDPI();
        unsigned char *imgData = rlReadScreenPixels((int)((float)size->x*scale.x), (int)((float)size->y*scale.y));
        Image image = { imgData, (int)((float)size->x*scale.x), (int)((float)size->y*scale.y), 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        char path[512] = { 0 };
        strcpy_s(path, TextFormat("%s%s", "", GetFileName(fileName)));

        ExportImage(image, path);           // WARNING: Module required: rtextures
        RL_FREE(imgData);

        if (FileExists(path)) TRACELOG(LOG_INFO, "SYSTEM: [%s] Screenshot taken successfully", path);
        else TRACELOG(LOG_WARNING, "SYSTEM: [%s] Screenshot could not be saved", path);
    }

    // Generate sphere mesh (standard sphere)
    static Mesh gen_mesh_icosphere(float radius)
    {
        Mesh mesh = { 0 };

        par_shapes_mesh *sphere = par_shapes_create_subdivided_sphere(3);
        par_shapes_scale(sphere, radius, radius, radius);
        
        mesh.vertices  = new float[sphere->ntriangles*3*3*sizeof(float)];
        mesh.texcoords = new float[sphere->ntriangles*2*2*sizeof(float)]{0};
        mesh.normals   = nullptr;//new float(sphere->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = sphere->ntriangles*3;
        mesh.triangleCount = sphere->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = sphere->points[sphere->triangles[k]*3];
            mesh.vertices[k*3 + 1] = sphere->points[sphere->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = sphere->points[sphere->triangles[k]*3 + 2];
        }

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);

        par_shapes_free_mesh(sphere);
        
        return mesh;
    }

    static void draw_grid(int slices, float spacing, const Color& color0, const Color& color1)
    {
        int half_slices = slices/2;

        rlBegin(RL_LINES);
        for (int i = -half_slices; i <= half_slices; i++)
        {
            if (i == 0)
            {
                auto col_ = ColorNormalize(color0);
                rlColor3f(col_.x, col_.y, col_.z);
            }
            else
            {
                auto col_ = ColorNormalize(color0);
                rlColor3f(col_.x, col_.y, col_.z);
            }

            rlVertex3f((float)i*spacing, 0.0f, (float)-half_slices*spacing);
            rlVertex3f((float)i*spacing, 0.0f, (float)half_slices*spacing);
            rlVertex3f((float)-half_slices*spacing, 0.0f, (float)i*spacing);
            rlVertex3f((float)half_slices*spacing, 0.0f, (float)i*spacing);
        }
        rlEnd();
    }

    static void draw_arc_3d(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, float arc, Color color)
    {
        rlPushMatrix();
            rlTranslatef(center.x, center.y, center.z);
            rlRotatef(rotationAngle, rotationAxis.x, rotationAxis.y, rotationAxis.z);

            rlBegin(RL_LINES);
                const auto step_size = 4;
                const auto steps = static_cast<int>((Clamp(std::abs(arc), 0.f, 1.0f) * 360));
            
                for (int i = 0, j = 0; i < steps; i += step_size, j++)
                {
                    rlColor4ub(color.r, color.g, color.b, j % 2 == 0 ? color.a : 0);
                    rlVertex3f(sinf(DEG2RAD*i)*radius, cosf(DEG2RAD*i)*radius, 0.0f);
                    rlVertex3f(sinf(DEG2RAD*(i + step_size))*radius, cosf(DEG2RAD*(i + step_size))*radius, 0.0f);
                }
            rlEnd();
        rlPopMatrix();
    }

    static void draw_arrow_3d(Vector3 start, Vector3 end, float shaftThickness, float arrowSize, Color color) 
    {
        Vector3 dir = Vector3Subtract(end, start);
        float length = Vector3Length(dir);
        if (length < 0.001f) return; 
    
        dir = Vector3Normalize(dir); 
    
        DrawLine3D(start, Vector3Subtract(end, Vector3Scale(dir, arrowSize)), color);
    
        Vector3 right = Vector3CrossProduct(dir, {0, 1, 0}); 
        if (Vector3Length(right) < 0.001f) right = {1, 0, 0}; 
        right = Vector3Normalize(right);
        
        Vector3 up = Vector3CrossProduct(right, dir);
        
        Vector3 tip = end;
        Vector3 base1 = Vector3Add(Vector3Subtract(end, Vector3Scale(dir, arrowSize)), Vector3Scale(right, arrowSize * 0.5f));
        Vector3 base2 = Vector3Add(Vector3Subtract(end, Vector3Scale(dir, arrowSize)), Vector3Scale(up, arrowSize * 0.5f));
        Vector3 base3 = Vector3Subtract(Vector3Subtract(end, Vector3Scale(dir, arrowSize)), Vector3Scale(right, arrowSize * 0.5f));
        Vector3 base4 = Vector3Subtract(Vector3Subtract(end, Vector3Scale(dir, arrowSize)), Vector3Scale(up, arrowSize * 0.5f));
    
        DrawTriangle3D(tip, base2, base1, color);
        DrawTriangle3D(tip, base3, base2, color);
        DrawTriangle3D(tip, base4, base3, color);
        DrawTriangle3D(tip, base1, base4, color);

        DrawTriangle3D(base1, base2, base3, color);
        DrawTriangle3D(base1, base3, base4, color);
    }
}


