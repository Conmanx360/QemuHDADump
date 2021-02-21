# QemuHDADump
This program dumps the CORB buffer containing HDA verbs from a virtual machine instance.
This is useful for capturing verbs from an operating system that has a driver for your
device (i.e Windows), and then using this to enable support in other operating systems.


For a guide on how to setup Qemu to enable traces and create a virtual machine, look [here.](https://github.com/Conmanx360/QemuHDADump/wiki/Setup-and-usage-of-the-program)


Once the frame files have been generated, point `ExtractHDADump` to the folder containing
the frame files, and it will create a combined file of all the verb buffers. You can then 
use any program you like to look through this. For ca0132 codecs, I'd reccomend using the
program  `ca0132-frame-dump-formatted` from my [ca0132-tools](https://github.com/Conmanx360/ca0132-tools)
repo.
