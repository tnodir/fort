/* Windows IP Filter Configuration Reader */

#include "wipfconf.h"


static BOOL
wipf_conf_ipblocked (const PWIPF_CONF conf, UINT32 remote_ip,
                     UINT32 path_len, const PVOID path, PBOOL notify)
{
  *notify = TRUE;
  return FALSE;
}

