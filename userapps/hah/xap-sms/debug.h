/* $Id$
 */

/* usage:
      printf(WHERESTR ": hey, x=%d\n", WHEREARG, x);
 */
#define WHERESTR "[file %s, line %d] "
#define WHEREARG __FILE__,__LINE__

/* usage:
      int foo;
      dumpme(foo,"%d")
*/
#define dumpme(x, fmt) printf("%s:%u: %s=" fmt, WHEREARG, #x, x)

#define in(x) if(g_debuglevel>8) printf("%s: %s <-- enter\n", __FILE__, x)
#define out(x) if(g_debuglevel>8) printf("%s: %s --> exit\n", __FILE__, x)
