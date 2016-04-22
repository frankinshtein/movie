#	include "movie/movie_node.h"

#	include "movie_memory.h"
#	include "movie_math.h"
#	include "movie_utils.h"

#	ifndef AE_MOVIE_MAX_LAYER_NAME
#	define AE_MOVIE_MAX_LAYER_NAME 128
#	endif

//////////////////////////////////////////////////////////////////////////
static float __make_movie_layer_properties_fixed( ae_matrix4_t _out, const aeMovieLayerData * _layer, uint32_t _index )
{
	float anchor_point[3];
	float position[3];
	float scale[3];
	float rotation[3];
	float opacity;

#	define AE_FIXED_PROPERTY( Mask, immutableName, propertyName, outName )\
	if( _layer->immutable_property_mask & Mask )\
		{\
		outName = _layer->immutableName;\
		}\
		else\
		{\
		outName = _layer->propertyName[_index];\
		}

	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_ANCHOR_POINT_X, immuttable_anchor_point_x, property_anchor_point_x, anchor_point[0] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_ANCHOR_POINT_Y, immuttable_anchor_point_y, property_anchor_point_y, anchor_point[1] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_ANCHOR_POINT_Z, immuttable_anchor_point_z, property_anchor_point_z, anchor_point[2] );

	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_POSITION_X, immuttable_position_x, property_position_x, position[0] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_POSITION_Y, immuttable_position_y, property_position_y, position[1] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_POSITION_Z, immuttable_position_z, property_position_z, position[2] );

	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_SCALE_X, immuttable_scale_x, property_scale_x, scale[0] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_SCALE_Y, immuttable_scale_y, property_scale_y, scale[1] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_SCALE_Z, immuttable_scale_z, property_scale_z, scale[2] );

	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_ROTATION_X, immuttable_rotation_x, property_rotation_x, rotation[0] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_ROTATION_Y, immuttable_rotation_y, property_rotation_y, rotation[1] );
	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_ROTATION_Z, immuttable_rotation_z, property_rotation_z, rotation[2] );

	AE_FIXED_PROPERTY( AE_MOVIE_IMMUTABLE_OPACITY, immuttable_opacity, property_opacity, opacity );

#	undef AE_FIXED_PROPERTY

	make_transformation_m4( _out, position, anchor_point, scale, rotation );

	return opacity;
}
//////////////////////////////////////////////////////////////////////////
static float __make_movie_layer_properties_interpolate( ae_matrix4_t _out, const aeMovieLayerData * _layer, uint32_t _index, float _t )
{
	float anchor_point[3];
	float position[3];
	float scale[3];
	float rotation[3];
	float opacity;

#	define AE_LINERP_PROPERTY( Mask, immutableName, propertyName, outName )\
	if( _layer->immutable_property_mask & Mask )\
				{\
		outName = _layer->immutableName;\
				}\
								else\
				{\
		float value0 = _layer->propertyName[_index + 0];\
		float value1 = _layer->propertyName[_index + 1];\
		outName = linerp_f1( value0, value1, _t );\
				}

	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_ANCHOR_POINT_X, immuttable_anchor_point_x, property_anchor_point_x, anchor_point[0] );
	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_ANCHOR_POINT_Y, immuttable_anchor_point_y, property_anchor_point_y, anchor_point[1] );
	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_ANCHOR_POINT_Z, immuttable_anchor_point_z, property_anchor_point_z, anchor_point[2] );

	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_POSITION_X, immuttable_position_x, property_position_x, position[0] );
	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_POSITION_Y, immuttable_position_y, property_position_y, position[1] );
	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_POSITION_Z, immuttable_position_z, property_position_z, position[2] );

	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_SCALE_X, immuttable_scale_x, property_scale_x, scale[0] );
	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_SCALE_Y, immuttable_scale_y, property_scale_y, scale[1] );
	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_SCALE_Z, immuttable_scale_z, property_scale_z, scale[2] );

	AE_LINERP_PROPERTY( AE_MOVIE_IMMUTABLE_OPACITY, immuttable_opacity, property_opacity, opacity );

#	undef AE_LINERP_PROPERTY

#	define AE_LINERP_PROPERTY2( Mask, immutableName, propertyName, outName )\
	if( _layer->immutable_property_mask & Mask )\
				{\
		outName = _layer->immutableName; \
				}\
								else\
				{\
		float value0 = _layer->propertyName[_index + 0];\
		float value1 = _layer->propertyName[_index + 1];\
		float correct_rotate_from = angle_norm( value0 );\
		float correct_rotate_to = angle_correct_interpolate_from_to( correct_rotate_from, value1 );\
		outName = linerp_f1( correct_rotate_from, correct_rotate_to, _t ); \
				}

	AE_LINERP_PROPERTY2( AE_MOVIE_IMMUTABLE_ROTATION_X, immuttable_rotation_x, property_rotation_x, rotation[0] );
	AE_LINERP_PROPERTY2( AE_MOVIE_IMMUTABLE_ROTATION_Y, immuttable_rotation_y, property_rotation_y, rotation[1] );
	AE_LINERP_PROPERTY2( AE_MOVIE_IMMUTABLE_ROTATION_Z, immuttable_rotation_z, property_rotation_z, rotation[2] );

#	undef AE_LINERP_PROPERTY2

	make_transformation_m4( _out, position, anchor_point, scale, rotation );

	return opacity;
}
//////////////////////////////////////////////////////////////////////////
static aeMovieNode * __find_node_by_layer( aeMovieNode * _nodes, uint32_t _begin, uint32_t _end, const aeMovieLayerData * _layer )
{
	for( aeMovieNode
		*it_node = _nodes + _begin,
		*it_node_end = _nodes + _end;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		const aeMovieLayerData * node_layer = node->layer;

		if( node_layer == _layer )
		{
			return node;
		}
	}

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
static const aeMovieLayerData * __find_layer_by_index( const aeMovieCompositionData * _compositionData, uint32_t _index )
{
	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		uint32_t layer_index = layer->index;

		if( layer_index == _index )
		{
			return layer;
		}
	}

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
static void __setup_movie_node_relative( aeMovieNode * _nodes, uint32_t * _iterator, const aeMovieCompositionData * _compositionData, aeMovieNode * _parent )
{
	uint32_t begin_index = *_iterator;

	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		aeMovieNode * node = _nodes + ((*_iterator)++);

		node->layer = layer;
		node->matrix_revision = 0;

		node->active = AE_FALSE;
		node->animate = AE_MOVIE_NODE_ANIMATE_STATIC;

		if( layer->parent_index == 0 )
		{
			node->relative = _parent;
		}

		switch( layer->type )
		{
		case AE_MOVIE_LAYER_TYPE_MOVIE:
		case AE_MOVIE_LAYER_TYPE_SUB_MOVIE:
			{
				__setup_movie_node_relative( _nodes, _iterator, layer->sub_composition, node );				
			}break;
		default:
			{
			}break;
		}
	}

	uint32_t end_index = *_iterator;

	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		uint32_t parent_index = layer->parent_index;

		if( parent_index == 0 )
		{
			continue;
		}

		aeMovieNode * node = __find_node_by_layer( _nodes, begin_index, end_index, layer );

		const aeMovieLayerData * parent_layer = __find_layer_by_index( _compositionData, parent_index );

		aeMovieNode * parent_node = __find_node_by_layer( _nodes, begin_index, end_index, parent_layer );

		node->relative = parent_node;
	}
}
//////////////////////////////////////////////////////////////////////////
static void __setup_movie_node_time( aeMovieNode * _nodes, uint32_t * _iterator, const aeMovieCompositionData * _compositionData, aeMovieNode * _parent, float _stretch, float _startTime )
{
	float frameDuration = _compositionData->frameDuration;
	float frameDurationInv = 1.f / frameDuration;

	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		aeMovieNode * node = _nodes + ((*_iterator)++);

		if( _parent == AE_NULL )
		{
			node->in_time = layer->in_time;
			node->out_time = layer->out_time;
			node->stretch = _stretch;
		}
		else
		{
			node->in_time = _parent->in_time + layer->in_time * _stretch - _startTime;

			float layer_out = _parent->in_time + layer->out_time * _stretch - _startTime;
			float parent_out = _parent->out_time;

			node->out_time = min_f_f( layer_out, parent_out );
			node->stretch = _stretch;
		}

		node->in_frame = (uint32_t)(node->in_frame * frameDurationInv);
		node->out_frame = (uint32_t)(node->out_time * frameDurationInv);

		switch( layer->type )
		{
		case AE_MOVIE_LAYER_TYPE_MOVIE:
		case AE_MOVIE_LAYER_TYPE_SUB_MOVIE:
			{
				float to_stretch = _stretch * layer->stretch;
				float to_startTime = _startTime + layer->start_time;

				__setup_movie_node_time( _nodes, _iterator, layer->sub_composition, node, to_stretch, to_startTime );
			}break;
		default:
			{
			}break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
static void __setup_movie_node_blend_mode( aeMovieNode * _nodes, uint32_t * _iterator, const aeMovieCompositionData * _compositionData, aeMovieNode * _parent, aeMovieBlendMode _blendMode )
{
	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		aeMovieNode * node = _nodes + ((*_iterator)++);

		if( _blendMode != AE_MOVIE_BLEND_NORMAL )
		{
			node->blend_mode = _blendMode;
		}
		else
		{
			node->blend_mode = layer->blend_mode;
		}

		aeMovieBlendMode composition_blend_mode = AE_MOVIE_BLEND_NORMAL;

		if( layer->sub_composition != AE_NULL )
		{
			composition_blend_mode = node->blend_mode;
		}

		switch( layer->type )
		{
		case AE_MOVIE_LAYER_TYPE_MOVIE:
		case AE_MOVIE_LAYER_TYPE_SUB_MOVIE:
			{
				__setup_movie_node_blend_mode( _nodes, _iterator, layer->sub_composition, node, composition_blend_mode );				
			}break;		
		default:
			{
			}break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
static void __setup_movie_node_camera( aeMovieComposition * _composition, uint32_t * _iterator, const aeMovieCompositionData * _compositionData, aeMovieNode * _parent, const void * _cameraData )
{
	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		aeMovieNode * node = _composition->nodes + ((*_iterator)++);

		if( _compositionData->has_threeD == AE_TRUE )
		{
			float width = _compositionData->width;
			float height = _compositionData->height;
			float zoom = _compositionData->cameraZoom;

			ae_vector3_t camera_position;
			camera_position[0] = width * 0.5f;
			camera_position[1] = height * 0.5f;
			camera_position[2] = -zoom;

			ae_vector3_t camera_direction;
			camera_direction[0] = 0.f;
			camera_direction[1] = 0.f;
			camera_direction[2] = 1.f;

			float camera_fov = make_camera_fov( height, zoom );

			node->camera_data = (*_composition->providers.camera_provider)(_compositionData->name, camera_position, camera_direction, camera_fov, width, height, _composition->provider_data);
		}
		else
		{
			node->camera_data = _cameraData;
		}

		switch( layer->type )
		{
		case AE_MOVIE_LAYER_TYPE_SUB_MOVIE:
		case AE_MOVIE_LAYER_TYPE_MOVIE:
			{
				__setup_movie_node_camera( _composition, _iterator, layer->sub_composition, node, node->camera_data );
			}break;
		default:
			{
			}break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
static void __setup_movie_node_loop( aeMovieComposition * _composition )
{
	uint32_t frameCount = _composition->composition_data->frameCount;

	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		if( node->in_frame == 0 && node->out_time == frameCount )
		{
			node->loop = AE_TRUE;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
static uint32_t __get_movie_composition_data_node_count( const aeMovieData * _movie, const aeMovieCompositionData * _compositionData )
{
	uint32_t count = _compositionData->layer_count;

	for( const aeMovieLayerData
		*it_layer = _compositionData->layers,
		*it_layer_end = _compositionData->layers + _compositionData->layer_count;
	it_layer != it_layer_end;
	++it_layer )
	{
		const aeMovieLayerData * layer = it_layer;

		uint8_t layer_type = layer->type;

		switch( layer_type )
		{
		case AE_MOVIE_LAYER_TYPE_MOVIE:
		case AE_MOVIE_LAYER_TYPE_SUB_MOVIE:
			{
				uint32_t movie_layer_count = __get_movie_composition_data_node_count( _movie, layer->sub_composition );

				count += movie_layer_count;
			}break;
		default:
			{
			}break;
		}
	}

	return count;
}
//////////////////////////////////////////////////////////////////////////
void __setup_movie_composition_element( aeMovieComposition * _composition )
{
	const aeMovieCompositionData * compositionData = _composition->composition_data;

	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		uint8_t type = node->layer->type;
				
		switch( type )
		{
		case AE_MOVIE_LAYER_TYPE_VIDEO:
			{
				node->element_data = (*_composition->providers.node_provider)(node->layer, node->layer->resource, _composition->provider_data);
			}break;
		case AE_MOVIE_LAYER_TYPE_SOUND:
			{
				node->element_data = (*_composition->providers.node_provider)(node->layer, node->layer->resource, _composition->provider_data);
			}break;
		case AE_MOVIE_LAYER_TYPE_PARTICLE:
			{
				node->element_data = (*_composition->providers.node_provider)(node->layer, node->layer->resource, _composition->provider_data);
			}break;
		case AE_MOVIE_LAYER_TYPE_SOCKET:
			{
				node->element_data = (*_composition->providers.node_provider)(node->layer, node->layer->resource, _composition->provider_data);
			}break;
		case AE_MOVIE_LAYER_TYPE_SLOT:
			{
				node->element_data = (*_composition->providers.node_provider)(node->layer, node->layer->resource, _composition->provider_data);
			}break;
		case AE_MOVIE_LAYER_TYPE_EVENT:
			{
				node->element_data = (*_composition->providers.node_provider)(node->layer, node->layer->resource, _composition->provider_data);
			}break;
		default:
			{
				node->element_data = AE_NULL;
			}break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static void * dummy_ae_movie_composition_node_camera( const ae_string_t _name, const ae_vector3_t _position, const ae_vector3_t _direction, float _fov, float _width, float _height, void * _data )
{
	(void)_name;
	(void)_position;
	(void)_direction;
	(void)_fov;
	(void)_width;
	(void)_height;
	(void)_data;

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
static void * dummy_ae_movie_composition_node_provider( const aeMovieLayerData * _layerData, const aeMovieResource * _resource, void * _data )
{
	(void)_layerData;
	(void)_resource;
	(void)_data;

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
static void dummy_ae_movie_composition_node_destroyer( const void * _element, uint32_t _type, void * _data )
{
	(void)_element;
	(void)_type;
	(void)_data;
}
//////////////////////////////////////////////////////////////////////////
static void dummy_ae_movie_node_animate_update( const void * _element, uint32_t _type, const ae_matrix4_t _matrix, float _opacity, void * _data )
{
	(void)_element;
	(void)_type;
	(void)_matrix;
	(void)_opacity;
	(void)_data;
}
//////////////////////////////////////////////////////////////////////////
static void dummy_ae_movie_node_animate_begin( const void * _element, uint32_t _type, float _offset, void * _data )
{
	(void)_element;
	(void)_type;
	(void)_offset;
	(void)_data;
}
//////////////////////////////////////////////////////////////////////////
static void dummy_ae_movie_node_animate_end( const void * _element, uint32_t _type, void * _data )
{
	(void)_element;
	(void)_type;
	(void)_data;
}
//////////////////////////////////////////////////////////////////////////
void dummy_ae_movie_node_event( const void * _element, const ae_matrix4_t _matrix, float _opacity, void * _data )
{
	(void)_element;
	(void)_matrix;
	(void)_opacity;
	(void)_data;
}
//////////////////////////////////////////////////////////////////////////
void dummy_ae_movie_composition_state( struct aeMovieComposition * _composition, aeMovieCompositionStateFlag _state, void * _data )
{
	(void)_composition;
	(void)_state;
	(void)_data;
}
//////////////////////////////////////////////////////////////////////////
aeMovieComposition * create_movie_composition( const aeMovieData * _movieData, const aeMovieCompositionData * _compositionData, const aeMovieCompositionProviders * providers, void * _data )
{
	aeMovieComposition * composition = NEW( _movieData->instance, aeMovieComposition );

	composition->movie_data = _movieData;
	composition->composition_data = _compositionData;

	composition->update_revision = 0;
	composition->timing = 0.f;
	composition->loop = AE_FALSE;

	composition->play = AE_TRUE;
	composition->pause = AE_FALSE;	

	uint32_t node_count = __get_movie_composition_data_node_count( _movieData, _compositionData );

	composition->node_count = node_count;
	composition->nodes = NEWN( _movieData->instance, aeMovieNode, node_count );

	composition->providers.camera_provider = providers->camera_provider ? providers->camera_provider : &dummy_ae_movie_composition_node_camera;
	composition->providers.node_provider = providers->node_provider ? providers->node_provider : &dummy_ae_movie_composition_node_provider;
	composition->providers.node_destroyer = providers->node_destroyer ? providers->node_destroyer : &dummy_ae_movie_composition_node_destroyer;
	composition->providers.animate_update = providers->animate_update ? providers->animate_update : &dummy_ae_movie_node_animate_update;
	composition->providers.animate_begin = providers->animate_begin ? providers->animate_begin : &dummy_ae_movie_node_animate_begin;
	composition->providers.animate_end = providers->animate_end ? providers->animate_end : &dummy_ae_movie_node_animate_end;
	composition->providers.event = providers->event ? providers->event : &dummy_ae_movie_node_event;
	composition->providers.composition_state = providers->composition_state ? providers->composition_state : &dummy_ae_movie_composition_state;

	composition->provider_data = _data;
	
	uint32_t node_relative_iterator = 0;

	__setup_movie_node_relative( composition->nodes, &node_relative_iterator, _compositionData, AE_NULL );

	uint32_t node_time_iterator = 0;

	__setup_movie_node_time( composition->nodes, &node_time_iterator, _compositionData, AE_NULL, 1.f, 0.f );

	uint32_t node_blend_mode_iterator = 0;

	__setup_movie_node_blend_mode( composition->nodes, &node_blend_mode_iterator, _compositionData, AE_NULL, AE_MOVIE_BLEND_NORMAL );

	uint32_t node_camera_iterator = 0;

	__setup_movie_node_camera( composition, &node_camera_iterator, composition->composition_data, AE_NULL, AE_NULL );

	__setup_movie_node_loop( composition );

	__setup_movie_composition_element( composition );

	return composition;
}
//////////////////////////////////////////////////////////////////////////
void destroy_movie_composition( const aeMovieComposition * _composition )
{
	for( const aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		const aeMovieNode * node = it_node;

		(*_composition->providers.node_destroyer)(node->element_data, node->layer->type, _composition->provider_data);
	}

	const aeMovieInstance * instance = _composition->movie_data->instance;

	DELETE( instance, _composition->nodes );

	DELETE( instance, _composition );
}
//////////////////////////////////////////////////////////////////////////
void set_movie_composition_loop( aeMovieComposition * _composition, ae_bool_t _loop )
{
	_composition->loop = _loop;
}
//////////////////////////////////////////////////////////////////////////
void set_movie_composition_play_count( aeMovieComposition * _composition, uint32_t _playCount )
{
	_composition->play_count = _playCount;
}
//////////////////////////////////////////////////////////////////////////
void play_movie_composition( aeMovieComposition * _composition, float _timing )
{
	if( _composition->play == AE_TRUE )
	{
		return;
	}

	_composition->play = AE_TRUE;

	if( _composition->pause == AE_FALSE )
	{
		_composition->play_iterator = 0;
		_composition->timing = _timing;
		
		set_movie_composition_timing( _composition, _timing );
	}
	else
	{
		_composition->pause = AE_FALSE;
	}

	(_composition->providers.composition_state)(_composition, AE_MOVIE_COMPOSITION_PLAY, _composition->provider_data);
}
//////////////////////////////////////////////////////////////////////////
void stop_movie_composition( aeMovieComposition * _composition )
{
	if( _composition->play == AE_FALSE )
	{
		_composition->pause = AE_FALSE;
	}
	else
	{
		_composition->play = AE_FALSE;
		_composition->pause = AE_FALSE;
	}

	(_composition->providers.composition_state)(_composition, AE_MOVIE_COMPOSITION_STOP, _composition->provider_data);
}
//////////////////////////////////////////////////////////////////////////
void pause_movie_composition( aeMovieComposition * _composition )
{
	if( _composition->play == AE_FALSE )
	{
		return;
	}

	_composition->pause = AE_TRUE;

	(_composition->providers.composition_state)(_composition, AE_MOVIE_COMPOSITION_PAUSE, _composition->provider_data);
}
//////////////////////////////////////////////////////////////////////////
void resume_movie_composition( aeMovieComposition * _composition )
{
	if( _composition->play == AE_FALSE )
	{
		return;
	}

	_composition->pause = AE_FALSE;

	(_composition->providers.composition_state)(_composition, AE_MOVIE_COMPOSITION_RESUME, _composition->provider_data);
}
//////////////////////////////////////////////////////////////////////////
static void __update_node_matrix_fixed( aeMovieNode * _node, uint32_t _revision, uint32_t _frame )
{
	if( _node->matrix_revision == _revision )
	{
		return;
	}

	_node->matrix_revision = _revision;

	if( _node->relative == AE_NULL )
	{
		float local_opacity = __make_movie_layer_properties_fixed( _node->matrix, _node->layer, _frame );

		_node->composition_opactity = _node->opacity;
		_node->opacity = local_opacity;

		return;
	}

	aeMovieNode * node_relative = _node->relative;

	if( node_relative->matrix_revision != _revision )
	{
		__update_node_matrix_fixed( node_relative, _revision, _frame );
	}

	ae_matrix4_t local_matrix;
	float local_opacity = __make_movie_layer_properties_fixed( local_matrix, _node->layer, _frame );

	mul_m4_m4( _node->matrix, local_matrix, node_relative->matrix );

	if( _node->layer->sub_composition != AE_NULL )
	{
		_node->composition_opactity = node_relative->composition_opactity * local_opacity;
	}
	else
	{
		_node->composition_opactity = node_relative->composition_opactity;
	}

	_node->opacity = node_relative->composition_opactity * local_opacity;
}
//////////////////////////////////////////////////////////////////////////
static void __update_node_matrix_interpolate( aeMovieNode * _node, uint32_t _revision, uint32_t _frame, float _t )
{
	if( _node->matrix_revision == _revision )
	{
		return;
	}
	
	_node->matrix_revision = _revision;

	if( _node->relative == AE_NULL )
	{
		float local_opacity = __make_movie_layer_properties_interpolate( _node->matrix, _node->layer, _frame, _t );

		_node->composition_opactity = _node->opacity;
		_node->opacity = local_opacity;	

		return;
	}

	aeMovieNode * node_relative = _node->relative;

	if( node_relative->matrix_revision != _revision )
	{
		__update_node_matrix_interpolate( node_relative, _revision, _frame, _t );
	}

	ae_matrix4_t local_matrix;
	float local_opacity = __make_movie_layer_properties_interpolate( local_matrix, _node->layer, _frame, _t );

	mul_m4_m4( _node->matrix, local_matrix, node_relative->matrix );

	if( _node->layer->sub_composition != AE_NULL )
	{
		_node->composition_opactity = node_relative->composition_opactity * local_opacity;
	}
	else
	{
		_node->composition_opactity = node_relative->composition_opactity;
	}

	_node->opacity = node_relative->composition_opactity * local_opacity;
}
//////////////////////////////////////////////////////////////////////////
static void __update_movie_composition_node_begin( aeMovieComposition * _composition, aeMovieNode * _node )
{
	if( _node->animate == AE_MOVIE_NODE_ANIMATE_STATIC || _node->animate == AE_MOVIE_NODE_ANIMATE_END )
	{
		_node->animate = AE_MOVIE_NODE_ANIMATE_BEGIN;

		if( _node->element_data != AE_NULL )
		{
			(*_composition->providers.animate_begin)(_node->element_data, _node->layer->type, _node->layer->start_time, _composition->provider_data);
		}
	}
	else
	{
		_node->animate = AE_MOVIE_NODE_ANIMATE_PROCESS;
	}
}
//////////////////////////////////////////////////////////////////////////
static void __update_movie_composition_node_end( aeMovieComposition * _composition, aeMovieNode * _node )
{
	if( _node->animate == AE_MOVIE_NODE_ANIMATE_PROCESS || _node->animate == AE_MOVIE_NODE_ANIMATE_BEGIN )
	{
		_node->animate = AE_MOVIE_NODE_ANIMATE_END;

		if( _node->element_data != AE_NULL )
		{
			(*_composition->providers.animate_end)(_node->element_data, _node->layer->type, _composition->provider_data);
		}
	}
	else
	{
		_node->animate = AE_MOVIE_NODE_ANIMATE_STATIC;		
	}
}
//////////////////////////////////////////////////////////////////////////
static void __update_node_matrix( aeMovieComposition * _composition, aeMovieNode * _node, uint32_t _revision, uint32_t _frame, float _t, ae_bool_t _interpolate, ae_bool_t _begin )
{
	if( _interpolate == AE_TRUE )
	{
		__update_node_matrix_interpolate( _node, _revision, _frame, _t );
	}
	else
	{
		__update_node_matrix_fixed( _node, _revision, _frame );
	}

	(*_composition->providers.animate_update)(_node->element_data, _node->layer->type, _node->matrix, _node->opacity, _composition->provider_data);

	if( _begin == AE_TRUE )
	{
		__update_movie_composition_node_begin( _composition, _node );
	}
	else
	{
		__update_movie_composition_node_end( _composition, _node );
	}
}
//////////////////////////////////////////////////////////////////////////
void __update_movie_composition_node( aeMovieComposition * _composition, uint32_t _revision, uint32_t _beginFrame, uint32_t _endFrame )
{
	float frameDuration = _composition->composition_data->frameDuration;
	float frameDurationInv = 1.f / frameDuration;

	float end_timing = _composition->timing;

	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		const aeMovieLayerData * layer = node->layer;

		uint32_t indexIn = node->in_frame;
		uint32_t indexOut = node->out_frame;

		if( indexIn > _endFrame || indexOut < _beginFrame )
		{
			continue;
		}

		if( node->layer->type == AE_MOVIE_LAYER_TYPE_EVENT )
		{
			if( _beginFrame < indexIn && _endFrame >= indexIn )
			{
				__update_node_matrix_fixed( node, _revision, 0 );

				(*_composition->providers.event)(node->element_data, node->matrix, node->opacity, _composition->provider_data);
			}
		}
		else
		{
			float current_time = end_timing - node->in_time;

			node->current_time = current_time;

			float frame_time = (current_time) / node->stretch * frameDurationInv;

			uint32_t frameId = (uint32_t)frame_time;

			float t = frame_time - (float)frameId;

			if( (_composition->loop == AE_TRUE || _composition->play_count > 1) || (layer->params & AE_MOVIE_LAYER_PARAM_LOOP) || (node->loop == AE_TRUE) )
			{
				node->active = AE_TRUE;

				__update_node_matrix( _composition, node, _revision, frameId, t, AE_FALSE, AE_TRUE );
			}
			else
			{
				if( _beginFrame < indexIn && _endFrame >= indexIn && _endFrame < indexOut )
				{
					node->active = AE_TRUE;

					__update_node_matrix( _composition, node, _revision, frameId, t, (_endFrame + 1) < indexOut, AE_TRUE );
				}
				else if( _endFrame >= indexOut && _beginFrame >= indexIn && _beginFrame < indexOut )
				{
					node->active = AE_FALSE;

					__update_node_matrix( _composition, node, _revision, frameId, t, (_endFrame + 1) < indexOut, AE_FALSE );
				}
				else if( _beginFrame >= indexIn && _endFrame >= indexIn && _endFrame < indexOut )
				{
					node->active = AE_TRUE;

					__update_node_matrix( _composition, node, _revision, frameId, t, (_endFrame + 1) < indexOut, AE_TRUE );
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void update_movie_composition( aeMovieComposition * _composition, float _timing )
{
	if( _composition->play == AE_FALSE )
	{
		return;
	}

	if( _composition->pause == AE_TRUE )
	{
		return;
	}

	_composition->update_revision++;
	uint32_t update_revision = _composition->update_revision;

	float begin_timing = _composition->timing;

	_composition->timing += _timing;

	float frameDuration = _composition->composition_data->frameDuration;

	uint32_t beginFrame = (uint32_t)(begin_timing / frameDuration);
	uint32_t endFrame = (uint32_t)(_composition->timing / frameDuration);

	float duration = _composition->composition_data->duration;

	while( _composition->timing >= duration )
	{
		uint32_t lastFrame = (uint32_t)(duration / frameDuration);

		__update_movie_composition_node( _composition, update_revision, beginFrame, lastFrame );

		if( _composition->loop == AE_FALSE && _composition->play_count == 0 )
		{
			_composition->timing = duration - frameDuration;

			_composition->play = AE_FALSE;

			(*_composition->providers.composition_state)(_composition, AE_MOVIE_COMPOSITION_END, _composition->provider_data);

			return;
		}
		else
		{
			beginFrame = 0;

			_composition->timing -= duration;

			endFrame = (uint32_t)(_composition->timing / frameDuration);

			(*_composition->providers.composition_state)(_composition, AE_MOVIE_COMPOSITION_LOOP_END, _composition->provider_data);
		}
	}

	__update_movie_composition_node( _composition, update_revision, beginFrame, endFrame );
}
//////////////////////////////////////////////////////////////////////////
void set_movie_composition_timing( aeMovieComposition * _composition, float _time )
{
	float duration = _composition->composition_data->duration;

	if( _time < 0.f && _time > duration )
	{
		return;
	}

	if( equal_f_f( _composition->timing, _time ) == AE_TRUE )
	{
		return;
	}

	float frameDuration = _composition->composition_data->frameDuration;
	float frameDurationInv = 1.f / frameDuration;

	if( _composition->timing > _time )
	{
		uint32_t frame = (uint32_t)(_composition->timing * frameDurationInv);

		for( aeMovieNode
			*it_node = _composition->nodes,
			*it_node_end = _composition->nodes + _composition->node_count;
		it_node != it_node_end;
		++it_node )
		{
			aeMovieNode * node = it_node;

			const aeMovieLayerData * layer = node->layer;

			uint32_t indexIn = node->in_frame;
			uint32_t indexOut = node->out_frame;

			if( frame >= indexIn && frame < indexOut )
			{
				node->active = AE_FALSE;

				__update_movie_composition_node_end( _composition, node );
			}
		}
	}

	_composition->timing = 0.f;

	_composition->update_revision++;
	uint32_t update_revision = _composition->update_revision;

	float begin_timing = 0.f;

	_composition->timing = _time;

	uint32_t beginFrame = 0;
	uint32_t endFrame = (uint32_t)(_time * frameDurationInv);

	__update_movie_composition_node( _composition, update_revision, beginFrame, endFrame );
}
//////////////////////////////////////////////////////////////////////////
ae_bool_t set_movie_composition_slot( aeMovieComposition * _composition, const char * _slotName, void * _slotData )
{
	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		const aeMovieLayerData * layer = node->layer;

		if( layer->type != AE_MOVIE_LAYER_TYPE_SLOT )
		{
			continue;
		}

		if( ae_strncmp( layer->name, _slotName, AE_MOVIE_MAX_LAYER_NAME ) != 0 )
		{
			continue;
		}
		
		node->element_data = _slotData;

		return AE_TRUE;
	}

	return AE_FALSE;
}
//////////////////////////////////////////////////////////////////////////
void * get_movie_composition_slot( aeMovieComposition * _composition, const char * _slotName )
{
	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		const aeMovieLayerData * layer = node->layer;

		if( layer->type != AE_MOVIE_LAYER_TYPE_SLOT )
		{
			continue;
		}

		if( ae_strncmp( layer->name, _slotName, AE_MOVIE_MAX_LAYER_NAME ) != 0 )
		{
			continue;
		}

		return node->element_data;
	}

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
ae_bool_t has_movie_composition_slot( aeMovieComposition * _composition, const char * _slotName )
{
	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		const aeMovieLayerData * layer = node->layer;

		if( layer->type != AE_MOVIE_LAYER_TYPE_SLOT )
		{
			continue;
		}

		if( ae_strncmp( layer->name, _slotName, AE_MOVIE_MAX_LAYER_NAME ) != 0 )
		{
			continue;
		}

		return AE_TRUE;
	}

	return AE_FALSE;
}
//////////////////////////////////////////////////////////////////////////
void * remove_movie_composition_slot( aeMovieComposition * _composition, const char * _slotName )
{
	for( aeMovieNode
		*it_node = _composition->nodes,
		*it_node_end = _composition->nodes + _composition->node_count;
	it_node != it_node_end;
	++it_node )
	{
		aeMovieNode * node = it_node;

		const aeMovieLayerData * layer = node->layer;

		if( layer->type != AE_MOVIE_LAYER_TYPE_SLOT )
		{
			continue;
		}

		if( ae_strncmp( layer->name, _slotName, AE_MOVIE_MAX_LAYER_NAME ) != 0 )
		{
			continue;
		}

		void * prev_element_data = node->element_data;

		node->element_data = AE_NULL;

		return prev_element_data;
	}

	return AE_NULL;
}
//////////////////////////////////////////////////////////////////////////
static aeMovieResult __count_movie_redner_context( const aeMovieComposition * _composition, aeMovieRenderContext * _context )
{
	uint32_t render_count = 0;

	for( uint32_t iterator = 0; iterator != _composition->node_count; ++iterator )
	{
		const aeMovieNode * node = _composition->nodes + iterator;

		if( node->active == AE_FALSE )
		{
			continue;
		}

		const aeMovieLayerData * layer = node->layer;

		if( layer->renderable == AE_FALSE )
		{
			continue;
		}

		_context->render_node_indices[render_count] = iterator;

		++render_count;

		if( render_count == AE_MOVIE_MAX_RENDER_NODE )
		{
			return AE_MOVIE_FAILED;
		}
	}

	_context->render_count = render_count;

	return AE_MOVIE_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
aeMovieResult begin_movie_render_context( const aeMovieComposition * _composition, aeMovieRenderContext * _context )
{
	_context->composition = _composition;

	float * sprite_uv = _context->sprite_uv;

	*sprite_uv++ = 0.f;
	*sprite_uv++ = 0.f;
	*sprite_uv++ = 1.f;
	*sprite_uv++ = 0.f;
	*sprite_uv++ = 1.f;
	*sprite_uv++ = 1.f;
	*sprite_uv++ = 0.f;
	*sprite_uv++ = 1.f;

	uint16_t * sprite_indices = _context->sprite_indices;

	*sprite_indices++ = 0;
	*sprite_indices++ = 3;
	*sprite_indices++ = 1;
	*sprite_indices++ = 1;
	*sprite_indices++ = 3;
	*sprite_indices++ = 2;

	if( __count_movie_redner_context( _composition, _context ) == AE_MOVIE_FAILED )
	{
		return AE_MOVIE_FAILED;
	}

	return AE_MOVIE_SUCCESSFUL;
}
//////////////////////////////////////////////////////////////////////////
static void __make_mesh_vertices( const aeMovieLayerMesh * _layerMesh, uint32_t _frame, const ae_matrix4_t _matrix, aeMovieRenderMesh * _renderMesh )
{
	if( _layerMesh->immutable == AE_TRUE )
	{
		const aeMovieMesh * mesh = &_layerMesh->immutable_mesh;

		_renderMesh->vertexCount = mesh->vertex_count;
		_renderMesh->indexCount = mesh->indices_count;

		for( uint32_t i = 0; i != mesh->vertex_count; ++i )
		{
			mul_v3_v2_m4( _renderMesh->position + i * 3, mesh->positions + i * 2, _matrix );
		}

		_renderMesh->uv = mesh->uvs;
		_renderMesh->indices = mesh->indices;
	}
	else
	{
		const aeMovieMesh * mesh = _layerMesh->meshes + _frame;

		_renderMesh->vertexCount = mesh->vertex_count;
		_renderMesh->indexCount = mesh->indices_count;

		for( uint32_t i = 0; i != mesh->vertex_count; ++i )
		{
			mul_v3_v2_m4( _renderMesh->position + i * 3, mesh->positions + i * 2, _matrix );
		}

		_renderMesh->uv = mesh->uvs;
		_renderMesh->indices = mesh->indices;
	}
}
//////////////////////////////////////////////////////////////////////////
static void __make_sprite_vertices( const aeMovieRenderContext * _context, float _offset_x, float _offset_y, float _width, float _height, const ae_matrix4_t _matrix, aeMovieRenderMesh * _renderMesh )
{
	float v_position[8];

	float * v = v_position;

	*v++ = _offset_x + _width * 0.f;
	*v++ = _offset_y + _height * 0.f;
	*v++ = _offset_x + _width * 1.f;
	*v++ = _offset_y + _height * 0.f;
	*v++ = _offset_x + _width * 1.f;
	*v++ = _offset_y + _height * 1.f;
	*v++ = _offset_x + _width * 0.f;
	*v++ = _offset_y + _height * 1.f;

	_renderMesh->vertexCount = 4;
	_renderMesh->indexCount = 6;

	mul_v3_v2_m4( _renderMesh->position + 0, v_position + 0, _matrix );
	mul_v3_v2_m4( _renderMesh->position + 3, v_position + 2, _matrix );
	mul_v3_v2_m4( _renderMesh->position + 6, v_position + 4, _matrix );
	mul_v3_v2_m4( _renderMesh->position + 9, v_position + 6, _matrix );

	_renderMesh->uv = _context->sprite_uv;

	_renderMesh->indices = _context->sprite_indices;
}
//////////////////////////////////////////////////////////////////////////
void compute_movie_mesh( const aeMovieRenderContext * _context, uint32_t _index, aeMovieRenderMesh * _vertices )
{
	const aeMovieComposition * composition = _context->composition;
	uint32_t node_index = _context->render_node_indices[_index];

	const aeMovieNode * node = composition->nodes + node_index;

	const aeMovieLayerData * layer = node->layer;
	const aeMovieResource * resource = layer->resource;

	uint8_t layer_type = layer->type;

	_vertices->layer_type = layer_type;

	_vertices->animate = node->animate;
	_vertices->blend_mode = node->blend_mode;

	if( resource != AE_NULL )
	{
		_vertices->resource_type = resource->type;
		_vertices->resource_data = resource->data;
	}
	else
	{
		_vertices->resource_type = AE_MOVIE_RESOURCE_NONE;
		_vertices->resource_data = AE_NULL;
	}

	_vertices->camera_data = node->camera_data;
	_vertices->element_data = node->element_data;

	switch( layer_type )
	{
	case AE_MOVIE_LAYER_TYPE_SLOT:
	case AE_MOVIE_LAYER_TYPE_PARTICLE:
		{
			_vertices->vertexCount = 0;
			_vertices->indexCount = 0;

			_vertices->uv = AE_NULL;
			_vertices->indices = AE_NULL;

			_vertices->r = 1.f;
			_vertices->g = 1.f;
			_vertices->b = 1.f;
			_vertices->a = node->opacity;
		}break;
	case AE_MOVIE_LAYER_TYPE_SOLID:
		{
			aeMovieResourceSolid * resource_solid = (aeMovieResourceSolid *)resource;

			if( layer->mesh == AE_NULL )
			{
				float width = resource_solid->width;
				float height = resource_solid->height;

				__make_sprite_vertices( _context, 0.f, 0.f, width, height, node->matrix, _vertices );
			}
			else
			{
				float frameDuration = composition->composition_data->frameDuration;

				uint32_t frame = (uint32_t)(node->current_time / frameDuration);

				__make_mesh_vertices( layer->mesh, frame, node->matrix, _vertices );
			}

			_vertices->r = resource_solid->r;
			_vertices->g = resource_solid->g;
			_vertices->b = resource_solid->b;
			_vertices->a = node->opacity;
		}break;
	case AE_MOVIE_LAYER_TYPE_SEQUENCE:
		{
			aeMovieResourceSequence * resource_sequence = (aeMovieResourceSequence *)resource;

			float frameDuration = resource_sequence->frameDuration;

			uint32_t frame_sequence;

			if( layer->reverse_time == AE_TRUE )
			{
				frame_sequence = (uint32_t)((node->out_time - node->in_time - node->current_time) / frameDuration);
			}
			else
			{
				frame_sequence = (uint32_t)(node->current_time / frameDuration);
			}

			frame_sequence %= resource_sequence->image_count;

			aeMovieResourceImage * resource_image = resource_sequence->images[frame_sequence];

			_vertices->resource_type = resource_image->type;
			_vertices->resource_data = resource_image->data;
	
			float offset_x = resource_image->offset_x;
			float offset_y = resource_image->offset_y;

			float width = resource_image->trim_width;
			float height = resource_image->trim_height;

			__make_sprite_vertices( _context, offset_x, offset_y, width, height, node->matrix, _vertices );

			_vertices->r = 1.f;
			_vertices->g = 1.f;
			_vertices->b = 1.f;
			_vertices->a = node->opacity;
		}break;
	case AE_MOVIE_LAYER_TYPE_VIDEO:
		{
			aeMovieResourceVideo * resource_video = (aeMovieResourceVideo *)resource;

			float width = resource_video->width;
			float height = resource_video->height;

			__make_sprite_vertices( _context, 0.f, 0.f, width, height, node->matrix, _vertices );

			_vertices->r = 1.f;
			_vertices->g = 1.f;
			_vertices->b = 1.f;
			_vertices->a = node->opacity;
		}break;
	case AE_MOVIE_LAYER_TYPE_IMAGE:
		{
			aeMovieResourceImage * resource_image = (aeMovieResourceImage *)resource;

			float offset_x = resource_image->offset_x;
			float offset_y = resource_image->offset_y;

			float width = resource_image->trim_width;
			float height = resource_image->trim_height;

			__make_sprite_vertices( _context, offset_x, offset_y, width, height, node->matrix, _vertices );

			_vertices->r = 1.f;
			_vertices->g = 1.f;
			_vertices->b = 1.f;
			_vertices->a = node->opacity;
		}break;
	}
}
//////////////////////////////////////////////////////////////////////////