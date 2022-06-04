#include "lua.h"
#include "lauxlib.h"

#define l_getc(f)		getc(f)
#define l_lockfile(f)   ((void)0)
#define l_unlockfile(f)	((void)0)

static int read_line (lua_State *L, FILE *f, int chop) {
  luaL_Buffer b;
  char c = '\0';

  luaL_buffinit(L, &b);
  
  char nl = '\n';
  
  if (f == stdin) {
      nl = '\r';
  }
  
  while ((c != (char)EOF) && (c != nl)) {  /* repeat until end of line */
    char *buff = luaL_prepbuffer(&b);  /* pre-allocate buffer */
    int i = 0;
    l_lockfile(f);  /* no memory errors can happen inside the lock */
    while ((i < LUAL_BUFFERSIZE) && ((c = l_getc(f)) != (char)EOF) && (c != nl)) {
      buff[i++] = c;
    
      if (f == stdin) {
          putc(c, stdout);
      }
    }
    
    l_unlockfile(f);
    luaL_addsize(&b, i);
  }
  if (!chop && c == nl)  /* want a newline and have one? */
    luaL_addchar(&b, c);  /* add ending newline to result */
  luaL_pushresult(&b);  /* close buffer */
  /* return ok if read something (either a newline or something else) */
  return (c == nl || lua_rawlen(L, -1) > 0);
}

#undef l_getc
#undef l_lockfile
#undef l_unlockfile
