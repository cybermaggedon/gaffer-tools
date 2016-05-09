
struct gaffer_comms_str;
typedef struct {

    int count;

    char** s;
    char** p;
    char** o;

} gaffer_results;

typedef struct gaffer_comms_str gaffer_comms;

gaffer_comms* gaffer_connect(const char* host);

void gaffer_disconnect(gaffer_comms*);

int gaffer_count(gaffer_comms*, const char* s, const char* p, const char* o);

int gaffer_test(gaffer_comms*);

int gaffer_add(gaffer_comms* gc,
	       const char* s, const char* p, const char* o);

#define GAFFER_BATCH_SIZE 100

int gaffer_add_batch(gaffer_comms* gcm,
		     char* batch[GAFFER_BATCH_SIZE][3], int rows, int columns);

int gaffer_remove(gaffer_comms* gc,
		  const char* s, const char* p, const char* o);

gaffer_results* gaffer_find(gaffer_comms*,
			    const char* s, const char* p, const char* o);

void gaffer_free_results(gaffer_results* res);

int gaffer_size(gaffer_comms *);


