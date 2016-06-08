
#include <curl/curl.h>
#include <gaffer_comms.h>
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// FIXME: Tidy up memory handling FTLOP.

struct gaffer_comms_str {
    CURL* curl;
    char* url;
};

typedef struct {
    char *buffer;
    size_t size;
} http_dload_buffer;
 
typedef struct {
    const char *buffer;
    size_t size;
    size_t pos;
} http_upload_buffer;
 
static 
void add_result(gaffer_results* res,
		const char* s, const char* p, const char* o,
		gaffer_type stype, gaffer_type ptype, gaffer_type otype)
{

    res->count++;

    if (res->results)
	res->results =
	    (gaffer_result*) realloc(res->results,
				     res->count * sizeof(gaffer_result));
	else
	res->results =
	    (gaffer_result*) malloc(res->count * sizeof(gaffer_result));

    gaffer_result* result = res->results + (res->count - 1);
    
    result->s.term = strdup(s);
    result->p.term = strdup(p);
    result->o.term = strdup(o);
    result->s.type = stype;
    result->p.type = ptype;
    result->o.type = otype;

}


static size_t
http_dload_callback(void *contents, size_t size, size_t nmemb, void *userp)
{

    size_t realsize = size * nmemb;
    http_dload_buffer *buffer = (http_dload_buffer *)userp;

    if (buffer->buffer == 0)
	buffer->buffer = malloc(buffer->size + realsize + 1);
    else
	buffer->buffer = realloc(buffer->buffer, buffer->size + realsize + 1);

    if(buffer->buffer == 0) {
	/* out of memory! */ 
	fprintf(stderr, "not enough memory (realloc returned NULL)\n");
	return 0;
    }

    memcpy(&(buffer->buffer[buffer->size]), contents, realsize);

    buffer->size += realsize;

    // NULL-terminate the string.  Useful if it's going to get JSON-parsed.
    buffer->buffer[buffer->size] = 0;
 
    return realsize;
}

static size_t http_upload_callback(void* ptr, size_t size, size_t nmemb,
				   void *stream)
{

    size_t retcode;
    curl_off_t nread;

    http_upload_buffer* buffer = (http_upload_buffer*) stream;

    size_t xfer = size * nmemb;
    if (xfer > (buffer->size - buffer->pos))
	xfer = buffer->size - buffer->pos;

    memcpy(ptr, buffer->buffer + buffer->pos, xfer);

    buffer->pos += xfer;

    return xfer;

}

static
int gaffer_http_get(gaffer_comms* gc, const char* url,
		    http_dload_buffer* buffer,
		    long* http_code)
{

    // Web look-up for a random page.  FIXME: WHY?!
    CURLcode res;
    buffer->buffer = malloc(1);
    buffer->buffer[0] = 0;
    buffer->size = 0;

    char* url2 = (char*) malloc(strlen(gc->url) + strlen(url) + 10);
    if (url2 == 0) return -1;

    sprintf(url2, "%s%s", gc->url, url);

    curl_easy_setopt(gc->curl, CURLOPT_URL, url2);
    curl_easy_setopt(gc->curl, CURLOPT_WRITEFUNCTION, &http_dload_callback);
    curl_easy_setopt(gc->curl, CURLOPT_WRITEDATA, (void*) buffer);
    curl_easy_setopt(gc->curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(gc->curl, CURLOPT_UPLOAD, 0);
    res = curl_easy_perform(gc->curl);

    free(url2);

    if (res != CURLE_OK) {
	fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	return -1;
    }

    
    curl_easy_getinfo(gc->curl, CURLINFO_RESPONSE_CODE,
		      http_code);

    return 0;

}

static
int gaffer_http_post(gaffer_comms* gc, const char* url,
		     http_upload_buffer* up, http_dload_buffer* down,
		     long* http_code)
{

    CURLcode res;

    up->pos = 0;

    down->buffer = (char *) malloc(1);
    down->buffer[0] = 0;
    down->size = 0;

    char* url2 = (char*) malloc(strlen(gc->url) + strlen(url) + 10);
    if (url2 == 0) return -1;

    sprintf(url2, "%s%s", gc->url, url);

    curl_easy_setopt(gc->curl, CURLOPT_URL, url2);
    curl_easy_setopt(gc->curl, CURLOPT_WRITEFUNCTION, &http_dload_callback);
    curl_easy_setopt(gc->curl, CURLOPT_WRITEDATA, (void*) down);

    if (up) {
	curl_easy_setopt(gc->curl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(gc->curl, CURLOPT_INFILESIZE_LARGE, up->size);
	curl_easy_setopt(gc->curl, CURLOPT_READDATA, (void*) up);
	curl_easy_setopt(gc->curl, CURLOPT_READFUNCTION, &http_upload_callback);
	curl_easy_setopt(gc->curl, CURLOPT_CUSTOMREQUEST, "POST");
#ifdef CURLOPT_EXPECT_100_TIMEOUT_MS
	curl_easy_setopt(gc->curl, CURLOPT_EXPECT_100_TIMEOUT_MS, 1);
#endif
    } else {
	curl_easy_setopt(gc->curl, CURLOPT_UPLOAD, 0);
	curl_easy_setopt(gc->curl, CURLOPT_CUSTOMREQUEST, "GET");
    }

    res = curl_easy_perform(gc->curl);

    free(url2);

    if (res != CURLE_OK) {
	fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	return -1;
    }

    curl_easy_getinfo(gc->curl, CURLINFO_RESPONSE_CODE,
		      http_code);

    return 0;

}

gaffer_comms* gaffer_connect(const char* url)
{

    gaffer_comms* gc = malloc(sizeof(gaffer_comms));
    if (gc == 0)
	return 0;

    gc->url = strdup(url);
    if (gc->url == 0) {
	free(gc);
	return 0;
    }

    gc->curl = curl_easy_init();
    if (gc->curl == 0) {
	free(gc->url);
	free(gc);
	return 0;
    }

    return gc;

}

void gaffer_disconnect(gaffer_comms* gc)
{
    curl_easy_cleanup(gc->curl);
    free(gc->url);
    free(gc);
}

int gaffer_count(gaffer_comms* gc,
		 gaffer_term s, gaffer_term p, gaffer_term o)
{

    // FIXME: Not implemented.
    return 100;

#ifdef BROKEN
    json_object* obj = json_object_new_object();

    if (s) {
	json_object* js = json_object_new_string(s);
	json_object_object_add(obj, "s", js);
    }

    if (p) {
	json_object* jp = json_object_new_string(p);
	json_object_object_add(obj, "p", jp);
    }

    if (o) {
	json_object* jo = json_object_new_string(o);
	json_object_object_add(obj, "o", jo);
    }

    const char* j = json_object_to_json_string(obj);

    // Web look-up for a random page.  FIXME: WHY?!
    http_dload_buffer buffer;

    long http_code;
    
    int ret = gaffer_http_get(gc, "count", &buffer, &http_code);

    if (ret < 0) {
	json_object_put(obj);
	free(buffer.buffer);
	return -1;
    }

    if (http_code != 200) {
	free(buffer.buffer);
	return -1;
    }

    json_object_put(obj);
    
    obj = json_tokener_parse(buffer.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(buffer.buffer);
	return -1;
    }

    free(buffer.buffer);

    json_object* jc;
    
    if (!json_object_object_get_ex(obj, "count", &jc)) {
	fprintf(stderr, "No count parameter\n");
	return -1;
    }

    int count = json_object_get_int(jc);

    json_object_put(obj);

    return count;

#endif

}


int gaffer_test(gaffer_comms* gc)
{

    // FIXME: Use Gaffer status service.
    http_dload_buffer buffer;

    long http_code;
    int ret = gaffer_http_get(gc, "status", &buffer, &http_code);

    if (ret < 0) {
	free(buffer.buffer);
	return -1;
    }

    free(buffer.buffer);

    if (http_code != 200)
	return -1;

    return 0;

}

static
void add_node_object(json_object* elts, const char* node)
{
    // We can get by with just edges.
}

static
void add_edge_object(json_object* elts, const char* edge,
		     const char* src, const char* dest, const char* type)
{
    
    json_object* elt = json_object_new_object();
    json_object_object_add(elt, "class",
			   json_object_new_string("gaffer.data.element.Edge"));

    json_object* namep = json_object_new_object();

    json_object* namefm = json_object_new_object();
    json_object_object_add(namefm, edge, json_object_new_int(1));

    json_object_object_add(namep,
			   "gaffer.function.simple.types.FreqMap", namefm);

    json_object_object_add(namefm, type, json_object_new_int(1));

    json_object* props = json_object_new_object();
    json_object_object_add(props, "name", namep);

    json_object_object_add(elt, "properties", props);

    json_object_object_add(elt, "group", json_object_new_string("BasicEdge"));
    
    json_object_object_add(elt, "source", json_object_new_string(src));
    json_object_object_add(elt, "destination", json_object_new_string(dest));

    json_object_object_add(elt, "directed", json_object_new_boolean(1));


    json_object_array_add(elts, elt);

}

static
char type_to_char(gaffer_type type)
{
    if (type == GAFFER_STRING) return 's';
    if (type == GAFFER_URI) return 'u';
    if (type == GAFFER_INTEGER) return 'i';
    if (type == GAFFER_FLOAT) return 'f';
    if (type == GAFFER_BLANK) return 'b';
    if (type == GAFFER_NONE) return 'n';
}

static
gaffer_type char_to_type(char type)
{
    if (type == 's') return GAFFER_STRING;
    if (type == 'u') return GAFFER_URI;
    if (type == 'i') return GAFFER_INTEGER;
    if (type == 'f') return GAFFER_FLOAT;
    if (type == 'b') return GAFFER_BLANK;
    if (type == 'n') return GAFFER_NONE;
}

int gaffer_add(gaffer_comms* gc,
	       gaffer_term s, gaffer_term p, gaffer_term o)
{
    
    char* snode = (char*) malloc(strlen(s.term) + 5);
    sprintf(snode, "n:%c:%s", type_to_char(s.type), s.term);
    
    char* pnode = (char*) malloc(strlen(p.term) + 5);
    sprintf(pnode, "r:%c:%s", type_to_char(p.type), p.term);
    
    char* onode = (char*) malloc(strlen(o.term) + 5);
    sprintf(onode, "n:%c:%s", type_to_char(o.type), o.term);

    // FIXME: Doesn't check for it's existing.

    json_object* obj = json_object_new_object();

    json_object* elts = json_object_new_array();

    /* Create S node */
    add_node_object(elts, snode);

    /* Create P node */
    add_node_object(elts, pnode);

    /* Create O node */
    add_node_object(elts, onode);

    /* Create S,O -> P */
    add_edge_object(elts, pnode, snode, onode, "@r");

    /* Create S,P -> O */
    add_edge_object(elts, onode, snode, pnode, "@n");

    free(snode);
    free(pnode);
    free(onode);

    json_object_object_add(obj, "elements", elts);

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/add/elements",
			       &upload, &download, &http_code);

    free(download.buffer);
    json_object_put(obj);

    if (ret < 0) {
	return -1;
    }

    if (http_code != 204) {
	fprintf(stderr, "Gaffer REST API error\n");
	return -1;
    }

    return 0;

}

int gaffer_add_batch(gaffer_comms* gc,
		     gaffer_term batch[GAFFER_BATCH_SIZE][3],
		     int rows, int columns)
{

    json_object* obj = json_object_new_object();

    json_object* elts = json_object_new_array();
  
    int i;
    for(i = 0; i < rows; i++) {

	char* s = batch[i][0].term;
	char* p = batch[i][1].term;
	char* o = batch[i][2].term;
      
	char* snode = (char*) malloc(strlen(s) + 5);
	sprintf(snode, "n:%c:%s", type_to_char(batch[i][0].type), s);
      
	char* pnode = (char*) malloc(strlen(p) + 5);
	sprintf(pnode, "r:%c:%s", type_to_char(batch[i][1].type), p);
      
	char* onode = (char*) malloc(strlen(o) + 5);
	sprintf(onode, "n:%c:%s", type_to_char(batch[i][2].type), o);

	/* Create S node */
	add_node_object(elts, snode);
      
	/* Create P node */
	add_node_object(elts, pnode);

	/* Create O node */
	add_node_object(elts, onode);

	/* Create S,O -> P */
	add_edge_object(elts, pnode, snode, onode, "@r");
      
	/* Create S,P -> O */
	add_edge_object(elts, onode, snode, pnode, "@n");

	free(snode);
	free(pnode);
	free(onode);

    }

    json_object_object_add(obj, "elements", elts);

    const char* j = json_object_to_json_string(obj);

    // Web look-up for a random page.  FIXME: WHY?!
    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/add/elements",
			       &upload, &download, &http_code);

    free(download.buffer);
    json_object_put(obj);

    if (ret < 0) {
	return -1;
    }

    if (http_code != 204) {
	fprintf(stderr, "Gaffer REST API error\n");
	return -1;
    }

    return 0;

}

int gaffer_remove(gaffer_comms* gc,
		  gaffer_term s, gaffer_term p, gaffer_term o)
{

    // FIXME: Not implemented!
    return 0;
#ifdef BROKEN
    json_object* obj = json_object_new_object();

    json_object* js = json_object_new_string(s);
    json_object_object_add(obj, "s", js);

    json_object* jp = json_object_new_string(p);
    json_object_object_add(obj, "p", jp);

    json_object* jo = json_object_new_string(o);
    json_object_object_add(obj, "o", jo);

    const char* j = json_object_to_json_string(obj);

    // Web look-up for a random page.  FIXME: WHY?!
    http_dload_buffer buffer;

    long http_code;
    
    int ret = gaffer_http_get(gc, "remove", &buffer, &http_code);

    if (ret < 0) {
	json_object_put(obj);
	free(buffer.buffer);
	return -1;
    }

    json_object_put(obj);

    free(buffer.buffer);

    if (http_code != 200 && http_code != 204)
	return -1;

    return 0;

#endif

}

gaffer_results* gaffer_relatededge_results_parse(json_object* obj, int spo)
{

    json_object* jc;
    
    gaffer_results* res = malloc(sizeof (gaffer_results));
    if (res == 0) {
	return 0;
    }

    res->results = 0;
    res->count = 0;

    int triples = json_object_array_length(obj);

    int i;
    for(i = 0; i < triples; i++) {

	json_object* jres = json_object_array_get_idx(obj, i);
	if (jres == 0) continue;

	json_object* source;
	json_object* destination;

	const char* s;
	const char* d;

	if (!json_object_object_get_ex(jres, "source", &source))
	    continue;
	
	if (!json_object_object_get_ex(jres, "destination", &destination))
	    continue;

	s = json_object_get_string(source);
	d = json_object_get_string(destination);

	if (strlen(s) < 4) continue;
	if (strlen(d) < 4) continue;

	if (s[0] != 'n' && s[1] != ':') continue;

	if (spo)
	    if (d[0] != 'n' && d[1] != ':') continue;
	else
	    if (d[0] != 'r' && d[1] != ':') continue;
	
	json_object* property;
	
	if (!json_object_object_get_ex(jres, "properties", &property))
	    continue;
	if (!json_object_object_get_ex(property, "name", &property))
	    continue;
	if (!json_object_object_get_ex(property,
				       "gaffer.function.simple.types.FreqMap",
				       &property))
	    continue;

	struct lh_table* hash = json_object_get_object(property);

	struct lh_entry* hash_entry = hash->head;
	for(; hash_entry; hash_entry = hash_entry->next) {
	
	    json_object* val = (json_object*) hash_entry->v;

//	    json_type type = json_object_get_type(val);
//	    printf("%s: %d\n", k, type);
	
	    char* k = (char*) hash_entry->k;

	    // These aren't real relationships.
	    if (k[0] == '@') continue;

	    // Shouldn't happen.
	    if (strlen(k) < 4) continue;
	    
	    if (spo)
		if (k[0] != 'r' && k[1] != ':') continue;
	    else
		if (k[0] != 'n' && k[1] != ':') continue;

	    // Shouldn't happen.
	    if (k[3] != ':') continue;

	    if (spo)
		add_result(res, s + 4, k + 4, d + 4,
			   char_to_type(s[2]), char_to_type(k[2]),
			   char_to_type(d[2]));
	    else
		add_result(res, s + 4, d + 4, k + 4, char_to_type(s[2]),
			   char_to_type(d[2]), char_to_type(k[2]));

	}

    }

    return res;

}

static
void configure_entity_seed(json_object* obj, char* node)
{
				       
    json_object* seeds = json_object_new_array();

    json_object* seed = json_object_new_object();

    json_object* entseed = json_object_new_object();

    json_object_object_add(entseed, "vertex", json_object_new_string(node));
    
    json_object_object_add(seed, "gaffer.operation.data.EntitySeed", entseed);

    json_object_array_add(seeds, seed);

    json_object_object_add(obj, "seeds", seeds);

}

static
void configure_edge_seeds(json_object* obj, const char* node1,
			  const char* node2)
{
    
    json_object* seeds = json_object_new_array();

    json_object* seed = json_object_new_object();

    json_object* edgeseed = json_object_new_object();

    json_object_object_add(edgeseed, "source",
			   json_object_new_string(node1));

    json_object_object_add(edgeseed, "destination",
			   json_object_new_string(node2));

    json_object_object_add(edgeseed, "directed",
			   json_object_new_string("true"));

    json_object_object_add(seed, "gaffer.operation.data.EdgeSeed", edgeseed);

    json_object_array_add(seeds, seed);

    json_object_object_add(obj, "seeds", seeds);

}

static
void configure_edge_filter_view(json_object* obj, const char* edge)
{

    json_object* view = json_object_new_object();

    json_object* edges = json_object_new_object();

    json_object* basicedge = json_object_new_object();

    json_object* filterfunctions = json_object_new_array();

    json_object* filterfunction = json_object_new_object();

    json_object* function = json_object_new_object();

    json_object_object_add(function, "class",
			   json_object_new_string("gaffer.function.simple.filter.MapContains"));
    json_object_object_add(function, "key", json_object_new_string(edge));

    json_object* selections = json_object_new_array();

    json_object* selection = json_object_new_object();

    json_object_object_add(selection, "key",
			   json_object_new_string("name"));

    json_object_array_add(selections, selection);

    json_object_object_add(filterfunction, "function", function);
    json_object_object_add(filterfunction, "selection", selections);

    json_object_array_add(filterfunctions, filterfunction);

    json_object_object_add(basicedge, "filterFunctions", filterfunctions);

    json_object_object_add(edges, "BasicEdge", basicedge);

    json_object_object_add(view, "edges", edges);

    json_object_object_add(obj, "view", view);

}

// FIXME: Code is almost identical to edge_filter_view.
static
void configure_relationship_filter_view(json_object* obj)
{

    json_object* view = json_object_new_object();

    json_object* edges = json_object_new_object();

    json_object* basicedge = json_object_new_object();

    json_object* filterfunctions = json_object_new_array();

    json_object* filterfunction = json_object_new_object();

    json_object* function = json_object_new_object();

    json_object_object_add(function, "class",
			   json_object_new_string("gaffer.function.simple.filter.MapContains"));
    json_object_object_add(function, "key", json_object_new_string("@r"));

    json_object* selections = json_object_new_array();

    json_object* selection = json_object_new_object();

    json_object_object_add(selection, "key",
			   json_object_new_string("name"));

    json_object_array_add(selections, selection);

    json_object_object_add(filterfunction, "function", function);
    json_object_object_add(filterfunction, "selection", selections);

    json_object_array_add(filterfunctions, filterfunction);

    json_object_object_add(basicedge, "filterFunctions", filterfunctions);

    json_object_object_add(edges, "BasicEdge", basicedge);

    json_object_object_add(view, "edges", edges);

    json_object_object_add(obj, "view", view);

}

gaffer_results* gaffer_query_(gaffer_comms* gc,
			      gaffer_term s, gaffer_term p, gaffer_term o)
{

    json_object* obj = json_object_new_object();

    json_object* operations = json_object_new_array();

    json_object* operation = json_object_new_object();

    json_object_object_add(operation, "class",
			   json_object_new_string("gaffer.accumulostore.operation.impl.GetEdgesInRanges"));

    json_object* seeds = json_object_new_array();

    json_object* seed = json_object_new_object();

    json_object* pair = json_object_new_object();

    json_object* first = json_object_new_object();

    json_object* first_seed = json_object_new_object();

    json_object_object_add(first_seed, "vertex", json_object_new_string("n:"));

    json_object_object_add(first, "gaffer.operation.data.EntitySeed",
			   first_seed);

    json_object* second = json_object_new_object();
    
    json_object* second_seed = json_object_new_object();

    json_object_object_add(second_seed, "vertex", json_object_new_string("n;"));

    json_object_object_add(second, "gaffer.operation.data.EntitySeed",
			   second_seed);

    json_object_object_add(pair, "first", first);
    json_object_object_add(pair, "second", second);

    json_object_object_add(seed, "gaffer.accumulostore.utils.Pair", pair);

    json_object_array_add(seeds, seed);

    json_object_object_add(operation, "seeds", seeds);

    json_object_object_add(operation, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    json_object_array_add(operations, operation);

    json_object_object_add(obj, "operations", operations);

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation",
			       &upload, &download, &http_code);

    json_object_put(obj);

    	
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);

    free(download.buffer);

    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	return 0;
    }

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 1);

    json_object_put(obj);
    
    return res;

}

gaffer_results* gaffer_query_s(gaffer_comms* gc,
			       gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* snode = (char*) malloc(strlen(s.term) + 5);
    sprintf(snode, "n:%c:%s", type_to_char(s.type), s);

    json_object* obj = json_object_new_object();

    configure_relationship_filter_view(obj);

    configure_entity_seed(obj, snode);

    free(snode);

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("OUTGOING"));

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);

    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 1);

    json_object_put(obj);
    
    return res;

}

gaffer_results* gaffer_query_o(gaffer_comms* gc,
			       gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* onode = (char*) malloc(strlen(o.term) + 5);
    sprintf(onode, "n:%c:%s", type_to_char(o.type), o);

    json_object* obj = json_object_new_object();

    configure_entity_seed(obj, onode);

    free(onode);
    
    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 1);

    json_object_put(obj);

    return res;

}

gaffer_results* gaffer_query_p(gaffer_comms* gc,
			       gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* pnode = (char*) malloc(strlen(p.term) + 5);
    sprintf(pnode, "r:%c:%s", type_to_char(p.type), p);

    json_object* obj = json_object_new_object();

    configure_entity_seed(obj, pnode);

    free(pnode);

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 0);

    json_object_put(obj);
    
    return res;

}

gaffer_results* gaffer_query_sp(gaffer_comms* gc,
				gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* snode = (char*) malloc(strlen(s.term) + 5);
    sprintf(snode, "n:%c:%s", type_to_char(s.type), s.term);

    char* pnode = (char*) malloc(strlen(p.term) + 5);
    sprintf(pnode, "r:%c:%s", type_to_char(p.type), p.term);

    json_object* obj = json_object_new_object();

    configure_edge_seeds(obj, snode, pnode);

    free(snode);
    free(pnode);

    // FIXME: SHould be outgoing?
    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 0);
    
    json_object_put(obj);
    
    return res;

}

gaffer_results* gaffer_query_po(gaffer_comms* gc,
				gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* pnode = (char*) malloc(strlen(p.term) + 5);
    sprintf(pnode, "r:%c:%s", type_to_char(p.type), p.term);

    char* onode = (char*) malloc(strlen(o.term) + 5);
    sprintf(onode, "n:%c:%s", type_to_char(o.type), o.term);

    json_object* obj = json_object_new_object();

    configure_edge_filter_view(obj, pnode);

    configure_entity_seed(obj, onode);

    free(pnode);
    free(onode);

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 1);
    
    json_object_put(obj);
    
    return res;

}

gaffer_results* gaffer_query_so(gaffer_comms* gc,
				gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* snode = (char*) malloc(strlen(s.term) + 5);
    sprintf(snode, "n:%c:%s", type_to_char(s.type), s.term);

    char* onode = (char*) malloc(strlen(o.term) + 5);
    sprintf(onode, "n:%c:%s", type_to_char(o.type), o.term);

    json_object* obj = json_object_new_object();

    configure_edge_filter_view(obj, onode);

    configure_entity_seed(obj, snode);

    free(snode);
    free(onode);

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("OUTGOING"));

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 0);
    
    json_object_put(obj);
    
    return res;

}


// FIXME: Would this ever be called?
gaffer_results* gaffer_query_spo(gaffer_comms* gc,
				 gaffer_term s, gaffer_term p, gaffer_term o)
{

    char* snode = (char*) malloc(strlen(s.term) + 5);
    sprintf(snode, "n:%c:%s", type_to_char(s.type), s.term);

    char* pnode = (char*) malloc(strlen(p.term) + 5);
    sprintf(pnode, "r:%c:%s", type_to_char(p.type), p.term);

    char* onode = (char*) malloc(strlen(o.term) + 5);
    sprintf(onode, "n:%c:%s", type_to_char(o.type), o.term);

    json_object* obj = json_object_new_object();

    configure_edge_filter_view(obj, pnode);

    configure_edge_seeds(obj, snode, onode);

    free(snode);
    free(pnode);
    free(onode);

    /* Add elements to object */
    const char* j = json_object_to_json_string(obj);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/get/edges/related",
			       &upload, &download, &http_code);

    json_object_put(obj);
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    gaffer_results* res = gaffer_relatededge_results_parse(obj, 1);
    
    json_object_put(obj);
    
    return res;

}

typedef gaffer_results* (*query_function)(gaffer_comms* gc,
					  gaffer_term s,
					  gaffer_term p,
					  gaffer_term o);

gaffer_results* gaffer_find(gaffer_comms* gc,
			    gaffer_term s, gaffer_term p, gaffer_term o)
{

    /* ... s.. .p. sp. ..o s.o .po spo */
    query_function functions[8] = {
	&gaffer_query_,
	&gaffer_query_s,
	&gaffer_query_p,
	&gaffer_query_sp,
	&gaffer_query_o,
	&gaffer_query_so,
	&gaffer_query_po,
	&gaffer_query_spo
    };

    int num = 0;

    if (o.term) num += 4;
    if (p.term) num += 2;
    if (s.term) num++;

    return (*functions[num])(gc, s, p, o);

}

void gaffer_free_results(gaffer_results* gres)
{

    int i;
    for(i = 0; i < gres->count; i++) {
	free(gres->results[i].s.term);
	free(gres->results[i].p.term);
	free(gres->results[i].o.term);
    }

    free(gres->results);

    free(gres);

}

int gaffer_size(gaffer_comms* gc)
{

    json_object* obj = json_object_new_object();
    json_object* opt = json_object_new_string("yes");

    json_object_object_add(obj, "size", opt);

    const char* j = json_object_to_json_string(obj);

    // Web look-up for a random page.  FIXME: WHY?!
    http_dload_buffer buffer;

    long http_code;
    
    int ret = gaffer_http_get(gc, "size", &buffer, &http_code);

    if (ret < 0) {
	json_object_put(obj);
	free(buffer.buffer);
	return -1;
    }

    json_object_put(obj);
    
    obj = json_tokener_parse(buffer.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(buffer.buffer);
	return -1;
    }

    free(buffer.buffer);

    json_object* jc;
    
    if (!json_object_object_get_ex(obj, "size", &jc)) {
	fprintf(stderr, "No count parameter\n");
	return -1;
    }

    int size = json_object_get_int(jc);

    json_object_put(obj);

    return size;

}

