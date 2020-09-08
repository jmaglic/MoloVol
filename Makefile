
CC := g++
SRCDIR := src
BUILDDIR := build
BINDIR := bin
TARGET := bin/ballpit

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CXXFLAGS := -O0 -g -std=c++17 -Wall -Werror 
CFLAGS := -O0 -g -std=c++17 -Wno-unused-command-line-argument
INC := -I include

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	mkdir -p $(BINDIR)
	$(CC) $(CXXFLAGS) $^ `wx-config --cxxflags --libs` -framework OpenCL -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INC) -c `wx-config --cxxflags --libs` -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean
