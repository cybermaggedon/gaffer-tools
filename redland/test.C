
#include <iostream>
#include <redland.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef STORE
#define STORE "sqlite"
#endif

#ifndef STORE_NAME
#define STORE_NAME "STORE.db"
#endif

const std::string query_string1 =
    "CONSTRUCT { <http://gaffer.test/#fred> ?b ?c . } WHERE { <http://gaffer.test/#fred> ?b ?c . }";

const std::string query_string2 =
    "CONSTRUCT { ?a ?b <http://gaffer.test/#cat> . } WHERE { ?a ?b <http://gaffer.test/#cat> . }";

const std::string query_string3 =
    "CONSTRUCT { ?a <http://gaffer.test/#is_a> ?c . } WHERE { ?a <http://gaffer.test/#is_a> ?c . }";

const std::string query_string4 =
    "CONSTRUCT { <http://gaffer.test/#fred> <http://gaffer.test/#is_a> ?c . } WHERE {  <http://gaffer.test/#fred> <http://gaffer.test/#is_a> ?c . }";

const std::string query_string5 =
    "CONSTRUCT { ?a <http://gaffer.test/#is_a> <http://gaffer.test/#cat> . } WHERE { ?a <http://gaffer.test/#is_a> <http://gaffer.test/#cat> . }";

const std::string query_string6 =
    "CONSTRUCT { <http://gaffer.test/#fred> ?b <http://gaffer.test/#cat> . } WHERE { <http://gaffer.test/#fred> ?b <http://gaffer.test/#cat> . }";

const std::string query_string7 =
    "ASK { <http://gaffer.test/#fred> <http://gaffer.test/#is_a> <http://gaffer.test/#cat> . }";

const std::string query_string8 =
    "CONSTRUCT { ?a ?b ?c . } WHERE { ?a ?b ?c . }";

void output_node(librdf_node* n)
{

    if (librdf_node_is_literal(n)) {
	std::cout << librdf_node_get_literal_value(n);
    }

    if (librdf_node_is_resource(n)) {
	librdf_uri* uri = librdf_node_get_uri(n);
	std::cout << librdf_uri_as_string(uri);
    }

}

void run_query(librdf_world* world, librdf_model* model, const std::string& q)
{
    librdf_stream* strm;

    std::cout << "** Query: " << q << std::endl;

    librdf_uri* uri1 =
	librdf_new_uri(world, (const unsigned char*) "http://bunchy.org");

    if (uri1 == 0)
	throw std::runtime_error("Couldn't parse URI");
    
    librdf_uri* uri2 =
	librdf_new_uri(world, (const unsigned char*) "http://bunchy.org");
    if (uri2 == 0)
	throw std::runtime_error("Couldn't parse URI");
    
    librdf_query* qry =
	librdf_new_query(world, "sparql", uri1,
			 (const unsigned char*) q.c_str(),
			 uri2);
    
    librdf_free_uri(uri1);
    librdf_free_uri(uri2);
    
    if (qry == 0)
	throw std::runtime_error("Couldn't parse query.");
    
    librdf_query_results* results = librdf_query_execute(qry, model);
    if (results == 0)
	throw std::runtime_error("Couldn't execute query");
    
    librdf_free_query(qry);
    
    strm = librdf_query_results_as_stream(results);
    if (strm == 0)
	throw std::runtime_error("Couldn't stream results");
    
    librdf_serializer* srl = librdf_new_serializer(world, "ntriples",
						   0, 0);
    if (srl == 0)
	throw std::runtime_error("Couldn't create serialiser");
    
    size_t len;
    
    unsigned char* out =
	librdf_serializer_serialize_stream_to_counted_string(srl, 0,
							     strm, &len);
    
    std::cout << "--------------------------------------------------------"
	      << std::endl;
    write(1, out, len);
    std::cout << "--------------------------------------------------------"
	      << std::endl;
    
    free(out);
    
    librdf_free_serializer(srl);

    librdf_free_stream(strm);
    
    librdf_free_query_results(results);
    
}

void run_query2(librdf_world* world, librdf_model* model, const std::string& q)
{

    librdf_stream* strm;

    std::cout << "** Query: " << q << std::endl;

    librdf_uri* uri1 =
	librdf_new_uri(world, (const unsigned char*) "http://bunchy.org");

    if (uri1 == 0)
	throw std::runtime_error("Couldn't parse URI");
    
    librdf_uri* uri2 =
	librdf_new_uri(world, (const unsigned char*) "http://bunchy.org");
    if (uri2 == 0)
	throw std::runtime_error("Couldn't parse URI");
    
    librdf_query* qry =
	librdf_new_query(world, "sparql", uri1,
			 (const unsigned char*) q.c_str(),
			 uri2);
    
    librdf_free_uri(uri1);
    librdf_free_uri(uri2);
    
    if (qry == 0)
	throw std::runtime_error("Couldn't parse query.");
    
    librdf_query_results* results = librdf_query_execute(qry, model);
    if (results == 0)
	throw std::runtime_error("Couldn't execute query");
    
    librdf_free_query(qry);

    std::cout << "--------------------------------------------------------"
	      << std::endl;

    while (true) {
      
	if (librdf_query_results_finished(results)) break;
      
	int bcount = librdf_query_results_get_bindings_count(results);
	if (bcount < 0)
	    throw std::runtime_error("Couldn't get query bindings count");
      
	for(int i = 0; i < bcount; i++) {
	
	    if (i != 0) std::cout << " ";
	
	    librdf_node* value =
		librdf_query_results_get_binding_value(results, i);
	
	    if (value == 0)
		throw std::runtime_error("Couldn't get results binding");
	
	    output_node(value);

	    librdf_free_node(value);

	}

	std::cout << std::endl;

	librdf_query_results_next(results);

    }

    std::cout << "--------------------------------------------------------"
	      << std::endl;

    librdf_free_query_results(results);
    
}

int main(int argc, char** argv)
{

    try {

	/*********************************************************************/
	/* Initialise                                                        */
	/*********************************************************************/

	std::cout << "** Initialise" << std::endl;

	librdf_world* world = librdf_new_world();

	if (world == 0)
	    throw std::runtime_error("Didn't get world");

	librdf_storage* storage =
	    librdf_new_storage(world, STORE, STORE_NAME, "new='yes'");
	if (storage == 0)
	    throw std::runtime_error("Didn't get storage");

	librdf_model* model =
	    librdf_new_model(world, storage, 0);
	if (model == 0)
	    throw std::runtime_error("Couldn't construct model");

	/*********************************************************************/
	/* Create in-memory model                                            */
	/*********************************************************************/

	std::cout << "** Create in-memory model" << std::endl;

	librdf_storage* mstorage =
	    librdf_new_storage(world, "memory", 0, 0);
	if (storage == 0)
	    throw std::runtime_error("Didn't get storage");

	librdf_model* mmodel =
	    librdf_new_model(world, mstorage, 0);
	if (model == 0)
	    throw std::runtime_error("Couldn't construct model");

	for(int i = 0; i < 10; i++) {

	    char sbuf[256];
	    char obuf[256];

	    sprintf(sbuf, "http://gaffer.test/number#%d", i);
	    sprintf(obuf, "http://gaffer.test/number#%d", i+1);

	    librdf_node *s =
		librdf_new_node_from_uri_string(world,
						(const unsigned char *) sbuf);

	    librdf_node *p =
		librdf_new_node_from_uri_string(world,
						(const unsigned char *)
						"http://gaffer.test/number#is_before");

	    librdf_node *o =
		librdf_new_node_from_uri_string(world,
						(const unsigned char*) obuf);

	    librdf_statement* st = librdf_new_statement_from_nodes(world,
								   s, p, o);

	    librdf_model_add_statement(mmodel, st);

	    librdf_free_statement(st);

	}

	/*********************************************************************/
	/* Size                                                              */
	/*********************************************************************/

	std::cout << "** Model size is " << librdf_model_size(model)
		  << std::endl;

	/*********************************************************************/
	/* Add statement                                                     */
	/*********************************************************************/


	std::cout << "** Add statement" << std::endl;

	const char* fred = "http://gaffer.test/#fred";
	const char* is_a = "http://gaffer.test/#is_a";
	const char* cat = "http://gaffer.test/#cat";

	librdf_node *s =
	    librdf_new_node_from_uri_string(world,
					    (const unsigned char *) fred);
	librdf_node *p =
	    librdf_new_node_from_uri_string(world,
					    (const unsigned char *) is_a);
	librdf_node *o =
	    librdf_new_node_from_uri_string(world,
					    (const unsigned char *) cat);

	librdf_statement* st = librdf_new_statement_from_nodes(world, s, p, o);

	librdf_model_add_statement(model, st);

	/*********************************************************************/
	/* Add memory model statements                                       */
	/*********************************************************************/

	std::cout << "** Add statements" << std::endl;

	librdf_stream* strm = librdf_model_as_stream(mmodel);
	if (strm == 0)
	    throw std::runtime_error("Couldn't get memory stream");
	
	int ret = librdf_model_add_statements(model, strm);
	if (ret != 0)
	    throw std::runtime_error("Couldn't add_statements");

	librdf_free_stream(strm);

	librdf_free_model(mmodel);

	librdf_free_storage(mstorage);

	/*********************************************************************/
	/* Run queries                                                       */
	/*********************************************************************/

	run_query(world, model, query_string8);
	run_query(world, model, query_string1);
	run_query(world, model, query_string2);
	run_query(world, model, query_string3);
	run_query(world, model, query_string4);
	run_query(world, model, query_string5);
	run_query(world, model, query_string6);
	run_query2(world, model, query_string7);

	/*********************************************************************/
	/* Remove statement                                                  */
	/*********************************************************************/

	std::cout << "** Remove statements" << std::endl;

	librdf_model_remove_statement(model, st);
	
	/*********************************************************************/
	/* Serialise                                                         */
	/*********************************************************************/

	std::cout << "** Serialise" << std::endl;

	strm = librdf_model_as_stream(model);
	if (strm == 0)
	    throw std::runtime_error("Couldn't get model as stream");

	librdf_serializer* srl = librdf_new_serializer(world, "ntriples",
						       0, 0);
	if (srl == 0)
	    throw std::runtime_error("Couldn't create serialiser");

	size_t len;
	unsigned char* out =
	    librdf_serializer_serialize_stream_to_counted_string(srl, 0,
								 strm, &len);

	std::cout << "--------------------------------------------------------"
		  << std::endl;
	write(1, out, len);
	std::cout << "--------------------------------------------------------"
		  << std::endl;

	free(out);

	librdf_free_stream(strm);

	librdf_free_serializer(srl);
	
	/*********************************************************************/
	/* Cleanup                                                           */
	/*********************************************************************/

	librdf_free_statement(st);

	librdf_free_model(model);

	librdf_free_storage(storage);

	librdf_free_world(world);

    } catch (std::exception& e) {

	std::cerr << e.what() << std::endl;

    }

}

