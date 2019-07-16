# Admiral - Modules for VCV Rack

This is a work in progress collection for Rack version 1.

## Watches - Switched Multiples

![modules screenshot](./images/watches.png)

Watches is a passive multi-connector similar to a classic multiple.
What's different is that each socket has a 3-position switch to
connect the jack to one of three internal buses.

All **inputs on a bus are added together and sent to the output
jacks** connected to that bus.

The top section has three inputs and two outputs, the bottom section
is the opposite and has two inputs and three outputs.

Normally Watches is separated in a top and bottom section that act
independently. When the `+` button is red the top three buses are
connected to their bottom counterparts and the module act as one.

The **neutral switch position** connect jacks to the middle bus.
The middle bus is special and acts differently according to the `2:3`
switch:
  - On `2` only two buses are active, the middle bus mutes.
  - On `3` the middle bus is independent in each section.
  - In between the middle bus is shared between the two sections even
    when `+` is not active and they otherwise act independently.


## Shifts - Hybrid Bernoulli Mixer

![modules screenshot](./images/shifts.png)

Shifts is a hybrid mixer and Bernoulli gate. Each knob mixes linearly
between two things as long as the knob is in the top part. When going
beyond the gray markers and pointing the know down it starts acting as
a Bernoulli gate letting the other side take over more and more.

When a knob is in the Bernoulli zone it will always fully choose one
of its two channels: deciding which one by flipping a coin at each
trigger from `trig` if connected or from `ab` if not. The probability
of the coin toss is skewed according to the position of the knob. This
is how all three knobs of the module work.

**The top section** distributes `ab` or pans it between `A` and `B`
according to the top knob and can be modulated with +/-5V CV through
`aΦb`. This can act like **a panner** or a **standard Bernoulli
gate**.

**The middle section** controls the input of the bottom section. The
two options are `x` for using the external `a` and `b` inputs or `X`
for using the outputs from the top section `A` and `B`. The knob acts
the same way as the other knobs so it is possible to mix between the
top section `X` and the external input `x` either linearly or randomly
shifting from one to the other using Bernoulli gates. Both the `A`/`a`
and `B`/`b` mix will be affected the same and can be modulated with
+/-5V CV through `XΦx`. This can act like **a send-return** to two
different effects or **a simple mixer.**

**The bottom section:** `AB` is mixed from `a` and `b` according to
the top knob and can be modulated with +/-5V CV through `AΦB`. `AB` is
a linear mix of `a` and `b` or shifts from one to the other if the
knob is in the Bernoulli zone. This can act like **a mixer** or a
**reversed Bernoulli gate**.
