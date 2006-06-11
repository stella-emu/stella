/* $Id: phtoelm.h,v 1.1 2006-06-11 07:13:26 urchlay Exp $
*/
struct rsynth_s;
extern unsigned phone_to_elm (char *s, int n, darray_ptr elm, darray_ptr f0);
extern void say_pho(struct rsynth_s *rsynth, const char *path, int dodur,char *phoneset);

