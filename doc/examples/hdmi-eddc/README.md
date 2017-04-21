# HDMI EDID

Modern displays have a plug and play feature allowing the data source (CPU,
FPGA, or in this case an RPi) to request data from the sink (Monitor), and to
learn what modes it supports and whether or not it has a preferred mode.

This communication takes place over I2C.  So, after a brief search online,
I found [this diagram](sparkfun.png) of an I2C transaction.  It wasn't what I
remembered of how I2C worked, but the diagram was simple enough that I could
build a core to match it.  Five hours later, I had a core that matched the spec,
together with a test bench proving that this core worked.

I placed the core into my design and ... nothing worked.  The HDMI source
(an RPi) declared to me over and over that there was no monitor attached.
What was wrong?  My core "worked", in that it passed its test bench, but ...
what was going wrong?

To found out, I needed to place a scope into my design.  I2C, though, is a
_very_ slow protocol.  If I captured what happened on every clock cycle, I might
never see a complete transaction.  Instead, I chose to use a compressed scope.
In order to make certain that the compression worked, and compressed things as
desired, I only placed four wires into the scope's data port: the I2C wires
as they exist on the line, as well as whether or not I was commanding any I2C
wires.  (The way this "commanding" works, is that if any output wire is zero,
I pull the wire down, otherwise I leave it at high impedence.)

When I placed the result into GTKwave, I was presented with a screen shot
looking like [this](actual.png).

By looking at this diagram, I learned several things:
- The master starts by _writing_ to the slave.  Under the
[sparkfun](sparkfun.png) diagram model, and given a ROM memory (EDID is
supposed to be ROM), writing to the display monitor makes no sense.
- The first byte the master writes to the slave is 0xa0.  This matches my
memory of I2C, if the master is trying to write to slave ID 0x50--which just
happens to be the slave ID of the monitor in HDMI.  (The sparkfun diagram model didn't discuss slave addresses ...)
- There's also a strange feature at the end of the second byte that the master writes to the slave.  In general, it is a violation of protocol to change the data while the clock is high.  However, if the data line is dropped while the clock is high, this is termed a start bit.  In this case, it's what's known as a repeated start bit, and is a more modern extension to I2C.  

My biggest conclusion?  I didn't understand the I2C standard used by the
E-DDC, and all my work building to this standard was done ... in error.

Time to start over.  I'm not done yet, but I wouldn't have gotten this far
without the scope.
