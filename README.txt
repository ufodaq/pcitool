/** installation **/

1. download the repository from UFO source tree.
2. from base repository : "cmake -g ."
3. put in the pcitool/config.txt file the pwd to the xml file you want
4. you may just replace the camera.xml file with the one you want with the
same name

/**utilization**/
--> BEWARE : for now, this is still an unstable version, "pci -l[l]" will work
for sure, but "pci -r" and "pci -w" work only depending on the protocol used
for registers

the utilization is the same as usual, the user just have to modify the xml
file for changing the configuration. Here only xml is supported, views aren't
still put in it.

the only new command on this part is "pci -v" that will validate the xml file
regarding the xsd file in the config.txt file

/** complementary documentation**/
the user may find a more complete documentation (still in writing) in ziio/
documentation repository on the ufo source tree.
