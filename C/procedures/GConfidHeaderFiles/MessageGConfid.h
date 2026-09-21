#ifndef MESSAGEGCONFID_H
#define MESSAGEGCONFID_H

/* Define either the macro _en or _fr in makefile */
#if defined (_en)
#include "MessageGConfid_en.h"
#elif defined (_fr)
#include "MessageGConfid_fr.h"
#else
#error "Define either the macro _en or _fr"
#endif

#endif
