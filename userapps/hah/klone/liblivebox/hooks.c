/* $Id$
 */
int startup_hooks() {
	 const_init();
     return 0;
}

void hooks_setup (void)
{
    hook_server_init(startup_hooks);
}
