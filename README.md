ext4-undelete
=============

Application for undelete demonstration under ext4 fs 

MAKE:
This application is using e2fslibs. If you want to make this 
application, you will need e2fslibs-dev package.

Type make to build this app.

RUN:
To run this application, just type ./ext-undelete and help
info will be printed to your screen.

EXAMPLE:
To undelete file, which had before unlink process inode 
number 12, and was stored on /dev/sda2 partition type:

./ext-undelete /dev/sda2 -i 12

If undeleted file contains zero bytes in the end of file 
and you want to remove them, just add -s option:

./ext-undelete /dev/sda2 -i 12 -s

File is going to be stored in out.undeleted file in the 
same directory, as you executed this app. If you wanna select 
different filename, specify it by -o option.

./ext-undelete /dev/sda2 -i 12 -s /tmp/hello.txt

ATTENTION:
Please make sure, before using ext-undelete, unmount partition
from which you want to undelete file.

TESTS:
Scripts for testing undelete are under scripts folder. Please
read README file in scripts folder to learn, how to execute
scripts.