# === Compilation ===
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lm

# === Fichiers source ===
SRC = main.c
OBJ = $(SRC:.c=.o)
EXEC = nuts

# === Dossiers des assets ===
FONT_SRC = /home/yassine/Downloads/Untitled\ Folder\ 2/assets/fonts
FONT_DST = assets/fonts

# === Règle principale ===
all: $(EXEC) copy-fonts

# === Compilation de l'exécutable ===
$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# === Compilation des fichiers objets ===
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

# === Copie des polices TTF (compatible dash) ===
copy-fonts:
	mkdir -p $(FONT_DST)
	for font in $(shell find $(FONT_SRC) -name '*.ttf'); do \
		dest="$(FONT_DST)/$$(basename "$$font")"; \
		if [ "$$font" != "$$dest" ]; then \
			cp -u "$$font" "$$dest"; \
		fi \
	done || echo "Warning: Could not copy fonts"

# === Nettoyage ===
clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean copy-fonts

