/*
==================================================
	EXAMPLE #2

Add resource, node provider, composition state & other callbacks.
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
	// Usually it can be played at this moment.
	//

	EX_LOG("The movie composition, resources & nodes have loaded successfully. See further examples.\n");

	return 0;
}
