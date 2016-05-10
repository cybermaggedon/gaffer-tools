/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_storage_gaffer.c - RDF Storage using Gaffer implementation
 *
 * Copyright (C) 2004-2010, David Beckett http://www.dajobe.org/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 * 
 */


#ifdef HAVE_CONFIG_H
#include <rdf_config.h>
#endif

#ifdef WIN32
#include <win32_rdf_config.h>
#endif

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <redland.h>
#include <rdf_storage.h>
#include <rdf_heuristics.h>

#include <gaffer_comms.h>

static const char* const gaffer_synchronous_flags[4] = {
    "off", "normal", "full", NULL
};

typedef struct
{
    librdf_storage *storage;

    int is_new;
  
    char *name;
    size_t name_len;  

    gaffer_comms* comms;
    
} librdf_storage_gaffer_instance;



/* prototypes for local functions */
static int librdf_storage_gaffer_init(librdf_storage* storage, const char *name, librdf_hash* options);
static int librdf_storage_gaffer_open(librdf_storage* storage, librdf_model* model);
static int librdf_storage_gaffer_close(librdf_storage* storage);
static int librdf_storage_gaffer_size(librdf_storage* storage);
static int librdf_storage_gaffer_add_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_gaffer_add_statements(librdf_storage* storage, librdf_stream* statement_stream);
static int librdf_storage_gaffer_remove_statement(librdf_storage* storage, librdf_statement* statement);
static int librdf_storage_gaffer_contains_statement(librdf_storage* storage, librdf_statement* statement);
static librdf_stream* librdf_storage_gaffer_serialise(librdf_storage* storage);
static librdf_stream* librdf_storage_gaffer_find_statements(librdf_storage* storage, librdf_statement* statement);

/* serialising implementing functions */
static int gaffer_results_stream_end_of_stream(void* context);
static int gaffer_results_stream_next_statement(void* context);
static void* gaffer_results_stream_get_statement(void* context, int flags);
static void gaffer_results_stream_finished(void* context);

/* context functions */
static int librdf_storage_gaffer_context_add_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_gaffer_context_remove_statement(librdf_storage* storage, librdf_node* context_node, librdf_statement* statement);
static int librdf_storage_gaffer_context_contains_statement(librdf_storage* storage, librdf_node* context, librdf_statement* statement);
static librdf_stream* librdf_storage_gaffer_context_serialise(librdf_storage* storage, librdf_node* context_node);

/* helper functions for contexts */

static librdf_iterator* librdf_storage_gaffer_get_contexts(librdf_storage* storage);

/* transactions */
static int librdf_storage_gaffer_transaction_start(librdf_storage *storage);
static int librdf_storage_gaffer_transaction_commit(librdf_storage *storage);
static int librdf_storage_gaffer_transaction_rollback(librdf_storage *storage);

static void librdf_storage_gaffer_register_factory(librdf_storage_factory *factory);
#ifdef MODULAR_LIBRDF
void librdf_storage_module_register_factory(librdf_world *world);
#endif


/* functions implementing storage api */
static int
librdf_storage_gaffer_init(librdf_storage* storage, const char *name,
                           librdf_hash* options)
{
    char *name_copy;
    librdf_storage_gaffer_instance* context;
  
    if(!name) {
	if(options)
	    librdf_free_hash(options);
	return 1;
    }
  
    context = LIBRDF_CALLOC(librdf_storage_gaffer_instance*, 1, sizeof(*context));
    if(!context) {
	if(options)
	    librdf_free_hash(options);
	return 1;
    }

    librdf_storage_set_instance(storage, context);
  
    context->storage = storage;

    context->name_len = strlen(name);

    int append_slash = 0;

    if (name[strlen(name)-1] != '/') {
      append_slash = 1;
      context->name_len++;
    }

    name_copy = LIBRDF_MALLOC(char*, context->name_len + 1);
    if(!name_copy) {
	if(options)
	    librdf_free_hash(options);
	return 1;
    }

    strcpy(name_copy, name);
    if (append_slash) strcat(name_copy, "/");
    context->name = name_copy;

    fprintf(stderr, "Name: %s\n", context->name);

    // Add options here.

    /* no more options, might as well free them now */
    if(options)
	librdf_free_hash(options);

    context->comms = gaffer_connect(context->name);
    if (context->comms == 0) {
	free(context->name);
	free(context);
	return 1;
    }

    return 0;
}


static void
librdf_storage_gaffer_terminate(librdf_storage* storage)
{
    librdf_storage_gaffer_instance* context;

    context = (librdf_storage_gaffer_instance*)storage->instance;
  
    if (context == NULL)
	return;

    if(context->name)
	LIBRDF_FREE(char*, context->name);
  
    LIBRDF_FREE(librdf_storage_gaffer_terminate, storage->instance);
}

static int
statement_helper(librdf_storage* storage,
		 librdf_statement* statement,
		 const unsigned char* fields[4],
		 librdf_node* context_node)
{
    librdf_node* nodes[4];
    int i;
  
    nodes[0] = statement ? librdf_statement_get_subject(statement) : 0;
    nodes[1] = statement ? librdf_statement_get_predicate(statement) : 0;
    nodes[2] = statement ? librdf_statement_get_object(statement) : 0;
    nodes[3] = context_node;

    librdf_uri* uri;
    
    for(i = 0; i < 4; i++) {

	if (nodes[i] == 0) {
	    fields[i] = 0;
	    continue;
	}
	
	switch (librdf_node_get_type(nodes[i])) {
	case LIBRDF_NODE_TYPE_RESOURCE:
	    uri = librdf_node_get_uri(nodes[i]);
	    fields[i] = librdf_uri_as_string(uri);
	    break;
		
	case LIBRDF_NODE_TYPE_LITERAL:
	    fields[i] = librdf_node_get_literal_value(nodes[i]);
	    break;
		
	case LIBRDF_NODE_TYPE_BLANK:
	    fields[i] = librdf_node_get_blank_identifier(nodes[i]);
	    break;

	case LIBRDF_NODE_TYPE_UNKNOWN:
	    break;
	}

    }

    return 0;
}
  
static int
librdf_storage_gaffer_open(librdf_storage* storage, librdf_model* model)
{
    librdf_storage_gaffer_instance* context;

    context = (librdf_storage_gaffer_instance*)storage->instance;

    int ret = gaffer_test(context->comms);

    if (ret < 0) {
	fprintf(stderr, "gaffer_test() failed to connect\n");
	return 1;
    }

    return 0;
}


/**
 * librdf_storage_gaffer_close:
 * @storage: the storage
 *
 * Close the gaffer storage.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_gaffer_close(librdf_storage* storage)
{
    librdf_storage_gaffer_instance* context;
    context = (librdf_storage_gaffer_instance*)storage->instance;

    if (context->comms) {
	gaffer_disconnect(context->comms);
	context->comms = 0;
    }

    return 0;
}


static int
librdf_storage_gaffer_size(librdf_storage* storage)
{
    librdf_storage_gaffer_instance* context =
	(librdf_storage_gaffer_instance*) storage->instance;

    return gaffer_size(context->comms);

}


static int
librdf_storage_gaffer_add_statement(librdf_storage* storage, 
                                    librdf_statement* statement)
{
    return librdf_storage_gaffer_context_add_statement(storage, NULL, statement);
}


static int
librdf_storage_gaffer_add_statements(librdf_storage* storage,
                                     librdf_stream* statement_stream)
{

    librdf_storage_gaffer_instance* context;
    context = (librdf_storage_gaffer_instance*)storage->instance;

    char* batch[GAFFER_BATCH_SIZE][3];

    const unsigned char* fields[4];
    
    int rows = 0;

    for(; !librdf_stream_end(statement_stream);
	librdf_stream_next(statement_stream)) {

	librdf_statement* statement;
	librdf_node* context_node;
    
	statement = librdf_stream_get_object(statement_stream);
	context_node = librdf_stream_get_context2(statement_stream);

	if(!statement) {
	    break;
	}

	statement_helper(storage, statement, fields, context_node);

	for(int j = 0; j < 3; j++)
	    batch[rows][j] = strdup(fields[j]);

	rows++;

	if (rows >= GAFFER_BATCH_SIZE) {

	    gaffer_add_batch(context->comms, batch, rows, 3);

	    for(int i = 0; i < rows; i++)
		for(int j = 0; j < 3; j++)
		    free(batch[i][j]);

	    rows = 0;

	}

    }

    if (rows > 0)
	gaffer_add_batch(context->comms, batch, rows, 3);

    for(int i = 0; i < rows; i++)
	for(int j = 0; j < 3; j++)
	    free(batch[i][j]);

    return 0;
}


static int
librdf_storage_gaffer_remove_statement(librdf_storage* storage,
                                       librdf_statement* statement)
{
    return librdf_storage_gaffer_context_remove_statement(storage, NULL, 
							  statement);
}

static int
librdf_storage_gaffer_contains_statement(librdf_storage* storage, 
                                         librdf_statement* statement)
{
    return librdf_storage_gaffer_context_contains_statement(storage, NULL,
							    statement);
}


static int
librdf_storage_gaffer_context_contains_statement(librdf_storage* storage,
                                                 librdf_node* context_node,
                                                 librdf_statement* statement)
{
    
    librdf_storage_gaffer_instance* context;
    context = (librdf_storage_gaffer_instance*)storage->instance;

    /* librdf_storage_gaffer_instance* context; */
    const unsigned char* fields[4];

    statement_helper(storage, statement, fields, context_node);

    int count = gaffer_count(context->comms,
			     (const char*) fields[0],
			     (const char*) fields[1],
			     (const char*) fields[2]);
    if (count < 0)
	return -1;
    
    return (count > 0);

}

typedef struct {
    
    librdf_storage *storage;
    librdf_storage_gaffer_instance* gaffer_context;

    librdf_statement *statement;
    librdf_node* context;

    gaffer_results* results;
    int row;

} gaffer_results_stream;

static int
gaffer_results_stream_end_of_stream(void* context)
{
    gaffer_results_stream* scontext;

    scontext = (gaffer_results_stream*)context;

    if (scontext->row >= scontext->results->count)
	return 1;

    if (scontext->statement) {
	librdf_free_statement(scontext->statement);
	scontext->statement = 0;
    }

    int row = scontext->row;

    librdf_node* s;
    s = librdf_new_node_from_uri_string(scontext->storage->world,
					(unsigned char*)
					scontext->results->s[row]);

    librdf_node* p;
    p = librdf_new_node_from_uri_string(scontext->storage->world,
					(unsigned char*) 
					scontext->results->p[row]);

    librdf_node* o;
    if (librdf_heuristic_object_is_literal(scontext->results->o[row]))
	o = librdf_new_node_from_literal(scontext->storage->world,
					 (unsigned char*) 
					 scontext->results->o[row],
					 0, 0);
    else
	o = librdf_new_node_from_uri_string(scontext->storage->world,
					    (unsigned char*) 
					    scontext->results->o[row]);

    scontext->statement =
	librdf_new_statement_from_nodes(scontext->storage->world,
					s, p, o);

    return 0;

}


static int
gaffer_results_stream_next_statement(void* context)
{
    gaffer_results_stream* scontext;

    scontext = (gaffer_results_stream*)context;

    scontext->row++;

    if (scontext->row >= scontext->results->count)
	return 1;

    if (scontext->statement) {
	librdf_free_statement(scontext->statement);
	scontext->statement = 0;
    }

    int row = scontext->row;

    librdf_node* s;
    s = librdf_new_node_from_uri_string(scontext->storage->world,
					(unsigned char*)
					scontext->results->s[row]);

    librdf_node* p;
    p = librdf_new_node_from_uri_string(scontext->storage->world,
					(unsigned char*)
					scontext->results->p[row]);

    librdf_node* o;
    if (librdf_heuristic_object_is_literal(scontext->results->o[row]))
	o = librdf_new_node_from_literal(scontext->storage->world,
					 (unsigned char*)
					 scontext->results->o[row],
					 0, 0);
    else
	o = librdf_new_node_from_uri_string(scontext->storage->world,
					    (unsigned char*)
					    scontext->results->o[row]);

    scontext->statement =
	librdf_new_statement_from_nodes(scontext->storage->world,
					s, p, o);

    return 0;
    
}


static void*
gaffer_results_stream_get_statement(void* context, int flags)
{
    gaffer_results_stream* scontext;

    scontext = (gaffer_results_stream*)context;

    switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:
	return scontext->statement;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
	return scontext->context;

    default:
	librdf_log(scontext->storage->world,
		   0, LIBRDF_LOG_ERROR, LIBRDF_FROM_STORAGE, NULL,
		   "Unknown iterator method flag %d", flags);
	return NULL;
    }
}


static void
gaffer_results_stream_finished(void* context)
{
    gaffer_results_stream* scontext;

    scontext  = (gaffer_results_stream*)context;

    if (scontext->results) {
	gaffer_free_results(scontext->results);
	scontext->results = 0;
    }

    if(scontext->storage)
	librdf_storage_remove_reference(scontext->storage);

    if(scontext->statement)
	librdf_free_statement(scontext->statement);

    if(scontext->context)
	librdf_free_node(scontext->context);

    LIBRDF_FREE(librdf_storage_gaffer_find_statements_stream_context, scontext);
}

static librdf_stream*
librdf_storage_gaffer_serialise(librdf_storage* storage)
{
    
    librdf_storage_gaffer_instance* context;
    gaffer_results_stream* scontext;
    librdf_stream* stream;
  
    context = (librdf_storage_gaffer_instance*) storage->instance;

    scontext =
	LIBRDF_CALLOC(gaffer_results_stream*,
		      1, sizeof(*scontext));
    if(!scontext)
	return NULL;

    scontext->storage = storage;
    librdf_storage_add_reference(scontext->storage);

    scontext->gaffer_context = context;

    gaffer_results* gres = gaffer_find(context->comms, 0, 0, 0);

    scontext->results = gres;
    scontext->row = 0;

    stream =
	librdf_new_stream(storage->world,
			  (void*)scontext,
			  &gaffer_results_stream_end_of_stream,
			  &gaffer_results_stream_next_statement,
			  &gaffer_results_stream_get_statement,
			  &gaffer_results_stream_finished);
    if(!stream) {
	gaffer_results_stream_finished((void*)scontext);
	return NULL;
    }
  
    return stream;

}

/**
 * librdf_storage_gaffer_find_statements:
 * @storage: the storage
 * @statement: the statement to match
 *
 * .
 * 
 * Return a stream of statements matching the given statement (or
 * all statements if NULL).  Parts (subject, predicate, object) of the
 * statement can be empty in which case any statement part will match that.
 * Uses #librdf_statement_match to do the matching.
 * 
 * Return value: a #librdf_stream or NULL on failure
 **/
static librdf_stream*
librdf_storage_gaffer_find_statements(librdf_storage* storage,
                                      librdf_statement* statement)
{
    librdf_storage_gaffer_instance* context;
    gaffer_results_stream* scontext;
    librdf_stream* stream;
    const unsigned char* fields[4];
  
    context = (librdf_storage_gaffer_instance*)storage->instance;

    scontext =
	LIBRDF_CALLOC(gaffer_results_stream*, 1, sizeof(*scontext));
    if(!scontext)
	return NULL;

    scontext->storage = storage;
    librdf_storage_add_reference(scontext->storage);

    scontext->gaffer_context = context;

    statement_helper(storage, statement, fields, 0);

    gaffer_results* gres = gaffer_find(context->comms,
				       (const char*) fields[0],
				       (const char*) fields[1],
				       (const char*) fields[2]);

    scontext->results = gres;
    scontext->row = 0;

    stream =
	librdf_new_stream(storage->world,
			  (void*)scontext,
			  &gaffer_results_stream_end_of_stream,
			  &gaffer_results_stream_next_statement,
			  &gaffer_results_stream_get_statement,
			  &gaffer_results_stream_finished);
    if(!stream) {
	gaffer_results_stream_finished((void*)scontext);
	return NULL;
    }
  
    return stream;

}

/**
 * librdf_storage_gaffer_context_add_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to add
 *
 * Add a statement to a storage context.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_gaffer_context_add_statement(librdf_storage* storage,
                                            librdf_node* context_node,
                                            librdf_statement* statement) 
{
    const unsigned char* fields[4];

#ifdef DUPLICATE_STATEMENT_CHECKING
    /* Do not add duplicate statements */
    if (librdf_storage_gaffer_context_contains_statement(storage, context_node, statement))
	return 0;
#endif

    statement_helper(storage, statement, fields, context_node);

    librdf_storage_gaffer_instance* context; 
    context = (librdf_storage_gaffer_instance*)storage->instance;

    int ret = gaffer_add(context->comms,
			 (const char*) fields[0],
			 (const char*) fields[1],
			 (const char*) fields[2]);
    if (ret < 0) return -1;

    return 0;

}


/**
 * librdf_storage_gaffer_context_remove_statement:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 * @statement: #librdf_statement statement to remove
 *
 * Remove a statement from a storage context.
 * 
 * Return value: non 0 on failure
 **/
static int
librdf_storage_gaffer_context_remove_statement(librdf_storage* storage, 
                                               librdf_node* context_node,
                                               librdf_statement* statement) 
{
    const unsigned char* fields[4];

    statement_helper(storage, statement, fields, context_node);

    librdf_storage_gaffer_instance* context;
    context = (librdf_storage_gaffer_instance*)storage->instance;

    int ret = gaffer_remove(context->comms,
			    (const char*) fields[0],
			    (const char*) fields[1],
			    (const char*) fields[2]);
    if (ret < 0) return -1;

    return 0;

}


static  int
librdf_storage_gaffer_context_remove_statements(librdf_storage* storage, 
                                                librdf_node* context_node)
{

    //FIXME: Not implemented.

    return -1;

}

/**
 * librdf_storage_gaffer_context_serialise:
 * @storage: #librdf_storage object
 * @context_node: #librdf_node object
 *
 * Gaffer all statements in a storage context.
 * 
 * Return value: #librdf_stream of statements or NULL on failure or context is empty
 **/
static librdf_stream*
librdf_storage_gaffer_context_serialise(librdf_storage* storage,
                                        librdf_node* context_node) 
{

    //FIXME: Not implemented.

    return 0;

}

/**
 * librdf_storage_gaffer_context_get_contexts:
 * @storage: #librdf_storage object
 *
 * Gaffer all context nodes in a storage.
 * 
 * Return value: #librdf_iterator of context_nodes or NULL on failure or no contexts
 **/
static librdf_iterator*
librdf_storage_gaffer_get_contexts(librdf_storage* storage) 
{
    // FIXME: Not implemented.

    return 0;

}

/**
 * librdf_storage_gaffer_get_feature:
 * @storage: #librdf_storage object
 * @feature: #librdf_uri feature property
 *
 * Get the value of a storage feature.
 * 
 * Return value: #librdf_node feature value or NULL if no such feature
 * exists or the value is empty.
 **/
static librdf_node*
librdf_storage_gaffer_get_feature(librdf_storage* storage, librdf_uri* feature)
{
    /* librdf_storage_gaffer_instance* scontext; */
    unsigned char *uri_string;

    /* scontext = (librdf_storage_gaffer_instance*)storage->instance; */

    if(!feature)
	return NULL;

    uri_string = librdf_uri_as_string(feature);
    if(!uri_string)
	return NULL;

    // FIXME: This is a lie.  Contexts not implemented. :-/
    if(!strcmp((const char*)uri_string, LIBRDF_MODEL_FEATURE_CONTEXTS)) {
	return librdf_new_node_from_typed_literal(storage->world,
						  (const unsigned char*)"1",
						  NULL, NULL);
    }

    return NULL;
}


/**
 * librdf_storage_gaffer_transaction_start:
 * @storage: #librdf_storage object
 *
 * Start a new transaction unless one is already active.
 * 
 * Return value: 0 if transaction successfully started, non-0 on error
 * (including a transaction already active)
 **/
static int
librdf_storage_gaffer_transaction_start(librdf_storage *storage)
{

    //FIXME: Not implemented.  Would need to batch stuff up, into single REST
    // call.
    return -1;

}


/**
 * librdf_storage_gaffer_transaction_commit:
 * @storage: #librdf_storage object
 *
 * Commit an active transaction.
 * 
 * Return value: 0 if transaction successfully committed, non-0 on error
 * (including no transaction active)
 **/
static int
librdf_storage_gaffer_transaction_commit(librdf_storage *storage)
{

    //FIXME: Not implemented.
    return -1;

}


/**
 * librdf_storage_gaffer_transaction_rollback:
 * @storage: #librdf_storage object
 *
 * Roll back an active transaction.
 * 
 * Return value: 0 if transaction successfully committed, non-0 on error
 * (including no transaction active)
 **/
static int
librdf_storage_gaffer_transaction_rollback(librdf_storage *storage)
{

    // FIXME: Not implemented.
    return -1;

}

/** Local entry point for dynamically loaded storage module */
static void
librdf_storage_gaffer_register_factory(librdf_storage_factory *factory) 
{
    LIBRDF_ASSERT_CONDITION(!strcmp(factory->name, "gaffer"));

    factory->version            = LIBRDF_STORAGE_INTERFACE_VERSION;
    factory->init               = librdf_storage_gaffer_init;
    factory->terminate          = librdf_storage_gaffer_terminate;
    factory->open               = librdf_storage_gaffer_open;
    factory->close              = librdf_storage_gaffer_close;
    factory->size               = librdf_storage_gaffer_size;
    factory->add_statement      = librdf_storage_gaffer_add_statement;
    factory->add_statements     = librdf_storage_gaffer_add_statements;
    factory->remove_statement   = librdf_storage_gaffer_remove_statement;
    factory->contains_statement = librdf_storage_gaffer_contains_statement;
    factory->serialise          = librdf_storage_gaffer_serialise;
    factory->find_statements    = librdf_storage_gaffer_find_statements;
    factory->context_add_statement    = librdf_storage_gaffer_context_add_statement;
    factory->context_remove_statement = librdf_storage_gaffer_context_remove_statement;
    factory->context_remove_statements = librdf_storage_gaffer_context_remove_statements;
    factory->context_serialise        = librdf_storage_gaffer_context_serialise;
    factory->get_contexts             = librdf_storage_gaffer_get_contexts;
    factory->get_feature              = librdf_storage_gaffer_get_feature;
    factory->transaction_start        = librdf_storage_gaffer_transaction_start;
    factory->transaction_commit       = librdf_storage_gaffer_transaction_commit;
    factory->transaction_rollback     = librdf_storage_gaffer_transaction_rollback;
}

#ifdef MODULAR_LIBRDF

/** Entry point for dynamically loaded storage module */
void
librdf_storage_module_register_factory(librdf_world *world)
{
    librdf_storage_register_factory(world, "gaffer", "Gaffer",
				    &librdf_storage_gaffer_register_factory);
}

#else

/*
 * librdf_init_storage_gaffer:
 * @world: world object
 *
 * INTERNAL - Initialise the built-in storage_gaffer module.
 */
void
librdf_init_storage_gaffer(librdf_world *world)
{
    librdf_storage_register_factory(world, "gaffer", "Gaffer",
				    &librdf_storage_gaffer_register_factory);
}

#endif

