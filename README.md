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
