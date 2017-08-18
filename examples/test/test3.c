/*
==================================================
	EXAMPLE #3

Updates & rendering.
==================================================
*/

#include "common.h"

int main( int argc, char *argv[] ) {
	//
	// Initialize library instance, movie & composition names.
	//

	ex_init();

	//
	// Set pointer to resource loader callback.
	//

	ex.resource_provider = &ex_callback_resource_provider;

	//
	// Load movie data structure, which in particular contains info about all compositions in the movie.
	//

	ex_load_movie_data();

	//
	// Set composition data provider callbacks here.
	//

	ex.comp_providers.camera_provider = &ex_callback_camera_provider;
	ex.comp_providers.node_provider = &ex_callback_node_provider;
	ex.comp_providers.node_destroyer = &ex_callback_node_destroyer;
	ex.comp_providers.node_update = &ex_callback_node_update;
	ex.comp_providers.track_matte_update = &ex_callback_track_matte_update;
	ex.comp_providers.composition_event = &ex_callback_composition_event;
	ex.comp_providers.composition_state = &ex_callback_composition_state;

	//
	// Select the composition & load its data (nodes, cameras etc.).
	//

	ex_set_composition();

	//
	// Play the movie.
	//

	EX_LOG("The movie is playing in loop now. Press Ctrl-C to break...\n");

	ae_set_movie_composition_loop(ex.composition, AE_TRUE);
	ae_play_movie_composition(ex.composition, 0.f);

	int last = ex_get_time();

	while ( 1 ) {
		int t = ex_get_time();
		int dt = t - last;
		last = t;

		ex_update(dt);
		ex_render();
	}

	//
	// Never gets here.
	//

	return 0;
}
