include Config
include ModuleConfig

	BUILD_CONFIG_FILES = Makefile Config ModuleConfig 

	HEADERS = $(wildcard *.h) $(wildcard *.hpp)

	SOURCES_CPP = $(wildcard *.cpp) $(foreach dir,$(MODULESUBS_INCL), $(wildcard $(dir)/*.cpp))
	SOURCES_CPP_SUBS = $(foreach dir, $(MODULESUBS), $(wildcard $(dir)/*.cpp))
	SOURCES_CPP_LIB = $(foreach dir, $(MODULELIBS), $(wildcard $(dir)/*.cpp))

	OBJECTS = $(SOURCES_C:.c=.o) $(SOURCES_CPP:.cpp=.o)
	OBJECTS_SUBS = $(SOURCES_CPP_SUBS:.cpp=.o)
	OBJECTS_LIB = $(SOURCES_CPP_LIB:.cpp=.o)
	
	DEBUG_LIBEFENCE =  ../../libefence.a # -lefence -L../ # ../libefence.a

	MODULELIBS_TARGET = $(foreach lib, $(MODULELIBS), $(lib).so)
	MODULELIBS_LIBPREFIX = libxem-
	# MODULELINKFLAGS += $(DEBUG_LIBEFENCE)

.SUFFIXES : .c .cpp .o .so

all: startup $(BUILD_CONFIG_FILES) $(HEADERS) subs link libs $(SOURCES_CPP_LIB) $(MODULESUBS_LIB_NAME)
	@echo All $(MOD) : Ok.
	@echo All libs obj = $(OBJECTS_LIB)
	@echo All libs $(MODULELIBS_TARGET)

startup:
	@echo Compiling Xemeiah : version $(VERSION)
#	@echo OBJECTS_SUB : $(OBJECTS_SUBS)
#	rm -f libxemeiah.so.*

libs: $(BUILD_CONFIG_FILES) $(OBJECTS_LIB) $(MODULELIBS_TARGET)
	 @echo Compiled libs

dep: cleanDepends makeDepends

cleanDepends:
	@rm -f .depends

.depends makeDepends:
	@echo Building Depends
	@$(foreach source, $(SOURCES_CPP) $(SOURCES_CPP_LIB) $(SOURCES_CPP_SUBS), echo $(source) ; $(CC) -M -MT $(source:.cpp=.o) $(CFLAGS) $(MODULECOMPILEFLAGS) $(MODULECOMPILES) $(source) >> .depends ; )
include .depends

doc:
	@echo Generating doc to $(DESTDIR)
	doxygen

needs:
	@echo Checking Needs : $(MODULENEEDS)
	@$(foreach dir, $(MODULENEEDS), pwd && cd $(dir) && $(MAKE) compile ; )

subs: $(BUILD_CONFIG_FILES) $(HEADERS) $(OBJECTS_SUBS) $(SOURCES_CPP_SUBS) $(MODULESUBS_LIB_NAME) 

$(MODULESUBS_LIB_NAME): $(BUILD_CONFIG_FILES) $(HEADERS) $(OBJECTS_SUBS) $(SOURCES_CPP_SUBS)
	@echo Compiled SUBS : $(MODULESUBS_LIB_NAME)
	$(CC) $(CFLAGS) $(MODULECOMPILEFLAGS) $(MODULELINKFLAGS_SUBS) -shared -o $(MODULESUBS_LIB_NAME) $(OBJECTS_SUBS)
# $(foreach src,$(wildcard $(OBJECTS_SUBS)),$(src:.cpp=.o))

$(MODULESUBS_SYMLINK_NAME): $(MODULESUBS_LIB_NAME)
	@echo Symlink $(MODULESUBS_LIB_NAME) $(MODULESUBS_SYMLINK_NAME)
	rm -f $(MODULESUBS_SYMLINK_NAME)
	ln -s $(MODULESUBS_LIB_NAME) $(MODULESUBS_SYMLINK_NAME)

link: compile needs Config ModuleConfig subs $(MODULESUBS_LIB_NAME) $(MODULESUBS_SYMLINK_NAME)
	echo Compiling to : $(MOD)
	$(CC) $(CFLAGS) $(MODULECOMPILEFLAGS) -o $(MOD) $(OBJECTS) $(MODULELINKFLAGS) $(MODULELINKFLAGS_MAIN) 

clean:
	@echo Cleaning...
	@rm -f $(OBJECTS) *~ *.a *.so *stackdump log err gmon.out
	@rm -f *.o $(SOURCES_CPP_SUBS:.cpp=.o) $(SOURCES_CPP_LIB:.cpp=.o) $(MODULELIBSPATH)/*.so
	@rm -f $(MOD)__UT $(MOD)
	@rm -f $(MOD).exe
	@rm -f lib*.so.*
	@rm -f .depends
	@rm -rf doc/*
	@rm -rf .cccc/

cleanall: clean
	@echo Cleaning NEEDS
	@$(foreach dir, $(MODULENEEDS), cd $(dir) && $(MAKE) clean ; )
	@echo Cleaning SUBS
	@$(foreach dir, $(MODULESUBS), cd $(dir) && $(MAKE) clean ; )
	@echo Cleaning DEPENDS
	@$(foreach dir, $(MODULEDEPENDS), cd $(dir) && $(MAKE) clean ; )

compile: $(OBJECTS) # $(HEADERS) # $(SOURCES) $(HEADERS)
	@echo Compiled $(MOD) Objects : $(OBJECTS)

.c.o: $< $(HEADERS) Config ModuleConfig
	$(CC) $(LANG) -c $(CFLAGS) $(MODULECOMPILEFLAGS) -D__PURE_C__ -o $@ $<

.cpp.o: $< $(BUILD_CONFIG_FILES)
	$(CC) $(LANG) -c $(CFLAGS) $(MODULECOMPILEFLAGS) $(MODULECOMPILES) -o $@ $<

$(MODULELIBS_TARGET): $(MODULESUBS_LIB_NAME) $(OBJECTS_LIB) $(HEADERS) subs
	@echo Compiling $*
	@echo Sources = $(wildcard $*/*.cpp)
	@echo Objs = $(foreach src,$(wildcard $*/*.cpp),$(src:.cpp=.o))
	mkdir -p $(MODULELIBSPATH)
	$(CC) $(CFLAGS) $(MODULECOMPILEFLAGS) $(MODULELINKFLAGS_LIB)  -shared -o $(MODULELIBSPATH)/$(MODULELIBS_LIBPREFIX)$@ `cat $*/LibConfig` $(foreach src,$(wildcard $*/*.cpp),$(src:.cpp=.o))


install: link
	mkdir -p $(DESTDIR)/bin
	cp -v $(MOD) $(DESTDIR)/bin

install_sub_lib: subs
	@echo Installing lib $(MODULESUBS_LIB_NAME)
	mkdir -p $(DESTDIR)/lib
	cp -v $(MODULESUBS_LIB_NAME) $(DESTDIR)/lib
	rm -f $(DESTDIR)/lib/$(MODULESUBS_SYMLINK_NAME)
	ln -s $(MODULESUBS_LIB_NAME) $(DESTDIR)/lib/$(MODULESUBS_SYMLINK_NAME) 

install_module_lib: 
	@echo Installing Module Lib $(MODULELIB_NAME)
	mkdir -p $(DESTDIR)/lib/$(MODULELIBSPATH)
	cp -v $(MODULELIBSPATH)/$(MODULELIBS_LIBPREFIX)$(MODULELIB_NAME).so $(DESTDIR)/lib/$(MODULELIBSPATH)

install_all_module_libs:
	$(foreach lib,$(MODULELIBS), $(MAKE) install_module_lib MODULELIB_NAME=$(lib) ; )

install_common:
	mkdir -p $(DESTDIR)/share/xemeiah/webserver
	cp -vR ../xem-standard $(DESTDIR)/share/xemeiah/webserver
	cp -vR ../xem-standard-static $(DESTDIR)/share/xemeiah/webserver
	find  $(DESTDIR)/share/xemeiah/webserver -name .svn | xargs rm -rf

install_all: install install_sub_lib install_all_module_libs install_common
	@echo Installed

.PHONY: install_all install install_sub_lib install_all_module_libs

