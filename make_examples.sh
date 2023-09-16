#/bin/sh

rm -r example_bin
mkdir -p example_bin
cd examples
for file in ./*; do
  ../bin/jaspl $file > ../example_bin/$(basename $file .jaspl).nasm
done
cd ../example_bin
for file in ./*; do
  nasm -f elf $file
done
for file in ./*.o; do
   gcc -m32 -nostdlib $file -o $(basename $file .o)
done
rm *.o *.nasm