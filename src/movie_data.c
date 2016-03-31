#	include "movie/movie_data.h"

#	include "memory.h"
#	include "stream.h"

#	include <memory.h>

aeMovieData * create_movie_data( const aeMovieInstance * _instance )
{
	aeMovieData * m = NEW( _instance, aeMovieData );

	return m;
}

void delete_movie_data( const aeMovieInstance * _instance, aeMovieData * _movieData )
{
	DELETE( _instance, _movieData );
}

aeMovieResult load_movie_data( const aeMovieInstance * _instance, aeMovieData * _movie, const aeMovieStream * _stream )
{
	char magic[4];
	READN( _stream, magic, 4 );

	if( memcmp( magic, "AEM1", 4 ) != 0 )
	{
		return AE_MOVIE_FAILED;
	}

	uint32_t version;
	READ( _stream, version );

	if( version != ae_movie_version )
	{
		return AE_MOVIE_FAILED;
	}

	READSTR( _instance, _stream, _movie->name );

	uint32_t composition_count = READZ( _stream );

	_movie->compositions = NEWN( _instance, aeMovieCompositionData, composition_count );

	for( uint32_t composition_index = 0; composition_index != composition_count; ++composition_index )
	{
		aeMovieCompositionData * compositionData = _movie->compositions + composition_index;

		READSTR( _instance, _stream, compositionData->name );

		READ( _stream, compositionData->width );
		READ( _stream, compositionData->height );

		READ( _stream, compositionData->frameDuration );
		READ( _stream, compositionData->duration );

		compositionData->flags = 0;

		for( ;; )
		{
			uint8_t flag;
			READ( _stream, flag );

			switch( flag )
			{
			case 0:
				{
				}break;
			case 1:
				{
					READN( _stream, compositionData->loopSegment, 2 );

					compositionData->flags |= AE_MOVIE_COMPOSITION_LOOP_SEGMENT;
				}break;
			case 2:
				{
					READN( _stream, compositionData->anchorPoint, 3 );

					compositionData->flags |= AE_MOVIE_COMPOSITION_ANCHOR_POINT;
				}break;
			case 3:
				{
					READN( _stream, compositionData->offsetPoint, 3 );

					compositionData->flags |= AE_MOVIE_COMPOSITION_OFFSET_POINT;
				}break;
			case 4:
				{
					READN( _stream, compositionData->bounds, 4 );

					compositionData->flags |= AE_MOVIE_COMPOSITION_BOUNDS;
				}break;
			default:
				{
					return AE_MOVIE_FAILED;
				}break;
			};

			if( flag == 0 )
			{
				break;
			}
		}

		uint32_t layer_count = READZ( _stream );

		compositionData->layers = NEWN( _instance, aeMovieCompositionLayerData, layer_count );

		for( uint32_t layer_index = 0; layer_index != layer_count; ++layer_index )
		{
			aeMovieCompositionLayerData * layer = compositionData->layers + layer_index;

			frame_count
		}
	}

	return AE_MOVIE_SUCCESSFUL;
}