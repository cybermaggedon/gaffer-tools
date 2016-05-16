
typedef enum {
    GAFFER_INTEGER = 4,
    GAFFER_FLOAT = 5,
    GAFFER_STRING = 6,
    GAFFER_URI = 7,
    GAFFER_BLANK = 8,
    GAFFER_NONE = 9
} gaffer_type;

typedef struct {
    char *term;
    gaffer_type type;
} gaffer_term;

typedef struct {
    gaffer_term s;
    gaffer_term p;
    gaffer_term o;
} gaffer_result;

typedef struct {
    int count;
    gaffer_result* results;
} gaffer_results;

struct gaffer_comms_str;
typedef struct gaffer_comms_str gaffer_comms;

gaffer_comms* gaffer_connect(const char* host);

void gaffer_disconnect(gaffer_comms*);

int gaffer_count(gaffer_comms*,
		 gaffer_term s, gaffer_term p, gaffer_term o);

int gaffer_test(gaffer_comms*);

int gaffer_add(gaffer_comms* gc,
	       gaffer_term s, gaffer_term p, gaffer_term o);

#define GAFFER_BATCH_SIZE 500

int gaffer_add_batch(gaffer_comms* gcm,
		     gaffer_term batch[GAFFER_BATCH_SIZE][3],
		     int rows, int columns);

int gaffer_remove(gaffer_comms* gc,
		  gaffer_term s, gaffer_term p, gaffer_term o);

gaffer_results* gaffer_find(gaffer_comms*,
			    gaffer_term s, gaffer_term p, gaffer_term o);

void gaffer_free_results(gaffer_results* res);

int gaffer_size(gaffer_comms *);


