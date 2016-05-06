#	ifndef MOVIE_TRANSFORMATION_H_
#	define MOVIE_TRANSFORMATION_H_

#	include "movie/movie_instance.h"
#	include "movie/movie_type.h"

typedef enum aeMoviePropertyImmutableEnum
{
	AE_MOVIE_IMMUTABLE_ANCHOR_POINT_X = 0x0001,
	AE_MOVIE_IMMUTABLE_ANCHOR_POINT_Y = 0x0002,
	AE_MOVIE_IMMUTABLE_ANCHOR_POINT_Z = 0x0004,
	AE_MOVIE_IMMUTABLE_POSITION_X = 0x0008,
	AE_MOVIE_IMMUTABLE_POSITION_Y = 0x0010,
	AE_MOVIE_IMMUTABLE_POSITION_Z = 0x0020,
	AE_MOVIE_IMMUTABLE_ROTATION_X = 0x0040,
	AE_MOVIE_IMMUTABLE_ROTATION_Y = 0x0080,
	AE_MOVIE_IMMUTABLE_ROTATION_Z = 0x0100,
	AE_MOVIE_IMMUTABLE_SCALE_X = 0x0400,
	AE_MOVIE_IMMUTABLE_SCALE_Y = 0x0800,
	AE_MOVIE_IMMUTABLE_SCALE_Z = 0x1000,
	AE_MOVIE_IMMUTABLE_OPACITY = 0x2000,
	__AE_MOVIE_IMMUTABLE_END__
} aeMoviePropertyImmutableEnum;

typedef struct aeMovieLayerTransformation
{
	uint32_t immutable_property_mask;

	float immuttable_anchor_point_x;
	float immuttable_anchor_point_y;
	float immuttable_anchor_point_z;
	float immuttable_position_x;
	float immuttable_position_y;
	float immuttable_position_z;
	float immuttable_rotation_x;
	float immuttable_rotation_y;
	float immuttable_rotation_z;
	float immuttable_scale_x;
	float immuttable_scale_y;
	float immuttable_scale_z;
	float immuttable_opacity;

	float * property_anchor_point_x;
	float * property_anchor_point_y;
	float * property_anchor_point_z;
	float * property_position_x;
	float * property_position_y;
	float * property_position_z;
	float * property_rotation_x;
	float * property_rotation_y;
	float * property_rotation_z;
	float * property_scale_x;
	float * property_scale_y;
	float * property_scale_z;
	float * property_opacity;
} aeMovieLayerTransformation;

aeMovieResult load_movie_layer_transformation( const aeMovieInstance * _instance, const aeMovieStream * _stream, aeMovieLayerTransformation * _transformation, uint32_t _count );
void delete_movie_layer_transformation( const aeMovieInstance * _instance, const aeMovieLayerTransformation * _transformation );

float make_movie_layer_transformation( ae_matrix4_t _out, const aeMovieLayerTransformation * _transformation, uint32_t _index, ae_bool_t _interpolate, float _t );

#	endif