CPPFLAGS_WARN=-Wall -Wextra -Werror -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal -Wpointer-arith -Wpedantic
CPPFLAGS=-std=gnu++1y -g -O2 -fomit-frame-pointer -fno-builtin-log $(CPPFLAGS_WARN)

SRCDIR=src
OBJDIR=obj

HEADERS=$(shell find src -name '*.hpp' -o -name '*.h')
SRC=$(shell find src -name '*.cpp')
OBJ=$(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
DEP=$(OBJ:.o=.dep)

.PHONY: format clean

ij: $(OBJ)
	$(CXX) $(CPPFLAGS) $^ -o $@ -m64 -lm -lstdc++

-include $(DEP)

format:
	clang-format -i $(SRC) $(HEADERS)

clean:
	rm -rf $(OBJDIR) $(TARGET)
	rm -f ij
$(OBJDIR)/%.o: src/%.cpp
	@# make directory if it doesnt exist, gcc cant do this :P
	+@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -c -o $@ $^
	$(CXX) -MM $(CPPFLAGS) $^ -o $(@:.o=.dep)