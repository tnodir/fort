#ifndef FORTTLSF_H
#define FORTTLSF_H

#include "fortdrv.h"

#define TLSF_API FORT_API

#define tlsf_assert NT_ASSERT
#define tlsf_printf

#define TLSF_MAX_POOL_SIZE (16 * 1024 * 1024)

#include "../3rdparty/tlsf/tlsf.h"

#endif // FORTTLSF_H
