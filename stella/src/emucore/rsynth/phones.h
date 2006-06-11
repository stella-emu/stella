#define PHONE(nm,st,br,am,ex) nm
enum phone_e { SIL,
#include "phones.def"
END };
#undef PHONE
extern char *ph_name[];
extern char *ph_br[];
extern char *ph_am[];
