rem Mount the disk image file as drive y. If you
rem want to use a different drive letter, change it 
rem in the line below and in the line at the end of the file.
imdisk -a -t file -f uodos.img -o rem -m y:
rem
XCOPY "C:\Users\Peter\OneDrive\uni\Systems Programming\Step9\TESTFILES" "y:" /i /e
rem
imdisk -D -m y:
