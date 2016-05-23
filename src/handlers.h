void unknown_handler();
void unknown_software_handler();


void df_handler();
void gp_handler();
void pf_handler();
void ud_handler();

/*******
 * IRQ *
 *******/
void irq1_handler();

// NOOP interrupt
void int32_handler();
// PERMAHALT interrupt
void int33_handler();
// SERIAL PRINT interrupt
void int34_handler();
// FORK interrupt (rcx = the new IP of fork)
void int35_handler();
// YIELD interrupt
void int36_handler();
// ALLOC interrupt (address in rax)
void int37_handler();
