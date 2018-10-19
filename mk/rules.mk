$(OBJECTS): | $(OBJ_DIR)
$(OBJ_DIR):
	@[ ! -d $(OBJ_DIR) ] && mkdir $(OBJ_DIR) 

# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
%.d: %.c
	@set -e; rm -f $@; \
	$(TOOLCHAIN)gcc -MM -MT '$*.o' $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(TOOLCHAIN)g++ $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(TOOLCHAIN)gcc $(CFLAGS) -c -o $@ $<

#+@printf "copying '$(notdir $<)' -> '$(notdir $@)' ("
#@stat --printf="%s" $@
#+@echo " / 262144)"
%.bin: %.elf
	$(TOOLCHAIN)objcopy -O binary $< $@

$(OBJ_DIR)/%$(BUILD_FPRINT).elf: src/app/%.o $(LIBRARY)
	$(TOOLCHAIN)gcc $(CFLAGS) $< $(LFLAGS) -Wl,-Map=$(OBJ_DIR)/$*$(BUILD_FPRINT).map -o $@

%.a: $(OBJECTS)
	ar rcs $@ $^

clean:
	@find . -name '*.o' -delete
	@find . -name '*.d' -delete
	@find . -name '*.d.*' -delete
	@rm -rf $(OBJ_DIR)

# specific target declarations
include mk/outputs.mk
