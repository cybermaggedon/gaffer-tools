
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <redland.h>
#include <stdexcept>

#ifndef STORE
#define STORE "gaffer"
#endif

#ifndef STORE_NAME
#define STORE_NAME "http://localhost:8080/example-rest/v1"
#endif

#ifndef BASE_URI
#define BASE_URI "https://github.com/cybermaggedon/"
#endif

librdf_world* world;
librdf_model* model;
librdf_uri* base_uri;

/* HTTP request handler. */
static int request_handler(void* cls, struct MHD_Connection * connection,
			   const char * url, const char * method,
			   const char * version, const char * upload_data,
			   size_t * upload_data_size, void ** ptr)
{
    
    struct MHD_Response * response;
    int ret;

    /* This detects first call of the function when we have the HTTP header. 
       We accept all headers. */
    if (*ptr) {
	/* The first time we only have headers, don't respond. */
	*ptr = (void*) 1;
	return MHD_YES;
    }

    const char* cb;
    const char* output;
    const char* query;
    
    librdf_query* qry = 0;
    librdf_query_results* results = 0;
    unsigned char* out = 0;

    std::string mime_type;

    size_t len = 0;

    try {

	/* Get arguments: output, query and callback. */
	output = MHD_lookup_connection_value(connection,
					     MHD_GET_ARGUMENT_KIND,
					     "output");
	query = MHD_lookup_connection_value(connection,
					    MHD_GET_ARGUMENT_KIND,
					    "query");
	cb = MHD_lookup_connection_value(connection,
					 MHD_GET_ARGUMENT_KIND,
					 "callback");

	/* Only respond to the GET method. */
	if (0 != strcmp(method, "GET"))
	    return MHD_NO; /* unexpected method */
	
	/* Flip out if GET has a payload. */
	if (0 != *upload_data_size)
	    return MHD_NO; /* upload data in a GET!? */
	
	*ptr = NULL; /* clear context pointer */

	/* Create new query */
	qry = librdf_new_query(world, "sparql", 0,
			       (const unsigned char*) query,
			       base_uri);
	if (qry == 0)
	    throw std::runtime_error("Couldn't create query");

	/* Execute query */
	results = librdf_query_execute(qry, model);
	if (results == 0)
	    throw std::runtime_error("Couldn't execute query");
	
	librdf_free_query(qry);
	qry = 0;

	/* Convert query results to either JSON or XML */
	if (output && strcmp(output, "json") == 0) {

	    /* JSON */
	    mime_type = "application/sparql-results+json";
	    out = librdf_query_results_to_counted_string2(results, "json",
							  0, 0, 0,
							  &len);
	} else {

	    /* XML */
	    mime_type = "text/plain";  // FIXME
	    out = librdf_query_results_to_counted_string2(results, 0, 0, 0, 0,
							  &len);
	}

	if (out == 0)
	    throw std::runtime_error("Couldn't output query results as string");
	
    } catch (std::exception& e) {

	if (qry)
	    librdf_free_query(qry);

	if (results)
	    librdf_free_query_results(results);

	response = MHD_create_response_from_buffer(0, (void*) "",
						   MHD_RESPMEM_MUST_COPY);

	ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
				 response);
	MHD_destroy_response(response);

	return ret;

    }

    if (cb != 0) {

	/* JSONP request case.  Need to convert DATA into callback(DATA) */
	int len2 = strlen(cb) + 2 + len;
	char* out2 = (char*) malloc(len2);
	char* ptr = out2;
	strcpy(ptr, cb);
	ptr += strlen(cb);
	*(ptr++) = '(';
	memcpy(ptr, out, len);
	ptr += len;
	*ptr = ')';

	/* Create HTTP response. */
	response = MHD_create_response_from_buffer (len2, 
						    (void*) out2,
						    MHD_RESPMEM_MUST_COPY);
	free(out2);
	librdf_free_memory(out);
	
    } else {

	/* Create HTTP response */
	response = MHD_create_response_from_buffer (len, 
						    (void*) out,
						    MHD_RESPMEM_MUST_COPY);
	librdf_free_memory(out);

    }

    librdf_free_query_results(results);

    /* HTTP header */
    MHD_add_response_header(response, "Content-Type", mime_type.c_str());
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Expires", "0");
    MHD_add_response_header(response, "Cache-Control",
			    "no-cache, no-store, must-revalidate");
    MHD_add_response_header(response, "Pragma", "no-cache");
    MHD_add_response_header(response, "Server", "Mark's SPARQL Server");

    /* Queue the response for sending */
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    
    MHD_destroy_response(response);
    
    return ret;
}

int main(int argc, char ** argv)
{

    /* Create RDF world. */
    world = librdf_new_world();

    /* Connect to storage. */
    librdf_storage* storage =
	librdf_new_storage(world, STORE, STORE_NAME, "");
    if (storage == 0)
	throw std::runtime_error("Didn't get storage");

    /* Triple store on the storage. */
    model = librdf_new_model(world, storage, 0);
    if (model == 0)
	throw std::runtime_error("Couldn't construct model");

    /* Base URI for relative data. */
    base_uri =
    	librdf_new_uri(world, (const unsigned char*) BASE_URI);
    if (base_uri == 0)
    	throw std::runtime_error("Couldn't parse URI.");
  
    struct MHD_Daemon * d;

    if (argc != 2) {
	printf("%s PORT\n", argv[0]);
	return 1;
    }

    /* libmicrohttpd web server. */
    d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, atoi(argv[1]),
			 NULL, NULL, &request_handler, (void *) model,
			 MHD_OPTION_END);
    if (d == NULL)
	exit(1);

    /* Wait forever. */
    while (1) {
	sleep(10);
    }

    MHD_stop_daemon(d);

    exit(0);
    
}


