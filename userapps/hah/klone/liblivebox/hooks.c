/* $Id: hooks.c 3 2009-11-09 12:19:52Z brett $
 */
int startup_hooks() {
	 const_init();
     return 0;
}

void hooks_setup (void)
{
    hook_server_init(startup_hooks);
}
