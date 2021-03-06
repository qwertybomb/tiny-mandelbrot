// standard headers
#include <stdint.h>
#include <stdbool.h>

// windows headers
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>

// opengl headers
#define WGL_WGLEXT_PROTOTYPES
#include <gl/GL.h>
#include "glext.h"
#include "wglext.h"
#include "opengl.h"

// needed when we use floats
extern int _fltused;
int _fltused;

typedef struct Window
{
    HDC device_context;
    float aspect_ratio;
    float scale, pos[2];
    float smooth_scale, smooth_pos[2];
    int32_t max_iterations;
} Window;


typedef enum Keys
{
    KEY_W = 'W',
    KEY_S = 'S',
    KEY_A = 'A',
    KEY_D = 'D',
    KEY_PLUS1 = VK_ADD,
    KEY_PLUS2 = VK_OEM_PLUS,
    KEY_MINUS1 = VK_SUBTRACT,
    KEY_MINUS2 = VK_OEM_MINUS,
    KEY_UP = VK_UP,
    KEY_DOWN = VK_DOWN,
    KEY_R = 'R',
    KEY_CTRL = VK_CONTROL,
    KEY_LENGTH // needed to keep track of number of keys
} Keys;

// NOTE: we could use GetWindowLongPtr and SetWindowLongPtr
// however this is much more easier
static Window global_window;
static bool keys[256];

static LRESULT CALLBACK WinProc(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SYSKEYDOWN:
        case WM_SYSCHAR:
        case WM_SYSKEYUP:
        {
        } break;
        
        case WM_SIZE:
        {
            // the width and height are stored in the low and high word of lParam respectively
            LPARAM const width = lParam & 0xFFFF;
            LPARAM const height = (lParam >> 16) & 0xFFFF;
            
            // store the aspect ratio
            global_window.aspect_ratio = (float)width / (float)height;
            
            glViewport(0, 0, width, height);
        } break;
        
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY:
        {
            ExitProcess(0);
        }
        
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            bool const should_flip = ((lParam >> 30) & 0x1) == ((lParam >> 31) & 0x1);
            
            if (should_flip)
            {
                keys[wParam] = !keys[wParam];
            }
            
            if (wParam == VK_ESCAPE) ExitProcess(0);
        } break;
        
        default:
        {
            return DefWindowProc(window_handle, message, wParam, lParam);
        }
    }
    
    return 0;
}

static void create_opengl_context(HDC const device_context)
{
    // same as: 
    //  static PIXELFORMATDESCRIPTOR const pfd = {
    // .nSize = sizeof(pfd), 
    // .dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW,
    // .iPixelType = PFD_TYPE_RGBA,
    // .cColorBits = 32,
    // .cDepthBits = 32,
    // .iLayerType = PFD_MAIN_PLANE
    // };
    SetPixelFormat(device_context, 9, NULL);
    
    // create an opengl 3.3 context
    HGLRC const opengl_context = wglCreateContext(device_context);
    
    // make the new opengl context current and active
    wglMakeCurrent(device_context, opengl_context);
    
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
    wglSwapIntervalEXT(0);
}

static void create_window(int32_t width, int32_t height)
{
    // create a window class
    static WNDCLASSA const wndclass = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = &WinProc,
        .lpszClassName = "0",
    };
    
    RegisterClassA(&wndclass);
    
    // create a window
    HWND const window_handle = CreateWindowA(wndclass.lpszClassName,
                                             "", 
                                             WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             width, height, NULL, NULL,
                                             NULL, NULL);
    
    // create a device context
    HDC const device_context = GetDC(window_handle);
    
    // create an opengl context
    create_opengl_context(device_context);
    
    // load opengl extensions after creating an opengl context
    load_extensions();
    
    // setup global window
    {
        global_window.device_context = device_context;
        global_window.aspect_ratio = (float)width / (float)height;
        global_window.scale = 1.0f;
        global_window.smooth_scale = 0.5f;
        global_window.max_iterations = 200;
    }
    
    // show the window
    ShowWindow(window_handle, SW_SHOWDEFAULT);
}

static unsigned int compile_shaders(char const *vertex_shader_source,
                                    char const *fragment_shader_source)
{
    // compile vertex shader
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    
    // compile fragment shader
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source,  NULL);
    glCompileShader(fragment_shader);
    
    // only needed for debugging
#ifdef DEBUG_MODE
    {
        int success;
        char info_log[512];
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        
        if(success == GL_FALSE)
        {
            glGetShaderInfoLog(fragment_shader, sizeof(info_log), 
                               NULL, info_log);
            WriteFile((HANDLE)(STD_OUTPUT_HANDLE),
                      info_log, (DWORD)lstrlenA(info_log),
                      NULL, NULL);
        }
    }
#endif
    
    // link the shaders
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader); 
    glAttachShader(shader_program, fragment_shader); 
    glLinkProgram(shader_program);
    
    return shader_program;
}

static float lerp(float v0, float v1, float t)
{
    return (1.0f - t) * v0 + t * v1;
}

__declspec(noreturn) void __stdcall entry(void)
{
    create_window(800, 600);
    
    // by making this smaller we can save space at the cost of readabilty
#define VERTEX_SHADER                                                                         \
"#version 330\n"                                                                          \
"out vec2 u;void main(){u=vec2[](vec2(0),vec2(1,0),vec2(0,1),vec2(1))[gl_VertexID];"      \
"gl_Position=vec4(vec2[](vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(1))[gl_VertexID],0,1);}"  \
    
#define FRAGMENT_SHADER                                                     \
"#version 330\n"                                                        \
"#define B 200000.0\n"                                                  \
"out vec4 F;in vec2 u;uniform int I;uniform float A;uniform vec4 D;"    \
"void main(){vec2 c=((u*2-1)*vec2(A,1)*D.y-D.zw);vec2 z=vec2(0);int i;" \
"for(i=0;i<I&&dot(z,z)<B;++i)z=vec2(z.x*z.x-z.y*z.y,z.x*z.y*2)+c;"      \
"float s=sqrt((i-log2(log(dot(z,z))/log(B)))/float(I));"                \
"F=(sin(D.x+20*s*vec4(1.5,1.8,2.1,0))*0.5+0.5)*float(i!=I);}"           \
    
    unsigned int const shader_program = compile_shaders(VERTEX_SHADER, 
                                                        FRAGMENT_SHADER);
    
    glUseProgram(shader_program);
    float color_offset = 0.0f;
    MSG msg;
    for(;;)
    {
        // process events and messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        else
        {
            // pass uniforms
            glUniform1f(glGetUniformLocation(shader_program, "A"),
                        global_window.aspect_ratio);
            glUniform4f(glGetUniformLocation(shader_program, "D"), color_offset, global_window.smooth_scale, 
                        global_window.smooth_pos[0], global_window.smooth_pos[1]);
            glUniform1i(glGetUniformLocation(shader_program, "I"),
                        global_window.max_iterations);
            
            // draw a quad
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            // finally draw to the screen
            SwapBuffers(global_window.device_context);
            
            // the smooth values will smoothly converge to the real values
            {
                global_window.smooth_pos[0] = lerp(global_window.smooth_pos[0], 
                                                   global_window.pos[0], 0.005f);
                global_window.smooth_pos[1] = lerp(global_window.smooth_pos[1], 
                                                   global_window.pos[1], 0.005f);
                
                global_window.smooth_scale = lerp(global_window.smooth_scale,
                                                  global_window.scale, 0.005f);
            }
            
            color_offset += 0.001f;
        }
        
        // handle input
        {
            // some keyboards have two plus keys(number row and numpad)
            if (keys[KEY_PLUS1] || keys[KEY_PLUS2])
            {
                global_window.scale *= 1.0f - 0.003f;
            }
            
            // see the above comment
            if(keys[KEY_MINUS1] || keys[KEY_MINUS2])
            {
                global_window.scale *= 1.0f + 0.003f;
            }
            
            if (keys[KEY_W])
            {
                global_window.pos[1] -= global_window.scale * 0.003f; 
            }
            
            if (keys[KEY_S])
            {
                global_window.pos[1] += global_window.scale * 0.003f; 
            }
            if (keys[KEY_A])
            {
                global_window.pos[0] += global_window.scale * 0.003f; 
            }
            
            if  (keys[KEY_D])
            {
                global_window.pos[0] -= global_window.scale * 0.003f; 
            }
            
            // if ctrl-r is pressed reset the scale and pos
            if (keys[KEY_CTRL] && keys[KEY_R])
            {
                global_window.pos[0] = 0.0f;
                global_window.pos[1] = 0.0f;
                global_window.scale = 1.0f;
            }
            
            if (keys[KEY_UP])
            {
                global_window.max_iterations += 1;
            }
            
            if (keys[KEY_DOWN] && 
                global_window.max_iterations > 2)
            {
                global_window.max_iterations -= 1;
            }
        }
    }
}
