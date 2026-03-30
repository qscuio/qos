#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

char *data[] = { "alpha", "bravo", "charlie", "delta",
      "echo", "foxtrot", "golf", "hotel", "india", "juliet",
      "kilo", "lima", "mike", "november", "oscar", "papa",
      "quebec", "romeo", "sierra", "tango", "uniform",
      "victor", "whisky", "x-ray", "yankee", "zulu"
       };

int
main(void)
{
    ENTRY e, *ep;
    struct hsearch_data *htab;
    int i;
    
    /*dynamically allocate memory for a single table.*/ 
    htab = (struct hsearch_data*) malloc( sizeof(struct hsearch_data) );
    /*zeroize the table.*/
    memset(htab, 0, sizeof(struct hsearch_data) );
    /*create 30 table entries.*/
    assert( hcreate_r(30,htab) != 0 );
    
    /*add data to 24 entries.*/
    for (i = 0; i < 24; i++) {
        e.key = data[i];
        /* data is just an integer, instead of a
           pointer to something */
        e.data = (void *)(size_t) i;
        
        /* there should be no failures */
        if (hsearch_r(e, ENTER, &ep, htab) == 0) {
            fprintf(stderr, "entry failed\n");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 22; i < 26; i++) {
        /* print two entries from the table, and
           show that two are not in the table */
        e.key = data[i];
        
        if (hsearch_r(e, FIND, &ep, htab) == 0) {
            ep = NULL;
        }
        printf("%9.9s -> %9.9s:%lu\n", e.key,
               ep ? ep->key : "NULL", ep ? (long)(ep->data) : 0);
    }
    
    //dispose of the hash table and free heap memory
    hdestroy_r(htab);
    free(htab);
    
    exit(EXIT_SUCCESS);
}
