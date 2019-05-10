# beepr 0.1.4
  
The beepr program generated from this source generate beep sound
using various methods. Using ioctl() requires root priviledges to
write to /dev/tty0.
  
Typical use:  
  host@~$ beepr -b 01
  host@~$ beepr -f 440 --ioctl
  
To compile the beepr program in a terminal, go to the source 
directory, type:  
make  
  
Copy the executable to /bin or wherever you prefer
