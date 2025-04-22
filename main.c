#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 700
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
    int moveCount;
    Uint32 startTime;
} GameState;

// Palette de couleurs attrayante pour les jetons
SDL_Color COLORS[6] = {
    {231, 76, 60, 255},   // Rouge-corail
    {46, 204, 113, 255},  // Vert-émeraude
    {52, 152, 219, 255},  // Bleu-ciel
    {241, 196, 15, 255},  // Jaune-or
    {142, 68, 173, 255},  // Violet-améthyste
    {22, 160, 133, 255},  // Turquoise
};

// Couleurs UI
SDL_Color SELECTED_COLOR = {255, 215, 0, 255};      // Doré pour la sélection
SDL_Color TEXT_COLOR = {236, 240, 241, 255};        // Blanc cassé pour le texte
SDL_Color BACKGROUND_COLOR = {44, 62, 80, 255};     // Bleu foncé pour l'arrière-plan
SDL_Color BUTTON_COLOR = {52, 73, 94, 255};         // Bleu-gris pour les boutons
SDL_Color BUTTON_HOVER_COLOR = {41, 128, 185, 255}; // Bleu vif pour le survol
SDL_Color PILE_BORDER_COLOR = {189, 195, 199, 255}; // Gris clair pour les bordures de pile

typedef struct {
    SDL_Rect rect;
    char* text;
    bool hover;
    void (*action)(void*);
    void* data;
} Button;

// Structure pour les effets d'animation
typedef struct {
    bool active;
    int sourceX;
    int sourceY;
    int destX;
    int destY;
    int colorIndex;
    float progress;
    Uint32 startTime;
} TokenAnimation;

TokenAnimation currentAnimation = {false};

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
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

void renderTextCentered(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
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
    
    SDL_Rect dst = {x - surface->w / 2, y - surface->h / 2, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Dessiner un rectangle avec des coins arrondis
void drawRoundedRect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius) {
    // Les quatre coins
    for (int i = 0; i <= radius; ++i) {
        double angle = i * M_PI / (2 * radius);
        int xDelta = (int)(radius - radius * cos(angle));
        int yDelta = (int)(radius - radius * sin(angle));
        
        // Coin supérieur gauche
        SDL_RenderDrawLine(renderer, x + radius - xDelta, y, x + w - radius + xDelta, y);
        // Coin supérieur droit
        SDL_RenderDrawLine(renderer, x, y + radius - yDelta, x, y + h - radius + yDelta);
        // Coin inférieur gauche
        SDL_RenderDrawLine(renderer, x + radius - xDelta, y + h, x + w - radius + xDelta, y + h);
        // Coin inférieur droit
        SDL_RenderDrawLine(renderer, x + w, y + radius - yDelta, x + w, y + h - radius + yDelta);
    }
}

// Remplir un rectangle avec des coins arrondis
void fillRoundedRect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius) {
    SDL_Rect rect = {x + radius, y, w - 2 * radius, h};
    SDL_RenderFillRect(renderer, &rect);
    
    // Second rectangle needs to be initialized differently
    rect.x = x;
    rect.y = y + radius;
    rect.w = w;
    rect.h = h - 2 * radius;
    SDL_RenderFillRect(renderer, &rect);
    
    // Les quatre coins
    for (int i = 0; i <= radius; ++i) {
        double angle = i * M_PI / (2 * radius);
        int xDelta = (int)(radius * cos(angle));
        int yDelta = (int)(radius * sin(angle));
        
        // Coin supérieur gauche
        SDL_RenderDrawLine(renderer, x + radius - xDelta, y + radius - yDelta, 
                                    x + radius - xDelta, y + radius - yDelta);
        // Coin supérieur droit
        SDL_RenderDrawLine(renderer, x + w - radius + xDelta, y + radius - yDelta,
                                    x + w - radius + xDelta, y + radius - yDelta);
        // Coin inférieur gauche
        SDL_RenderDrawLine(renderer, x + radius - xDelta, y + h - radius + yDelta,
                                    x + radius - xDelta, y + h - radius + yDelta);
        // Coin inférieur droit
        SDL_RenderDrawLine(renderer, x + w - radius + xDelta, y + h - radius + yDelta,
                                    x + w - radius + xDelta, y + h - radius + yDelta);
        
        SDL_RenderDrawLine(renderer, x + radius - xDelta, y + radius - yDelta, 
                                    x + radius - xDelta, y + h - radius + yDelta);
        SDL_RenderDrawLine(renderer, x + w - radius + xDelta, y + radius - yDelta,
                                    x + w - radius + xDelta, y + h - radius + yDelta);
    }

    // Coins avec des cercles remplis
    for (int i = 0; i <= radius; ++i) {
        for (int j = 0; j <= radius; ++j) {
            if (i*i + j*j <= radius*radius) {
                // Coin supérieur gauche
                SDL_RenderDrawPoint(renderer, x + radius - i, y + radius - j);
                // Coin supérieur droit
                SDL_RenderDrawPoint(renderer, x + w - radius + i, y + radius - j);
                // Coin inférieur gauche
                SDL_RenderDrawPoint(renderer, x + radius - i, y + h - radius + j);
                // Coin inférieur droit
                SDL_RenderDrawPoint(renderer, x + w - radius + i, y + h - radius + j);
            }
        }
    }
}

void renderButton(SDL_Renderer *renderer, TTF_Font *font, Button *button) {
    SDL_Color color = button->hover ? BUTTON_HOVER_COLOR : BUTTON_COLOR;
    
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    fillRoundedRect(renderer, button->rect.x, button->rect.y, button->rect.w, button->rect.h, 10);
    
    // Bordure du bouton
    SDL_SetRenderDrawColor(renderer, 
                          (color.r + TEXT_COLOR.r) / 2, 
                          (color.g + TEXT_COLOR.g) / 2, 
                          (color.b + TEXT_COLOR.b) / 2, 
                          255);
    drawRoundedRect(renderer, button->rect.x, button->rect.y, button->rect.w, button->rect.h, 10);
    
    // Texte centré
    renderTextCentered(renderer, font, button->text, 
                     button->rect.x + button->rect.w / 2, 
                     button->rect.y + button->rect.h / 2, 
                     TEXT_COLOR);
}

bool isPointInRect(int x, int y, SDL_Rect *rect) {
    return (x >= rect->x && x <= rect->x + rect->w &&
            y >= rect->y && y <= rect->y + rect->h);
}

// Fonctions pour les boutons
void actionPlayEasy(void *data) {
    DifficultyLevel *level = (DifficultyLevel *)data;
    *level = LEVEL_EASY;
}

void actionPlayMedium(void *data) {
    DifficultyLevel *level = (DifficultyLevel *)data;
    *level = LEVEL_MEDIUM;
}

void actionPlayHard(void *data) {
    DifficultyLevel *level = (DifficultyLevel *)data;
    *level = LEVEL_HARD;
}

void actionRestart(void *data) {
    GameState *game = (GameState *)data;
    game->status = GAME_PLAYING;
    // Réinitialisation sera gérée dans la boucle principale
}

void actionBackToMenu(void *data) {
    GameState *game = (GameState *)data;
    game->status = GAME_PLAYING;
    game->currentLevel = LEVEL_NONE;
}

DifficultyLevel showLevelMenu(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *titleFont) {
    SDL_Event event;
    DifficultyLevel selected = LEVEL_NONE;
    Button buttons[3] = {
        {
            {WINDOW_WIDTH / 2 - 100, 250, 200, 60},
            "Easy",
            false,
            actionPlayEasy,
            &selected
        },
        {
            {WINDOW_WIDTH / 2 - 100, 330, 200, 60},
            "Medium",
            false,
            actionPlayMedium,
            &selected
        },
        {
            {WINDOW_WIDTH / 2 - 100, 410, 200, 60},
            "Hard",
            false,
            actionPlayHard,
            &selected
        }
    };

    // Créer un dégradé d'arrière-plan
    SDL_Surface *background = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0, 0, 0, 0);
    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        float factor = (float)y / WINDOW_HEIGHT;
        Uint32 color = SDL_MapRGB(background->format,
                                 BACKGROUND_COLOR.r * (1.0 - factor*0.2),
                                 BACKGROUND_COLOR.g * (1.0 - factor*0.2),
                                 BACKGROUND_COLOR.b * (1.0 - factor*0.2));
        
        SDL_Rect line = {0, y, WINDOW_WIDTH, 1};
        SDL_FillRect(background, &line, color);
    }
    SDL_Texture *bgTexture = SDL_CreateTextureFromSurface(renderer, background);
    SDL_FreeSurface(background);

    while (selected == LEVEL_NONE) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_DestroyTexture(bgTexture);
                exit(0);
            } else if (event.type == SDL_MOUSEMOTION) {
                int x = event.motion.x;
                int y = event.motion.y;
                
                for (int i = 0; i < 3; i++) {
                    buttons[i].hover = isPointInRect(x, y, &buttons[i].rect);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;
                
                for (int i = 0; i < 3; i++) {
                    if (isPointInRect(x, y, &buttons[i].rect)) {
                        buttons[i].action(buttons[i].data);
                    }
                }
            }
        }

        // Afficher l'arrière-plan
        SDL_RenderCopy(renderer, bgTexture, NULL, NULL);
        
        // Titre stylisé
        renderTextCentered(renderer, titleFont, "NUTS PUZZLE", WINDOW_WIDTH / 2, 100, TEXT_COLOR);
        renderTextCentered(renderer, font, "Select Difficulty", WINDOW_WIDTH / 2, 160, TEXT_COLOR);
        
        // Afficher les descriptions de niveau sous chaque bouton
        renderTextCentered(renderer, font, "(4 piles, 3 couleurs, max 3 jetons)", 
                          WINDOW_WIDTH / 2, buttons[0].rect.y + buttons[0].rect.h + 20, TEXT_COLOR);
        
        renderTextCentered(renderer, font, "(6 piles, 4 couleurs, max 4 jetons)", 
                          WINDOW_WIDTH / 2, buttons[1].rect.y + buttons[1].rect.h + 20, TEXT_COLOR);
        
        renderTextCentered(renderer, font, "(8 piles, 5 couleurs, max 5 jetons)", 
                          WINDOW_WIDTH / 2, buttons[2].rect.y + buttons[2].rect.h + 20, TEXT_COLOR);
        
        // Dessiner les boutons
        for (int i = 0; i < 3; i++) {
            renderButton(renderer, font, &buttons[i]);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyTexture(bgTexture);
    return selected;
}

void initGame(GameState *game, DifficultyLevel level) {
    game->selected = -1;
    game->status = GAME_PLAYING;
    game->currentLevel = level;
    game->moveCount = 0;
    game->startTime = SDL_GetTicks();
    currentAnimation.active = false;

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

// Dessiner un jeton avec une apparence 3D
void drawToken(SDL_Renderer *renderer, int x, int y, int width, int height, SDL_Color color) {
    // Jeton principal
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect mainToken = {x, y, width, height};
    SDL_RenderFillRect(renderer, &mainToken);
    
    // Highlight (haut et gauche)
    SDL_SetRenderDrawColor(renderer, 
                          fmin(color.r * 1.2, 255), 
                          fmin(color.g * 1.2, 255), 
                          fmin(color.b * 1.2, 255), 
                          color.a);
    
    SDL_Rect highlight = {x, y, width, 5};
    SDL_RenderFillRect(renderer, &highlight);
    
    highlight.h = height;
    highlight.w = 5;
    SDL_RenderFillRect(renderer, &highlight);
    
    // Ombre (bas et droite)
    SDL_SetRenderDrawColor(renderer, 
                          color.r * 0.7, 
                          color.g * 0.7, 
                          color.b * 0.7, 
                          color.a);
    
    SDL_Rect shadow = {x, y + height - 5, width, 5};
    SDL_RenderFillRect(renderer, &shadow);
    
    shadow.x = x + width - 5;
    shadow.y = y;
    shadow.w = 5;
    shadow.h = height;
    SDL_RenderFillRect(renderer, &shadow);
    
    // Bordure du jeton
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderDrawRect(renderer, &mainToken);
}

void startTokenAnimation(int srcX, int srcY, int destX, int destY, int colorIndex) {
    currentAnimation.active = true;
    currentAnimation.sourceX = srcX;
    currentAnimation.sourceY = srcY;
    currentAnimation.destX = destX;
    currentAnimation.destY = destY;
    currentAnimation.colorIndex = colorIndex;
    currentAnimation.progress = 0.0f;
    currentAnimation.startTime = SDL_GetTicks();
}

void updateAnimation() {
    if (!currentAnimation.active) return;
    
    Uint32 currentTime = SDL_GetTicks();
    float elapsed = (currentTime - currentAnimation.startTime) / 500.0f; // Animation dure 500ms
    
    currentAnimation.progress = elapsed;
    
    if (currentAnimation.progress >= 1.0f) {
        currentAnimation.active = false;
    }
}

void renderWinScreen(SDL_Renderer *renderer, TTF_Font *font, TTF_Font *largeFont, GameState *game) {
    // Superposition semi-transparente
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Temps écoulé
    Uint32 elapsedTime = game->status == GAME_WON ? 
                         SDL_GetTicks() - game->startTime : SDL_GetTicks();
    int minutes = (elapsedTime / 1000) / 60;
    int seconds = (elapsedTime / 1000) % 60;
    
    // Effet de scintillement pour le texte de victoire
    SDL_Color winColor = {
        241 + sin(SDL_GetTicks() * 0.003) * 15,
        196 + sin(SDL_GetTicks() * 0.005) * 15,
        15 + sin(SDL_GetTicks() * 0.004) * 15,
        255
    };
    
    // Afficher message de victoire
    renderTextCentered(renderer, largeFont, "YOU WIN!", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 80, winColor);
    
    char statsText[100];
    sprintf(statsText, "Moves: %d   Time: %02d:%02d", game->moveCount, minutes, seconds);
    renderTextCentered(renderer, font, statsText, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, TEXT_COLOR);
    
    // Boutons
    Button restartButton = {
        {WINDOW_WIDTH / 2 - 210, WINDOW_HEIGHT / 2 + 60, 180, 60},
        "Play Again",
        false,
        actionRestart,
        game
    };
    
    Button menuButton = {
        {WINDOW_WIDTH / 2 + 30, WINDOW_HEIGHT / 2 + 60, 180, 60},
        "Main Menu",
        false,
        actionBackToMenu,
        game
    };
    
    // Vérifier si la souris survole les boutons
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    restartButton.hover = isPointInRect(mouseX, mouseY, &restartButton.rect);
    menuButton.hover = isPointInRect(mouseX, mouseY, &menuButton.rect);
    
    renderButton(renderer, font, &restartButton);
    renderButton(renderer, font, &menuButton);
}

void renderGame(SDL_Renderer *renderer, GameState *game, TTF_Font *font, TTF_Font *largeFont) {
    // Arrière-plan avec dégradé
    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        float factor = (float)y / WINDOW_HEIGHT;
        SDL_SetRenderDrawColor(renderer, 
                              BACKGROUND_COLOR.r * (1.0 - factor*0.2),
                              BACKGROUND_COLOR.g * (1.0 - factor*0.2),
                              BACKGROUND_COLOR.b * (1.0 - factor*0.2),
                              255);
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }

    // En-tête avec infos de niveau
    SDL_SetRenderDrawColor(renderer, 52, 73, 94, 240);
    SDL_Rect headerRect = {0, 0, WINDOW_WIDTH, 80};
    SDL_RenderFillRect(renderer, &headerRect);
    
    // Afficher le niveau actuel
    char levelText[20];
    sprintf(levelText, "%s", 
            game->currentLevel == LEVEL_EASY ? "EASY" : 
            game->currentLevel == LEVEL_MEDIUM ? "MEDIUM" : "HARD");
    renderText(renderer, largeFont, levelText, 30, 20, TEXT_COLOR);
    
    // Instructions
    renderTextCentered(renderer, font, "Sort tokens by color", WINDOW_WIDTH / 2, 40, TEXT_COLOR);
    
    // Afficher le compteur de mouvements
    char moveText[20];
    sprintf(moveText, "Moves: %d", game->moveCount);
    renderText(renderer, font, moveText, WINDOW_WIDTH - 150, 20, TEXT_COLOR);
    
    // Afficher le chronomètre
    Uint32 elapsedTime = SDL_GetTicks() - game->startTime;
    int minutes = (elapsedTime / 1000) / 60;
    int seconds = (elapsedTime / 1000) % 60;
    char timeText[20];
    sprintf(timeText, "Time: %02d:%02d", minutes, seconds);
    renderText(renderer, font, timeText, WINDOW_WIDTH - 150, 50, TEXT_COLOR);

    int pileWidth = 80;
    int pileHeight = 350;
    int pileSpacing = 30;
    int startX = (WINDOW_WIDTH - (game->numPiles * (pileWidth + pileSpacing) - pileSpacing)) / 2;
    int startY = 120;
    int tokenHeight = 50;
    int tokenSpacing = 10;

    // Dessiner les piles
    for (int i = 0; i < game->numPiles; i++) {
        SDL_Rect rect = {startX + i * (pileWidth + pileSpacing), startY, pileWidth, pileHeight};
        
        // Fond de la pile avec une légère transparence
        SDL_SetRenderDrawColor(renderer, 30, 40, 50, 180);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        fillRoundedRect(renderer, rect.x, rect.y, rect.w, rect.h, 10);
        
        // Pile sélectionnée - contour doré
        if (i == game->selected) {
            SDL_SetRenderDrawColor(renderer, SELECTED_COLOR.r, SELECTED_COLOR.g, SELECTED_COLOR.b, SELECTED_COLOR.a);
            // Dessiner un contour plus épais et arrondi
            for (int t = 0; t < 3; t++) {
                drawRoundedRect(renderer, rect.x - t, rect.y - t, rect.w + 2*t, rect.h + 2*t, 10);
            }
        } else {
            // Pile normale - contour subtil
            SDL_SetRenderDrawColor(renderer, PILE_BORDER_COLOR.r, PILE_BORDER_COLOR.g, PILE_BORDER_COLOR.b, 180);
            drawRoundedRect(renderer, rect.x, rect.y, rect.w, rect.h, 10);
        }

        // Dessiner les jetons
        for (int j = 0; j < game->piles[i].count; j++) {
            // Position du jeton dans la pile
            int tokenY = rect.y + pileHeight - (j + 1) * (tokenHeight + tokenSpacing);
            int tokenX = rect.x + 5;
            
            drawToken(renderer, tokenX, tokenY, pileWidth - 10, tokenHeight, 
                    COLORS[game->piles[i].colors[j]]);
        }
    }
    
    // Dessiner animation de jeton si active
    if (currentAnimation.active) {
        // Calculer la position intermédiaire
        float easedProgress = sin(currentAnimation.progress * M_PI / 2); // Easing function
        int x = currentAnimation.sourceX + (currentAnimation.destX - currentAnimation.sourceX) * easedProgress;
        int y = currentAnimation.sourceY + (currentAnimation.destY - currentAnimation.sourceY) * easedProgress;
        
        // Dessiner le jeton animé
        drawToken(renderer, x, y, pileWidth - 10, tokenHeight, 
                COLORS[currentAnimation.colorIndex]);
    }
    
    // Afficher l'écran de victoire si le jeu est gagné
    if (game->status == GAME_WON) {
        renderWinScreen(renderer, font, largeFont, game);
    }

    SDL_RenderPresent(renderer);
}

void handleClick(GameState *game, int x, int y) {
    if (game->status == GAME_WON) {
        // Vérifier les clics sur les boutons de l'écran de victoire
        Button restartButton = {
            {WINDOW_WIDTH / 2 - 210, WINDOW_HEIGHT / 2 + 60, 180, 60},
            "Play Again",
            false,
            actionRestart,
            game
        };
        
        Button menuButton = {
            {WINDOW_WIDTH / 2 + 30, WINDOW_HEIGHT / 2 + 60, 180, 60},
            "Main Menu",
            false,
            actionBackToMenu,
            game
        };
        
        if (isPointInRect(x, y, &restartButton.rect)) {
            restartButton.action(restartButton.data);
            return;
        }
        
        if (isPointInRect(x, y, &menuButton.rect)) {
            menuButton.action(menuButton.data);
            return;
        }
        
        return;
    }
    
    if (currentAnimation.active) return; // Ne pas permettre de cliquer pendant l'animation
    
    // Calculer les dimensions des piles
    int pileWidth = 80;
    int pileHeight = 350;
    int pileSpacing = 30;
    int startX = (WINDOW_WIDTH - (game->numPiles * (pileWidth + pileSpacing) - pileSpacing)) / 2;
    int startY = 120;
    
    // Vérifier si le clic est sur une pile
    for (int i = 0; i < game->numPiles; i++) {
        SDL_Rect pileRect = {startX + i * (pileWidth + pileSpacing), startY, pileWidth, pileHeight};
        
        if (isPointInRect(x, y, &pileRect)) {
            // Si aucune pile n'est sélectionnée, sélectionner celle-ci
            if (game->selected == -1) {
                // Ne sélectionner que si la pile n'est pas vide
                if (game->piles[i].count > 0) {
                    game->selected = i;
                }
            } 
            // Si une pile est déjà sélectionnée
            else {
                // Si on clique sur la même pile, désélectionner
                if (game->selected == i) {
                    game->selected = -1;
                } 
                // Sinon, tenter de déplacer le jeton
                else {
                    // MODIFICATION: Vérifier uniquement si la pile de destination a de la place
                    // (suppression de la contrainte de couleur)
                    
                    Pile *src = &game->piles[game->selected];
                    Pile *dest = &game->piles[i];
                    
                    if (dest->count < game->maxTokens) {
                        int sourceTokenColor = src->colors[src->count - 1];
                        
                        // MODIFICATION: Autoriser le déplacement sans vérifier la couleur
                        // Calculer les positions pour l'animation
                        int tokenHeight = 50;
                        int tokenSpacing = 10;
                        int srcTokenX = startX + game->selected * (pileWidth + pileSpacing) + 5;
                        int srcTokenY = startY + pileHeight - src->count * (tokenHeight + tokenSpacing);
                        int destTokenX = startX + i * (pileWidth + pileSpacing) + 5;
                        int destTokenY = startY + pileHeight - (dest->count + 1) * (tokenHeight + tokenSpacing);
                        
                        // Démarrer l'animation
                        startTokenAnimation(srcTokenX, srcTokenY, destTokenX, destTokenY, sourceTokenColor);
                        
                        // Transférer le jeton
                        dest->colors[dest->count++] = sourceTokenColor;
                        src->count--;
                        
                        // Incrémenter le compteur de mouvements
                        game->moveCount++;
                        
                        // Désélectionner
                        game->selected = -1;
                        
                        // Vérifier si le joueur a gagné
                        if (checkWin(game)) {
                            game->status = GAME_WON;
                        }
                    }
                }
            }
            break;
        }
    }
}
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    // Initialisation de SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erreur d'initialisation de SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialisation de SDL_ttf
    if (TTF_Init() != 0) {
        printf("Erreur d'initialisation de SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Création de la fenêtre
    SDL_Window *window = SDL_CreateWindow("Nuts Puzzle",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Erreur de création de la fenêtre: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Création du renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Erreur de création du renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Chargement des polices
    TTF_Font* font = TTF_OpenFont("assets/fonts/Roboto-Bold.ttf", 24);

    //TTF_Font *font = TTF_OpenFont("assets/fonts/Roboto-Regular.ttf", 20);
    TTF_Font *largeFont = TTF_OpenFont("assets/fonts/Roboto-Bold.ttf", 36);
    if (!font || !largeFont) {
        printf("Erreur de chargement des polices: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Initialisation du jeu
    GameState game;
    game.currentLevel = LEVEL_NONE;
    game.status = GAME_PLAYING;
    
    // Boucle principale
    SDL_Event event;
    bool quit = false;
    
    while (!quit) {
        // Gérer le niveau de difficulté
        if (game.currentLevel == LEVEL_NONE) {
            game.currentLevel = showLevelMenu(renderer, font, largeFont);
            if (game.currentLevel != LEVEL_NONE) {
                initGame(&game, game.currentLevel);
            }
        }
        
        // Traiter les événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handleClick(&game, event.button.x, event.button.y);
                }
            }
        }
        
        // Mettre à jour l'animation
        updateAnimation();
        
        // Réinitialiser le jeu si demandé
        if (game.status == GAME_PLAYING && game.currentLevel != LEVEL_NONE) {
            renderGame(renderer, &game, font, largeFont);
        }
        
        SDL_Delay(16); // ~60 FPS
    }
    
    // Libération des ressources
    TTF_CloseFont(font);
    TTF_CloseFont(largeFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}








