#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_PILES 12
#define MAX_TOKENS 6

typedef enum {
    LEVEL_NONE,
    LEVEL_EASY,
    LEVEL_MEDIUM,
    LEVEL_HARD
} DifficultyLevel;

typedef enum {
    GAME_PLAYING,
    GAME_WON
} GameStatus;

typedef struct {
    int colors[MAX_TOKENS];
    int count;
} Pile;

typedef struct {
    Pile piles[MAX_PILES];
    int numPiles;
    int maxTokens;
    int numColors;
    int selected;
    GameStatus status;
    DifficultyLevel currentLevel;
} GameState;

SDL_Color COLORS[6] = {
    {255, 0, 0, 255},     // Rouge
    {0, 255, 0, 255},     // Vert
    {0, 0, 255, 255},     // Bleu
    {255, 255, 0, 255},   // Jaune
    {255, 0, 255, 255},   // Magenta
    {0, 255, 255, 255},   // Cyan
};

SDL_Color SELECTED_COLOR = {255, 215, 0, 255}; // Doré pour la sélection
SDL_Color TEXT_COLOR = {255, 255, 255, 255};   // Blanc pour le texte

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        printf("Erreur de rendu du texte: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Erreur de création de texture: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

DifficultyLevel showLevelMenu(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Event event;
    DifficultyLevel selected = LEVEL_NONE;
    SDL_Color highlight = {255, 215, 0, 255}; // Couleur dorée pour la surbrillance
    
    int hoverOption = -1;

    while (selected == LEVEL_NONE) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            } else if (event.type == SDL_MOUSEMOTION) {
                int y = event.motion.y;
                if (y > 200 && y < 240) hoverOption = 0;
                else if (y > 260 && y < 300) hoverOption = 1;
                else if (y > 320 && y < 360) hoverOption = 2;
                else hoverOption = -1;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int y = event.button.y;
                if (y > 200 && y < 240) selected = LEVEL_EASY;
                else if (y > 260 && y < 300) selected = LEVEL_MEDIUM;
                else if (y > 320 && y < 360) selected = LEVEL_HARD;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
        SDL_RenderClear(renderer);

        renderText(renderer, font, "Select Difficulty", 300, 100, TEXT_COLOR);
        
        // Afficher des descriptions de niveau
        renderText(renderer, font, "Easy", 350, 200, hoverOption == 0 ? highlight : TEXT_COLOR);
        renderText(renderer, font, "(4 piles, 3 couleurs, max 3 jetons)", 200, 225, hoverOption == 0 ? highlight : TEXT_COLOR);
        
        renderText(renderer, font, "Medium", 350, 260, hoverOption == 1 ? highlight : TEXT_COLOR);
        renderText(renderer, font, "(6 piles, 4 couleurs, max 4 jetons)", 200, 285, hoverOption == 1 ? highlight : TEXT_COLOR);
        
        renderText(renderer, font, "Hard", 350, 320, hoverOption == 2 ? highlight : TEXT_COLOR);
        renderText(renderer, font, "(8 piles, 5 couleurs, max 5 jetons)", 200, 345, hoverOption == 2 ? highlight : TEXT_COLOR);

        SDL_RenderPresent(renderer);
    }

    return selected;
}

void initGame(GameState *game, DifficultyLevel level) {
    game->selected = -1;
    game->status = GAME_PLAYING;
    game->currentLevel = level;

    // Réinitialiser toutes les piles
    for (int i = 0; i < MAX_PILES; i++) {
        game->piles[i].count = 0;
        for (int j = 0; j < MAX_TOKENS; j++) {
            game->piles[i].colors[j] = 0;
        }
    }

    switch (level) {
        case LEVEL_EASY:
            game->numColors = 3;
            game->numPiles = 4;
            game->maxTokens = 3;
            break;
        case LEVEL_MEDIUM:
            game->numColors = 4;
            game->numPiles = 6;
            game->maxTokens = 4;
            break;
        case LEVEL_HARD:
            game->numColors = 5;
            game->numPiles = 8;
            game->maxTokens = 5;
            break;
        default:
            game->numColors = 3;
            game->numPiles = 4;
            game->maxTokens = 3;
    }

    int colorCounts[6] = {0};
    int total = game->numColors * game->maxTokens;
    srand(time(NULL));

    // S'assurer que chaque couleur est utilisée exactement maxTokens fois
    for (int i = 0; i < total;) {
        int color = rand() % game->numColors;
        if (colorCounts[color] < game->maxTokens) {
            int pileIndex;
            do {
                pileIndex = rand() % game->numPiles;
            } while (game->piles[pileIndex].count >= game->maxTokens);
            game->piles[pileIndex].colors[game->piles[pileIndex].count++] = color;
            colorCounts[color]++;
            i++;
        }
    }
}

bool checkWin(GameState *game) {
    // Une pile est triée si tous les jetons sont de la même couleur
    for (int i = 0; i < game->numPiles; i++) {
        if (game->piles[i].count > 0) {
            int firstColor = game->piles[i].colors[0];
            for (int j = 1; j < game->piles[i].count; j++) {
                if (game->piles[i].colors[j] != firstColor) {
                    return false;
                }
            }
            
            // Si la pile n'est pas complète (maxTokens jetons) avec la même couleur
            if (game->piles[i].count != game->maxTokens && game->piles[i].count != 0) {
                return false;
            }
        }
    }
    
    return true;
}

void renderGame(SDL_Renderer *renderer, GameState *game, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
    SDL_RenderClear(renderer);

    // Afficher le niveau actuel
    char levelText[50];
    sprintf(levelText, "Level: %s", 
            game->currentLevel == LEVEL_EASY ? "Easy" : 
            game->currentLevel == LEVEL_MEDIUM ? "Medium" : "Hard");
    renderText(renderer, font, levelText, 20, 20, TEXT_COLOR);

    // Instructions
    renderText(renderer, font, "Arrange tokens by color", WINDOW_WIDTH / 2 - 150, 50, TEXT_COLOR);

    int pileWidth = 60;
    int pileSpacing = 20;
    int startX = (WINDOW_WIDTH - (game->numPiles * (pileWidth + pileSpacing))) / 2;

    for (int i = 0; i < game->numPiles; i++) {
        SDL_Rect rect = {startX + i * (pileWidth + pileSpacing), 150, pileWidth, 300};
        
        // Dessiner le contour de la pile
        if (i == game->selected) {
            // Pile sélectionnée - contour doré
            SDL_SetRenderDrawColor(renderer, SELECTED_COLOR.r, SELECTED_COLOR.g, SELECTED_COLOR.b, SELECTED_COLOR.a);
            // Dessiner un contour plus épais
            for (int t = 0; t < 3; t++) {
                SDL_Rect outline = {rect.x - t, rect.y - t, rect.w + 2*t, rect.h + 2*t};
                SDL_RenderDrawRect(renderer, &outline);
            }
        } else {
            // Pile normale - contour gris
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }

        // Dessiner les jetons
        for (int j = 0; j < game->piles[i].count; j++) {
            SDL_Rect token = {rect.x + 5, rect.y + 300 - (j + 1) * 60, pileWidth - 10, 50};
            SDL_Color c = COLORS[game->piles[i].colors[j]];
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_RenderFillRect(renderer, &token);
            
            // Bordure du jeton
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderDrawRect(renderer, &token);
        }
    }
    
    // Afficher message de victoire si le jeu est gagné
    if (game->status == GAME_WON) {
        SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &overlay);
        
        renderText(renderer, font, "You Win!", WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 50, TEXT_COLOR);
        renderText(renderer, font, "Press SPACE to play again", WINDOW_WIDTH / 2 - 170, WINDOW_HEIGHT / 2, TEXT_COLOR);
        renderText(renderer, font, "Press ESC to change level", WINDOW_WIDTH / 2 - 170, WINDOW_HEIGHT / 2 + 50, TEXT_COLOR);
    }

    SDL_RenderPresent(renderer);
}

void handleClick(GameState *game, int x, int y) {
    if (game->status == GAME_WON) return;
    
    int pileWidth = 60;
    int pileSpacing = 20;
    int startX = (WINDOW_WIDTH - (game->numPiles * (pileWidth + pileSpacing))) / 2;

    for (int i = 0; i < game->numPiles; i++) {
        SDL_Rect rect = {startX + i * (pileWidth + pileSpacing), 150, pileWidth, 300};
        if (x >= rect.x && x <= rect.x + pileWidth && y >= rect.y && y <= rect.y + 300) {
            if (game->selected == -1) {
                if (game->piles[i].count > 0)
                    game->selected = i;
            } else {
                if (i != game->selected) {
                    if (game->piles[i].count < game->maxTokens) {
                        // Transférer le jeton du haut
                        int color = game->piles[game->selected].colors[--game->piles[game->selected].count];
                        game->piles[i].colors[game->piles[i].count++] = color;
                        
                        // Vérifier si le joueur a gagné
                        if (checkWin(game)) {
                            game->status = GAME_WON;
                        }
                    }
                }
                game->selected = -1;
            }
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc; // Pour éviter l'avertissement de paramètre non utilisé
    (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erreur d'initialisation de SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() != 0) {
        printf("Erreur d'initialisation de TTF: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Nuts Puzzle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        printf("Erreur de création de fenêtre: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erreur de création de renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!font) {
        printf("Font load error: %s\n", TTF_GetError());
        // Essayer un autre chemin courant pour les polices
        font = TTF_OpenFont("DejaVuSans.ttf", 24);
        if (!font) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return 1;
        }
    }

    DifficultyLevel level = showLevelMenu(renderer, font);
    GameState game = {0};
    initGame(&game, level);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (game.status == GAME_WON) {
                        // Retourner au menu de sélection de niveau
                        level = showLevelMenu(renderer, font);
                        initGame(&game, level);
                    }
                } else if (event.key.keysym.sym == SDLK_SPACE) {
                    if (game.status == GAME_WON) {
                        // Redémarrer le même niveau
                        initGame(&game, game.currentLevel);
                    }
                } else if (event.key.keysym.sym == SDLK_r) {
                    // Raccourci pour redémarrer le niveau actuel
                    initGame(&game, game.currentLevel);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                handleClick(&game, event.button.x, event.button.y);
            }
        }
        renderGame(renderer, &game, font);
        SDL_Delay(16); // Environ 60 FPS
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
