#ifndef MOVIE_MOVIE_RESOURCE_H_
#define MOVIE_MOVIE_RESOURCE_H_

#	include <movie/movie_type.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		uint8_t type;

	} aeMovieResource;

	typedef struct
	{
		aeMovieResource base;

		ae_string_t name;

	} aeMovieResourceInternal;

	typedef struct
	{
		aeMovieResource base;

		aeMoviePolygon * polygons;

	} aeMovieResourceSocketShape;

	typedef struct
	{
		aeMovieResource base;

		ae_string_t path;

	} aeMovieResourceSocketImage;
	
	typedef struct
	{
		aeMovieResource base;

		float width;
		float height;
		float r;
		float g;
		float b;

	} aeMovieResourceSolid;

	typedef struct
	{
		aeMovieResource base;

		ae_string_t path;

		uint8_t alpha;
		float frameRate;
		float duration;

	} aeMovieResourceVideo;

	typedef struct
	{
		aeMovieResource base;

		ae_string_t path;

		float duration;

	} aeMovieResourceSound;

	typedef struct
	{
		aeMovieResource base;

		ae_string_t path;

		float base_width;
		float base_height;
		float trim_width;
		float trim_height;
		float trim_offset_x;
		float trim_offset_y;

	} aeMovieResourceImage;

	typedef struct
	{
		aeMovieResource base;

		float frameDuration;

		uint32_t count;
		aeMovieResourceImage ** images;

	} aeMovieResourceImageSequence;

#ifdef __cplusplus
}
#endif

#endif