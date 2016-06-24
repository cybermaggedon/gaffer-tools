
#include <curl/curl.h>
#include <gaffer_comms.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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

static size_t
http_dload_callback(void *contents, size_t size, size_t nmemb, void *userp)
{

    size_t realsize = size * nmemb;
    http_dload_buffer *buffer = (http_dload_buffer *)userp;

    // Add one byte for the NULL-termination.
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
#ifdef CURLOPT_EXPECT_100_TIMEOUT_MS
    curl_easy_setopt(gc->curl, CURLOPT_EXPECT_100_TIMEOUT_MS, 1);
#endif
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
		     long* http_code, char* method,
		     char* content_type)
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
#ifdef CURLOPT_EXPECT_100_TIMEOUT_MS
    curl_easy_setopt(gc->curl, CURLOPT_EXPECT_100_TIMEOUT_MS, 1);
#endif

    struct curl_slist* headers = 0;
    char *cth = malloc(strlen(content_type) + 20);
    if (cth == 0) {
	fprintf(stderr, "Malloc failed");
	return -1;
    }
    sprintf(cth, "Content-Type: %s", content_type);

    if (up) {

	headers = curl_slist_append(headers, cth);
	curl_easy_setopt(gc->curl, CURLOPT_HTTPHEADER, (void*) headers);
	curl_easy_setopt(gc->curl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(gc->curl, CURLOPT_INFILESIZE_LARGE, up->size);
	curl_easy_setopt(gc->curl, CURLOPT_READDATA, (void*) up);
	curl_easy_setopt(gc->curl, CURLOPT_READFUNCTION, &http_upload_callback);
	curl_easy_setopt(gc->curl, CURLOPT_CUSTOMREQUEST, method);
    } else {
	curl_easy_setopt(gc->curl, CURLOPT_UPLOAD, 0);
	curl_easy_setopt(gc->curl, CURLOPT_CUSTOMREQUEST, method);
    }

    res = curl_easy_perform(gc->curl);

    if (headers)
	curl_slist_free_all(headers);

    free(cth);

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

void gaffer_add_edge_object(gaffer_elements* obj, const char* edge,
			    const char* src, const char* dest, const char* type,
			    int weight)
{

    json_object* elts;

    if (!json_object_object_get_ex(obj, "elements", &elts))
	return;
    
    json_object* elt = json_object_new_object();
    json_object_object_add(elt, "class",
			   json_object_new_string("gaffer.data.element.Edge"));

    json_object* namep = json_object_new_object();

    json_object* namefm = json_object_new_object();
    json_object_object_add(namefm, edge, json_object_new_int(weight));
    json_object_object_add(namefm, type, json_object_new_int(1));
    
    json_object_object_add(namep,
			   "gaffer.function.simple.types.FreqMap", namefm);

    json_object* props = json_object_new_object();
    json_object_object_add(props, "name", namep);

    json_object_object_add(elt, "properties", props);

    json_object_object_add(elt, "group", json_object_new_string("BasicEdge"));
    
    json_object_object_add(elt, "source", json_object_new_string(src));
    json_object_object_add(elt, "destination", json_object_new_string(dest));

    json_object_object_add(elt, "directed", json_object_new_boolean(1));

    json_object_array_add(elts, elt);

}

gaffer_elements* gaffer_elements_create()
{

    json_object* obj = json_object_new_object();
    json_object* elts = json_object_new_array();

    json_object_object_add(obj, "elements", elts);

    return obj;

}

void gaffer_elements_free(gaffer_elements* obj)
{
    json_object_put(obj);
}

int gaffer_add_elements(gaffer_comms* gc, gaffer_elements* elts)
{

    /* Add elements to object */
    const char* j = json_object_to_json_string(elts);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, "graph/doOperation/add/elements",
			       &upload, &download, &http_code, "PUT",
			       "application/json");

    free(download.buffer);

    if (ret < 0) {
	return -1;
    }

    if (http_code != 204) {
	fprintf(stderr, "Gaffer REST API error\n");
	return -1;
    }

    return 0;

}

gaffer_results* gaffer_find(gaffer_comms* gc, const char* path,
			    gaffer_query* query)
{
    
    /* Add elements to object */
    const char* j = json_object_to_json_string(query);

    http_dload_buffer download;

    http_upload_buffer upload;
    upload.buffer = j;
    upload.size = strlen(j);

    long http_code;
 
    int ret = gaffer_http_post(gc, path, &upload, &download, &http_code,
			       "POST", "application/json");
    
    if (ret < 0) {
	free(download.buffer);
	return 0;
    }

    json_object* obj = json_tokener_parse(download.buffer);
    if (obj == 0) {
	fprintf(stderr, "JSON parse failed\n");
	free(download.buffer);
	return 0;
    }

    free(download.buffer);

    return obj;

}

void gaffer_results_free(gaffer_results* results)
{
    json_object_put(results);
}

