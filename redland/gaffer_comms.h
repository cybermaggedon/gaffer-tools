
#include <gaffer_query.h>
#include <json-c/json.h>

struct gaffer_comms_str;
typedef struct gaffer_comms_str gaffer_comms;

typedef struct json_object gaffer_elements;

gaffer_elements* gaffer_elements_create();
void gaffer_elements_free(gaffer_elements*);

void gaffer_add_node_object(gaffer_elements* obj, const char* node);
void gaffer_add_edge_object(gaffer_elements* obj, const char* edge,
			    const char* src, const char* dest,
			    const char* type);

gaffer_comms* gaffer_connect(const char* host);

void gaffer_disconnect(gaffer_comms*);
int gaffer_test(gaffer_comms*);

int gaffer_add_elements(gaffer_comms* gc, gaffer_elements* elts);

int gaffer_add(gaffer_comms* gc, const char*, const char*, const char*);

gaffer_results* gaffer_find(gaffer_comms*, const char* path, gaffer_query*);

