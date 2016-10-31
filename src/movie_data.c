#	include "movie/movie_data.h"

#	include "movie_transformation.h"
#	include "movie_memory.h"
#	include "movie_stream.h"

#	ifndef AE_MOVIE_MAX_COMPOSITION_NAME
#	define AE_MOVIE_MAX_COMPOSITION_NAME 128
#	endif
//////////////////////////////////////////////////////////////////////////
static const uint32_t ae_movie_version = 7;
//////////////////////////////////////////////////////////////////////////
aeMovieData * ae_create_movie_data( const aeMovieInstance * _instance )
{
	aeMovieData * m = NEW( _instance, aeMovieData );

	m->instance = _instance;

	return m;
}
//////////////////////////////////////////////////////////////////////////
static void __ae_delete_mesh_t( const aeMovieInstance * _instance, const aeMovieMesh * _mesh )
{
	DELETEN( _instance, _mesh->positions );
	DELETEN( _instance, _mesh->uvs );
	DELETEN( _instance, _mesh->indices );
}
//////////////////////////////////////////////////////////////////////////
static void __ae_delete_layer_mesh_t( const aeMovieInstance * _instance, const aeMovieLayerMesh * _layerMesh, uint32_t _count )
{
	if( _layerMesh->immutable == AE_TRUE )
	{
		__ae_delete_mesh_t( _instance, &_layerMesh->immutable_mesh );
	}
	else
	{
		aeMovieMesh * it_mesh = _layerMesh->meshes;
		aeMovieMesh * it_mesh_end = _layerMesh->meshes + _count;
		for( ; it_mesh != it_mesh_end; ++it_mesh )
		{
			const aeMovieMesh * mesh = it_mesh;

			__ae_delete_mesh_t( _instance, mesh );

			DELETE( _instance, mesh );
		}

		DELETEN( _instance, _layerMesh->meshes );
	}
}
//////////////////////////////////////////////////////////////////////////
void ae_delete_movie_data( const aeMovieData * _movieData )
{
	const aeMovieInstance * instance = _movieData->instance;

	uint32_t resource_count = _movieData->resource_count;

	const aeMovieResource * const * it_resource = _movieData->resources;
	const aeMovieResource * const * it_resource_end = _movieData->resources + _movieData->resource_count;
	for( ; it_resource != it_resource_end; ++it_resource )
	{
		const aeMovieResource * base_resource = *it_resource;

		uint8_t type = base_resource->type;

		switch( type )
		{
		case AE_MOVIE_RESOURCE_SOLID:
			{
				const aeMovieResourceSolid * resource = (const aeMovieResourceSolid *)base_resource;

				(void)resource;

			}break;
		case AE_MOVIE_RESOURCE_VIDEO:
			{
				const aeMovieResourceVideo * resource = (const aeMovieResourceVideo *)base_resource;

				DELETEN( instance, resource->path );

				(void)resource;

			}break;
		case AE_MOVIE_RESOURCE_SOUND:
			{
				const aeMovieResourceSound * resource = (const aeMovieResourceSound *)base_resource;

				DELETEN( instance, resource->path );

				(void)resource;

			}break;
		case AE_MOVIE_RESOURCE_IMAGE:
			{
				const aeMovieResourceImage * resource = (const aeMovieResourceImage *)base_resource;

				DELETEN( instance, resource->path );

				if( resource->mesh != AE_NULL )
				{
					__ae_delete_mesh_t( instance, resource->mesh );

					DELETE( instance, resource->mesh );
				}

				(void)resource;

			}break;
		case AE_MOVIE_RESOURCE_SEQUENCE:
			{
				const aeMovieResourceSequence * resource = (const aeMovieResourceSequence *)base_resource;

				DELETEN( instance, resource->images );

				(void)resource;

			}break;
		case AE_MOVIE_RESOURCE_PARTICLE:
			{
				const aeMovieResourceParticle * resource = (const aeMovieResourceParticle *)base_resource;

				DELETEN( instance, resource->path );

				(void)resource;
			}break;
		}

		DELETE( instance, base_resource );
	}

	const aeMovieCompositionData * it_composition = _movieData->compositions;
	const aeMovieCompositionData * it_composition_end = _movieData->compositions + _movieData->composition_count;
	for( ; it_composition != it_composition_end; ++it_composition )
	{
		const aeMovieCompositionData * composition = it_composition;

		const aeMovieLayerData * it_layer = composition->layers;
		const aeMovieLayerData * it_layer_end = composition->layers + composition->layer_count;
		for( ; it_layer != it_layer_end; ++it_layer )
		{
			const aeMovieLayerData * layer = it_layer;

			if( layer->timeremap != AE_NULL )
			{
				DELETEN( instance, layer->timeremap->times );

				DELETEN( instance, layer->timeremap );
			}

			if( layer->mesh != AE_NULL )
			{
				__ae_delete_layer_mesh_t( instance, layer->mesh, layer->frame_count );

				DELETEN( instance, layer->mesh );
			}

			if( layer->bezier_warp != AE_NULL )
			{
				DELETEN( instance, layer->bezier_warp->bezier_warps );

				DELETEN( instance, layer->bezier_warp );
			}

			if( layer->color_vertex != AE_NULL )
			{
				DELETEN( instance, layer->color_vertex->color_vertites_r );
				DELETEN( instance, layer->color_vertex->color_vertites_g );
				DELETEN( instance, layer->color_vertex->color_vertites_b );

				DELETEN( instance, layer->color_vertex );
			}

			if( layer->polygon != AE_NULL )
			{
				DELETEN( instance, layer->polygon->polygons );

				DELETEN( instance, layer->polygon );
			}

			delete_movie_layer_transformation( instance, layer->transformation );

			DELETE( instance, layer->transformation );

			DELETEN( instance, layer->name );
		}

		DELETEN( instance, composition->layers );

		DELETEN( instance, composition->name );
	}

	DELETEN( instance, _movieData->resources );
	DELETEN( instance, _movieData->compositions );

	DELETEN( instance, _movieData->name );

	DELETE( instance, _movieData );
}
//////////////////////////////////////////////////////////////////////////
static void __load_movie_data_composition_camera( aeMovieStream * _stream, aeMovieCompositionData * _compostionData )
{
	READ( _stream, _compostionData->cameraZoom );
}
//////////////////////////////////////////////////////////////////////////
static aeMovieResult __load_movie_data_layer( const aeMovieData * _movieData, const aeMovieCompositionData * _compositions, aeMovieStream * _stream, const aeMovieCompositionData * _compositionData, aeMovieLayerData * _layer )
{
	const aeMovieInstance * instance = _movieData->instance;

	READ_STRING( instance, _stream, _layer->name );

	_layer->index = READZ( _stream );

	_layer->is_track_matte = READB( _stream );
	_layer->has_track_matte = READB( _stream );

	READ( _stream, _layer->type );

	_layer->frame_count = READZ( _stream );

	_layer->timeremap = AE_NULL;
	_layer->mesh = AE_NULL;
	_layer->bezier_warp = AE_NULL;
	_layer->color_vertex = AE_NULL;
	_layer->polygon = AE_NULL;

	for( ;; )
	{
		uint8_t extension;
		READ( _stream, extension );

		switch( extension )
		{
		case 0:
			{
			}break;
		case 1:
			{
				aeMovieLayerTimeremap * layer_timeremap = NEW( instance, aeMovieLayerTimeremap );

				layer_timeremap->times = NEWN( instance, float, _layer->frame_count );

				READN( _stream, layer_timeremap->times, _layer->frame_count );

				_layer->timeremap = layer_timeremap;
			}break;
		case 2:
			{
				aeMovieLayerMesh * layer_mesh = NEW( instance, aeMovieLayerMesh );

				layer_mesh->immutable = READB( _stream );

				if( layer_mesh->immutable == AE_TRUE )
				{
					READ_MESH( instance, _stream, &layer_mesh->immutable_mesh );

					layer_mesh->meshes = AE_NULL;
				}
				else
				{
					layer_mesh->meshes = NEWN( instance, aeMovieMesh, _layer->frame_count );

					aeMovieMesh * it_mesh = layer_mesh->meshes;
					aeMovieMesh * it_mesh_end = layer_mesh->meshes + _layer->frame_count;
					for( ; it_mesh != it_mesh_end; ++it_mesh )
					{
						READ_MESH( instance, _stream, it_mesh );
					}
				}

				_layer->mesh = layer_mesh;
			}break;
		case 3:
			{
				aeMovieLayerBezierWarp * layer_bezier_warp = NEW( instance, aeMovieLayerBezierWarp );

				layer_bezier_warp->immutable = READB( _stream );

				if( layer_bezier_warp->immutable == AE_TRUE )
				{
					READN( _stream, layer_bezier_warp->immutable_bezier_warp.corners, 4 );
					READN( _stream, layer_bezier_warp->immutable_bezier_warp.beziers, 8 );

					layer_bezier_warp->bezier_warps = AE_NULL;
				}
				else
				{
					layer_bezier_warp->bezier_warps = NEWN( instance, aeMovieBezierWarp, _layer->frame_count );

					aeMovieBezierWarp * it_bezier_warp = layer_bezier_warp->bezier_warps;
					aeMovieBezierWarp * it_bezier_warp_end = layer_bezier_warp->bezier_warps + _layer->frame_count;
					for( ; it_bezier_warp != it_bezier_warp_end; ++it_bezier_warp )
					{
						READN( _stream, it_bezier_warp->corners, 4 );
						READN( _stream, it_bezier_warp->beziers, 8 );
					}
				}

				_layer->bezier_warp = layer_bezier_warp;
			}break;
		case 4:
			{
				aeMovieLayerColorVertex * layer_color_vertex = NEW( instance, aeMovieLayerColorVertex );

				layer_color_vertex->immutable_r = READB( _stream );

				if( layer_color_vertex->immutable_r == AE_TRUE )
				{
					READ( _stream, layer_color_vertex->immutable_color_vertex_r );

					layer_color_vertex->color_vertites_r = AE_NULL;
				}
				else
				{
					layer_color_vertex->immutable_color_vertex_r = 1.f;

					layer_color_vertex->color_vertites_r = NEWN( instance, ae_color_t, _layer->frame_count );

					READN( _stream, layer_color_vertex->color_vertites_r, _layer->frame_count );
				}

				layer_color_vertex->immutable_g = READB( _stream );

				if( layer_color_vertex->immutable_g == AE_TRUE )
				{
					READ( _stream, layer_color_vertex->immutable_color_vertex_g );

					layer_color_vertex->color_vertites_g = AE_NULL;
				}
				else
				{
					layer_color_vertex->immutable_color_vertex_g = 1.f;

					layer_color_vertex->color_vertites_g = NEWN( instance, ae_color_t, _layer->frame_count );

					READN( _stream, layer_color_vertex->color_vertites_g, _layer->frame_count );
				}

				layer_color_vertex->immutable_b = READB( _stream );

				if( layer_color_vertex->immutable_b == AE_TRUE )
				{
					READ( _stream, layer_color_vertex->immutable_color_vertex_b );

					layer_color_vertex->color_vertites_b = AE_NULL;
				}
				else
				{
					layer_color_vertex->immutable_color_vertex_b = 1.f;

					layer_color_vertex->color_vertites_b = NEWN( instance, ae_color_t, _layer->frame_count );

					READN( _stream, layer_color_vertex->color_vertites_b, _layer->frame_count );
				}

				_layer->color_vertex = layer_color_vertex;
			}break;
		case 5:
			{
				aeMovieLayerPolygon * layer_polygon = NEW( instance, aeMovieLayerPolygon );
				layer_polygon->immutable = READB( _stream );

				if( layer_polygon->immutable == AE_TRUE )
				{
					READ_POLYGON( instance, _stream, &layer_polygon->immutable_polygon );

					layer_polygon->polygons = AE_NULL;
				}
				else
				{
					uint32_t polygon_count = READZ( _stream );

					layer_polygon->polygons = NEWN( instance, aeMoviePolygon, polygon_count );

					aeMoviePolygon * it_polygon = layer_polygon->polygons;
					aeMoviePolygon * it_polygon_end = layer_polygon->polygons + polygon_count;
					for( ; it_polygon != it_polygon_end; ++it_polygon )
					{
						READ_POLYGON( instance, _stream, it_polygon );
					}
				}

				_layer->polygon = layer_polygon;
			}
		default:
			{
				return AE_MOVIE_FAILED;
			}break;
		}

		if( extension == 0 )
		{
			break;
		}
	}

	ae_bool_t is_resource_or_composition = READB( _stream );

	if( is_resource_or_composition == AE_TRUE )
	{
		uint32_t resource_index = READZ( _stream );

		if( resource_index == 0 )
		{
			_layer->resource = AE_NULL;
		}
		else
		{
			_layer->resource = _movieData->resources[resource_index - 1];
		}

		_layer->sub_composition = AE_NULL;
	}
	else
	{
		uint32_t composition_index = READZ( _stream );
		_layer->sub_composition = _compositions + composition_index;

		_layer->resource = AE_NULL;
	}

	uint32_t parent_index = READZ( _stream );

	_layer->parent_index = parent_index;

	_layer->reverse_time = READB( _stream );

	READ( _stream, _layer->start_time );
	READ( _stream, _layer->in_time );
	READ( _stream, _layer->out_time );

	if( _layer->in_time < 0.f )
	{
		_layer->start_time -= _layer->in_time;
		_layer->in_time = 0.f;
	}

	if( _layer->out_time > _compositionData->duration )
	{
		_layer->out_time = _compositionData->duration;
	}

	READ( _stream, _layer->blend_mode );
	_layer->threeD = READB( _stream );
	READ( _stream, _layer->params );

	_layer->play_count = READZ( _stream );

	READ( _stream, _layer->stretch );

	_layer->transformation = NEW( instance, aeMovieLayerTransformation );

	if( load_movie_layer_transformation( instance, _stream, _layer->transformation, _layer->frame_count ) == AE_MOVIE_FAILED )
	{
		return AE_MOVIE_FAILED;
	}

	if( _layer->is_track_matte == AE_TRUE )
	{
		_layer->renderable = AE_FALSE;
	}
	else
	{
		uint8_t layer_type = _layer->type;

		switch( layer_type )
		{
		case AE_MOVIE_LAYER_TYPE_MOVIE:
			{
				_layer->renderable = AE_FALSE;
			}break;
		case AE_MOVIE_LAYER_TYPE_EVENT:
			{
				_layer->renderable = AE_FALSE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SOCKET:
			{
				_layer->renderable = AE_FALSE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SHAPE:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SLOT:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_NULL:
			{
				_layer->renderable = AE_FALSE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SOLID:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SEQUENCE:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_VIDEO:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SOUND:
			{
				_layer->renderable = AE_FALSE;
			}break;
		case AE_MOVIE_LAYER_TYPE_PARTICLE:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_IMAGE:
			{
				_layer->renderable = AE_TRUE;
			}break;
		case AE_MOVIE_LAYER_TYPE_SUB_MOVIE:
			{
				_layer->renderable = AE_FALSE;
			}break;
		default:
			{
				return AE_MOVIE_FAILED;
			}break;
		}
	}

	return AE_MOVIE_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static aeMovieResult __load_movie_data_composition( const aeMovieData * _movieData, const aeMovieCompositionData * _compositions, aeMovieStream * _stream, aeMovieCompositionData * _compositionData )
{
	READ_STRING( _movieData->instance, _stream, _compositionData->name );

	READ( _stream, _compositionData->master );

	READ( _stream, _compositionData->width );
	READ( _stream, _compositionData->height );

	READ( _stream, _compositionData->duration );
	READ( _stream, _compositionData->frameDuration );
	READ( _stream, _compositionData->frameDurationInv );

	_compositionData->frameCount = (uint32_t)(_compositionData->duration * _compositionData->frameDurationInv + 0.5f);

	_compositionData->flags = 0;

	_compositionData->loop_segment[0] = 0.f;
	_compositionData->loop_segment[1] = _compositionData->duration;

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
				READ( _stream, _compositionData->loop_segment );

				_compositionData->flags |= AE_MOVIE_COMPOSITION_LOOP_SEGMENT;
			}break;
		case 2:
			{
				READ( _stream, _compositionData->anchor_point );

				_compositionData->flags |= AE_MOVIE_COMPOSITION_ANCHOR_POINT;
			}break;
		case 3:
			{
				READ( _stream, _compositionData->offset_point );

				_compositionData->flags |= AE_MOVIE_COMPOSITION_OFFSET_POINT;
			}break;
		case 4:
			{
				READ( _stream, _compositionData->bounds );

				_compositionData->flags |= AE_MOVIE_COMPOSITION_BOUNDS;
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

	_compositionData->has_threeD = READB( _stream );

	if( _compositionData->has_threeD == AE_TRUE )
	{
		__load_movie_data_composition_camera( _stream, _compositionData );
	}

	uint32_t layer_count = READZ( _stream );

	_compositionData->layer_count = layer_count;
	_compositionData->layers = NEWN( _movieData->instance, aeMovieLayerData, layer_count );

	aeMovieLayerData * it_layer = _compositionData->layers;
	aeMovieLayerData * it_layer_end = _compositionData->layers + layer_count;
	for( ; it_layer != it_layer_end; ++it_layer )
	{
		aeMovieLayerData * layer = it_layer;

		layer->composition = _compositionData;

		if( __load_movie_data_layer( _movieData, _compositions, _stream, _compositionData, layer ) == AE_MOVIE_FAILED )
		{
			return AE_MOVIE_FAILED;
		}
	}

	return AE_MOVIE_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __initialize_stream( aeMovieStream * _stream )
{
#	ifdef AE_MOVIE_STREAM_CACHE
	_stream->carriage = 0;
	_stream->capacity = 0;
	_stream->reading = 0;
#	endif
}
//////////////////////////////////////////////////////////////////////////
aeMovieResult ae_load_movie_data( aeMovieData * _movieData, aeMovieStream * _stream, ae_movie_data_resource_provider_t _provider, void * _data )
{
	__initialize_stream( _stream );

	uint8_t magic[4];
	READN( _stream, magic, 4 );

	if( magic[0] != 'A' ||
		magic[1] != 'E' ||
		magic[2] != 'M' ||
		magic[3] != '1' )
	{
		return AE_MOVIE_INVALID_MAGIC;
	}

	uint32_t version;
	READ( _stream, version );

	if( version != ae_movie_version )
	{
		return AE_MOVIE_INVALID_VERSION;
	}

	READ_STRING( _movieData->instance, _stream, _movieData->name );

	uint32_t resource_count = READZ( _stream );

	_movieData->resource_count = resource_count;
	_movieData->resources = NEWN( _movieData->instance, aeMovieResource *, resource_count );

	const aeMovieResource ** it_resource = _movieData->resources;
	const aeMovieResource ** it_resource_end = _movieData->resources + resource_count;
	for( ; it_resource != it_resource_end; ++it_resource )
	{
		uint8_t type;
		READ( _stream, type );

		switch( type )
		{
		case AE_MOVIE_RESOURCE_SOLID:
			{
				aeMovieResourceSolid * resource = NEW( _movieData->instance, aeMovieResourceSolid );

				READ( _stream, resource->width );
				READ( _stream, resource->height );
				READ( _stream, resource->r );
				READ( _stream, resource->g );
				READ( _stream, resource->b );

				*it_resource = (aeMovieResource *)resource;

				resource->type = type;
				resource->data = (*_provider)(*it_resource, _data);
			}break;
		case AE_MOVIE_RESOURCE_VIDEO:
			{
				aeMovieResourceVideo * resource = NEW( _movieData->instance, aeMovieResourceVideo );

				READ_STRING( _movieData->instance, _stream, resource->path );

				READ( _stream, resource->width );
				READ( _stream, resource->height );
				READ( _stream, resource->alpha );
				READ( _stream, resource->frameRate );
				READ( _stream, resource->duration );

				*it_resource = (aeMovieResource *)resource;

				resource->type = type;
				resource->data = (*_provider)(*it_resource, _data);
			}break;
		case AE_MOVIE_RESOURCE_SOUND:
			{
				aeMovieResourceSound * resource = NEW( _movieData->instance, aeMovieResourceSound );

				READ_STRING( _movieData->instance, _stream, resource->path );

				READ( _stream, resource->duration );

				*it_resource = (aeMovieResource *)resource;

				resource->type = type;
				resource->data = (*_provider)(*it_resource, _data);
			}break;
		case AE_MOVIE_RESOURCE_IMAGE:
			{
				aeMovieResourceImage * resource = NEW( _movieData->instance, aeMovieResourceImage );

				READ_STRING( _movieData->instance, _stream, resource->path );

				READ( _stream, resource->premultiplied );

				READ( _stream, resource->base_width );
				READ( _stream, resource->base_height );

				ae_bool_t is_trim = READB( _stream );

				if( is_trim == AE_TRUE )
				{
					READ( _stream, resource->trim_width );
					READ( _stream, resource->trim_height );
					READ( _stream, resource->offset_x );
					READ( _stream, resource->offset_y );
				}
				else
				{
					resource->trim_width = resource->base_width;
					resource->trim_height = resource->base_height;
					resource->offset_x = 0.f;
					resource->offset_y = 0.f;
				}

				ae_bool_t is_mesh = READB( _stream );

				if( is_mesh == AE_TRUE )
				{
					resource->mesh = NEW( _movieData->instance, aeMovieMesh );

					READ_MESH( _movieData->instance, _stream, resource->mesh );
				}
				else
				{
					resource->mesh = AE_NULL;
				}

				*it_resource = (aeMovieResource *)resource;

				resource->type = type;
				resource->data = (*_provider)(*it_resource, _data);
			}break;
		case AE_MOVIE_RESOURCE_SEQUENCE:
			{
				aeMovieResourceSequence * resource = NEW( _movieData->instance, aeMovieResourceSequence );

				READ( _stream, resource->frameDurationInv );

				uint32_t image_count = READZ( _stream );

				resource->image_count = image_count;
				resource->images = NEWN( _movieData->instance, aeMovieResourceImage *, image_count );

				aeMovieResourceImage ** it_image = resource->images;
				aeMovieResourceImage ** it_image_end = resource->images + image_count;
				for( ; it_image != it_image_end; ++it_image )
				{
					uint32_t resource_id = READZ( _stream );

					*it_image = (aeMovieResourceImage *)_movieData->resources[resource_id];
				}

				resource->data = AE_NULL;

				*it_resource = (aeMovieResource *)resource;

				resource->type = type;
				resource->data = (*_provider)(*it_resource, _data);
			}break;
		case AE_MOVIE_RESOURCE_PARTICLE:
			{
				aeMovieResourceParticle * resource = NEW( _movieData->instance, aeMovieResourceParticle );

				READ_STRING( _movieData->instance, _stream, resource->path );

				*it_resource = (aeMovieResource *)resource;

				resource->type = type;
				resource->data = (*_provider)(*it_resource, _data);
			}break;
		default:
			{
				return AE_MOVIE_FAILED;
			}break;
		}
	}

	uint32_t composition_count = READZ( _stream );

	_movieData->composition_count = composition_count;

	aeMovieCompositionData * compositions = NEWN( _movieData->instance, aeMovieCompositionData, composition_count );

	aeMovieCompositionData * it_composition = compositions;
	aeMovieCompositionData * it_composition_end = compositions + composition_count;
	for( ; it_composition != it_composition_end; ++it_composition )
	{
		if( __load_movie_data_composition( _movieData, compositions, _stream, it_composition ) == AE_MOVIE_FAILED )
		{
			return AE_MOVIE_FAILED;
		}
	}

	_movieData->compositions = compositions;

	return AE_MOVIE_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
void ae_trim_image_resources( aeMovieData * _movieData, ae_movie_data_tream_image_resource_t _provider, void * _data )
{
	const aeMovieResource ** it_resource = _movieData->resources;
	const aeMovieResource ** it_resource_end = _movieData->resources + _movieData->resource_count;
	for( ; it_resource != it_resource_end; ++it_resource )
	{
		const aeMovieResource * resource = *it_resource;

		if( resource->type == AE_MOVIE_RESOURCE_IMAGE )
		{
			const aeMovieResourceImage * imageResource = (const aeMovieResourceImage *)resource;

			float base_width;
			float base_height;
			float trim_width;
			float trim_height;
			float offset_x;
			float offset_y;

			if( (*_provider)(imageResource, &base_width, &base_height, &trim_width, &trim_height, &offset_x, &offset_y, _data) == AE_FALSE )
			{
				continue;
			}

			aeMovieResourceImage * mutable_imageResource = (aeMovieResourceImage *)imageResource;

			mutable_imageResource->base_width = base_width;
			mutable_imageResource->base_height = base_height;
			mutable_imageResource->trim_width = trim_width;
			mutable_imageResource->trim_height = trim_height;
			mutable_imageResource->offset_x = offset_x;
			mutable_imageResource->offset_y = offset_y;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
const aeMovieCompositionData * ae_get_movie_composition_data( const aeMovieData * _movieData, const ae_char_t * _name )
{
	const aeMovieInstance * instance = _movieData->instance;

	const aeMovieCompositionData * it_composition = _movieData->compositions;
	const aeMovieCompositionData * it_composition_end = _movieData->compositions + _movieData->composition_count;
	for( ; it_composition != it_composition_end; ++it_composition )
	{
		const aeMovieCompositionData * composition = it_composition;

		if( STRNCMP( instance, composition->name, _name, AE_MOVIE_MAX_COMPOSITION_NAME ) != 0 )
		{
			continue;
		}

		return composition;
	}

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
float ae_get_movie_composition_data_duration( const aeMovieCompositionData * _compositionData )
{
	return _compositionData->duration;
}
//////////////////////////////////////////////////////////////////////////
uint32_t ae_get_movie_composition_data_count( const aeMovieData * _movieData )
{
	return _movieData->composition_count;
}
//////////////////////////////////////////////////////////////////////////
const aeMovieCompositionData * ae_get_movie_composition_data_by_index( const aeMovieData * _movieData, uint32_t _index )
{
	return _movieData->compositions + _index;
}
//////////////////////////////////////////////////////////////////////////
uint32_t ae_get_composition_data_event_count( const aeMovieCompositionData * _compositionData )
{
	uint32_t count = 0;

	const aeMovieLayerData * it_layer = _compositionData->layers;
	const aeMovieLayerData * it_layer_end = _compositionData->layers + _compositionData->layer_count;
	for( ; it_layer != it_layer_end; ++it_layer )
	{
		uint8_t type = it_layer->type;

		if( type == AE_MOVIE_LAYER_TYPE_EVENT )
		{
			++count;
		}
		else if( type == AE_MOVIE_LAYER_TYPE_MOVIE )
		{
			count += ae_get_composition_data_event_count( it_layer->sub_composition );
		}
	}

	return count;
}
//////////////////////////////////////////////////////////////////////////
static ae_bool_t __ae_get_composition_data_event_name( const aeMovieCompositionData * _compositionData, uint32_t * _iterator, uint32_t _index, const ae_char_t ** _name )
{
	const aeMovieLayerData * it_layer = _compositionData->layers;
	const aeMovieLayerData * it_layer_end = _compositionData->layers + _compositionData->layer_count;
	for( ; it_layer != it_layer_end; ++it_layer )
	{
		uint8_t type = it_layer->type;

		if( type == AE_MOVIE_LAYER_TYPE_EVENT )
		{
			if( (*_iterator)++ == _index )
			{
				*_name = it_layer->name;

				return AE_TRUE;
			}
		}
		else if( type == AE_MOVIE_LAYER_TYPE_MOVIE )
		{
			if( __ae_get_composition_data_event_name( it_layer->sub_composition, _iterator, _index, _name ) == AE_TRUE )
			{
				return AE_TRUE;
			}
		}
	}

	return AE_FALSE;
}
//////////////////////////////////////////////////////////////////////////
const ae_char_t * ae_get_composition_data_event_name( const aeMovieCompositionData * _compositionData, uint32_t _index )
{
	uint32_t iterator = 0;
	const ae_char_t * name;
	if( __ae_get_composition_data_event_name( _compositionData, &iterator, _index, &name ) == AE_FALSE )
	{
		return AE_NULL;
	}

	return name;
}
//////////////////////////////////////////////////////////////////////////