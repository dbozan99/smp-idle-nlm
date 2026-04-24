# smp-idle-nlm
Multiple-CPU aware power-saving (HLT) for Novell Netware 5 and 6.

Are you still running Novel Netware 5 or 6 (or 6.5)? Well chances are that you're running it in a VM with one CPU because the only available NLM that tells the CPU to idle (instead of using the entire 100% in a polling loop) wasn't made to be multi-CPU aware. Netware is capable of seeing up to 32 CPUs (sockets, not threads) though. So I made this NLM to issue the HLT command on every CPU instead of just the first one.

This was a lot easier said than done though. Finding the SDK is tricky these days and even then the code refused to work on anything but CPU0 until I found the mpkxdc tool which was required during the build to tell Netware that yes, this program really is multi-thread-safe.

I've tried to make the code as bulletproof as I can so you should have no issues running it. To add more CPU's to your VM, make sure that you add them as Sockets with 1 thread each, otherwise Netware will not see them and will not initialize them.

If you're installing Netware, you may have better luck doing a manual install and when it asks about the PSMs, remove the ACPI driver and add the MPS driver instead. The ACPI driver seems to be very picky about the hardware it wants to run on.

## Installation and usage
Put `SMP-IDLE.NLM` at "`SYS:SYSTEM\SMP-IDLE.NLM`"

Load it with the "`LOAD SMP-IDLE.NLM`" command.

_(optional, but recommended)_ 
Add the following line to your "`SYS:SYSTEM\AUTOEXEC.NCF`" file:

`LOAD SMP-IDLE.NLM`

## **_Disclaimer_**
_Please note that I have tested this on bare-metal and in several VMs (Proxmox KVM/Qemu, ESXi, VMware Workstation, and VirtualBox) and have had no issues for several days in any of them, but I'm a hobbyist and I've not tested this in a "real environment" so I make no guarantees about it's stability._
