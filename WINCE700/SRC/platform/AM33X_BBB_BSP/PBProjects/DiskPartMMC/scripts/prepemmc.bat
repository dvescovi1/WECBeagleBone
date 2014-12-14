echo off
echo CAUTION: Only run this command when booted from uSD card!
echo CTRL C [^C] to exit, any key to continue.
pause
echo on
diskpartemmc -s \windows\partemmc.txt
cd \Storage Card
copy mlo \boot
copy ebootsd.nb0 \boot
copy nk.bin \boot
copy logo.bmp \boot
