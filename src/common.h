#ifndef COMMON_H
#define COMMON_H

typedef struct wipf_conf {
  UINT32 len;
  UCHAR data[1];
} WIPF_CONF, *PWIPF_CONF;

#define UNUSED(p)	((void) (p))

#endif COMMON_H
