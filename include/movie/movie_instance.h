#ifndef MOVIE_MOVIE_INSTANCE_H_
#define MOVIE_MOVIE_INSTANCE_H_

#	include "movie_type.h"

typedef void * (*ae_movie_alloc_t)(void * _data, uint32_t _size);
typedef void * (*ae_movie_alloc_n_t)(void * _data, uint32_t _size, uint32_t _n);
typedef void( *ae_movie_free_t )(void * _data, const void * _ptr);
typedef void( *ae_movie_free_n_t )(void * _data, const void * _ptr);

typedef int32_t( *ae_movie_strncmp_t )(void * _data, const ae_char_t * _src, const ae_char_t * _dst, uint32_t _count);

typedef enum
{
	AE_ERROR_INFO,
	AE_ERROR_WARNING,
	AE_ERROR_INTERNAL,
	AE_ERROR_UNSUPPORT,
	AE_ERROR_CRITICAL
} aeMovieErrorCode;

typedef void( *ae_movie_logerror_t )(void * _data, aeMovieErrorCode _code, const ae_char_t * _compositionName, const ae_char_t * _layerName, const ae_char_t * _message);
typedef void( *ae_movie_memory_info_t )(void * _data, const char * _type, ae_bool_t _alloc, uint32_t _size);

typedef struct aeMovieInstance aeMovieInstance;

aeMovieInstance * ae_create_movie_instance(
	ae_movie_alloc_t _alloc,
	ae_movie_alloc_n_t _alloc_n,
	ae_movie_free_t _free,
	ae_movie_free_n_t _free_n,
	ae_movie_strncmp_t _strncmp,
	ae_movie_logerror_t _error,
	ae_movie_memory_info_t _info,
	void * _data );

void ae_delete_movie_instance( aeMovieInstance * _instance );

#endif