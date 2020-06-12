DX7 Patch Converter for the XFM2 FM synth


Converts DX7 patch banks (32 patches) to sysex files for use with the XFM2
patch editor also found in this github repository. 


Note that the XFM2 will not load a patch in sysex format directly. 
The XFM2 will dump a patch as sysex but patches have to be loaded
by sending sysex messages one parameter at a time. The patch editor does this for you.


Example of use:


dx72xfm2 rom1.syx


where rom1.sys is a dx7 patch bank. Output will be 32 .syx files, one for each patch.
The name of the file is the name of the patch, spaces are converted to underscores.
The program spits out a bunch of DX7 patch info which I found useful for debugging and
fine tuning the settings of the XFM2. 


Format of the resulting .syx files:

5 byte header 0xf0 0x43 0x00 0x00 0x00   - 3rd byte is XFM2 unit number, always 0 in .syx files

512 parameters stored in MSB LSB midi format. range of most XFM2 parameters is 0-255

1 byte trailer 0xf7


The program doesn't convert a few parameters e.g. ratio/fixed flag and one or two
others that are rarely used. I will probably add them at some point.


I tested this with the DX7 rom1 patches comparing the sounds of the XFM2 to DEXED (I don't own a DX7). In most cases
the sounds are very close. You may have to fine tune operator levels, LFO rates, LFO level etc.
The intent is not to reproduce DX7 sounds exactly (which is almost impossible) but to get a starting
point for your own custom patches. The XFM2 is capable of much richer sounds than the DX7.


Compiled with gcc on windows 7.


R Heslip June 2020

