
#include <gaffer_query.h>
#include <json-c/json.h>
#include <stdio.h>

void gaffer_configure_entity_seed(json_object* obj, const char* node)
{
				       
    json_object* seeds = json_object_new_array();

    json_object* seed = json_object_new_object();

    json_object* entseed = json_object_new_object();

    json_object_object_add(entseed, "vertex", json_object_new_string(node));
    
    json_object_object_add(seed, "gaffer.operation.data.EntitySeed", entseed);

    json_object_array_add(seeds, seed);

    json_object_object_add(obj, "seeds", seeds);

}

void gaffer_configure_edge_seeds(json_object* obj, const char* node1,
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

void gaffer_configure_edge_filter_view(json_object* obj, const char* edge)
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

// FIXME: Code is almost identical to ...edge_filter_view.
void gaffer_configure_relationship_filter_view(json_object* obj)
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

gaffer_query* gaffer_create_query()
{
    return json_object_new_object();
}


gaffer_query* gaffer_configure_range_query(gaffer_query* obj, 
					   const char* start, const char* end)
{

    json_object* operations = json_object_new_array();

    json_object* operation = json_object_new_object();

    const char* geir = "gaffer.accumulostore.operation.impl.GetEdgesInRanges";
    json_object_object_add(operation, "class",
			   json_object_new_string(geir));

    json_object* seeds = json_object_new_array();

    json_object* seed = json_object_new_object();

    json_object* pair = json_object_new_object();

    json_object* first = json_object_new_object();

    json_object* first_seed = json_object_new_object();

    json_object_object_add(first_seed, "vertex", json_object_new_string(start));

    json_object_object_add(first, "gaffer.operation.data.EntitySeed",
			   first_seed);

    json_object* second = json_object_new_object();
    
    json_object* second_seed = json_object_new_object();

    json_object_object_add(second_seed, "vertex", json_object_new_string(end));

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

    return obj;

}

void gaffer_query_free(gaffer_query* qry)
{
    json_object_put(qry);
}

gaffer_results_iterator* gaffer_iterator_create(gaffer_results* results)
{

    gaffer_results_iterator* iter = malloc(sizeof(gaffer_results_iterator));
    if (iter == 0) {
	fprintf(stderr, "malloc failed\n");
	exit(1);
    }

    iter->elt_array = results;
    iter->elt_array_length = json_object_array_length(results);
    iter->elt_array_current = 0;

    /* Now hunt for first property. */

    while (1) {

	/* Get current element. If at end, just return iterator in a 'finished'
	   state */
	if (iter->elt_array_current >= iter->elt_array_length) {
	    iter->current = 0;
	    return iter;
	} else
	    iter->current = json_object_array_get_idx(results,
						      iter->elt_array_current);

	json_object* property;

	if (!json_object_object_get_ex(iter->current,
				       "source", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	iter->source = json_object_get_string(property);

	if (!json_object_object_get_ex(iter->current,
				       "destination", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	iter->dest = json_object_get_string(property);
	
	if (!json_object_object_get_ex(iter->current,
				       "properties", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	if (!json_object_object_get_ex(property, "name", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	if (!json_object_object_get_ex(property,
				       "gaffer.function.simple.types.FreqMap",
				       &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	struct lh_table* hash = json_object_get_object(property);

	if (hash == 0) {
	    iter->elt_array_current++;
	    continue;
	}

	iter->property = hash->head;
	if (iter->property == 0) {
	    iter->elt_array_current++;
	    continue;
	}

	break;

    }

    return iter;

}

void gaffer_iterator_next(gaffer_results_iterator* iter)
{

    if (iter->property)
	iter->property = iter->property->next;

    if (iter->property) return;

    iter->elt_array_current++;

    /* Hunt for next property. */

    
    while (1) {

	/* Get current element. If at end, just return iterator in a 'finished'
	   state */
	if (iter->elt_array_current >= iter->elt_array_length) {
	    iter->current = 0;
	    return;
	} else 
	    iter->current = json_object_array_get_idx(iter->elt_array,
						      iter->elt_array_current);

	json_object* property;

	if (!json_object_object_get_ex(iter->current,
				       "source", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	iter->source = json_object_get_string(property);

	if (!json_object_object_get_ex(iter->current,
				       "destination", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	iter->dest = json_object_get_string(property);

	if (!json_object_object_get_ex(iter->current,
				       "properties", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	if (!json_object_object_get_ex(property, "name", &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	if (!json_object_object_get_ex(property,
				       "gaffer.function.simple.types.FreqMap",
				       &property)) {
	    iter->elt_array_current++;
	    continue;
	}

	struct lh_table* hash = json_object_get_object(property);

	if (hash == 0) {
	    iter->elt_array_current++;
	    continue;
	}

	iter->property = hash->head;
	if (iter->property == 0) {
	    iter->elt_array_current++;
	    continue;
	}

	break;

    }

}

int gaffer_iterator_done(gaffer_results_iterator* iter)
{
    return iter->current == 0;
}

int gaffer_iterator_get(gaffer_results_iterator* iter,
			const char** src, const char** dest, const char** prop,
			int* val)
{
    if (iter->current == 0) return -1;

    *src = iter->source;
    *dest = iter->dest;
    *prop = iter->property->k;

    json_object* obj = (json_object*) iter->property->v;

    /* Shouldn't happen. */
    if (obj == 0) {
      *val = 0;
    } else {
      *val = json_object_get_int(obj);
    }

    return 0;

}

void gaffer_iterator_free(gaffer_results_iterator* iter)
{
    free(iter);
}
