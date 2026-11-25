.PHONY: all run clean
all:
	$(MAKE) -C jogo-grafico all
run:
	$(MAKE) -C jogo-grafico run
clean:
	$(MAKE) -C jogo-grafico clean
