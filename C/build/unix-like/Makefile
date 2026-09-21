##### DEFINE LIBRARY AND PROCEDURE PATHS ###################################### 
# variables like `DIR_<library_name>` are defined by the following Make file
REL_ROOT=../../../build/linux/
include $(REL_ROOT)MakeDirPaths

# by default, only build libraries
.PHONY: all
all: libs_open_source libs_internal sensitiv

.PHONY: test
test: sensitiv_test

.PHONY: sensitiv_test
sensitiv_test:
	@echo ""
	@echo "## Testing sensitiv"
	export GCONFID_DEBUG_STATS=1
	cd $(DIR_SENSITIV)   && $(MAKE) test
	
# build sensitiv
.PHONY: sensitiv
sensitiv: libs_open_source libs_internal
	@echo ""
	@echo "## Building sensitiv"
	cd $(DIR_SENSITIV)   && $(MAKE)
	
# build all libraries as static libraries
.PHONY: libs_internal
libs_internal: 
	@echo ""
	@echo "## Building GConfidAPI"
	cd $(DIR_GCONFIDAPI)   && $(MAKE) static
	
	@echo ""
	@echo "## Building CommonAPI"
	cd $(DIR_COMMONAPI)  && $(MAKE) static
	
	@echo ""
	@echo "## Building IOUtilities"
	cd $(DIR_IOUTIL)    && $(MAKE) static
	
.PHONY: libs_open_source
libs_open_source:
	@echo ""
	@echo "## Building Jansson"
	cd $(DIR_JANSSON)   && $(MAKE) static
	@echo ""
	@echo "## Building Arrow"
	cd $(DIR_ARROW)   && $(MAKE)
	
.PHONY: diagnose
diagnose:
	@echo ""
	@echo "## Diagnosing GConfidAPI"
	cd $(DIR_GCONFIDAPI)   && $(MAKE) diagnose
	@echo "DIR_GCONFIDAPI    $(DIR_GCONFIDAPI)"
	
	@echo "DIR_COMMONAPI   $(DIR_COMMONAPI)"
	
	@echo "DIR_IOUTIL    $(DIR_IOUTIL)"
	
	@echo "DIR_SENSITIV    $(DIR_SENSITIV)"
	
# clean all libraries and procedures/modules
.PHONY: clean
clean: clean_libs_open_source clean_libs_internal clean_procs

.PHONY: clean_procs
clean_procs:
	@echo ""
	@echo "## Cleaning sensitiv"
	cd $(DIR_SENSITIV) && $(MAKE) clean

.PHONY: clean_libs_internal
clean_libs_internal:
	@echo ""
	@echo "## Cleaning GConfidAPI"
	cd $(DIR_GCONFIDAPI)   && $(MAKE) clean
	
	@echo ""
	@echo "## Cleaning CommonAPI"
	cd $(DIR_COMMONAPI)  && $(MAKE) clean
	
	@echo ""
	@echo "## Cleaning IOUtilities"
	cd $(DIR_IOUTIL) && $(MAKE) clean
	
.PHONY: clean_libs_open_source
clean_libs_open_source:
	@echo ""
	@echo "## Cleaning Arrow"
	cd $(DIR_ARROW)   && $(MAKE) clean
	
	@echo ""
	@echo "## Cleaning Jansson"
	cd $(DIR_JANSSON)   && $(MAKE) clean

