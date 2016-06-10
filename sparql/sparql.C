
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

#define PAGE "<html><head><title>libmicrohttpd demo</title>"	\
    "</head><body>libmicrohttpd demo</body></html>"

librdf_world* world;
librdf_model* model;

static int request_handler(void* cls, struct MHD_Connection * connection,
			   const char * url, const char * method,
			   const char * version, const char * upload_data,
			   size_t * upload_data_size, void ** ptr)
{
    
    const char * page = (char*) cls;
    struct MHD_Response * response;
    int ret;

    if (*ptr) {
	/* The first time only the headers are valid, don't respond. */
	*ptr = (void*) 1;
	return MHD_YES;
    }

    const char* output = MHD_lookup_connection_value(connection,
						     MHD_GET_ARGUMENT_KIND,
						     "output");
  
    const char* query = MHD_lookup_connection_value(connection,
						    MHD_GET_ARGUMENT_KIND,
    						    "query");
  
    const char* cb = MHD_lookup_connection_value(connection,
    						 MHD_GET_ARGUMENT_KIND,
    						 "callback");
  
    if (0 != strcmp(method, "GET"))
    	return MHD_NO; /* unexpected method */
  
    if (0 != *upload_data_size)
    	return MHD_NO; /* upload data in a GET!? */
    
    *ptr = NULL; /* clear context pointer */
  
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
    			 (const unsigned char*) query,
    			 uri2);
    
    librdf_free_uri(uri1);
    librdf_free_uri(uri2);
    
    if (qry == 0)
    	throw std::runtime_error("Couldn't create query");

    librdf_query_results* results = librdf_query_execute(qry, model);
    if (results == 0)
    	throw std::runtime_error("Couldn't execute query");
    
    librdf_free_query(qry);

    size_t len = 0;
    unsigned char* out;

    std::string mime_type;
    
    if (output && strcmp(output, "json") == 0) {
    	mime_type = "application/sparql-results+json";
    	out = librdf_query_results_to_counted_string2(results, "json", 0, 0, 0,
    						      &len);
    } else {
    	mime_type = "text/plain";  // FIXME
    	out = librdf_query_results_to_counted_string2(results, 0, 0, 0, 0,
						      &len);
    }

    if (out == 0)
	throw std::runtime_error("Couldn't output query results as string");

    if (cb != 0) {
	int len2 = strlen(cb) + 2 + len;
	char* out2 = (char*) malloc(len2);
	char* ptr = out2;
	strcpy(ptr, cb);
	ptr += strlen(cb);
	*(ptr++) = '(';
	memcpy(ptr, out, len);
	ptr += len;
	*ptr = ')';

	response = MHD_create_response_from_buffer (len2, 
						    (void*) out2,
						    MHD_RESPMEM_MUST_COPY);

	free(out2);
	librdf_free_memory(out);
	
    } else {
      
	response = MHD_create_response_from_buffer (len, 
						    (void*) out,
						    MHD_RESPMEM_MUST_COPY);

	librdf_free_memory(out);

    }

    librdf_free_query_results(results);

    MHD_add_response_header(response, "Content-Type", mime_type.c_str());
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Expires", "0");
    MHD_add_response_header(response, "Cache-Control",
			    "no-cache, no-store, must-revalidate");
    MHD_add_response_header(response, "Pragma", "no-cache");
    MHD_add_response_header(response, "Server", "Mark's SPARQL Server");

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    
    MHD_destroy_response(response);
    
    return ret;
}

int main(int argc, char ** argv)
{

    world = librdf_new_world();

    librdf_storage* storage =
	librdf_new_storage(world, STORE, STORE_NAME, "");
    if (storage == 0)
	throw std::runtime_error("Didn't get storage");
  
    model = librdf_new_model(world, storage, 0);
    if (model == 0)
	throw std::runtime_error("Couldn't construct model");

  
    struct MHD_Daemon * d;

    if (argc != 2) {
	printf("%s PORT\n", argv[0]);
	return 1;
    }
    d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, atoi(argv[1]),
			 NULL, NULL, &request_handler, (void *) model,
			 MHD_OPTION_END);
    if (d == NULL)
	exit(1);

    while (1) {
	sleep(10);
    }

    MHD_stop_daemon(d);

    exit(0);
    
}


