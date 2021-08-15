#include <stdio.h>
#include <stdarg.h>
#include "my_debug.h"

void my_printf(const char *pcFormat, ...) {
    char pcBuffer[256] = {0};
    va_list xArgs;
    va_start(xArgs, pcFormat);
    vsnprintf(pcBuffer, sizeof(pcBuffer), pcFormat, xArgs);
    va_end(xArgs);
    printf("%s", pcBuffer);
    fflush(stdout);
}


void my_assert_func(const char *file, int line, const char *func,
                    const char *pred) {
    printf("assertion \"%s\" failed: file \"%s\", line %d, function: %s\n",
           pred, file, line, func);
    fflush(stdout);
    __asm volatile("cpsid i" : : : "memory"); /* Disable global interrupts. */
    while (1) {
        __asm("bkpt #0");
    };  // Stop in GUI as if at a breakpoint (if debugging, otherwise loop
        // forever)
}
