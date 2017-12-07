#include "movie/movie.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void EX_LOG( const char * _format, ... );

//
// Working structure of our application.
//

typedef struct {
	//
	// Movie parameters.
	//

	char license[41];
    char license_padding[3];
	char movie_path[256];
	char comp_name[128];

	//
	// Current working objects
	//

	aeMovieInstance * instance;
	aeMovieData * data;
	aeMovieComposition * composition;

	//
	// Callbacks for data loading & setup.
	//

	ae_movie_data_resource_provider_t resource_provider;
    ae_movie_data_resource_deleter_t resource_deleter;
	aeMovieCompositionProviders comp_providers;
} examples_t;

extern examples_t ex;

//
// Platform-dependent functions.
//

int ex_get_time( void );

//
// Callbacks for loading movie resource data (images, videos, sounds etc.)
// This data might be shared among different compositions.
//

ae_voidptr_t ex_callback_resource_provider_empty( const aeMovieResource * _resource, ae_voidptr_t _data );
void ex_callback_resource_deleter_empty( aeMovieResourceTypeEnum _type, ae_voidptr_t _data, ae_voidptr_t _ud );
ae_voidptr_t ex_callback_resource_provider( const aeMovieResource * _resource, ae_voidptr_t _data );

//
// Callbacks for loading composition data & managing it.
//

ae_voidptr_t ex_callback_camera_provider( const aeMovieCameraProviderCallbackData * _callbackData, ae_voidptr_t _data );
ae_voidptr_t ex_callback_node_provider( const aeMovieNodeProviderCallbackData * _callbackData, ae_voidptr_t _data );
void ex_callback_node_destroyer( const aeMovieNodeDeleterCallbackData * _callbackData, ae_voidptr_t _data );
void ex_callback_node_update( const aeMovieNodeUpdateCallbackData * _callbackData, ae_voidptr_t _data );
ae_voidptr_t ex_callback_track_matte_provider( const aeMovieTrackMatteProviderCallbackData * _callbackData, ae_voidptr_t _data );
void ex_callback_track_matte_deleter( const aeMovieTrackMatteDeleterCallbackData * _callbackData, ae_voidptr_t _data );
void ex_callback_track_matte_update( const aeMovieTrackMatteUpdateCallbackData * _callbackData, ae_voidptr_t _data );
void ex_callback_composition_event( const aeMovieCompositionEventCallbackData * _callbackData, ae_voidptr_t _data );
void ex_callback_composition_state( const aeMovieCompositionStateCallbackData * _callbackData, ae_voidptr_t _data );

//
// Miscellaneous & shared functions.
//

void ex_create_instance( void );
void ex_load_movie_data( void );
void ex_set_composition( void );
void ex_update( int dt );
void ex_render( void );

void ex_init( const char * license, const char * path, const char * composition );
void ex_shutdown( void );
