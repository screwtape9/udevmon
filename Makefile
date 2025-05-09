CXXFLAGS=-c -Wall -std=c++17
LDFLAGS=-ludev

ifeq ($(DEBUG),1)
CXXFLAGS+=-g
endif

OBJ:=main.o
EXE=demo

COMPILE.1=$(CXX) $(CXXFLAGS) -o $@ $<
ifeq ($(VERBOSE),)
COMPILE=@printf "  > compiling %s\n" $(<F) && $(COMPILE.1)
else
COMPILE=$(COMPILE.1)
endif

%.o: %.cpp
	$(COMPILE)

.PHONY: all clean rebuild

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)

clean:
	$(RM) $(EXE) $(OBJ)

rebuild: clean all
