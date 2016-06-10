
#ifndef GAFFER_QUERY_H

#define GAFFER_QUERY_H

#include <json-c/json.h>

typedef json_object gaffer_query;

gaffer_query* gaffer_create_query();

gaffer_query* gaffer_configure_range_query(gaffer_query*,
					   const char* a,
					   const char* b);

void gaffer_configure_relationship_filter_view(gaffer_query* obj);
void gaffer_configure_entity_seed(json_object* obj, const char* node);
void gaffer_configure_edge_seeds(json_object* obj, const char* node1,
				 const char* node2);
void gaffer_configure_edge_filter_view(json_object* obj, const char* edge);

void gaffer_query_free(gaffer_query*);

typedef json_object gaffer_results;

typedef struct {
    json_object* elt_array;
    long elt_array_length;
    long elt_array_current;
    json_object* current;
    struct lh_entry* property;

    const char* source;
    const char* dest;
    
} gaffer_results_iterator;

gaffer_results_iterator* gaffer_iterator_create(gaffer_results*);
int gaffer_iterator_done(gaffer_results_iterator*);
int gaffer_iterator_get(gaffer_results_iterator*, const char**, const char**, const char**, int* val);
void gaffer_iterator_next(gaffer_results_iterator*);

void gaffer_iterator_free(gaffer_results_iterator*);

#endif

