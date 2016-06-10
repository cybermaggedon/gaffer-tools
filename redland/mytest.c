
#include <gaffer_comms.h>
#include <gaffer_query.h>
#include <stdio.h>

int main(int argc, char** argv)

{

    gaffer_comms* gc = gaffer_connect("localhost:8080");
    if (gc == 0) {
	fprintf(stderr, "Couldn't connect\n");
	exit(1);
    }
    
    int ret = gaffer_test(gc);
    if (ret < 0) {
	fprintf(stderr, "Comms broken.\n");
	exit(1);
    }

    // ----------------------------------------------------------------------
    
    printf("*** ???\n");

    gaffer_query* qry = gaffer_create_query();
    gaffer_configure_range_query(qry, "n:", "n;");
    if (qry == 0) {
	fprintf(stderr, "Query create failed.\n");
	exit(1);
    }

    const char* path = "/graph/doOperation";

    gaffer_results* res = gaffer_find(gc, path, qry);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    gaffer_results_iterator* iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if ((c[0] != '@') && (b[0] != 'r'))
	    printf("%s %s %s\n", a, c, b);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** S??\n");

    json_object* obj = json_object_new_object();
	
    gaffer_configure_relationship_filter_view(obj);

    gaffer_configure_entity_seed(obj, "n:u:http://gaffer.test/#fred");

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("OUTGOING"));


    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, c, b);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** ?P?\n");

    obj = json_object_new_object();

    gaffer_configure_entity_seed(obj, "r:u:http://gaffer.test/#is_a");

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, b, c);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** ??O\n");

    obj = json_object_new_object();

    gaffer_configure_entity_seed(obj, "n:u:http://gaffer.test/#cat");

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, c, b);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** SP?\n");

    obj = json_object_new_object();

    gaffer_configure_edge_seeds(obj, "n:u:http://gaffer.test/#fred",
				"r:u:http://gaffer.test/#is_a");

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, b, c);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** S?O\n");

    obj = json_object_new_object();

    gaffer_configure_entity_seed(obj, "n:u:http://gaffer.test/#fred");

    gaffer_configure_edge_filter_view(obj, "n:u:http://gaffer.test/#cat");

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("OUTGOING"));

    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, b, c);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** ?PO\n");

    obj = json_object_new_object();

    gaffer_configure_entity_seed(obj, "n:u:http://gaffer.test/#cat");

    gaffer_configure_edge_filter_view(obj, "r:u:http://gaffer.test/#is_a");

    json_object_object_add(obj, "includeIncomingOutGoing",
			   json_object_new_string("INCOMING"));

    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, c, b);
	
	gaffer_iterator_next(iter);
    }

    // ----------------------------------------------------------------------
    
    printf("*** SPO\n");

    obj = json_object_new_object();

    gaffer_configure_edge_seeds(obj, "n:u:http://gaffer.test/#fred",
				"n:u:http://gaffer.test/#cat");

    gaffer_configure_edge_filter_view(obj, "r:u:http://gaffer.test/#is_a");

    path = "/graph/doOperation/get/edges/related";

    res = gaffer_find(gc, path, obj);
    if (res == 0) {
	fprintf(stderr, "Query execute failed.\n");
	exit(1);
    }

    iter = gaffer_iterator_create(res);

    while (!gaffer_iterator_done(iter)) {
	
	const char* a, * b, * c;

	gaffer_iterator_get(iter, &a, &b, &c);

	if (c[0] != '@')
	    printf("%s %s %s\n", a, c, b);
	
	gaffer_iterator_next(iter);
    }

    exit(0);
}
