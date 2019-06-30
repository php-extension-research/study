#ifndef ASM_CONTEXT_H
#define ASM_CONTEXT_H

#include "study.h"

typedef void* fcontext_t;

intptr_t jump_fcontext(fcontext_t *ofc, fcontext_t nfc, intptr_t vp, bool preserve_fpu = false);
fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(intptr_t));

#endif	/* ASM_CONTEXT_H */