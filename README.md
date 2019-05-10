# beepr 0.1.4
  
The beepr program generated from this source generate beep sound
using various methods. Using ioctl() requires root priviledges to
write to /dev/tty0. Also CONFIG_INPUT_PCSPKR has to be enabled in
the kernel's configuration.
  
Typical use:  
 'beepr -b 01'
 'beepr -f 440 --ioctl'
  
To compile the beepr program in a terminal, go to the source 
directory, type:  
make  
  
Copy the executable to /bin or wherever you prefer
