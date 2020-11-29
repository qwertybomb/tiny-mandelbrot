// standard headers
#include <stdint.h>
#include <stdbool.h>

// windows headers
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// opengl headers
#include <gl/GL.h>
#include <GL/glext.h>

#define WGL_WGLEXT_PROTOTYPES
#include <GL/wglext.h>
#include "opengl.h"

// needed when we use floats
extern int _fltused;
int _fltused;

typedef struct Window
{
    HWND handle;
    HDC device_context;
    HGLRC opengl_context;
    int32_t width, height;
    float scale, smooth_scale;
    float pos[2], smooth_pos[2];
    int32_t max_iterations;
} Window;

typedef enum Keys
{
    KEY_W,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_PLUS1,
    KEY_PLUS2,
    KEY_MINUS1,
    KEY_MINUS2,
    KEY_UP,
    KEY_DOWN,
    KEY_R,
    KEY_CTRL,
    KEY_LENGTH // needed to keep track of number of keys
} Keys;


// NOTE: we could use GetWindowLongPtr and SetWindowLongPtr
// however this is alot easier
static Window global_window;
static bool keys[KEY_LENGTH];

static LRESULT CALLBACK WinProc(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SYSKEYDOWN:
        {
            // check for alt-f4
            // if the 29-th bit is set the alt key is down
            if (((lParam >> 29) & 0x1) != 0 &&
                wParam == VK_F4)
            {
                ExitProcess(0);
            }
            
            /* fall through */
        }
        case WM_SYSCHAR:
        case WM_SYSKEYUP:
        {
            return 1;
        }
        
        case WM_SIZE:
        {
            // the width and height are stored in the low and high word of lParam respectively
            global_window.width = lParam & 0xFFFF;
            global_window.height = (lParam >> 16) & 0xFFFF;
            
            glViewport(0, 0, global_window.width, global_window.height);
        } break;
        
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            bool const key_down = (lParam >> 31) != 0;
            bool should_flip = ((lParam >> 30) & 0x1) != (lParam >> 31);
            should_flip = key_down ? should_flip : !should_flip;
            
            switch (wParam)
            {
                case 'W':
                {
                    if (should_flip)
                    {
                        keys[KEY_W] = !keys[KEY_W];
                    }
                } break;
                
                case 'S':
                {
                    if (should_flip)
                    {
                        keys[KEY_S] = !keys[KEY_S];
                    }
                } break;
                
                case 'A':
                {
                    if (should_flip)
                    {
                        keys[KEY_A] = !keys[KEY_A];
                    }
                } break;
                
                case 'D':
                {
                    if (should_flip)
                    {
                        keys[KEY_D] = !keys[KEY_D];
                    }
                } break;
                
                case 'R':
                {
                    if (should_flip)
                    {
                        keys[KEY_R] = !keys[KEY_R];
                    }
                } break;
                
                case VK_CONTROL:
                {
                    if (should_flip)
                    {
                        keys[KEY_CTRL] = !keys[KEY_CTRL];
                    }
                } break;
                
                case VK_OEM_PLUS:
                {
                    if (should_flip)
                    {
                        keys[KEY_PLUS1] = !keys[KEY_PLUS1];
                    }
                } break;
                
                case VK_ADD:
                {
                    if (should_flip)
                    {
                        keys[KEY_PLUS2] = !keys[KEY_PLUS2];
                    }
                } break;
                
                case VK_OEM_MINUS:
                {
                    if (should_flip)
                    {
                        keys[KEY_MINUS1] = !keys[KEY_MINUS1];
                    }
                } break;
                
                case VK_SUBTRACT:
                {
                    if (should_flip)
                    {
                        keys[KEY_MINUS2] = !keys[KEY_MINUS2];
                    }
                } break;
                
                case VK_UP:
                {
                    if (should_flip)
                    {
                        keys[KEY_UP] = !keys[KEY_UP];
                    }
                } break;
                
                case VK_DOWN:
                {
                    if (should_flip)
                    {
                        keys[KEY_DOWN] = !keys[KEY_DOWN];
                    }
                } break;
            }
        } break;
        
        default:
        {
            return DefWindowProcW(window_handle, message, wParam, lParam);
        }
    }
    
    return 0;
}

static HGLRC create_opengl_context(HDC const device_context)
{
    PIXELFORMATDESCRIPTOR const pfd = {
        .nSize = sizeof(pfd), 
        .dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 32,
        .iLayerType = PFD_MAIN_PLANE
    };
    
    int pixel_format_index = ChoosePixelFormat(device_context, &pfd);
    
    // ChoosePixelFormat returns zero on error
    if (pixel_format_index == 0)
    {
        ExitProcess(GetLastError());
    }
    
    if (SetPixelFormat(device_context, pixel_format_index, &pfd) == FALSE)
    {
        ExitProcess(GetLastError());
    }
    
    // create an opengl 3.3 context
    HGLRC const result = wglCreateContext(device_context);
    
    // make the new opengl context current and active
    wglMakeCurrent(device_context, result);
    
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
    wglSwapIntervalEXT(0);
    
    return result;
}

static void create_window(wchar_t const *title, int32_t width, int32_t height)
{
    HANDLE hInstance = GetModuleHandleW(NULL);
    
    // create a window class
    WNDCLASSEXW wndclassex = {
        .cbSize = sizeof(wndclassex),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = &WinProc,
        .hInstance = hInstance,
        .hIcon = LoadIconW(hInstance, IDI_APPLICATION),
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .lpszClassName = L"a",
        .hIconSm = LoadIconW(hInstance, IDI_APPLICATION)
    };
    
    if (RegisterClassExW(&wndclassex) == 0)
    {
        ExitProcess(GetLastError());
    }
    
    // create a window
    HWND const window_handle = CreateWindowW( 
                                             wndclassex.lpszClassName,
                                             title, 
                                             WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             width, height, NULL, NULL,
                                             hInstance, NULL);
    
    if (window_handle == NULL)
    {
        ExitProcess(GetLastError());
    }
    
    // create a device context
    HDC const device_context = GetDC(window_handle);
    if (device_context == NULL)
    {
        ExitProcess(GetLastError());
    }
    
    // create an opengl context
    HGLRC const opengl_context = create_opengl_context(device_context);
    
    // load opengl extensions after creating an opengl context
    load_extensions();
    
    global_window = (Window) {
        .handle = window_handle,
        .device_context = device_context,
        .opengl_context = opengl_context,
        .width = width,
        .height = height,
        .scale = 1.0f,
        .smooth_scale = 0.5f,
        .max_iterations = 200,
    };
    
    // show and update window
    ShowWindow(window_handle, SW_SHOWDEFAULT);
    UpdateWindow(window_handle);
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
#if 0
    {
        int success;
        char info_log[512];
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        
        if(success == GL_FALSE)
        {
            glGetShaderInfoLog(fragment_shader, sizeof(info_log), 
                               NULL, info_log);
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
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
    
    // delete shaders after we are done linking
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return shader_program;
}

static float lerp(float v0, float v1, float t)
{
    return (1.0f - t) * v0 + t * v1;
}

#define SHADER_CODE(...) #__VA_ARGS__

int __cdecl main(void)
{
    create_window(L"mandelbrot", 800, 600);
    
    // if only c had raw string litreals
    char const *vertex_shader =
        "#version 330 core\n"
        SHADER_CODE(out vec2 uv;
                    void main(void) 
                    {
                        const vec2 p[4] = vec2[](vec2(-1, -1),
                                                 vec2(+1, -1),
                                                 vec2(-1, +1),
                                                 vec2(+1, +1));
                        const vec2 c[4] = vec2[](vec2(0, 0),
                                                 vec2(1, 0),
                                                 vec2(0, 1),
                                                 vec2(1, 1));
                        
                        uv = c[gl_VertexID];
                        gl_Position = vec4(p[gl_VertexID], 0.0, 1.0);
                    });
    
    char const *fragment_shader =
        "#version 330 core\n"
        "#define BAILOUT 200000.0f\n"
        SHADER_CODE(out vec4 fragcolor;
                    in vec2 uv;
                    uniform int max_iterations;
                    uniform float aspect;
                    uniform vec4 data;
                    void main(void)
                    {
                        vec2 c = ((uv * 3.0 - 1.5) * data[1] - data.zw)  * vec2(aspect, 1);
                        vec2 z = vec2(0);
                        
                        int i;
                        for(i = 0; i < max_iterations && dot(z, z) < BAILOUT; ++i)
                            z = vec2(z.x * z.x - z.y * z.y, z.x * z.y * 2.0) + c;
                        
                        float s = sqrt((i - log2(log(dot(z, z)) / log(BAILOUT))) / float(max_iterations));
                        vec3 cs = (sin(data[0] + 20.0 * s * vec3(1.5, 1.8, 2.1)) * 0.5 + 0.5) 
                            * float(i != max_iterations);
                        fragcolor = vec4(cs, 1);
                    });
    
    unsigned int shader_program = compile_shaders(vertex_shader, 
                                                  fragment_shader);
    // generate a vao
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    
    // bind the vao
    glBindVertexArray(vao);
    
    float color_offset = 0.0f;
    
    MSG msg;
    for(;;)
    {
        // process events and messages
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            
            if (msg.message ==  WM_QUIT) break;
        }
        
        else
        {
            
            // use the shader
            glUseProgram(shader_program);
            
            // pass uniforms
            glUniform1f(glGetUniformLocation(shader_program, "aspect"),
                        (float)global_window.width /
                        (float)global_window.height);
            glUniform4f(glGetUniformLocation(shader_program, "data"), color_offset, global_window.smooth_scale, 
                        global_window.smooth_pos[0], global_window.smooth_pos[1]);
            glUniform1i(glGetUniformLocation(shader_program, "max_iterations"),
                        global_window.max_iterations);
            
            // draw a quad
            glBindVertexArray(vao);
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
                global_window.scale *= 1.0f - 0.001f;
            }
            
            // see the above comment
            if(keys[KEY_MINUS1] || keys[KEY_MINUS2])
            {
                global_window.scale *= 1.0f + 0.001f;
            }
            
            if (keys[KEY_W])
            {
                global_window.pos[1] -= global_window.scale * 0.001f; 
            }
            
            if (keys[KEY_S])
            {
                global_window.pos[1] += global_window.scale * 0.001f; 
            }
            if (keys[KEY_A])
            {
                global_window.pos[0] += global_window.scale * 0.001f; 
            }
            
            if  (keys[KEY_D])
            {
                global_window.pos[0] -= global_window.scale * 0.001f; 
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
            
            if (keys[KEY_DOWN])
            {
                if(global_window.max_iterations > 2)
                {
                    global_window.max_iterations -= 1;
                }
            }
        }
    }
    
    ExitProcess(msg.message);
}
