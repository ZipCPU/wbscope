# A Wishbone Controlled Scope for FPGA's

This is a generic/library routine for providing a bus accessed 'scope' or
(perhaps more appropriately) a bus accessed logic analyzer for use internal to
an FPGA.  The general operation is such that this 'scope' can record and report
on any 32 bit value transiting through the FPGA.  Once started and reset, the
scope records a copy of the input data every time the clock ticks with the
circuit enabled.  That is, it records these values up until the trigger.  Once
the trigger goes high, the scope will record for ``bw_holdoff`` more counts
before stopping.  Values may then be read from the buffer, oldest to most
recent.  After reading, the scope may then be reset for another run.

In general, therefore, operation happens in this fashion:
1. A reset is issued.
2. Recording starts, in a circular buffer, and continues until
3. The trigger line is asserted.
  The scope registers the asserted trigger by setting the ``o_triggered`` output flag.
4. A counter then ticks until the last value is written.  
  The scope registers that it has stopped recording by setting the ``o_stopped`` output flag.
5. The scope recording is then paused until the next reset.
6. While stopped, the CPU can read the data from the scope
7. -- oldest to most recent
8. -- one value per bus clock
9. Writes to the data register reset the address to the beginning of the buffer

