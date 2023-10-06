all:
#	gcc -Wl,-z,now,-z,relro main.c -o ../files/chall -mmanual-endbr -no-pie
	gcc main.c -c -fcf-protection=none -fno-plt -o chall.o
	/usr/lib/gcc/x86_64-linux-gnu/11/collect2 --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -dynamic-linker /lib64/ld-linux-x86-64.so.2 ./crt2.o /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/11/crtbegin.o -L/usr/lib/gcc/x86_64-linux-gnu/11 -L/usr/lib/x86_64-linux-gnu -L/usr/lib -L/lib/x86_64-linux-gnu -L/lib -L/usr/lib/x86_64-linux-gnu -z now -z relro chall.o -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/11/crtend.o /usr/lib/x86_64-linux-gnu/crtn.o -o chall
	mv chall ../files/xor
	cp ../files/xor ../solver/xor
	cp main.c ../files/
