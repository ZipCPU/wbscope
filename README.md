# A Wishbone Controlled Scope for FPGA's

This is a generic/library routine for providing a bus accessed 'scope' or
(perhaps more appropriately) a bus accessed logic analyzer for use internal to
an FPGA.  The general operation is such that this 'scope' can record and report
on any 32 bit value transiting through the FPGA that you have connected to the
scope.  Once started and reset, the
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

  - oldest to most recent
  - one value per bus clock

7. Writes to the data register reset the address to the beginning of the buffer

# Tutorials

The Wishbone scope was featured on [zipcpu.com](http://zipcpu.com) as [a
conclusion](http://zipcpu.com/blog/2017/07/08/getting-started-with-wbscope.html)
to the discussion of the example [debugging
bus](https://github.com/ZipCPU/dbgbus/tree/master/hexbus).
That example discussed how to hook up the scope to your logic, as well as how
to employ the [scope software](sw/scopecls.cpp) to create a VCD file
that could then be viewed in GTKWave.
The scope was also mentioned as a means of capturing [traces of button
bounces](http://zipcpu.com/blog/2017/08/02/debounce-teaser.html),
with the short discussion of how to set it up for that task
[here](http://zipcpu.com/blog/2017/08/07/bounce-dbgbus.html).

# Interfaces supported

1. [Wishbone B4/pipelined](rtl/wbscope.v)
2. [AXI lite](rtl/axilscope.v)
3. [Avalon](rtl/avscope.v)
4. [Memory backed scope, using AXI](rtl/memscope.v).  This is great for when
   your data capture can't git in the on-chip RAM of a device.  Using this
   core, you can store your capture in an off-chip SDRAM.  Beware, an
   SDRAM can hold a _LOT_ of data.
5. [_Compressed_ Memory backed scope, using AXI](rtl/memscopc.v).  This uses the
   same basic run-length compression scheme as the [original compressed
   Wishbone scope](rtl/wbscopc.v), only this time with the AXI memory
   back end.

# Commercial Applications

Should you find the GPLv3 license insufficient for your needs, other licenses
can be purchased from [Gisselquist Technology,
LLC](http://zipcpu.com/about/gisselquist-technology.html).
