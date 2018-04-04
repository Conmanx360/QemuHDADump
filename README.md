# QemuHDADump
Dumps HDA verbs from the CORB buffer of a virtual machine. Useful for reverse engineering drivers on different operating systems.

I thought up the idea, and could only find information on anyone else doing something similar here: https://hakzsam.wordpress.com/2015/02/21/471/ . This really helped me realize it was possible. As a disclaimer, I am pretty new to programming. This probably isn't the prettiest program ever, but it does work.

  This program is based off of the trace ability of QEMU. It uses the vfio_region_write trace in order to see what is being written to the MMIO pci regions, in this case the hda-verbs in the CORB buffer.
 
  In order to use this program, you'll need to compile your own qemu to disable MMIO and force writes to the memory mapped regions to go through a trap instruction. I might be using the terminology wrong, but I think most people understand what I mean. Don't let the regions get memory mapped, force them to go through the program each time to be accessed.
  
  In the qemu source, you'll need to edit the file pci.c located in the hw/vfio folder. Edit the value:
      DEFINE_PROP_UINT32("x-intx-mmap-timeout-ms", VFIOPCIDevice,
                       intx.mmap_timeout, 1100)
  
  From 1100 to 0 to prevent mmap'ing, and then also edit:
  
  DEFINE_PROP_BOOL("x-no-mmap", VFIOPCIDevice, vbasedev.no_mmap, true)
 
  From true to false to prevent it completely. The first timeout part might not be necessary, but I do it anyways.
  
  Now, before compiling, do a configure in the source directory, like this:
  
  ./configure --enable-trace-backends=log --target-list=x86_64-softmmu
  
  This will make the trace-events output to stderr, and only compile the x86_64 architecture of qemu. If you want to compile a different arch, then include it.
  
   Now, you'll need to create a trace-events file, that tells qemu which trace-events you want to see. I named mine events.txt . Just put "vfio_region_write" in the file. This will enable that trace-event. Now, what you'll need to do is add this to your qemu startup command:
   -trace events=/path/to/events.txt

  Of course, the /path/to/events.txt is meant to be the path to YOUR events.txt file. 
  
  Also, make sure qemu is setup to use the command line like so:
  
  -monitor stdio
  
  Now, to using QemuHDADump.
  
  This program accesses the current terminal it is opened in to write the dump commands in. So, you'll need to figure out your current terminal by running the tty command. Here's my output, for example:
  
connor@connor-Z97X-Gaming-7:~$ tty
/dev/pts/4

  So, now, here's the command you run:
  
  sudo ./script-for-qemu.sh 2>&1 >/dev/null | ./QemuHDADump /dev/pts/4
  
  The first command is your qemu startup script. I will include mine for example just in case. The second part, '2>&1 >/dev/null' redirects the stdout to /dev/null, so it doesn't confuse QemuHDADump. It probably wouldn't break it if you didn't, but I prefer keeping it away. It also redirects stderr to stdout, which is then piped to QemuHDADump.
  
  Now, QemuHDADump finds the CORB buffer address from the write to the register located at offset 0x40. It then uses this address when it runs the command pmemsave on the qemu terminal. It creates files named 'frameXX' (XX being the number of the frame) and a new one is created each time the CORB buffer rolls over from 0xff to 0x00. These files will be in the directory of wherever you ran the command from.
  
  The terminal of QemuHDADump also shows which address the verb that was just written will be at in the finished file, which is useful if you want to see what commands are run when a specific action is taken. For example, I've mapped out the commands for the Sound Blaster Z using this, writing down the address before the command was run and after.
  
  Once you're finished, shut down the virtual machine. Currently, the program just crashes. Not very elegant, but I'm a dumby. I'm sure someone more experienced could fix it, but it doesn't hurt anything (for me atleast) so I haven't bothered. But this also means you don't get the shutdown verbs. If you want those, I'd suggest disabling the driver inside the operating system. That's how I got the Sound Blaster Z shutdown. 
  
  Anyways, after shutdown, copy the frame files and put them in a folder, or don't, but you probably should, it makes the next program I wrote work better. If you don't, make sure there are not other files with the name "frame" inside your folder. Now, copy the "ExtractHDADump" program into the current directory, and run './ExtractHDADump ./' . This should create two files, allCORBframes, and allRIRBframes. The names should be self explanatory, but if they're not, the allCORBframes contains all of the hda-verbs in one file, and the allRIRBframes contains all of the response verbs in one file.
  
  These are raw data, so I'd suggest formatting them. I'll include my formatting program, it is pretty simple. It prints them out as hexadecimal unsigned int's, which are easier to read for me. Most hex editors just show them in byte order instead.
  
  To use the formatting program, just type:
  
  ./frameDumpFormatted /path/to/allCORBframes
  
  It will print the addresses on the left, and output it to the terminal. I usually tee the output to a file so I can mess with it and write comments myself.
  
  So now you have the hda-verbs as they ran inside the virtual machine. Do with them as you please. Maybe use it to make a sound card that works on one operating system work on another. That'd be nice of you.
  
  In this guide/explanation, I didn't include how to do VFIO stuffs. You should be able to google that yourself. If not, I can do a write-up, but I'm pretty sure there are better guides than I could write out there currently.


   Thanks for reading, and I hope this is helpful to someone else.
