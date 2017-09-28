#   include "em_movieplayer.h"

#   include "movie/movie.h"

#   include <stdint.h>
#   include <malloc.h>
#   include <math.h>
#   include <string.h>

#   include <GLES2/gl2.h>

#   include <SDL/SDL_image.h>

//////////////////////////////////////////////////////////////////////////
typedef struct em_render_vertex_t
{
    float position[3];
    uint32_t color;
    float uv[2];
} em_render_vertex_t;
//////////////////////////////////////////////////////////////////////////
typedef uint16_t em_render_index_t;
//////////////////////////////////////////////////////////////////////////
static const char * __opengl_get_error_string( GLenum _err )
{
    switch( _err )
    {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        {
        }
    }

    return "GL_UNKNOWN";
}
//////////////////////////////////////////////////////////////////////////
static void __opengl_error_check( const char * _method, const char * _file, int _line )
{
    GLenum err = glGetError();

    if( err == GL_NO_ERROR )
    {
        return;
    }

    const char * err_str = __opengl_get_error_string( err );

    emscripten_log( EM_LOG_CONSOLE, "opengl error method '%s' file '%s:%d' error '%s:%d'"
        , _method
        , _file
        , _line
        , err_str
        , err
    );
}
//////////////////////////////////////////////////////////////////////////
#   define GLCALL( Method, Args )\
    {\
        Method Args;\
        __opengl_error_check( #Method, __FILE__, __LINE__ );\
    }
//////////////////////////////////////////////////////////////////////////
#   define GLCALLR( R, Method, Args )\
    {\
        R = Method Args;\
        __opengl_error_check( #Method, __FILE__, __LINE__ );\
    }
//////////////////////////////////////////////////////////////////////////
static const ae_voidptr_t em_nullptr = 0;
//////////////////////////////////////////////////////////////////////////
typedef struct em_player_t
{
    aeMovieInstance * instance;

    float width;
    float height;

    GLuint program;

    GLint inVertLocation;
    GLint inColLocation;
    GLint inUVLocation;

    GLint viewMatrixLocation;
    GLint projectionMatrixLocation;

    GLint samplerLocation;

    em_render_vertex_t vertices[1024];

} em_player_t;
//////////////////////////////////////////////////////////////////////////
static void * __instance_alloc( void * _data, size_t _size )
{
    (void)_data;

    return malloc( _size );
}
//////////////////////////////////////////////////////////////////////////
static void * __instance_alloc_n( void * _data, size_t _size, size_t _count )
{
    (void)_data;

    size_t total = _size * _count;

    return malloc( total );
}
//////////////////////////////////////////////////////////////////////////
static void __instance_free( void * _data, const void * _ptr )
{
    (void)_data;

    free( (void *)_ptr );
}
//////////////////////////////////////////////////////////////////////////
static void __instance_free_n( void * _data, const void * _ptr )
{
    (void)_data;

    free( (void *)_ptr );
}
//////////////////////////////////////////////////////////////////////////
static void __instance_logerror( void * _data, aeMovieErrorCode _code, const char * _format, ... )
{
    (void)_data;
    (void)_code;
    (void)_format;
}
//////////////////////////////////////////////////////////////////////////
em_player_handle_t em_create_player( const char * _hashkey, float _width, float _height )
{
    em_player_t * em_player = malloc( sizeof( em_player_t ) );

    aeMovieInstance * ae_instance = ae_create_movie_instance( _hashkey
        , &__instance_alloc
        , &__instance_alloc_n
        , &__instance_free
        , &__instance_free_n
        , AE_NULL
        , &__instance_logerror
        , AE_NULL );

    em_player->instance = ae_instance;

    em_player->width = _width;
    em_player->height = _height;

    IMG_Init( IMG_INIT_PNG );

    GLuint fragmentShaderId;
    GLCALLR( fragmentShaderId, glCreateShader, (GL_FRAGMENT_SHADER) );

    if( fragmentShaderId == 0 )
    {
        return em_nullptr;
    }

    const char * fragmentShaderSource =
        "uniform sampler2D inSampler0;\n"
        "varying lowp vec4 v_Col;\n"
        "varying mediump vec2 v_UV;\n"        
        "void main( void ) {\n"
        "mediump vec4 c = v_Col * texture2D( inSampler0, v_UV );\n"
        "gl_FragColor = c;\n"
        "}\n";

    GLint fragmentShaderSourceSize = (GLint)strlen( fragmentShaderSource );

    GLCALL( glShaderSource, (fragmentShaderId, 1, &fragmentShaderSource, &fragmentShaderSourceSize) );
    GLCALL( glCompileShader, (fragmentShaderId) );

    GLint fragmentShaderStatus;
    GLCALL( glGetShaderiv, (fragmentShaderId, GL_COMPILE_STATUS, &fragmentShaderStatus) );

    if( fragmentShaderStatus == GL_FALSE )
    {
        GLchar errorLog[1024];
        GLCALL( glGetShaderInfoLog, (fragmentShaderId, sizeof( errorLog ) - 1, NULL, errorLog) );

        emscripten_log( EM_LOG_CONSOLE, "opengl compilation fragment shader error '%s'\n"
            , errorLog
            );

        return em_nullptr;
    }

    GLuint vertexShaderId;
    GLCALLR( vertexShaderId, glCreateShader, (GL_VERTEX_SHADER) );

    if( vertexShaderId == 0 )
    {
        return em_nullptr;
    }

    const char * vertexShaderSource =
        "uniform highp mat4 viewMatrix;\n"
        "uniform highp mat4 projectionMatrix;\n"
        "attribute highp vec4 inVert;\n"
        "attribute lowp vec4 inCol;\n"
        "attribute mediump vec2 inUV;\n"        
        "varying lowp vec4 v_Col;\n"
        "varying mediump vec2 v_UV;\n"
        "void main( void )\n"
        "{\n"
        "   gl_Position = projectionMatrix * viewMatrix * inVert;\n"
        "   v_Col = inCol;\n"
        "   v_UV = inUV;\n"
        "}\n";

    GLint vertexShaderSourceSize = (GLint)strlen( vertexShaderSource );

    GLCALL( glShaderSource, (vertexShaderId, 1, &vertexShaderSource, &vertexShaderSourceSize) );
    GLCALL( glCompileShader, (vertexShaderId) );

    GLint vertexShaderStatus;
    GLCALL( glGetShaderiv, (vertexShaderId, GL_COMPILE_STATUS, &vertexShaderStatus) );

    if( vertexShaderStatus == GL_FALSE )
    {
        GLchar errorLog[1024];
        GLCALL( glGetShaderInfoLog, (vertexShaderId, sizeof( errorLog ) - 1, NULL, errorLog) );

        emscripten_log( EM_LOG_CONSOLE, "opengl compilation fragment shader error '%s'\n"
            , errorLog
        );

        return em_nullptr;
    }

    GLuint program;
    GLCALLR( program, glCreateProgram, () );

    if( program == 0 )
    {
        return em_nullptr;
    }

    GLCALL( glAttachShader, (program, vertexShaderId) );
    GLCALL( glAttachShader, (program, fragmentShaderId) );

    GLCALL( glLinkProgram, (program) );

    GLint programLinkStatus;
    GLCALL( glGetProgramiv, (program, GL_LINK_STATUS, &programLinkStatus) );

    if( programLinkStatus == GL_FALSE )
    {
        GLchar errorLog[1024] = { 0 };
        GLCALL( glGetProgramInfoLog, (program, sizeof( errorLog ) - 1, NULL, errorLog) );

        emscripten_log( EM_LOG_CONSOLE, "opengl program linking error '%s'\n"
            , errorLog
            );

        return em_nullptr;
    }

    GLCALL( glDeleteShader, (vertexShaderId) );
    GLCALL( glDeleteShader, (fragmentShaderId) );

    int inVertLocation;
    GLCALLR( inVertLocation, glGetAttribLocation, (program, "inVert") );

    emscripten_log( EM_LOG_CONSOLE, "opengl attrib inVert '%d'\n"
        , inVertLocation
    );

    int inColLocation;
    GLCALLR( inColLocation, glGetAttribLocation, (program, "inCol") );

    emscripten_log( EM_LOG_CONSOLE, "opengl attrib inCol '%d'\n"
        , inColLocation
    );

    int inUVLocation;
    GLCALLR( inUVLocation, glGetAttribLocation, (program, "inUV") );

    emscripten_log( EM_LOG_CONSOLE, "opengl attrib inUV '%d'\n"
        , inUVLocation
    );
    
    int viewMatrixLocation;
    GLCALLR( viewMatrixLocation, glGetUniformLocation, (program, "viewMatrix") );
        
    emscripten_log( EM_LOG_CONSOLE, "opengl uniform viewMatrix '%d'\n"
        , viewMatrixLocation
    );

    int projectionMatrixLocation;
    GLCALLR( projectionMatrixLocation, glGetUniformLocation, (program, "projectionMatrix") );

    emscripten_log( EM_LOG_CONSOLE, "opengl uniform projectionMatrix '%d'\n"
        , projectionMatrixLocation
    );
    
    int samplerLocation;
    GLCALLR( samplerLocation, glGetUniformLocation, (program, "inSampler0") );

    emscripten_log( EM_LOG_CONSOLE, "opengl uniform inSampler0 '%d'\n"
        , samplerLocation
    );

    em_player->program = program;
    em_player->inVertLocation = inVertLocation;
    em_player->inColLocation = inColLocation;
    em_player->inUVLocation = inUVLocation;
    em_player->viewMatrixLocation = viewMatrixLocation;
    em_player->projectionMatrixLocation = projectionMatrixLocation;
    em_player->samplerLocation = samplerLocation;

    emscripten_log( EM_LOG_CONSOLE, "successful create player hashkey '%s' width '%f' height '%f'"
        , _hashkey
        , _width
        , _height
    );

    return em_player;
}
//////////////////////////////////////////////////////////////////////////
void em_delete_player( em_player_handle_t _player )
{
    em_player_t * em_player = (em_player_t *)_player;
    
    ae_delete_movie_instance( em_player->instance );

    free( em_player );

    emscripten_log( EM_LOG_CONSOLE, "successful delete movie instance" );
}
//////////////////////////////////////////////////////////////////////////
static ae_size_t __read_file( ae_voidptr_t _data, ae_voidptr_t _buff, ae_uint32_t _carriage, ae_uint32_t _size ) 
{
    (void)_carriage;

    FILE * f = (FILE *)_data;

    ae_size_t s = fread( _buff, 1, _size, f );

    //emscripten_log( EM_LOG_CONSOLE, "read_file %u", _size );

    return s;
}
//////////////////////////////////////////////////////////////////////////
static void __memory_copy( ae_voidptr_t _data, ae_constvoidptr_t _src, ae_voidptr_t _dst, ae_size_t _size ) 
{
    (void)_data;
    memcpy( _dst, _src, _size );

    //emscripten_log( EM_LOG_CONSOLE, "memory_copy %u", _size );
}
//////////////////////////////////////////////////////////////////////////
typedef struct em_resource_image_data
{
    GLuint texture_id;
} em_resource_image_data;
//////////////////////////////////////////////////////////////////////////
static ae_voidptr_t __data_resource_provider( const aeMovieResource * _resource, ae_voidptr_t _ud ) 
{
    (void)_resource;
    (void)_ud;

    switch( _resource->type ) 
    {
    case AE_MOVIE_RESOURCE_IMAGE:
        {
            const aeMovieResourceImage * r = (const aeMovieResourceImage *)_resource;

            emscripten_log( EM_LOG_CONSOLE, "Resource type: image.\n" );
            emscripten_log( EM_LOG_CONSOLE, " path        = '%s'\n", r->path );
            emscripten_log( EM_LOG_CONSOLE, " trim_width  = %f\n", r->trim_width );
            emscripten_log( EM_LOG_CONSOLE, " trim_height = %f\n", r->trim_height );

            em_resource_image_data * em_resource = ( em_resource_image_data * )malloc( sizeof( em_resource_image_data ) );

            SDL_Surface * surface = IMG_Load( r->path );

            if( surface == NULL )
            {
                emscripten_log( EM_LOG_CONSOLE, "bad load image '%s'\n", r->path );
            }

            GLuint texture_id;
            GLCALL( glGenTextures, (1, &texture_id) );
            GLCALL( glBindTexture, (GL_TEXTURE_2D, texture_id) );
            GLCALL( glTexParameteri, (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE) );
            GLCALL( glTexParameteri, (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE) );
            GLCALL( glTexParameteri, (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
            GLCALL( glTexParameteri, (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );

            GLCALL( glTexImage2D, (GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels) );

            SDL_FreeSurface( surface );
            
            em_resource->texture_id = texture_id;

            emscripten_log( EM_LOG_CONSOLE, "create texture %ux%u (%u) id '%d'\n"
                , surface->w
                , surface->h
                , surface->format->BytesPerPixel
                , texture_id 
            );

            return em_resource;

            break;
        }
    case AE_MOVIE_RESOURCE_SEQUENCE:
        {
            emscripten_log( EM_LOG_CONSOLE, "Resource type: image sequence.\n" );

            break;
        }
    case AE_MOVIE_RESOURCE_VIDEO:
        {
            //			const aeMovieResourceVideo * r = (const aeMovieResourceVideo *)_resource;

            emscripten_log( EM_LOG_CONSOLE, "Resource type: video.\n" );

            break;
        }
    case AE_MOVIE_RESOURCE_SOUND:
        {
            const aeMovieResourceSound * r = (const aeMovieResourceSound *)_resource;

            emscripten_log( EM_LOG_CONSOLE, "Resource type: sound.\n" );
            emscripten_log( EM_LOG_CONSOLE, " path        = '%s'", r->path );

            break;
        }
    case AE_MOVIE_RESOURCE_SLOT:
        {
            const aeMovieResourceSlot * r = (const aeMovieResourceSlot *)_resource;

            emscripten_log( EM_LOG_CONSOLE, "Resource type: slot.\n" );
            emscripten_log( EM_LOG_CONSOLE, " width  = %.2f\n", r->width );
            emscripten_log( EM_LOG_CONSOLE, " height = %.2f\n", r->height );

            break;
        }
    default:
        {
            emscripten_log( EM_LOG_CONSOLE, "Resource type: other (%i).\n", _resource->type );
            break;
        }
    }

    return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
static void __data_resource_deleter( aeMovieResourceTypeEnum _type, const ae_voidptr_t * _data, ae_voidptr_t _ud )
{    
    (void)_data;
    (void)_ud;

    switch( _type )
    {
    case AE_MOVIE_RESOURCE_IMAGE:
        {
            em_resource_image_data * image_data = (em_resource_image_data *)_data;

            glDeleteTextures( 1, &image_data->texture_id );

            free( (ae_voidptr_t *)_data );

            break;
        }
    case AE_MOVIE_RESOURCE_SEQUENCE:
        {
            break;
        }
    case AE_MOVIE_RESOURCE_VIDEO:
        {
            break;
        }
    case AE_MOVIE_RESOURCE_SOUND:
        {
            break;
        }
    case AE_MOVIE_RESOURCE_SLOT:
        {
            break;
        }
    default:
        {            
            break;
        }
    }
}
//////////////////////////////////////////////////////////////////////////
em_movie_data_handle_t em_create_movie_data( em_player_handle_t _player, const char * _path )
{
    em_player_t * em_instance = (em_player_t *)_player;

    aeMovieData * ae_movie_data = ae_create_movie_data( em_instance->instance, &__data_resource_provider, &__data_resource_deleter, AE_NULL );
    
    FILE * f = fopen( _path, "rb" );

    if( f == NULL )
    {
        emscripten_log( EM_LOG_CONSOLE, "invalid open file (%s)", _path );

        return AE_NULL;
    }

    fseek( f, 0L, SEEK_END );
    size_t sz = ftell( f );
    rewind( f );

    ae_voidptr_t * buffer = malloc( sz );
    fread( buffer, sz, 1, f );
    fclose( f );

    aeMovieStream * ae_stream = ae_create_movie_stream_memory( em_instance->instance, buffer, &__memory_copy, AE_NULL );

    if( ae_load_movie_data( ae_movie_data, ae_stream ) == AE_MOVIE_FAILED )
    {
        emscripten_log( EM_LOG_CONSOLE, "WRONG LOAD (%s)", _path );
    }

    ae_delete_movie_stream( ae_stream );

    free( buffer );

    emscripten_log( EM_LOG_CONSOLE, "successful create movie data from file '%s'", _path );

    return ae_movie_data;
}
//////////////////////////////////////////////////////////////////////////
void em_delete_movie_data( em_movie_data_handle_t _movieData )
{
    aeMovieData * ae_movie_data = (aeMovieData *)_movieData;

    ae_delete_movie_data( ae_movie_data );

    emscripten_log( EM_LOG_CONSOLE, "successful delete movie data" );
}
//////////////////////////////////////////////////////////////////////////
typedef struct ae_camera_t
{
    float view[16];
    float projection[16];
    float width;
    float height;
} ae_camera_t;
//////////////////////////////////////////////////////////////////////////
static float vector3_sqrlength( const float * _v )
{
    return	_v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2];
}
//////////////////////////////////////////////////////////////////////////
static float vector3_length( const float * _v )
{
    float sqrlen = vector3_sqrlength( _v );
    float len = sqrtf( sqrlen );

    return len;
}
//////////////////////////////////////////////////////////////////////////
static void vector3_normalize( float * _out, const float * _v )
{
    float len = vector3_length( _v );

    float inv_len = 1.f / len;

    _out[0] = _v[0] * inv_len;
    _out[1] = _v[1] * inv_len;
    _out[2] = _v[2] * inv_len;
}
//////////////////////////////////////////////////////////////////////////
static void vector3_negative( float * _out, const float * _v )
{
    _out[0] = -_v[0];
    _out[1] = -_v[1];
    _out[2] = -_v[2];
}
//////////////////////////////////////////////////////////////////////////
static void vector3_cross( float * _out, const float * _a, const float * _b )
{
    _out[0] = _a[1] * _b[2] - _a[2] * _b[1];
    _out[1] = _a[2] * _b[0] - _a[0] * _b[2];
    _out[2] = _a[0] * _b[1] - _a[1] * _b[0];
}
//////////////////////////////////////////////////////////////////////////
static float vector3_dot( const float * _a, const float * _b )
{
    return _a[0] * _b[0] + _a[1] * _b[1] + _a[2] * _b[2];
}
//////////////////////////////////////////////////////////////////////////
static void __make_lookat_m4( float * _out, const float * _position, const float * _target )
{
    ae_vector3_t direction;
    direction[0] = _target[0] - _position[0];
    direction[1] = _target[1] - _position[1];
    direction[2] = _target[2] - _position[2];

    ae_vector3_t zaxis;
    vector3_normalize( zaxis, direction );

    ae_vector3_t up = { 0.f, -1.f, 0.f };

    ae_vector3_t xaxis;
    vector3_cross( xaxis, up, zaxis );
    vector3_normalize( xaxis, xaxis );
    
    ae_vector3_t yaxis;
    vector3_cross( yaxis, zaxis, xaxis );

    vector3_negative( xaxis, xaxis );

    _out[0] = xaxis[0];
    _out[1] = yaxis[0];
    _out[2] = zaxis[0];
    _out[3] = 0.f;

    _out[4] = xaxis[1];
    _out[5] = yaxis[1];
    _out[6] = zaxis[1];
    _out[7] = 0.f;

    _out[8] = xaxis[2];
    _out[9] = yaxis[2];
    _out[10] = zaxis[2];
    _out[11] = 0.f;

    _out[12] = -vector3_dot( xaxis, _position );
    _out[13] = -vector3_dot( yaxis, _position );
    _out[14] = -vector3_dot( zaxis, _position );
    _out[15] = 1.f;
}
//////////////////////////////////////////////////////////////////////////
static void __make_projection_fov_m4( float * _out, float _width, float _height, float _fov )
{
    float aspect = _width / _height;

    float zn = 1.f;
    float zf = 10000.f;

    float h = 1.f / tanf( _fov * 0.5f );
    float w = h / aspect;

    float a = zf / (zf - zn);
    float b = -zn * zf / (zf - zn);

    _out[0] = w;
    _out[1] = 0.f;
    _out[2] = 0.f;
    _out[3] = 0.f;

    _out[4] = 0.f;
    _out[5] = h;
    _out[6] = 0.f;
    _out[7] = 0.f;

    _out[8] = 0.f;
    _out[9] = 0.f;
    _out[10] = a;
    _out[11] = 1.f;

    _out[12] = 0.f;
    _out[13] = 0.f;
    _out[14] = b;
    _out[15] = 0.f;
}
//////////////////////////////////////////////////////////////////////////
static void * ae_movie_callback_camera_provider( const aeMovieCameraProviderCallbackData * _callbackData, void * _data )
{
    (void)_data;

    ae_camera_t * camera = malloc( sizeof( ae_camera_t ) );

    __make_lookat_m4( camera->view, _callbackData->position, _callbackData->target );
    __make_projection_fov_m4( camera->projection, _callbackData->width, _callbackData->height, _callbackData->fov );

    camera->width = _callbackData->width;
    camera->height = _callbackData->height;

    return camera;
}
//////////////////////////////////////////////////////////////////////////
static void ae_movie_callback_camera_deleter( const aeMovieCameraDestroyCallbackData * _callbackData, ae_voidptr_t _data )
{
    (void)_data;

    ae_camera_t * camera = (ae_camera_t *)_callbackData->element;

    free( camera );
}
//////////////////////////////////////////////////////////////////////////
static void ae_movie_callback_camera_update( const aeMovieCameraUpdateCallbackData * _callbackData, void * _data )
{
    (void)_data;

    ae_camera_t * camera = (ae_camera_t *)_callbackData->element;

    __make_lookat_m4( camera->view, _callbackData->position, _callbackData->target );
}
//////////////////////////////////////////////////////////////////////////
typedef struct em_shader_t
{
    GLuint program;

    uint32_t parameter_count;
    const char * parameter_names[32];
    GLint parameter_locations[32];    

    GLint mvpMatrixLocation;
    GLint tex0Location;
} em_shader_t;
//////////////////////////////////////////////////////////////////////////
static ae_voidptr_t ae_movie_callback_shader_provider( const aeMovieShaderProviderCallbackData * _callbackData, ae_voidptr_t _data )
{
    em_shader_t * shader = malloc( sizeof( em_shader_t ) );

    GLuint fragmentShaderId;
    GLCALLR( fragmentShaderId, glCreateShader, (GL_FRAGMENT_SHADER) );

    if( fragmentShaderId == 0 )
    {
        return em_nullptr;
    }

    const char * fragmentShaderSource = _callbackData->shader_fragment;

    GLint fragmentShaderSourceSize = (GLint)strlen( fragmentShaderSource );

    GLCALL( glShaderSource, (fragmentShaderId, 1, &fragmentShaderSource, &fragmentShaderSourceSize) );
    GLCALL( glCompileShader, (fragmentShaderId) );

    GLint fragmentShaderStatus;
    GLCALL( glGetShaderiv, (fragmentShaderId, GL_COMPILE_STATUS, &fragmentShaderStatus) );

    if( fragmentShaderStatus == GL_FALSE )
    {
        GLchar errorLog[1024];
        GLCALL( glGetShaderInfoLog, (fragmentShaderId, sizeof( errorLog ) - 1, NULL, errorLog) );

        emscripten_log( EM_LOG_CONSOLE, "opengl compilation fragment shader '%s' version '%d' error '%s'\n"
            , _callbackData->name
            , _callbackData->version
            , errorLog
        );

        return em_nullptr;
    }

    GLuint vertexShaderId;
    GLCALLR( vertexShaderId, glCreateShader, (GL_VERTEX_SHADER) );

    if( vertexShaderId == 0 )
    {
        return em_nullptr;
    }

    const char * vertexShaderSource = _callbackData->shader_vertex;

    GLint vertexShaderSourceSize = (GLint)strlen( vertexShaderSource );

    GLCALL( glShaderSource, (vertexShaderId, 1, &vertexShaderSource, &vertexShaderSourceSize) );
    GLCALL( glCompileShader, (vertexShaderId) );

    GLint vertexShaderStatus;
    GLCALL( glGetShaderiv, (vertexShaderId, GL_COMPILE_STATUS, &vertexShaderStatus) );

    if( vertexShaderStatus == GL_FALSE )
    {
        GLchar errorLog[1024];
        GLCALL( glGetShaderInfoLog, (vertexShaderId, sizeof( errorLog ) - 1, NULL, errorLog) );

        emscripten_log( EM_LOG_CONSOLE, "opengl compilation vertex shader '%s' version '%d' error '%s'\n"
            , _callbackData->name
            , _callbackData->version
            , errorLog
        );

        return em_nullptr;
    }

    GLuint program;
    GLCALLR( program, glCreateProgram, () );

    if( program == 0 )
    {
        return em_nullptr;
    }

    GLCALL( glAttachShader, (program, vertexShaderId) );
    GLCALL( glAttachShader, (program, fragmentShaderId) );

    GLCALL( glLinkProgram, (program) );

    GLint programLinkStatus;
    GLCALL( glGetProgramiv, (program, GL_LINK_STATUS, &programLinkStatus) );

    if( programLinkStatus == GL_FALSE )
    {
        GLchar errorLog[1024] = { 0 };
        GLCALL( glGetProgramInfoLog, (program, sizeof( errorLog ) - 1, NULL, errorLog) );

        emscripten_log( EM_LOG_CONSOLE, "opengl program '%s' version '%d' linking error '%s'\n"
            , _callbackData->name
            , _callbackData->version
            , errorLog
        );

        return em_nullptr;
    }

    GLCALL( glDeleteShader, (vertexShaderId) );
    GLCALL( glDeleteShader, (fragmentShaderId) );

    shader->program = program;

    shader->parameter_count = _callbackData->parameter_count;

    for( uint32_t i = 0; i != _callbackData->parameter_count; ++i )
    {
        const char * parameter_name = _callbackData->parameter_names[i];

        GLint parameter_location;
        GLCALLR( parameter_location, glGetAttribLocation, (program, parameter_name) );

        shader->parameter_names[i] = parameter_name;
        shader->parameter_locations[i] = parameter_location;

        emscripten_log( EM_LOG_CONSOLE, "opengl attrib '%s' location '%d'\n"
            , parameter_name
            , parameter_location
        );
    }

    int mvpMatrixLocation;
    GLCALLR( mvpMatrixLocation, glGetUniformLocation, (program, "g_mvpMatrix") );

    emscripten_log( EM_LOG_CONSOLE, "opengl uniform mvpMatrix '%d'\n"
        , mvpMatrixLocation
    );

    shader->mvpMatrixLocation = mvpMatrixLocation;

    int tex0Location;
    GLCALLR( tex0Location, glGetUniformLocation, (program, "g_tex0") );

    emscripten_log( EM_LOG_CONSOLE, "opengl uniform tex0 '%d'\n"
        , tex0Location
    );

    shader->tex0Location = tex0Location;

    return shader;
}
//////////////////////////////////////////////////////////////////////////
static void ae_movie_callback_shader_property_update( const aeMovieShaderPropertyUpdateCallbackData * _callbackData, ae_voidptr_t _data )
{
    em_shader_t * shader = (em_shader_t *)_callbackData->element;

    GLint location = shader->parameter_locations[_callbackData->index];

    switch( _callbackData->type )
    {
    case 3:
        {            
            GLCALL( glUniform1f, (location, _callbackData->value) );
        }break;
    case 5:
        {
            GLCALL( glUniform3f, ( location, _callbackData->color_r, _callbackData->color_g, _callbackData->color_b ) );
        }
    }
}
//////////////////////////////////////////////////////////////////////////
static void ae_movie_callback_shader_deleter( const aeMovieShaderDeleterCallbackData * _callbackData, ae_voidptr_t _data )
{
    free( _callbackData->element );
}
//////////////////////////////////////////////////////////////////////////
em_movie_composition_handle_t em_create_movie_composition( em_movie_data_handle_t _movieData, const char * _name )
{
    aeMovieData * ae_movie_data = (aeMovieData *)_movieData;

    const ae_char_t * movieName = ae_get_movie_name( ae_movie_data );

    const aeMovieCompositionData * movieCompositionData = ae_get_movie_composition_data( ae_movie_data, _name );

    if( movieCompositionData == AE_NULL )
    {
        emscripten_log( EM_LOG_CONSOLE, "error get movie '%s' composition data '%s'"
            , movieName
            , _name
        );

        return AE_NULL;
    }

    aeMovieCompositionProviders providers;
    ae_initialize_movie_composition_providers( &providers );

    providers.camera_provider = &ae_movie_callback_camera_provider;
    providers.camera_deleter = &ae_movie_callback_camera_deleter;
    providers.camera_update = &ae_movie_callback_camera_update;

    providers.shader_provider = &ae_movie_callback_shader_provider;
    providers.shader_deleter = &ae_movie_callback_shader_deleter;
    providers.shader_property_update = &ae_movie_callback_shader_property_update;

    aeMovieComposition * movieComposition = ae_create_movie_composition( ae_movie_data, movieCompositionData, AE_TRUE, &providers, AE_NULL );

    if( movieComposition == AE_NULL )
    {
        emscripten_log( EM_LOG_CONSOLE, "error create movie '%s' composition '%s'"
            , movieName
            , _name
        );

        return AE_NULL;
    }        

    emscripten_log( EM_LOG_CONSOLE, "successful create movie '%s' composition '%s'"
        , movieName
        , _name
    );

    return movieComposition;
}
//////////////////////////////////////////////////////////////////////////
void em_delete_movie_composition( em_movie_composition_handle_t _movieComposition )
{
    const aeMovieComposition * ae_movie_composition = (const aeMovieComposition *)_movieComposition;
    
    const ae_char_t * movieName = ae_get_movie_composition_name( ae_movie_composition );

    emscripten_log( EM_LOG_CONSOLE, "successful delete movie composition '%s'"
        , movieName
    );
    
    ae_delete_movie_composition( ae_movie_composition );

}
//////////////////////////////////////////////////////////////////////////
void em_set_movie_composition_loop( em_movie_composition_handle_t _movieComposition, unsigned int _loop )
{
    const aeMovieComposition * ae_movie_composition = (const aeMovieComposition *)_movieComposition;

    ae_set_movie_composition_loop( ae_movie_composition, _loop );

    const ae_char_t * movieName = ae_get_movie_composition_name( ae_movie_composition );

    emscripten_log( EM_LOG_CONSOLE, "successful set movie composition '%s' loop '%u'"
        , movieName
        , _loop
    );
}
//////////////////////////////////////////////////////////////////////////
void em_play_movie_composition( em_movie_composition_handle_t _movieComposition, float _time )
{
    aeMovieComposition * ae_movie_composition = (aeMovieComposition *)_movieComposition;

    ae_play_movie_composition( ae_movie_composition, _time );

    const ae_char_t * movieName = ae_get_movie_composition_name( ae_movie_composition );

    emscripten_log( EM_LOG_CONSOLE, "successful play movie composition '%s' time '%f'"
        , movieName
        , _time
    );
}
//////////////////////////////////////////////////////////////////////////
void em_update_movie_composition( em_movie_composition_handle_t _movieComposition, float _time )
{
    aeMovieComposition * ae_movie_composition = (aeMovieComposition *)_movieComposition;

    ae_update_movie_composition( ae_movie_composition, _time * 1000.f );

    const ae_char_t * movieName = ae_get_movie_composition_name( ae_movie_composition );

    //emscripten_log( EM_LOG_CONSOLE, "successful update movie composition '%s' time '%f'"
    //    , movieName
    //    , _time
    //);
}
//////////////////////////////////////////////////////////////////////////
static uint32_t __make_argb( float _r, float _g, float _b, float _a )
{
    const float rgba_255 = 255.f;

    uint8_t a8 = (uint8_t)(_a * rgba_255);
    uint8_t b8 = (uint8_t)(_r * rgba_255);
    uint8_t g8 = (uint8_t)(_g * rgba_255);
    uint8_t r8 = (uint8_t)(_b * rgba_255);

    uint32_t argb = (a8 << 24) | (r8 << 16) | (g8 << 8) | (b8 << 0);

    return argb;
}
//////////////////////////////////////////////////////////////////////////
static void __make_orthogonal( float * _out )
{
    _out[0] = 1.f;
    _out[1] = 0.f;
    _out[2] = 0.f;
    _out[3] = 0.f;

    _out[4] = 0.f;
    _out[5] = 1.f;
    _out[6] = 0.f;
    _out[7] = 0.f;

    _out[8] = 0.f;
    _out[9] = 0.f;
    _out[10] = 1.f;
    _out[11] = 0.f;

    _out[12] = 0.f;
    _out[13] = 0.f;
    _out[14] = 0.f;
    _out[15] = 1.f;
}
//////////////////////////////////////////////////////////////////////////
static void __make_fov( float * _out, float _width, float _height )
{
    float zn = -1.f;
    float zf = 1.f;
    float r = 1.f;
    float l = 0.f;
    float t = 1.f;
    float b = 0.f;
    
    _out[0] = 2.f / _width;
    _out[1] = 0.f;
    _out[2] = 0.f;
    _out[3] = 0.f;

    _out[4] = 0.f;
    _out[5] = -2.f / _height;
    _out[6] = 0.f;
    _out[7] = 0.f;

    _out[8] = 0.f;
    _out[9] = 0.f;
    _out[10] = 2.f / (zf - zn);
    _out[11] = 0.f;

    _out[12] = -(r + l) / (r - l);
    _out[13] = (t + b) / (t - b);
    _out[14] = -zn / (zn - zf);
    _out[15] = 1.f;
}
//////////////////////////////////////////////////////////////////////////
void em_render_movie_composition( em_player_handle_t _player, em_movie_composition_handle_t _movieComposition )
{
    em_player_t * player = (em_player_t *)_player;

    const aeMovieComposition * ae_movie_composition = (const aeMovieComposition *)_movieComposition;
    
    uint32_t mesh_iterator = 0;

    aeMovieRenderMesh mesh;
    while( ae_compute_movie_mesh( ae_movie_composition, &mesh_iterator, &mesh ) == AE_TRUE )
    {
        size_t opengl_indices_buffer_size = mesh.indexCount * sizeof( em_render_index_t );
        size_t opengl_vertex_buffer_size = mesh.vertexCount * sizeof( em_render_vertex_t );

        GLuint texture_id = 0U;

        em_render_vertex_t * vertices = player->vertices;
        const ae_uint16_t * indices = mesh.indices;        

        switch( mesh.layer_type )
        {
        case AE_MOVIE_LAYER_TYPE_SHAPE:
        case AE_MOVIE_LAYER_TYPE_SOLID:
            {
                uint32_t color = __make_argb( mesh.r, mesh.g, mesh.b, mesh.a );

                for( uint32_t index = 0; index != mesh.vertexCount; ++index )
                {
                    em_render_vertex_t * v = vertices + index;

                    const float * mesh_position = mesh.position[index];

                    v->position[0] = mesh_position[0];
                    v->position[1] = mesh_position[1];
                    v->position[2] = mesh_position[2];

                    v->color = color;

                    const float * mesh_uv = mesh.uv[index];

                    v->uv[0] = mesh_uv[0];
                    v->uv[1] = mesh_uv[1];
                }
            }break;
        case AE_MOVIE_LAYER_TYPE_SEQUENCE:
        case AE_MOVIE_LAYER_TYPE_IMAGE:
            {
                const em_resource_image_data * resource_image = (const em_resource_image_data *)mesh.resource_data;

                texture_id = resource_image->texture_id;

                uint32_t color = __make_argb( mesh.r, mesh.g, mesh.b, mesh.a );

                for( uint32_t index = 0; index != mesh.vertexCount; ++index )
                {
                    em_render_vertex_t * v = vertices + index;

                    const float * mesh_position = mesh.position[index];

                    v->position[0] = mesh_position[0];
                    v->position[1] = mesh_position[1];
                    v->position[2] = mesh_position[2];

                    v->color = color;

                    const float * mesh_uv = mesh.uv[index];

                    v->uv[0] = mesh_uv[0];
                    v->uv[1] = mesh_uv[1];
                }
            }break;
        case AE_MOVIE_LAYER_TYPE_VIDEO:
            {
            }break;
        }

        GLCALL( glEnable, (GL_BLEND) );

        switch( mesh.blend_mode )
        {
        case AE_MOVIE_BLEND_ADD:
            {
                GLCALL( glBlendFunc, (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
            }break;
        case AE_MOVIE_BLEND_NORMAL:
            {
                GLCALL( glBlendFunc, (GL_SRC_ALPHA, GL_ONE) );
            }break;
        case AE_MOVIE_BLEND_ALPHA_ADD:
        case AE_MOVIE_BLEND_CLASSIC_COLOR_BURN:
        case AE_MOVIE_BLEND_CLASSIC_COLOR_DODGE:
        case AE_MOVIE_BLEND_CLASSIC_DIFFERENCE:
        case AE_MOVIE_BLEND_COLOR:
        case AE_MOVIE_BLEND_COLOR_BURN:
        case AE_MOVIE_BLEND_COLOR_DODGE:
        case AE_MOVIE_BLEND_DANCING_DISSOLVE:
        case AE_MOVIE_BLEND_DARKEN:
        case AE_MOVIE_BLEND_DARKER_COLOR:
        case AE_MOVIE_BLEND_DIFFERENCE:
        case AE_MOVIE_BLEND_DISSOLVE:
        case AE_MOVIE_BLEND_EXCLUSION:
        case AE_MOVIE_BLEND_HARD_LIGHT:
        case AE_MOVIE_BLEND_HARD_MIX:
        case AE_MOVIE_BLEND_HUE:
        case AE_MOVIE_BLEND_LIGHTEN:
        case AE_MOVIE_BLEND_LIGHTER_COLOR:
        case AE_MOVIE_BLEND_LINEAR_BURN:
        case AE_MOVIE_BLEND_LINEAR_DODGE:
        case AE_MOVIE_BLEND_LINEAR_LIGHT:
        case AE_MOVIE_BLEND_LUMINESCENT_PREMUL:
        case AE_MOVIE_BLEND_LUMINOSITY:
        case AE_MOVIE_BLEND_MULTIPLY:
        case AE_MOVIE_BLEND_OVERLAY:
        case AE_MOVIE_BLEND_PIN_LIGHT:
        case AE_MOVIE_BLEND_SATURATION:
        case AE_MOVIE_BLEND_SCREEN:
        case AE_MOVIE_BLEND_SILHOUETE_ALPHA:
        case AE_MOVIE_BLEND_SILHOUETTE_LUMA:
        case AE_MOVIE_BLEND_SOFT_LIGHT:
        case AE_MOVIE_BLEND_STENCIL_ALPHA:
        case AE_MOVIE_BLEND_STENCIL_LUMA:
        case AE_MOVIE_BLEND_VIVID_LIGHT:
            {

            }break;
        }

        GLuint opengl_vertex_buffer_id = 0;
        GLCALL( glGenBuffers, (1, &opengl_vertex_buffer_id) );
        GLCALL( glBindBuffer, (GL_ARRAY_BUFFER, opengl_vertex_buffer_id) );
        GLCALL( glBufferData, (GL_ARRAY_BUFFER, opengl_vertex_buffer_size, vertices, GL_STREAM_DRAW) );

        if( mesh.shader_data == AE_NULL )
        {
            GLCALL( glUseProgram, (player->program) );

            GLCALL( glActiveTexture, (GL_TEXTURE0) );

            if( texture_id != 0U )
            {
                GLCALL( glBindTexture, (GL_TEXTURE_2D, texture_id) );

                GLCALL( glUniform1i, (player->samplerLocation, 0) );
            }
            else
            {
                GLCALL( glBindTexture, (GL_TEXTURE_2D, 0U) );
            }

            if( mesh.camera_data == AE_NULL )
            {
                const aeMovieCompositionData * ae_movie_composition_data = ae_get_movie_composition_composition_data( ae_movie_composition );

                float viewMatrix[16];
                __make_orthogonal( viewMatrix );

                GLCALL( glUniformMatrix4fv, (player->viewMatrixLocation, 1, GL_FALSE, viewMatrix) );

                float composition_width = ae_get_movie_composition_data_width( ae_movie_composition_data );
                float composition_height = ae_get_movie_composition_data_width( ae_movie_composition_data );

                float projectionMatrix[16];
                __make_fov( projectionMatrix, composition_width, composition_height );

                GLCALL( glUniformMatrix4fv, (player->projectionMatrixLocation, 1, GL_FALSE, projectionMatrix) );

                glViewport( 0, 0, composition_width, composition_height );
            }
            else
            {
                ae_camera_t * camera = (ae_camera_t *)mesh.camera_data;

                GLCALL( glUniformMatrix4fv, (player->viewMatrixLocation, 1, GL_FALSE, camera->view) );
                GLCALL( glUniformMatrix4fv, (player->projectionMatrixLocation, 1, GL_FALSE, camera->projection) );

                glViewport( 0, 0, camera->width, camera->height );
            }

            GLCALL( glEnableVertexAttribArray, (player->inVertLocation) );
            GLCALL( glEnableVertexAttribArray, (player->inColLocation) );
            GLCALL( glEnableVertexAttribArray, (player->inUVLocation) );

            GLCALL( glVertexAttribPointer, (player->inVertLocation, 3, GL_FLOAT, GL_FALSE, sizeof( em_render_vertex_t ), (const GLvoid *)offsetof( em_render_vertex_t, position )) );
            GLCALL( glVertexAttribPointer, (player->inColLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( em_render_vertex_t ), (const GLvoid *)offsetof( em_render_vertex_t, color )) );
            GLCALL( glVertexAttribPointer, (player->inUVLocation, 2, GL_FLOAT, GL_FALSE, sizeof( em_render_vertex_t ), (const GLvoid *)offsetof( em_render_vertex_t, uv )) );
        }

        GLuint opengl_indices_buffer_id = 0;
        GLCALL( glGenBuffers, (1, &opengl_indices_buffer_id) );
        GLCALL( glBindBuffer, (GL_ELEMENT_ARRAY_BUFFER, opengl_indices_buffer_id) );
        GLCALL( glBufferData, (GL_ELEMENT_ARRAY_BUFFER, opengl_indices_buffer_size, indices, GL_STREAM_DRAW) );

        GLCALL( glDrawElements, (GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, 0) );

        if( mesh.shader_data == AE_NULL )
        {
            GLCALL( glDisableVertexAttribArray, (player->inVertLocation) );
            GLCALL( glDisableVertexAttribArray, (player->inColLocation) );
            GLCALL( glDisableVertexAttribArray, (player->inUVLocation) );
        }

        GLCALL( glActiveTexture, (GL_TEXTURE0) );
        GLCALL( glBindTexture, (GL_TEXTURE_2D, 0) );

        GLCALL( glUseProgram, (0) );
        
        GLCALL( glBindBuffer, (GL_ARRAY_BUFFER, 0) );
        GLCALL( glBindBuffer, (GL_ELEMENT_ARRAY_BUFFER, 0) );

        GLCALL( glDeleteBuffers, (1, &opengl_indices_buffer_id) );
        GLCALL( glDeleteBuffers, (1, &opengl_vertex_buffer_id) );

        //emscripten_log( EM_LOG_CONSOLE, "draw elements vertices '%d' indices '%d' texture '%d' index '%d'"
        //    , mesh.vertexCount
        //    , mesh.indexCount
        //    , texture_id
        //    , mesh_iterator
        //);
    }
}