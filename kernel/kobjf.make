default: $(objs)

CFLAGS = -w -O1 -m64 \
	-I../include \
	-mno-red-zone \
	-mmanual-endbr \
	-mgeneral-regs-only \
	-fno-stack-protector \
	-fcf-protection=none \
	-fno-asynchronous-unwind-tables

%.o: %.asm
	nasm -f elf64 -o $@ $<
	@cp $@ ..

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)
	@cp $@ ..

%.d: %.c
	@set -e; \
	rm -f $@; \
	$(CC) $(CFLAGS) -MM $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
-include $(objs:.o=.d)

.PHONY: clean
clean:
	-rm *.o *.d
