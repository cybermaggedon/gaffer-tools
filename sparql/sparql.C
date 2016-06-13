
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

typedef struct {

    struct MHD_PostProcessor* postproc;
    char* query;
    int query_len;
    char* output;
    int output_len;
    char* callback;
    int callback_len;
    
} connection_info;

const int post_buffer_size = 8192;

int iterate_post(void* cls, enum MHD_ValueKind kind, const char* key,
		 const char* filename, const char* content_type,
		 const char* transfer_encoding, const char* data,
		 uint64_t off, size_t size)
{
    connection_info* con = (connection_info*) cls;

    if (strcmp(key, "query") == 0) {
	
	if (con->query)
	    con->query = (char*) realloc(con->query, con->query_len + size + 1);
	else
	    con->query = (char*) malloc(size + 1);

	memcpy(con->query + con->query_len, data, size);

	con->query_len += size;
	con->query[con->query_len] = 0;

    }

    if (strcmp(key, "output") == 0) {
	
	if (con->output)
	    con->output = (char*) realloc(con->output,
					  con->output_len + size + 1);
	else
	    con->output = (char*) malloc(size + 1);

	memcpy(con->output + con->output_len, data, size);

	con->output_len += size;
	con->output[con->output_len] = 0;

    }

    if (strcmp(key, "callback") == 0) {
	
	if (con->callback)
	    con->callback = (char*) realloc(con->callback,
				    con->callback_len + size + 1);
	else
	    con->callback = (char*) malloc(size + 1);

	memcpy(con->callback + con->callback_len, data, size);

	con->callback_len += size;
	con->callback[con->callback_len] = 0;

    }

}

int sparql(MHD_Connection* connection, const char* query, const char* output,
	    const char* callback)
{

    struct MHD_Response* response;
    int ret;
    
    librdf_query* qry = 0;
    librdf_query_results* results = 0;
    unsigned char* out = 0;

    std::string mime_type;

    size_t len = 0;

    try {

	if (query) {
	    printf("Query: %s\n", query);
	}

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

	enum { IS_GRAPH, IS_BINDINGS, IS_BOOLEAN } results_type;

	if (librdf_query_results_is_graph(results))
	    results_type = IS_GRAPH;
	else if (librdf_query_results_is_bindings(results))
	    results_type = IS_BINDINGS;
	if (librdf_query_results_is_boolean(results))
	    results_type = IS_BOOLEAN;

	/* Convert query results to either JSON or XML */
	if (output && strcmp(output, "json") == 0) {

	    /* JSON */
	    mime_type = "application/sparql-results+json";
	    out = librdf_query_results_to_counted_string2(results, "json",
							  0, 0, 0,
							  &len);
	} else if (results_type == IS_GRAPH) {

	    mime_type = "application/rdf+xml";

	    librdf_stream* strm = librdf_query_results_as_stream(results);
	    if (strm == 0)
		throw std::runtime_error("Couldn't stream results");
	    
	    librdf_serializer* srl = librdf_new_serializer(world, "rdfxml",
							   0, 0);
	    if (srl == 0)
		throw std::runtime_error("Couldn't create serialiser");
    
	     out =
		 librdf_serializer_serialize_stream_to_counted_string(srl, 0,
								      strm,
								      &len);

	     librdf_free_serializer(srl);
	     librdf_free_stream(strm);

	} else {

	    /* XML */
	    mime_type = "application/sparql-results+xml";
	    out = librdf_query_results_to_counted_string2(results, "xml", 0, 0,
							  0, &len);
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

    if (callback != 0) {

	/* JSONP request case.  Need to convert DATA into callback(DATA) */
	int len2 = strlen(callback) + 2 + len;
	char* out2 = (char*) malloc(len2);
	char* ptr = out2;
	strcpy(ptr, callback);
	ptr += strlen(callback);
	*(ptr++) = '(';
	memcpy(ptr, out, len);
	ptr += len;
	*ptr = ')';

	write(1, out2, len2);
	/* Create HTTP response. */
	response = MHD_create_response_from_buffer (len2, 
						    (void*) out2,
						    MHD_RESPMEM_MUST_COPY);
	free(out2);
	librdf_free_memory(out);
	
    } else {
	
	write(1, out, len);

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

/* HTTP request handler. */
static int request_handler(void* cls, struct MHD_Connection* connection,
			   const char* url, const char* method,
			   const char* version, const char * upload,
			   size_t* upload_size, void** ptr)
{
    
    connection_info* con;

    /* This detects first call of the function when we have the HTTP header. 
       We accept all headers. */
    if (*ptr == 0) {

	con = (connection_info*) malloc(sizeof(connection_info));
	if (con == 0) {
	    fprintf(stderr, "malloc failed");
	    return MHD_NO;
	}

	con->postproc = 0;
	con->query = 0;
	con->query_len = 0;
	con->callback = 0;
	con->callback_len = 0;
	con->output = 0;
	con->output_len = 0;
	*ptr = con;

	if (strcmp(method, "POST") == 0) {
	    con->postproc = MHD_create_post_processor(connection,
						      post_buffer_size,
						      iterate_post,
						      (void*) con);
	    if (con->postproc == 0) {
		fprintf(stderr, "Create post processor failed.\n");
		free(con);
		return MHD_NO;
	    }
	    
	}

	return MHD_YES;
    }

    con = (connection_info* ) *ptr;

    if (strcmp(method, "GET") == 0) {

	const char* cb = 0;
	const char* output = 0;
	const char* query = 0;
	const char* accept = 0;

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

	// FIXME: Should use the accept information to work out what to send
	// back.
	accept = MHD_lookup_connection_value(connection,
					     MHD_HEADER_KIND,
					     "accept");

	if (query == 0)
	    return MHD_NO;
	
	return sparql(connection, query, output, cb); 
	
    }

    if (strcmp(method, "POST") == 0) {

	if (*upload_size != 0) {
	    MHD_post_process(con->postproc, upload, *upload_size);
	    *upload_size = 0;
	    return MHD_YES;
	}

	printf("DONE\n");

	return sparql(connection, con->query, con->output, con->callback);

    }

    return MHD_NO;

}

void request_completed(void* cls, struct MHD_Connection* connection,
		       void** con_cls, enum MHD_RequestTerminationCode code)
{

    connection_info* con = (connection_info*) *con_cls;
    if (con) {
	if (con->postproc)
	    MHD_destroy_post_processor(con->postproc);
	if (con->query) free(con->query);
	if (con->output) free(con->output);
	if (con->callback) free(con->callback);
	free(con);
    }
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
			 MHD_OPTION_NOTIFY_COMPLETED, &request_completed, 0,
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


