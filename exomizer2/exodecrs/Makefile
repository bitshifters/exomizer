
WFLAGS = -Wall -Wstrict-prototypes
CFLAGS = $(WFLAGS) -O3 -ffast-math -mtune=i686 -fomit-frame-pointer -fgcse -pedantic
LDFLAGS = -s

#CFLAGS = -g -mtune=i686 $(WFLAGS)
#LDFLAGS = -g -mtune=i686

TEST_OBJS = testrun.o ../src/6502emu.o ../src/exo_util.o ../src/log.o

SOURCES = main.os exodecrunch.os data.os
SOURCESF = mainf.os krilldecr.os dataf.os
SOURCES1 = main1.os exostreamdecr1.os streamdata.os
SOURCES2 = main2.os exostreamdecr2.os streamdata.os
NAME = test.prg
NAMEF = testf.prg
NAME1 = test1.prg
NAME2 = test2.prg


build: $(NAME) $(NAMEF) $(NAME1) $(NAME2)

testrun: $(TEST_OBJS)
	@echo "Linking $@"
	@$(CC) $(LDFLAGS) -o $@ $(TEST_OBJS)

$(NAME): $(MAKEFILE) testrun $(SOURCES)
	ld65 -t c64 $(SOURCES) -o $@
	./testrun $@ data.bin,0x3000

$(NAMEF): $(MAKEFILE) testrun $(SOURCESF)
	ld65 -t c64 $(SOURCESF) -o $@
	./testrun $@ data.bin,0x3000

$(NAME1): $(MAKEFILE) testrun $(SOURCES1)
	ld65 -t c64 $(SOURCES1) -o $@
	./testrun $@ data.bin,0xEA00

$(NAME2): $(MAKEFILE) testrun $(SOURCES2)
	ld65 -t c64 $(SOURCES2) -o $@
	./testrun $@ data.bin,0xEA00

%.os: %.s
	ca65 $< -o $@

clean:
	$(RM) $(TEST_OBJS) $(SOURCES) $(SOURCESF) $(SOURCES1) $(SOURCES2) \
	$(NAME) $(NAMEF) $(NAME1) $(NAME2) testrun data.exo dataf.exo \
	streamdata.exo

data.os: data.exo
dataf.os: dataf.exo
streamdata.os: streamdata.exo

data.exo: data.bin
	../src/exomizer mem -q data.bin,0x3000 -o data.exo

dataf.exo: data.bin
	../src/exomizer mem -q -f data.bin,0x3000 -o dataf.exo

streamdata.exo: data.bin
	../src/exomizer raw -q -b -c -m 1024 data.bin -o streamdata.exo

%.o:	%.c
	@echo "Compiling $<"
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

%:	%.o
	@$(CC) $(LDFLAGS) $< -o $@
