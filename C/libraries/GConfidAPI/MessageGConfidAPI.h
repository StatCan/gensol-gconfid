#ifndef MESSAGEGCONFIDAPI_H
#define MESSAGEGCONFIDAPI_H

#include "IOUtil.h"
/* Define either the macro _en or _fr in makefile */

#if defined (_en)
#include "MessageGConfidAPI_en.h"
#elif defined (_fr)
#include "MessageGConfidAPI_fr.h"
#else
#error "Define either the macro _en or _fr"
#endif

#endif
