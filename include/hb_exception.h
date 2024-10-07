#ifndef _HB_EXCEPTION_H
#define _HB_EXCEPTION_H

#include "os.h"
#include "vc.h"

enum ppc_exception {
    EX_RESET,
    EX_MACH_CHECK,
    EX_DSI,
    EX_ISI,
    EX_EXT_INTR,
    EX_ALIGN,
    EX_PROG,
    EX_FP_UNAVAIL,
    EX_DECR,
    EX_SYSCALL,
    EX_TRACE,
    EX_MAX
};

void init_hb_exceptions(void);
void __handle_exception(u8 type, OSContext* context);

#endif
