BINARY = swr
WFLAGS = -Werror -Wall -Wformat-security -Wpointer-arith -Wcast-align -Wwrite-strings -Wshadow -Wextra -Wno-unused-parameter -Wredundant-decls

#Comment out the next line if your system doesn't have dlsym functionality.
USE_DLSYM = -DSWR2_USE_DLSYM

#IMC2 - Comment out to dsable IMC2 support
IMC = 1

#Change from 0 to 1 to enable telnet debug logging.
DEBUG_TELNET = -DSWR2_DEBUG_TELNET=0

CCFLAGS = -g3 -MMD $(WFLAGS) -Iswr_support $(USE_DLSYM) $(DEBUG_TELNET)
LDFLAGS = -lm -Lswr_support -lswr_support -Wl,-rpath,./src/swr_support -lz

#On some systems (e.g. FreeBSD) you may need to remove -ldl
ifdef USE_DLSYM
LDFLAGS += -ldl -rdynamic
endif

ODIR = obj
_OBJ = act_comm.o act_info.o act_move.o act_obj.o act_wiz.o boards.o \
	build.o clans.o colony.o comm.o const.o copyover.o db.o fight.o \
	handler.o interp.o magic.o makeobjs.o \
	misc.o morespace.o mud_comm.o mud_prog.o nanny.o planets.o \
	player.o reset.o save.o shops.o skills.o skillpackage.o space.o \
	special.o swskills.o \
	tables.o telopt.o track.o update.o vector3_aux.o vote.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

ifdef IMC
OBJ += $(ODIR)/imc.o
CCFLAGS += -DSWR2_USE_IMC
endif

all:		 libs $(BINARY)

libs:
		$(MAKE) -C swr_support

$(BINARY):	$(OBJ)
		@echo "Linking" $(BINARY)
		@$(CC) -o $@ $^ $(LDFLAGS)
		@mv $(BINARY) ../
		@echo "Done."

$(ODIR)/%.o:	 %.c
		@echo "Compiling" $<
		@$(CC) -c -o $@ $< $(CCFLAGS)

-include $(ODIR)/*.d

clean :
		-rm -f $(BINARY) *~ $(ODIR)/*.o ../core* $(ODIR)/*.d

mos:
		$(MAKE) -f Makefile.MOS

