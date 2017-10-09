/*
    ips.c

    Created by Dmitrii Toksaitov on Thu Aug 29 16:41:58 KGT 2013
*/

#pragma mark - Standard Includes

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "ips_utils.h"

#pragma mark - Dependencies

#include <GL/glew.h>

#include "SDL.h"
#include "SDL_opengl.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <zlib.h>
#include <png.h>

#include <pthread.h>

#pragma mark - Constants

static const char *Window_Title = "IPS";

static const int Initial_Window_Width  = 1280,
                 Initial_Window_Height = 800;

static const int OpenGL_Context_Major_Version = 2,
                 OpenGL_Context_Minor_Version = 1;

static const int Multisample_Enabled       = 1,
                 Multisample_Samples_Count = 8;

static const GLint First_Texture_Unit  = 0;

static char *Vertex_Shader_Path   = "ips_shader.glsl.vs",
            *Fragment_Shader_Path = "ips_shader.glsl.fs";

static const float Minimum_Image_Brightness = -100,
                   Maximum_Image_Brightness = 100;

static const float Minimum_Image_Contrast = 0.1f,
                   Maximum_Image_Contrast = 5.0f;

#pragma mark - Data Types

typedef struct ips_raw_image
{
    png_bytep data;
    png_bytepp rows;
    png_uint_32 width,
                height;
    float contrast,
          brightness;
} ips_raw_image_t;

typedef struct ips_task
{
    png_bytepp image_rows_to_process;
    png_bytepp final_image_rows;
    png_uint_32 image_rows_start_index;
    png_bytepp source_image_rows;
    png_uint_32 number_of_rows_to_process;
    png_uint_32 image_width,
                image_height;
    float new_image_contrast,
          new_image_brightness;

    struct ips_task *next_task;
} ips_task_t;

typedef struct ips_task_pool
{
    ips_task_t *first_task;
    ips_task_t *last_task;

    size_t size;
} ips_task_pool_t;

#pragma mark - Function Prototypes

void ips_init_gl_window();
void ips_init_gl();

GLuint ips_create_shader_program(char *vertex_shader_source,
                                 char *fragment_shader_source);
GLuint ips_link_shader_program(GLuint vertex_shader_object,
                               GLuint fragment_shader_object);
GLuint ips_compile_shader(char *shader_source, GLenum shader_type);
void ips_delete_shader_program(GLuint shader_program);

GLuint ips_generate_quad_geometry();

ips_raw_image_t *ips_load_image_from_png_file(char *png_file_path);
ips_raw_image_t *ips_duplicate_image(ips_raw_image_t *image);
void ips_delete_image(ips_raw_image_t *image);

void ips_update_image_data(ips_raw_image_t *image, float dt);
void ips_create_image_processing_task_pool();
void *ips_thread_process_image_part(void *args);

GLuint ips_create_texture_from_image(ips_raw_image *image);
void ips_update_texture_from_image(GLuint texture, ips_raw_image *image);
void ips_delete_texture(GLuint texture);

void ips_update_matrices(int new_window_width, int new_window_height);
void ips_render_quad(GLuint shader_program, GLuint vertex_array_object, GLuint texture);

void ips_respond_to_window_resize_event();
void ips_measure_and_show_frame_rate();

void ips_start(char *dropped_file_path);
void ips_stop();

#pragma mark - Globals

int current_window_width  = Initial_Window_Width,
    current_window_height = Initial_Window_Width;

static unsigned int frames = 0;
static unsigned int previous_timer_tick = 0;

static GLint position_attribute_location            = -1,
             normal_attribute_location              = -1,
             color_attribute_location               = -1,
             texture_coordinates_attribute_location = -1;

static GLint mvp_matrix_uniform_location      = -1,
             texture_sampler_uniform_location = -1;

static glm::mat4 model_matrix,
                 view_matrix,
                 projection_matrix,
                 model_view_projection_matrix;

static SDL_Window *program_window = NULL;
static SDL_GLContext gl_context;

static ips_task_pool_t *tasks = NULL; /* The pool of tasks for consumer threads */
static int number_of_threads = 0;

static ips_raw_image_t *source_image = NULL; /* Source image without adjustments */

/* Values that can be used to animate transitions between adjustments */

static float image_brightness = 0,
             image_contrast   = 1;

static float image_brightness_difference = 1,
             image_contrast_difference   = 0.01f;

static ips_task_pool_t *pool;

#pragma mark - Function Definitions

void ips_create_image_processing_task_pool()
{
    pool =
        (ips_task_pool_t*) malloc(sizeof(*pool));

    pool->first_task = NULL;
    pool->last_task  = NULL;
    pool->size = 0;

    number_of_threads =
        ips_utils_get_number_of_cpu_cores();

    for (int i = 0; i < number_of_threads; ++i) {
        // ToDo...
    }
}

void ips_update_image(ips_raw_image_t *source_image, ips_raw_image_t *image, float dt) /* producer tasks; called each time before rendering a frame */
{
    for (png_uint_32 y = 0; y < image->height; y += number_of_threads) {
        for (png_uint_32 i = 0; i < number_of_threads && ((y + i) < image->height); ++i) {
            png_uint_32 current_row_index =
                y + i;

            ips_task_t *task;
            task =
                (ips_task_t *) malloc(sizeof(*task));
            task->image_rows_to_process =
                source_image->rows + current_row_index;
            task->final_image_rows =
                image->rows + current_row_index;
            task->image_rows_start_index =
                current_row_index;
            task->new_image_contrast =
                2.0f;
            task->new_image_brightness =
                50.0f;
            task->image_width =
                image->width;
            task->image_height =
                image->height;
            task->number_of_rows_to_process =
                1;

            task->next_task = NULL;

            if (!pool->first_task) {
                pool->first_task = task;
            }

            if (pool->last_task) {
                pool->last_task->next_task =
                    task;
            }

            pool->last_task = task;
            pool->size++;
        }
    }
}

void ips_set_brightness_and_contrast(ips_task_t *task)
{
    png_uint_32 x, y;
    png_bytep pixel;

    int channel;

    for (y = 0; y < 1; ++y) {
        for (x = 0; x < task->image_width; ++x) {
            pixel = &(task->final_image_rows[y][x * 3]);
            for (channel = 0; channel < 3; ++channel) {
                float newValue =
                    task->new_image_contrast * pixel[channel] +
                        task->new_image_brightness;
                newValue =
                    ips_utils_clamp(newValue, 0, 255);

                pixel[channel] =
                    (png_byte) newValue;
            }
        }
    }
}

/*
    ToDo: add a method to apply a Sobel filter to an image
 */

void *ips_thread_process_image_part(void *args) /* image processing tasks for each consumer thread */
{
    ips_task_pool_t *pool =
        (ips_task_pool_t *) args;

    ips_task_t *task, *previous_task;

    while (pool->size != 0) {
        if (pool->size > 0) {
            task =
                pool->first_task;

            pool->first_task =
                task->next_task;
            pool->size--;

            if (!pool->size) {
                pool->last_task =
                    NULL;
            }

            ips_set_brightness_and_contrast(task);
        }
    }

    return NULL;
}

/* ----- */

void ips_init_gl_window()
{
    GLenum glew_initialization_status;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL initialization failure: \"%s\"\n", SDL_GetError());

        SDL_Quit();

        exit(EXIT_FAILURE);
    }

    program_window =
        SDL_CreateWindow(
            Window_Title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            Initial_Window_Width,
            Initial_Window_Height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        );
    if (!program_window) {
        fprintf(
            stderr,
            "Failed to create an SDL window: \"%s\"\n",
            SDL_GetError()
        );

        SDL_Quit();

        exit(EXIT_FAILURE);
    }

    ips_update_matrices(
        Initial_Window_Width,
        Initial_Window_Height
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MAJOR_VERSION,
        OpenGL_Context_Major_Version
    );
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MINOR_VERSION,
        OpenGL_Context_Minor_Version
    );

    SDL_GL_SetAttribute(
        SDL_GL_MULTISAMPLEBUFFERS,
        Multisample_Enabled
    );
    SDL_GL_SetAttribute(
        SDL_GL_MULTISAMPLESAMPLES,
        Multisample_Samples_Count
    );

    gl_context = SDL_GL_CreateContext(program_window);
    if (!gl_context) {
        fprintf(
            stderr,
            "Failed to create an OpengGL context: \"%s\"\n",
            SDL_GetError()
        );

        SDL_DestroyWindow(program_window);
        SDL_Quit();

        exit(EXIT_FAILURE);
    }

    glewExperimental = GL_TRUE;
    glew_initialization_status = glewInit();
    if (glew_initialization_status != GLEW_OK) {
        fprintf(
            stderr,
            "GLEW initialization failure: %s\n",
            glewGetErrorString(glew_initialization_status)
        );
    }

    if (SDL_GL_SetSwapInterval(-1) < 0) { /* change to SDL_GL_SetSwapInterval(0) for unbound framerate */
        SDL_GL_SetSwapInterval(1);
    }

    SDL_DisableScreenSaver();
}

void ips_init_gl()
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0);

    glViewport(
        0, 0,
        (GLsizei) Initial_Window_Width,
        (GLsizei) Initial_Window_Height
    );

    glUseProgram(0);
}

GLuint ips_create_shader_program(
           char *vertex_shader_source,
           char *fragment_shader_source
       )
{
    GLuint shader_program = 0;

    GLuint vertex_shader_object   = 0,
           fragment_shader_object = 0;

    if (!vertex_shader_source) {
        fprintf(
            stderr,
            "Failed to read a vertex shader file \"%s\"\n",
            Vertex_Shader_Path
        );

        return shader_program;
    }

    if (!fragment_shader_source) {
        fprintf(
            stderr,
            "Failed to read a fragment shader file \"%s\"\n",
            Fragment_Shader_Path
        );

        return shader_program;
    }

    vertex_shader_object =
        ips_compile_shader(vertex_shader_source, GL_VERTEX_SHADER);

    if (vertex_shader_object) {
        fragment_shader_object =
            ips_compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER);

        if (fragment_shader_object) {
            shader_program =
                ips_link_shader_program(
                    vertex_shader_object,
                    fragment_shader_object
                );
        }

        glDeleteShader(vertex_shader_object);
        glDeleteShader(fragment_shader_object);
    }

    if (shader_program) {
        position_attribute_location =
            glGetAttribLocation(shader_program, "position");
        normal_attribute_location =
            glGetAttribLocation(shader_program, "normal");
        color_attribute_location =
            glGetAttribLocation(shader_program, "color");
        texture_coordinates_attribute_location =
            glGetAttribLocation(shader_program, "texture_coordinates");

        mvp_matrix_uniform_location =
            glGetUniformLocation(shader_program, "model_view_projection_matrix");
        texture_sampler_uniform_location =
            glGetUniformLocation(shader_program, "texture_sampler");
    }

    return shader_program;
}

GLuint ips_compile_shader(
           char *shader_source,
           GLenum shader_type
       )
{
    GLint status;

    GLuint shader_object =
        glCreateShader(shader_type);

    glShaderSource(
        shader_object,
        1,
        (const GLchar**) &shader_source,
        NULL
    );
    glCompileShader(shader_object);

    glGetShaderiv(
        shader_object,
        GL_COMPILE_STATUS,
        &status
    );

    if (status == GL_FALSE) {
        GLint info_log_length;
        glGetShaderiv(
            shader_object,
            GL_INFO_LOG_LENGTH,
            &info_log_length
        );

        if (info_log_length > 0) {
            GLchar *info_log =
                (GLchar *) malloc((size_t) info_log_length);

            glGetShaderInfoLog(
                shader_object,
                info_log_length,
                NULL,
                info_log
            );

            fprintf(
                stderr,
                "Failed to compile a vertex shader \"%s\"\n",
                shader_type == GL_VERTEX_SHADER ?
                    Vertex_Shader_Path : Fragment_Shader_Path
            );
            fprintf(
                stderr,
                "Compilation log:\n%s\n\n",
                info_log
            );

            free(info_log);
        }

        glDeleteShader(shader_object);
        shader_object = 0;
    }

    return shader_object;
}

GLuint ips_link_shader_program(
           GLuint vertex_shader_object,
           GLuint fragment_shader_object
       )
{
    GLint status;

    GLuint shader_program =
        glCreateProgram();

    glAttachShader(
        shader_program,
        vertex_shader_object
    );
    glAttachShader(
        shader_program,
        fragment_shader_object
    );

    glLinkProgram(shader_program);

    glGetProgramiv(
        shader_program,
        GL_LINK_STATUS,
        &status
    );

    if (status == GL_FALSE) {
        GLint info_log_length;
        glGetProgramiv(
            shader_program,
            GL_INFO_LOG_LENGTH,
            &info_log_length
        );

        if (info_log_length > 0) {
            GLchar *info_log =
                (GLchar *) malloc((size_t) info_log_length);

            glGetProgramInfoLog(
                shader_program,
                info_log_length,
                NULL,
                info_log
            );

            fprintf(stderr, "Failed to link a GPU program\n");
            fprintf(stderr, "Linker log:\n%s\n\n", info_log);

            free(info_log);
        }

        shader_program = 0;
    }

    glDetachShader(
        shader_program,
        vertex_shader_object
    );
    glDetachShader(
        shader_program,
        fragment_shader_object
    );

    return shader_program;
}

void ips_delete_shader_program(
         GLuint shader_program
     )
{
    glDeleteProgram(shader_program);
}

GLuint ips_generate_quad_geometry()
{
    GLuint vertex_array_object  = 0,
           vertex_buffer_object = 0;

    GLfloat vertex_data[] = {
    //   Position           Color (RGBA),            Texture coordinates (UV)
        -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);

    glGenBuffers(1, &vertex_buffer_object);
    glBindBuffer(
        GL_ARRAY_BUFFER,
        vertex_buffer_object
    );
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertex_data),
        vertex_data,
        GL_STATIC_DRAW
    );

    GLsizei stride =
        sizeof(GLfloat) * 9;

    glEnableVertexAttribArray(position_attribute_location);
    glVertexAttribPointer(
        position_attribute_location, 3, GL_FLOAT, GL_FALSE,
        stride, (const GLvoid *) 0
    );

    glEnableVertexAttribArray(color_attribute_location);
    glVertexAttribPointer(
        color_attribute_location, 4, GL_FLOAT, GL_FALSE,
        stride, (const GLvoid *) (sizeof(GLfloat) * 3)
    );

    glEnableVertexAttribArray(texture_coordinates_attribute_location);
    glVertexAttribPointer(
        texture_coordinates_attribute_location, 2, GL_FLOAT, GL_FALSE,
        stride, (const GLvoid *) (sizeof(GLfloat) * 7)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vertex_array_object;
}

ips_raw_image_t *ips_load_image_from_png_file(
                     char *png_file_path
                 )
{
#define IPS_ERROR(MESSAGE)                     \
do {                                           \
    fprintf(stderr, "Error: %s\n", (MESSAGE)); \
    status = 0;                                \
    goto cleanup;                              \
} while (0)

    ips_raw_image_t *result;

    result = (ips_raw_image *) malloc(sizeof(*result));
    result->data = NULL;
    result->rows = NULL;
    result->width  = 0;
    result->height = 0;

    int status = 1;

    FILE *input_image_file = NULL;

    png_byte input_image_header[1];
    png_structp png_input_image_struct = NULL;

    png_infop png_input_image_info = NULL;
    png_uint_32 input_image_width,
                input_image_height;

    int input_image_bit_depth,
        input_image_color_type,
        input_image_interlace_type,
        input_image_compression_type,
        input_image_filter_type;

    png_bytep image_data  = NULL;
    png_bytepp image_rows = NULL;
    png_size_t image_row_size;

    png_uint_32 i;

    input_image_file = fopen(png_file_path, "rb");
    if (!input_image_file) {
        IPS_ERROR(
            "failed to open the input image"
        );
    }

    fread(input_image_header, 1, sizeof(input_image_header), input_image_file);
    if (png_sig_cmp(input_image_header, 0, sizeof(input_image_header))) {
        IPS_ERROR(
            "input file is not a valid PNG file"
        );
    }

    png_input_image_struct =
        png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL
        );

    if (!png_input_image_struct) {
        IPS_ERROR(
            "internal error: libpng: read structure was not created"
        );
    }

    png_input_image_info = png_create_info_struct(png_input_image_struct);
    if (!png_input_image_info) {
        IPS_ERROR(
            "internal error: libpng: info structure for the "
            "input file was not created"
        );
    }

    if (setjmp(png_jmpbuf(png_input_image_struct))) {
        IPS_ERROR(
            "failed to read the image"
        );
    }

    png_init_io(
        png_input_image_struct,
        input_image_file
    );
    png_set_sig_bytes(
        png_input_image_struct,
        sizeof(input_image_header)
    );

    png_read_info(
        png_input_image_struct,
        png_input_image_info
    );

    png_get_IHDR(
        png_input_image_struct,
        png_input_image_info,
        &input_image_width,
        &input_image_height,
        &input_image_bit_depth,
        &input_image_color_type,
        &input_image_interlace_type,
        &input_image_compression_type,
        &input_image_filter_type
    );

    if (input_image_interlace_type != PNG_INTERLACE_NONE) {
        IPS_ERROR(
            "interlaced images are not supported"
        );
    }

    if (input_image_color_type != PNG_COLOR_TYPE_RGB ||
            input_image_bit_depth  != 8) {
        IPS_ERROR(
            "only RGB 8-bit images can be processed"
        );
    }

#undef IPS_ERROR

    image_row_size =
        png_get_rowbytes(
            png_input_image_struct,
            png_input_image_info
        );

    image_data =
        (png_bytep) png_malloc(
                        png_input_image_struct,
                        input_image_height * image_row_size
                    );
    image_rows =
        (png_bytepp) png_malloc(
                         png_input_image_struct,
                         input_image_height * sizeof(png_bytep)
                     );

    for (i = 0; i < input_image_height; ++i) {
        image_rows[input_image_height - i - 1] =
            image_data + i * image_row_size;
    }

    png_read_image(
        png_input_image_struct,
        image_rows
    );

cleanup:

    if (status) {
        result->data =
            image_data;
        result->rows =
            image_rows;
        result->width =
            input_image_width;
        result->height =
            input_image_height;
    } else {
        if (result) {
            free(result);
            result = NULL;
        }

        if (image_data) {
            png_free(png_input_image_struct, image_data);
            image_data = NULL;
        }

        if (image_rows) {
            png_free(png_input_image_struct, image_rows);
            image_rows = NULL;
        }
    }

    fclose(input_image_file);
    input_image_file = NULL;

    if (input_image_file) {
        fclose(input_image_file);
        input_image_file = NULL;
    }

    if (png_input_image_info) {
        png_free_data(
            png_input_image_struct,
            png_input_image_info,
            PNG_FREE_ALL,
            -1
        );
    }

    if (png_input_image_struct) {
        png_destroy_read_struct(
            &png_input_image_struct,
            NULL, NULL
        );
    }

    return result;
}

ips_raw_image_t *ips_duplicate_image(
                     ips_raw_image_t *image
                 )
{
    ips_raw_image_t *duplicate = NULL;

    png_bytep data = NULL,
              duplicate_data = NULL;

    png_bytepp duplicate_rows = NULL;

    png_uint_32 i, width, height;
    size_t image_size, image_row_size;

    if (image) {
        duplicate = (ips_raw_image_t *) malloc(sizeof(*image));
        duplicate->data = NULL;
        duplicate->rows = NULL;
        duplicate->width  = 0;
        duplicate->height = 0;

        data = image->data;
        if (data) {
            width =
                image->width;
            height =
                image->height;
            image_row_size =
                width * sizeof(*data) * 3;
            image_size =
                height * image_row_size;

            duplicate_data = duplicate->data =
                (png_bytep) malloc(image_size);
            duplicate_rows = duplicate->rows =
                (png_bytepp) malloc(height * sizeof(*duplicate_rows));

            memcpy(duplicate_data, data, image_size);
            for (i = 0; i < height; ++i) {
                duplicate_rows[height - i - 1] =
                    duplicate_data + i * image_row_size;
            }

            duplicate->width =
                width;
            duplicate->height =
                height;
        }
    }

    return duplicate;
}

void ips_delete_image(
         ips_raw_image_t *image
     )
{
    if (image) {
        if (image->data) {
            free(image->data);
            image->data = NULL;
        }

        if (image->rows) {
            free(image->rows);
            image->rows = NULL;
        }

        free(image);
    }
}

GLuint ips_create_texture_from_image(
           ips_raw_image *image
       )
{
    GLuint texture = 0;

    if (image) {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB,
            (GLsizei) image->width,
            (GLsizei) image->height,
            0, GL_RGB, GL_UNSIGNED_BYTE,
            (GLvoid *) image->data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return texture;
}

void ips_update_texture_from_image(
         GLuint texture,
         ips_raw_image *image
     )
{
    if (texture && image) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, image->width,
            image->height,
            GL_RGB, GL_UNSIGNED_BYTE,
            image->data
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void ips_delete_texture(
         GLuint texture
     )
{
    glDeleteTextures(1, &texture);
}

void ips_update_matrices(
         int new_window_width,
         int new_window_height
     )
{
    model_matrix =
        glm::mat4(1.0f);
    view_matrix =
        glm::lookAt<float>(
            glm::vec3(0, 0, -1),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );

    float aspect_ration =
        fabsf(
            (float) new_window_width /
                (float) new_window_height
        );
    float zoom = 1.2f;

    projection_matrix =
        glm::ortho<float>(
            -(zoom * aspect_ration), zoom * aspect_ration,
            -zoom, zoom,
            0.1f, 100
        );

    model_view_projection_matrix =
        projection_matrix * view_matrix * model_matrix;
}

void ips_render_quad(
         GLuint shader_program,
         GLuint vertex_array_object,
         GLuint texture
     )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vertex_array_object);

    if (texture && texture_sampler_uniform_location != -1) {
        glActiveTexture(GL_TEXTURE0 + First_Texture_Unit);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(
            texture_sampler_uniform_location,
            First_Texture_Unit
        );
    }

    glUniformMatrix4fv(
        mvp_matrix_uniform_location,
        1, GL_FALSE,
        glm::value_ptr(model_view_projection_matrix)
    );

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    SDL_GL_SwapWindow(program_window);

    ++frames;
}

void ips_respond_to_window_resize_event()
{
    SDL_GetWindowSize(
        program_window,
        &current_window_width,
        &current_window_height
    );

    ips_update_matrices(
        current_window_width,
        current_window_height
    );

    glViewport(
        0, 0,
        (GLsizei) current_window_width,
        (GLsizei) current_window_height
    );
}

void ips_measure_and_show_frame_rate()
{
    char *title_format =
        "%s: %d X %d at %.2f FPS";
    char *title =
        (char *) malloc(
                     strlen(title_format) +
                         strlen(Window_Title) + 512
                 );

    unsigned int timer_tick =
        SDL_GetTicks();

    float frame_rate =
        1000.0f / (timer_tick - previous_timer_tick);
    previous_timer_tick =
        timer_tick;

    sprintf(
        title,
        title_format,
        Window_Title,
        current_window_width,
        current_window_height,
        frame_rate
    );

    SDL_SetWindowTitle(
        program_window,
        title
    );

    free(title);
}

void ips_start(char *dropped_file_path)
{
    SDL_Event event;
    float dt; int timer_tick, should_measure_and_show_frame_rate;

    ips_raw_image *image = NULL;

    GLuint shader_program      = 0,
           texture             = 0,
           vertex_array_object = 0;

    ips_init_gl_window();
    ips_init_gl();

    shader_program =
        ips_create_shader_program(
            ips_utils_read_text_file(Vertex_Shader_Path),
            ips_utils_read_text_file(Fragment_Shader_Path)
        );

    vertex_array_object =
        ips_generate_quad_geometry();

    ips_create_image_processing_task_pool();

    for (;;) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    ips_respond_to_window_resize_event();
                }
            } else if (event.type == SDL_DROPFILE) {
                dropped_file_path = event.drop.file;
            } else if (event.type == SDL_QUIT) {
                ips_stop();
            }
        }

        if (dropped_file_path) {
            ips_raw_image *new_image;
            if ((new_image = ips_load_image_from_png_file(dropped_file_path))) {
                ips_delete_image(source_image);
                source_image =
                    new_image;

                ips_delete_image(image);
                image =
                    ips_duplicate_image(source_image);

                ips_delete_texture(texture);
                texture =
                    ips_create_texture_from_image(image);
            }

            SDL_free(dropped_file_path);
            dropped_file_path = NULL;
        }

        timer_tick = SDL_GetTicks();
        dt = (timer_tick - previous_timer_tick) / 1000.0f;
        previous_timer_tick = timer_tick;

        if (source_image && image) {
            ips_update_image(source_image, image, dt);
            ips_thread_process_image_part(pool);
            ips_update_texture_from_image(texture, image);
            image = NULL;
        }

        ips_render_quad(
            shader_program,
            vertex_array_object,
            texture
        );

        should_measure_and_show_frame_rate = frames % 240 == 0;
        if (should_measure_and_show_frame_rate) {
            ips_measure_and_show_frame_rate();
        }
    }

    ips_delete_image(source_image);
    source_image = NULL;

    ips_delete_image(image);
    image = NULL;

    ips_delete_texture(texture);
    texture = 0;
}

void ips_stop()
{
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(program_window);
    SDL_EnableScreenSaver();
    SDL_Quit();

#if defined _WIN32 && defined PTW32_STATIC_LIB
    pthread_win32_process_detach_np();
#endif

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    char *image_file_path = NULL; size_t length = 0;
    if (argc > 1) {
        length = strlen(argv[1]);
        image_file_path =
            (char *) SDL_malloc(
                         sizeof(*image_file_path) * (length + 1)
                     );
        strncpy(image_file_path, argv[1], length);
        image_file_path[length] = '\0';
    }

#if defined _WIN32 && defined PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
#endif

    ips_start(image_file_path);

#if defined _WIN32 && defined PTW32_STATIC_LIB
    pthread_win32_process_detach_np();
#endif

    return 0;
}

