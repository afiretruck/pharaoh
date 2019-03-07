###########################################################################################################################
# tells the makefile to use bash as the shell
SHELL := /bin/bash

# Some default variables
RELEASE?=n

# Where are we putting the build stuff?
ifeq "$(RELEASE)" "n"
	BINDIR=bin/debug/x86-64-GCC-Linux/
	CXX_FLAGS= -std=c++14 -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-comment -Werror -DGLEW_STATIC -DDEBUG -D_REENTRANT -ggdb
	CC_FLAGS= -std=c99 -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-comment -Werror -DGLEW_STATIC -DDEBUG -D_REENTRANT -ggdb
	DEPFLAGS= -MT $@ -MMD -MP -MF $@.Td
	POSTCOMPILE= @mv -f $@.Td $@.d && touch $@
else
	BINDIR=bin/release/x86-64-GCC-Linux/
	CXX_FLAGS= -std=c++14 -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-comment -Werror -DGLEW_STATIC -D_REENTRANT -o3
	CC_FLAGS= -std=c99 -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-comment -Werror -DGLEW_STATIC -D_REENTRANT -o3
	DEPFLAGS= 
	POSTCOMPILE= 
endif

# output directory lists
OBJDIR=$(BINDIR)obj

# Are we using the system GCC or the locally compiled one?
# get the system GCC version (maybe we can use that instead of compiling our own)
GCCVERSIONMAJOR=$(shell gcc -dumpversion | cut -f1 -d.)
GCCVERSIONMINOR=$(shell gcc -dumpversion | cut -f2 -d.) 
GCC61PLUS=$(shell expr $(GCCVERSIONMAJOR) "==" 6 "&" $(GCCVERSIONMINOR) ">=" 1)
GCC70PLUS=$(shell expr $(GCCVERSIONMAJOR) ">=" 6)

ifeq "$(GCC61PLUS)" "1"
	CC=$(shell which gcc)
	CXX=$(shell which g++)
	AR=$(shell which gcc-ar)
else ifeq "$(GCC70PLUS)" "1"
	CC=$(shell which gcc)
	CXX=$(shell which g++)
	AR=$(shell which gcc-ar)
else
	CC=Lib/Linux/gcc/opt/bin/gcc
	CXX=Lib/Linux/gcc/opt/bin/g++
	AR=Lib/Linux/gcc/opt/bin/gcc-ar
endif

GCCVERSION=$(shell $(CC) -dumpversion)

MANAGERSRC=\
main.cpp \
WindowManager.cpp

COMPILE.cxx= @echo "  CXX    "$< && $(CXX) 
COMPILE.c= @echo "  CC     "$< && $(CC)
COMPILE.link= @echo "  LINK   "$@ && $(CXX)
COMPILE.linkstat= @echo "  AR     "$@ && $(AR) cr
COMPILE.template= @echo "  TMPLT  "$@ && cd $< && tar -czf $(CURDIR)/$@ *

# change the extension to .o & add obj/ prefix
MANAGER_OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o, $(basename $(MANAGERSRC))))

###########################################################################################################################
# targets

# top targets
pharaoh: $(BINDIR)pharaoh


# target for build directories
.PRECIOUS: $(BINDIR)%/
$(BINDIR)%/:
	@mkdir $@ -p
	@echo "  DIR    "$@
	
# cleanup target
clean:
	rm -rf bin
#	rm -f Tools/Version.h

# source files
.SECONDEXPANSION:
$(OBJDIR)/%.o: %.cpp $(OBJDIR)/%.o.d | $$(@D)/ 
	$(COMPILE.cxx) $(CXX_FLAGS) $(DEPFLAGS) $(DEFINES) -fpic -o $@ -c $<
	$(POSTCOMPILE)

.SECONDEXPANSION:
$(OBJDIR)/%.o: %.c $(OBJDIR)/%.o.d | $$(@D)/
	$(COMPILE.c) $(CC_FLAGS)) $(DEPFLAGS) $(DEFINES) -fpic -o $@ -c $<
	$(POSTCOMPILE)
	
# dependency dummy - stops header deps getting killed off
$(OBJDIR)/%.o.d: ;
.PRECIOUS: $(OBJDIR)/%.o.d
	
# pharaoh
$(BINDIR)pharaoh: $(MANAGER_OBJS)
	$(COMPILE.link) $(MANAGER_OBJS) -lX11 -static-libstdc++ -o $@ 
	

# header dependency includes
include $(wildcard $(patsubst %,%.d,$(MANAGER_OBJS)))
