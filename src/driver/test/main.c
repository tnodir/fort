/* Fort Firewall Driver: Test */

#include "test.h"

#include "tst_conf.c"
#include "tst_host.c"
#include "tst_pcb.c"
#include "tst_utl.c"

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    tst_pcb_main();
    tst_utl_main();
    tst_conf_main();
    tst_host_main();

    return 0;
}
