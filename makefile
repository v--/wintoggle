.PHONY: all

wintoggle: wintoggle.c
	cc -std=c99 -O3 -Wall -Wextra -lX11 --output wintoggle wintoggle.c

man/wintoggle.1: man/wintoggle.1.ronn
	ronn --roff man/wintoggle.1.ronn

README.md: man/wintoggle.1.ronn badges.md
	cp badges.md README.md
	cat man/wintoggle.1.ronn | ronn --pipe --markdown >> README.md

all: wintoggle man/wintoggle.1 README.md
