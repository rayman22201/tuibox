all: tuibox

CC=cc

LIBS=-lm
CFLAGS=-O3 -pipe
DEBUGCFLAGS=-Og -pipe -g

.PHONY: tuibox clean debug help
tuibox:
	$(CC) demos/demo_basic.c -o demos/demo_basic $(LIBS) $(CFLAGS)
	$(CC) demos/demo_bounce.c -o demos/demo_bounce $(LIBS) $(CFLAGS)
	$(CC) demos/demo_drag.c -o demos/demo_drag $(LIBS) $(CFLAGS)

# Debug build
debug:
	$(CC) demos/demo_basic.c -o demos/demo_basic $(LIBS) $(DEBUGCFLAGS)
	$(CC) demos/demo_bounce.c -o demos/demo_bounce $(LIBS) $(DEBUGCFLAGS)
	$(CC) demos/demo_drag.c -o demos/demo_drag $(LIBS) $(DEBUGCFLAGS)

# Clean build artifacts
clean:
	rm -f demos/demo_basic demos/demo_bounce demos/demo_drag
	rm -f demos/*.o *.o

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build all demo programs (default)"
	@echo "  debug   - Build with debug symbols"
	@echo "  clean   - Remove all build artifacts"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make [target]"
