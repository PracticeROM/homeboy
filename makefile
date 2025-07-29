ifneq ($(strip $(DEVKITPPC)),)
PREFIX      = $(DEVKITPPC)/bin/powerpc-eabi-
else
PREFIX      = powerpc-eabi-
endif

CC          = $(PREFIX)gcc
AS          = $(PREFIX)gcc -x assembler-with-cpp
LD          = $(CC)
OBJCOPY     = $(PREFIX)objcopy
CFILES      = *.c
SFILES      = *.s
SRCDIR      = src
OBJDIR      = obj
BINDIR      = bin
VC_VERSIONS = D43J D43E PZLJ PZLE NACJ NACE NARJ NARE
NAME        = homeboy
RESDESC     = res.json

ADDRESS             = 0x817F8000
ALL_CFLAGS          = -c -std=gnu11 -Iinclude -mcpu=750 -meabi -mhard-float -G 0 -O3 -ffunction-sections -fdata-sections $(CFLAGS)
ALL_CPPFLAGS        = $(CPPFLAGS)
ALL_LDFLAGS         = -T build.ld -G 0 -nostartfiles -specs=nosys.specs -Wl,--gc-sections,--section-start,.init=$(ADDRESS) $(LDFLAGS)
ALL_OBJCOPYFLAGS    = -S -O binary --set-section-flags .bss=alloc,load,contents $(OBJCOPYFLAGS)

HB-D43J     = $(COBJ-hb-D43J) $(ELF-hb-D43J)
HB-D43E     = $(COBJ-hb-D43E) $(ELF-hb-D43E)
HB-PZLJ     = $(COBJ-hb-PZLJ) $(ELF-hb-PZLJ)
HB-PZLE     = $(COBJ-hb-PZLE) $(ELF-hb-PZLE)
HB-NACJ     = $(COBJ-hb-NACJ) $(ELF-hb-NACJ)
HB-NACE     = $(COBJ-hb-NACE) $(ELF-hb-NACE)
HB-NARJ     = $(COBJ-hb-NARJ) $(ELF-hb-NARJ)
HB-NARE     = $(COBJ-hb-NARE) $(ELF-hb-NARE)

HOMEBOY     = $(foreach v,$(VC_VERSIONS),hb-$(v))

all         : $(HOMEBOY)

clean       :
	rm -rf $(OBJDIR) $(BINDIR)

format      :
	find include src -name '*.h' -o -name '*.c' | xargs clang-format -i

.PHONY      : all clean format

define bin_template
SRCDIR-$(1)      = src
OBJDIR-$(1)      = obj/$(1)
BINDIR-$(1)      = bin/$(1)
SYMS-$(1)        = lib/$(1).txt
CSRC-$(1)       := $$(foreach s,$$(CFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
SSRC-$(1)       := $$(foreach s,$$(SFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
COBJ-$(1)        = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CSRC-$(1)))
SOBJ-$(1)        = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(SSRC-$(1)))
ELF-$(1)         = $$(BINDIR-$(1))/$(2).elf
BIN-$(1)         = $$(BINDIR-$(1))/$(2).bin
OUTDIR-$(1)      = $$(OBJDIR-$(1)) $$(BINDIR-$(1))
BUILD-$(1)       = $(1)
CLEAN-$(1)       = clean-$(1)

$$(ELF-$(1))      : LDFLAGS += -Wl,--defsym,init=$$(ADDRESS)
$$(BUILD-$(1))    : $$(BIN-$(1))
$$(CLEAN-$(1))    :
	rm -rf $$(OUTDIR-$(1))

$$(COBJ-$(1))     : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$(CC) $$(ALL_CPPFLAGS) $$(ALL_CFLAGS) $$< -o $$@
$$(SOBJ-$(1))     : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$(AS) -c -mregnames $$(ALL_CPPFLAGS) $$< -o $$@
$$(RESOBJ-$(1))   : $$(OBJDIR-$(1))/%.o: $$(RESDIR-$(1))/% | $$(OBJDIR-$(1))
	$(GRC) $$< -d $(RESDESC) -o $$@
$$(ELF-$(1))      : $$(COBJ-$(1)) $$(SOBJ-$(1)) $$(SYMS-$(1)) | $$(BINDIR-$(1))
	$(LD) $$(ALL_LDFLAGS) -T $$(SYMS-$(1)) -Wl,-Map=$${@:.elf=.map} $$(COBJ-$(1)) $$(SOBJ-$(1)) -o $$@
$$(BIN-$(1))      : $$(ELF-$(1)) | $$(BINDIR-$(1))
	$(OBJCOPY) $$(ALL_OBJCOPYFLAGS) $$< $$@
$$(OUTDIR-$(1))   :
	mkdir -p $$@
endef

$(foreach v,$(VC_VERSIONS),$(eval $(call bin_template,hb-$(v),homeboy)))

$(HB-D43J)      : ALL_CPPFLAGS += -DVC_VERSION=D43J
$(HB-D43E)      : ALL_CPPFLAGS += -DVC_VERSION=D43E
$(HB-PZLJ)      : ALL_CPPFLAGS += -DVC_VERSION=PZLJ
$(HB-PZLE)      : ALL_CPPFLAGS += -DVC_VERSION=PZLE
$(HB-NACJ)      : ALL_CPPFLAGS += -DVC_VERSION=NACJ
$(HB-NACE)      : ALL_CPPFLAGS += -DVC_VERSION=NACE
$(HB-NARJ)      : ALL_CPPFLAGS += -DVC_VERSION=NARJ
$(HB-NARE)      : ALL_CPPFLAGS += -DVC_VERSION=NARE
