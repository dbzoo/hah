/* $Id
 */
struct pach_resource {
     char *api;
     int feedid;
};

typedef struct pach_resource *pach_t;

pach_t pach_new(char *apikey, int feedid);
int pach_createFeed(pach_t p, char *title);
void pach_destroy(pach_t p);
int pach_updateDatastreamXml(pach_t p, char *xml);
