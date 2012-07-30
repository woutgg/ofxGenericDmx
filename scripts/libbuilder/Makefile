# TODO:
# - see why the $(FTDI1_DIR) rule is not satisfied by existance of the dir created by clone_git_repo


comma := ,

OS_NAME := $(shell uname -s)
ifeq ($(OS_NAME), Darwin)
OS_NAME := osx
ARCH_SPECS := -arch ppc -arch i386 -arch x86_64
endif
ifeq ($(OS_NAME), Linux)
OS_NAME := linux
ARCH_SPECS :=
endif

PWD := $(CURDIR)
BASE_DIR := $(PWD)/libbuild-dir
BUILD_DIR := $(BASE_DIR)/build
RESULT_DIR := $(BASE_DIR)/result
TOKEN_BUILT_SUFFIX := -built

USBX_VERSION := 1.0.12
USBX_BASE_NAME := libusbx-$(USBX_VERSION)
USBX_ARCH_NAME := $(USBX_BASE_NAME).tar.bz2
USBX_BUILT_TOKEN := $(BASE_DIR)/.$(USBX_BASE_NAME)$(TOKEN_BUILT_SUFFIX)
USBX_DL_URL := http://downloads.sourceforge.net/project/libusbx/releases/$(USBX_VERSION)/source/$(USBX_ARCH_NAME)

FTDI1_BASE_NAME := libftdi1
FTDI1_DIR := $(FTDI1_BASE_NAME).git
FTDI1_NAME := $(FTDI1_DIR)
FTDI1_TOKEN := $(BASE_DIR)/.$(FTDI1_DIR)-cloned
FTDI1_BUILT_TOKEN := $(BASE_DIR)/.$(FTDI1_DIR)$(TOKEN_BUILT_SUFFIX)
FTDI1_REPO := git://developer.intra2net.com/libftdi
FTDI1_SHA1 := 33e27edfe5bc48502215d2fc4a39916df97717c6
FTDI1_BUILD_PATCH := libftdi1-build.patch
FTDI1_CMAKE_ARGS := -DCMAKE_OSX_ARCHITECTURES="ppc;ppc64;i386;x86_64" -DLIBUSB_INCLUDE_DIR:PATH=$(BUILD_DIR)/include/libusb-1.0 -DLIBUSB_LIBRARIES:FILEPATH=$(BUILD_DIR)/lib/libusb-1.0.a -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR) -DDOCUMENTATION=OFF -DEXAMPLES=OFF -DFTDIPP=OFF -DFTDI_EEPROM=OFF -DCMAKE_COLOR_MAKEFILE=OFF -DPYTHON_BINDINGS=OFF

EXTRA_CFLAGS :=
EXTRA_LDFLAGS :=
VPATH = $(BASE_DIR)



#############
# FUNCTIONS #
#############

# $(message TEXT, IS_CONTINUATION)
define message
	if [ "$(2)" == "" ]; then \
		echo '*** ' $(1); \
	else \
		echo '    ' $(1); \
	fi
endef

# $(call mkdir_silent, DIRNAME)
define mkdir_silent
	@if [ ! -d "$(1)" ]; then \
		mkdir -p $(1); \
	fi
endef

# $(call download_if_necessary BASE_DIR, FILENAME, URL)
define download_if_necessary
	@if [ ! -f "$(1)/$(2)" ]; then \
		$(call message,"Downloading $(2) from $(3)"); \
		wget "$(3)" -O "$(1)/$(2)" >/dev/null 2>&1; \
	fi
endef

# $(call clone_git_repo, DIRNAME, REPO_URL, SHA1)
define clone_git_repo
	#@$(call mkdir_silent,$(1))
	@if [ ! -d "$(1)" ]; then \
		$(call message,"Cloning $(2) into $(1)"); \
		git clone $(2) $(1) >/dev/null 2>&1; \
		if [ "x$(3)" != "x" ]; then \
			$(call message,"and checking out commit $(3)", true); \
			cd $(1) && git checkout $(3) >/dev/null 2>&1; \
		fi; \
	fi
endef



#########
# RULES #
#########
help:
	@$(call message,"This Makefile is built to automatically fetch$(comma) build and extract target files")
	@$(call message,"for the libraries needed by the openFrameworks ofxGenericDmx addon.", true)
	@$(call message,"Available targets:")
	@$(call message,"help\tShow this message", true)
	@$(call message,"all\tBuild all required libraries", true)
	@$(call message,"clean\tRemove all generated files and directories", true)
	@$(call message,"copy-files\tCopy the relevant built files to the result directory", true)
	@$(call message,"install\tCopy results to the addon directory (TODO)", true)
	@$(call message,"", true)

all: $(USBX_BUILT_TOKEN) $(FTDI1_BUILT_TOKEN)


$(USBX_BUILT_TOKEN): $(BASE_DIR) $(USBX_ARCH_NAME)
	@$(call message,"Building $(USBX_BASE_NAME) for $(OS_NAME)")
	cd $(BASE_DIR) && tar xjf $(USBX_ARCH_NAME)
	cd $(BASE_DIR)/$(USBX_BASE_NAME) && \
	CFLAGS="$(CFLAGS) $(EXTRA_CFLAGS) $(ARCH_SPECS)" LDFLAGS="$(LDFLAGS) $(EXTRA_LDFLAGS)" ./configure --prefix=$(BUILD_DIR) --disable-shared --disable-dependency-tracking && \
	make all install
	cd $(BASE_DIR) && rm -Rf $(BASE_DIR)/$(USBX_BASE_NAME)
	touch $(USBX_BUILT_TOKEN)

$(USBX_ARCH_NAME):
	$(call download_if_necessary,$(BASE_DIR),$(USBX_ARCH_NAME),$(USBX_DL_URL))


$(FTDI1_BUILT_TOKEN): $(BASE_DIR) $(USBX_BUILT_TOKEN) $(FTDI1_TOKEN)
	@$(call message,"Building $(FTDI1_NAME) for $(OS_NAME)")
	cd $(BASE_DIR)/$(FTDI1_DIR) && \
	cmake $(FTDI1_CMAKE_ARGS) -G "Unix Makefiles" && \
	make ftdi1-static install
	touch $(FTDI1_BUILT_TOKEN)

$(FTDI1_TOKEN):
	$(call clone_git_repo,$(BASE_DIR)/$(FTDI1_DIR),$(FTDI1_REPO),$(FTDI1_SHA1))
	cd $(BASE_DIR)/$(FTDI1_DIR); patch -Np1 < $(PWD)/$(FTDI1_BUILD_PATCH)
	touch $(FTDI1_TOKEN)


install: copy-files
	@$(call message,"The libraries have been built and copied into a newly created directory structure which is compatible with the ofxGenericDmx addon.")
	@$(call message,"Please check if all files are present (libraries + headers for libusbx, libusb-compat and libftdi) and copy them to the addon.", true)
	@$(call message,"Be careful not to accidentally overwrite files installed for other operating systems.", true)

copy-files: $(USBX_BUILT_TOKEN) $(FTDI1_BUILT_TOKEN)
	@$(call message,"Creating target directory structure")
	@$(call mkdir_silent,$(RESULT_DIR)/libftdi1/include)
	@$(call mkdir_silent,$(RESULT_DIR)/libftdi1/lib/$(OS_NAME))
	@$(call mkdir_silent,$(RESULT_DIR)/libusbx/include)
	@$(call mkdir_silent,$(RESULT_DIR)/libusbx/lib/$(OS_NAME))
	@$(call message,"Copying $(FTDI1_BASE_NAME) files")
	@cp $(BUILD_DIR)/include/libftdi1/ftdi.h $(RESULT_DIR)/libftdi1/include
	@cp $(BUILD_DIR)/lib/libftdi1.a $(RESULT_DIR)/libftdi1/lib/$(OS_NAME)
	@$(call message,"Copying $(USBX_BASE_NAME) files")
	@cp $(BUILD_DIR)/include/libusb-1.0/libusb.h $(RESULT_DIR)/libusbx/include
	@cp $(BUILD_DIR)/lib/libusb-1.0.a $(RESULT_DIR)/libusbx/lib/$(OS_NAME)




##################
# HELPER TARGETS #
##################

.PHONY: libusbx libftdi1

$(BASE_DIR):
	@$(call mkdir_silent,$(BASE_DIR))

clean: $(BASE_DIR)
	@$(call message,"Removing downloaded/cloned files and directories")
	@rm -Rf $(BASE_DIR)/$(USBX_BASE_NAME)
	@rm -f $(BASE_DIR)/$(USBX_ARCH_NAME)
	@rm -Rf $(BASE_DIR)/$(FTDI1_DIR)
	@#
	@$(call message,"Removing token files")
	@rm -f $(USBX_BUILT_TOKEN)
	@rm -f $(FTDI1_TOKEN)
	@rm -f $(FTDI1_BUILT_TOKEN)
	@#
	rm -Rf $(BUILD_DIR)
	rm -Rf $(RESULT_DIR)
	rmdir $(BASE_DIR)

