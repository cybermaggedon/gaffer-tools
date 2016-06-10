
#include <stdio.h>
#include <iostream>
#include <redland.h>
#include <stdlib.h>
#include <unistd.h>
#include <rdf_parser.h>

#ifndef STORE
#define STORE "gaffer"
#endif

#ifndef STORE_NAME
#define STORE_NAME "http://localhost:8080/example-rest/v1"
#endif

int main(int argc, char** argv)
{

    if (argc != 2) {
	fprintf(stderr, "Arguments:\n\tbulk_load <file>\n");
	exit(1);
    }

    FILE* fp = fopen(argv[1], "r");
    if (fp == 0) {
	perror("fopen");
	exit(1);
    }

    librdf_world* world = librdf_new_world();

    librdf_uri* uri1 =
	librdf_new_uri(world, (const unsigned char*) "http://bunchy.org");

    // Get RDF parser.
    librdf_parser* parser = librdf_new_parser(world, "turtle", 0, 0);
    if (parser == 0) {
	fprintf(stderr, "Couldn't create RDF parser.\n");
	exit(1);
    }

    librdf_stream* stream  =
	librdf_parser_parse_file_handle_as_stream(parser, fp, 1, uri1);
    if (stream == 0) {
	fprintf(stderr, "Couldn't create RDF stream.\n");
	exit(1);
    }

    librdf_storage* storage =
	librdf_new_storage(world, STORE, STORE_NAME, 0);
    if (storage == 0)
	throw std::runtime_error("Didn't get storage");

    librdf_model* model =
	librdf_new_model(world, storage, 0);
    if (model == 0)
	    throw std::runtime_error("Couldn't construct model");

    librdf_model_add_statements(model, stream);

    exit(0);
    
}

