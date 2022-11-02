#
# OMNeT++/OMNEST Makefile for $(LIB_PREFIX)rdp
#
# This file was generated with the command:
#  opp_makemake --make-so -f --deep -o rdp -KECMP_PROJ=../../ecmp -KINET4_4_PROJ=../../inet4.4 -DINET_IMPORT -I$$\(INET4_4_PROJ\)/src -L$$\(ECMP_PROJ\)/src -L$$\(INET4_4_PROJ\)/src -lecmp$$\(D\) -lINET$$\(D\)
#

# Name of target to be created (-o option)
TARGET_DIR = .
TARGET_NAME = $(LIB_PREFIX)rdp$(D)
TARGET = $(TARGET_NAME)$(SHARED_LIB_SUFFIX)
TARGET_IMPLIB = $(TARGET_NAME)$(IMPLIB_SUFFIX)
TARGET_IMPDEF = $(TARGET_NAME)$(IMPDEF_SUFFIX)
TARGET_FILES = $(TARGET_DIR)/$(TARGET)

# C++ include paths (with -I)
INCLUDE_PATH = -I$(INET4_4_PROJ)/src

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)$(ECMP_PROJ)/src $(LDFLAG_LIBPATH)$(INET4_4_PROJ)/src  -lecmp$(D) -lINET$(D)

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/application/rdpapp/RdpAppBase.o \
    $O/application/rdpapp/RdpBasicClientApp.o \
    $O/application/rdpapp/RdpSinkApp.o \
    $O/common/ResultFiltersThroughput.o \
    $O/common/TopologyEcmp.o \
    $O/common/queue/RdpMarkingSwitchQueue.o \
    $O/common/queue/RdpSwitchQueue.o \
    $O/common/scheduler/CentralSchedulerRdp.o \
    $O/transportlayer/common/L4ToolsRdp.o \
    $O/transportlayer/contract/rdp/RdpSocket.o \
    $O/transportlayer/rdp/Rdp.o \
    $O/transportlayer/rdp/RdpConnectionBase.o \
    $O/transportlayer/rdp/RdpConnectionEventProc.o \
    $O/transportlayer/rdp/RdpConnectionRcvSegment.o \
    $O/transportlayer/rdp/RdpConnectionUtil.o \
    $O/transportlayer/rdp/RdpReceiveQueue.o \
    $O/transportlayer/rdp/RdpSendQueue.o \
    $O/transportlayer/rdp/RdpSendQueueOptimisation.o \
    $O/transportlayer/rdp/flavours/DumbRdp.o \
    $O/transportlayer/rdp/flavours/HrdpMark.o \
    $O/transportlayer/rdp/flavours/RdpAIAD.o \
    $O/transportlayer/rdp/flavours/RdpAIMD.o \
    $O/transportlayer/rdp/flavours/RdpAIMD2.o \
    $O/transportlayer/rdp/flavours/RdpMark.o \
    $O/transportlayer/rdp/flavours/RdpMarking.o \
    $O/transportlayer/rdp/rdp_common/RdpHeader.o \
    $O/application/rdpapp/GenericAppMsgRdp_m.o \
    $O/transportlayer/contract/rdp/RdpCommand_m.o \
    $O/transportlayer/rdp/RdpConnection_m.o \
    $O/transportlayer/rdp/RdpConnectionState_m.o \
    $O/transportlayer/rdp/rdp_common/RdpHeader_m.o

# Message files
MSGFILES = \
    application/rdpapp/GenericAppMsgRdp.msg \
    transportlayer/contract/rdp/RdpCommand.msg \
    transportlayer/rdp/RdpConnection.msg \
    transportlayer/rdp/RdpConnectionState.msg \
    transportlayer/rdp/rdp_common/RdpHeader.msg

# SM files
SMFILES =

# Other makefile variables (-K)
ECMP_PROJ=../../ecmp
INET4_4_PROJ=../../inet4.4

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
CONFIGFILE = $(shell opp_configfilepath)
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = -loppenvir$D $(KERNEL_LIBS) $(SYS_LIBS)
ifneq ($(PLATFORM),win32)
LIBS += -Wl,-rpath,$(abspath $(ECMP_PROJ)/src) -Wl,-rpath,$(abspath $(INET4_4_PROJ)/src)
endif

COPTS = $(CFLAGS) $(IMPORT_DEFINES) -DINET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE (if COPTS has changed since last build)
# and make the object files depend on it
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
  $(shell $(MKPATH) "$O")
  $(file >$(COPTS_FILE),$(COPTS))
endif

# On Windows, the target has additional file(s). An import lib and an optional debug symbol file is created too.
ifeq ($(PLATFORM),win32)
  TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPLIB)
  LDFLAGS+= $(LDFLAG_IMPLIB)$O/$(TARGET_IMPLIB)
  ifeq ($(TOOLCHAIN_NAME),clang-msabi)
    ifeq ($(MODE),debug)
      TARGET_FILES+=$(TARGET_DIR)/$(TARGET_NAME).pdb
    endif
  else
    TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPDEF)
    LDFLAGS+= $(LDFLAG_IMPDEF)$O/$(TARGET_IMPDEF)
  endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
-include makefrag

#------------------------------------------------------------------------------

# Main target
all: $(TARGET_FILES)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clang-msabi)
	-$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib) 2>/dev/null

$O/$(TARGET_NAME).pdb: $O/$(TARGET)
endif

$O/$(TARGET) $O/$(TARGET_IMPDEF) $O/$(TARGET_IMPLIB) &: $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating shared library: $@
	$(Q)$(SHLIB_LD) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS) $(LDFLAGS)
	$(Q)$(SHLIB_POSTPROCESS) $O/$(TARGET)
ifeq ($(PLATFORM),win32)
	$(Q)llvm-ar d $O/$(TARGET_IMPLIB) $(TARGET) # WORKAROUND: throw away the first file from the archive to make the LLD generated import lib valid
endif

.PHONY: all clean cleanall depend msgheaders smheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $<)_m.h.d $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_FILES)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(CLEANALL_COMMAND)
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

help:
	@echo "$$HELP_SYNOPSYS"
	@echo "$$HELP_TARGETS"
	@echo "$$HELP_VARIABLES"
	@echo "$$HELP_EXAMPLES"

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
