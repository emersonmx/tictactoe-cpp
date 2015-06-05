#include <cstdio>
#include <memory>
#include <sstream>

#include <SDL.h>
#include <SDL_image.h>

#include "gmx/application.hpp"
#include "gmx/graphics.hpp"

#define WINDOW_TITLE "TicTacToe"
#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500

#define CENTER(n) (n / 2.f)

#define BOARD_LINE_SIZE 3
#define BOARD_SIZE (BOARD_LINE_SIZE * BOARD_LINE_SIZE)
#define WIN_CONDITION BOARD_LINE_SIZE
#define MARK_DELAY 0.5f

enum {
    MARK_X = -1, NO_MARK, MARK_O
};

enum {
    IGNORE_RESULT = -1,
    DRAW, WINNER_O, WINNER_X,
    RESULT_SIZE
};

enum {
    NO_MESSAGE = -1,
    X_WIN_MESSAGE, O_WIN_MESSAGE, DRAW_MESSAGE,
    MESSAGE_SIZE
};

using namespace std;

class Texture : public gmx::Texture<SDL_Texture*> {
    public:
        Texture(SDL_Texture* texture, int width, int height)
            : gmx::Texture<SDL_Texture*>(texture, width, height) {}
        virtual ~Texture() {
            SDL_DestroyTexture(texture);
        }
};

typedef shared_ptr<Texture> TexturePtr;

class Timer {
    public:
        Timer() : delta(0.f), lastTicks(0) {}
        inline float getDelta() { return delta; }

        void setup() {
            lastTicks = SDL_GetTicks();
        }

        float update() {
            Uint32 ticks = SDL_GetTicks();
            delta = (ticks - lastTicks) / 1000.f;
            lastTicks = ticks;
            return delta;
        }

    private:
        float delta;
        Uint32 lastTicks;
};

bool pointInRect(int x, int y, const SDL_Rect* rect) {
    return (x >= rect->x) && (x < (rect->x + rect->w)) &&
           (y >= rect->y) && (y < (rect->y + rect->h));
}

class TicTacToe : public gmx::Application {
    protected:
        virtual void create() {
            try {
                initSDL();
                createWindow();
                createRenderer();
                initSDLImage();
                loadAssets();
                setup();
            } catch (exception& e) {
                printf("%s", e.what());
                exit(-1);
            } catch (...) {
                printf("Ocorreu um erro desconhecido.");
                exit(-1);
            }
        }

        virtual void dispose() {
            markO.reset();
            markX.reset();
            board.reset();

            for (int i = 0; i < MESSAGE_SIZE; ++i) {
                messages[i].reset();
            }

            IMG_Quit();

            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }

        virtual void update() {
            handleInput();
            processLogic();
            draw();
        }

    private:
        Texture* loadTexture(const char* filename) {
            SDL_Texture* sdltexture = IMG_LoadTexture(renderer, filename);
            if (sdltexture == NULL) {
                printf("Não foi possível criar a texture. SDL_Error: %s\n", SDL_GetError());
                return NULL;
            }

            int w, h;
            SDL_QueryTexture(sdltexture, NULL, NULL, &w, &h);

            return new Texture(sdltexture, w, h);
        }

        void drawTexture(Texture* texture, int x, int y) {
            SDL_Rect offset;
            offset.x = x - CENTER(texture->getWidth());
            offset.y = y - CENTER(texture->getHeight());
            offset.w = texture->getWidth();
            offset.h = texture->getHeight();
            SDL_RenderCopy(renderer, texture->getTexture(), NULL, &offset);
        }

        void initSDL() {
            Uint32 flags = SDL_INIT_VIDEO;
            if (SDL_Init(flags) != 0) {
                stringbuf.str("");
                stringbuf << "A SDL não foi inicializada. SDL_Error: " << SDL_GetError() << endl;
                throw logic_error(stringbuf.str());
            }
        }

        void createWindow() {
            Uint32 flags = SDL_WINDOW_SHOWN;
            int x=SDL_WINDOWPOS_CENTERED, y=SDL_WINDOWPOS_CENTERED;

            window = SDL_CreateWindow(WINDOW_TITLE, x, y, WINDOW_WIDTH, WINDOW_HEIGHT, flags);
            if (window == NULL) {
                stringbuf.str("");
                stringbuf << "Não foi possível criar a janela. SDL_Error: " << SDL_GetError() << endl;
                throw logic_error(stringbuf.str());
            }
        }

        void createRenderer() {
            Uint32 flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
            renderer = SDL_CreateRenderer(window, -1, flags);
            if (renderer == NULL) {
                stringbuf.str("");
                stringbuf << "Não foi possível criar o renderer. SDL_Error: " << SDL_GetError() << endl;
                throw logic_error(stringbuf.str());
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        }

        void initSDLImage() {
            Uint32 flags = IMG_INIT_PNG;
            if (!(IMG_Init(flags) & flags)) {
                stringbuf.str("");
                stringbuf << "Não foi possível iniciar a SDL_Image. IMG_Error: " << IMG_GetError() << endl;
                throw logic_error(stringbuf.str());
            }
        }

        void loadAssets() {
            bool error = false;

            Texture* tmp = loadTexture("assets/board.png");
            error = (tmp == NULL);
            board.reset(tmp);

            tmp = loadTexture("assets/mark_x.png");
            error = (tmp == NULL);
            markX.reset(tmp);

            tmp = loadTexture("assets/mark_o.png");
            error = (tmp == NULL);
            markO.reset(tmp);

            tmp = loadTexture("assets/message_x_winner.png");
            error = (tmp == NULL);
            messages[X_WIN_MESSAGE].reset(tmp);

            tmp = loadTexture("assets/message_o_winner.png");
            error = (tmp == NULL);
            messages[O_WIN_MESSAGE].reset(tmp);

            tmp = loadTexture("assets/message_draw.png");
            error = (tmp == NULL);
            messages[DRAW_MESSAGE].reset(tmp);

            if (error) {
                throw logic_error("Não foi possível carregar as textures.\n");
            }
        }

        void setup() {
            int x=0, y=0, index;
            int tileWidth = WINDOW_WIDTH / BOARD_LINE_SIZE;
            int tileHeight = WINDOW_HEIGHT / BOARD_LINE_SIZE;
            for (int i = 0; i < BOARD_LINE_SIZE; ++i) {
                for (int j = 0; j < BOARD_LINE_SIZE; ++j) {
                    index = i * BOARD_LINE_SIZE + j;
                    rects[index].x = x;
                    rects[index].y = y;
                    rects[index].w = tileWidth;
                    rects[index].h = tileHeight;
                    x += tileWidth;
                }
                x = 0;
                y += tileHeight;
            }

            resetGame();
        }

        void resetGame() {
            srand(time(0));

            message = NO_MESSAGE;
            showMark = true;
            showMarkDelay = MARK_DELAY;
            lastPosition = currentPosition = -1;
            pressedRect = -1;

            countMarks = 0;
            currentPlayer = getRandomPlayer();

            for (int i = 0; i < BOARD_SIZE; ++i) {
                gameBoard[i] = NO_MARK;
            }

            gameResult = IGNORE_RESULT;
            timer.setup();
        }

        inline int getRandomPlayer() {
            return (rand() % 2 == 0) ? MARK_X : MARK_O;
        }

        inline int getMark(int i, int j) {
            return gameBoard[i * BOARD_LINE_SIZE + j];
        }

        void handleInput() {
            lastPosition = currentPosition;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    exit(0);
                }
                if (message != NO_MESSAGE) {
                    if (event.type == SDL_MOUSEBUTTONUP) {
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            resetGame();
                        }
                    }

                    continue;
                }

                if (event.type == SDL_MOUSEMOTION) {
                    for (int i = 0; i < BOARD_SIZE; ++i) {
                        if (pointInRect(event.motion.x, event.motion.y, &(rects[i]))) {
                            currentPosition = i;
                            break;
                        }
                    }
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        pressedRect = currentPosition;
                    }
                } else if (event.type == SDL_MOUSEBUTTONUP) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (pressedRect == currentPosition) {
                            markAt(currentPlayer, currentPosition);
                        }
                    }
                }
            }
        }

        void markAt(int player, int position) {
            if (gameBoard[position] != NO_MARK) {
                return;
            }
            gameBoard[position] = player;
            countMarks++;
            swapPlayer();
            checkWinner();
            if (gameResult != IGNORE_RESULT) {
                switch (gameResult) {
                    case WINNER_X:
                        message = X_WIN_MESSAGE;
                        break;
                    case WINNER_O:
                        message = O_WIN_MESSAGE;
                        break;
                    case DRAW:
                        message = DRAW_MESSAGE;
                        break;
                }
            }
        }

        inline void swapPlayer() {
            currentPlayer = (currentPlayer == MARK_O) ? MARK_X : MARK_O;
        }

        void checkWinner() {
            checkLines();
            if (gameResult != IGNORE_RESULT) {
                return;
            }
            checkRows();
            if (gameResult != IGNORE_RESULT) {
                return;
            }
            checkDiagonals();
            if (gameResult != IGNORE_RESULT) {
                return;
            }

            if (countMarks >= BOARD_SIZE) {
                gameResult = DRAW;
            }
        }

        void checkLines() {
            int conditions[BOARD_LINE_SIZE] = { 0, 0, 0 };
            for (int i = 0; i < BOARD_LINE_SIZE; ++i) {
                for (int j = 0; j < BOARD_LINE_SIZE; ++j) {
                    conditions[i] += getMark(i, j);
                }
            }
            for (int i = 0; i < BOARD_LINE_SIZE; ++i) {
                gameResult = getWinnerByMarkCount(conditions[i]);
                if (gameResult != IGNORE_RESULT) {
                    break;
                }
            }
        }

        void checkRows() {
            int conditions[BOARD_LINE_SIZE] = { 0, 0, 0 };
            for (int i = 0; i < BOARD_LINE_SIZE; ++i) {
                for (int j = 0; j < BOARD_LINE_SIZE; ++j) {
                    conditions[i] += getMark(j, i);
                }
            }
            for (int i = 0; i < BOARD_LINE_SIZE; ++i) {
                gameResult = getWinnerByMarkCount(conditions[i]);
                if (gameResult != IGNORE_RESULT) {
                    break;
                }
            }
        }

        void checkDiagonals() {
            int conditions[2] = { 0, 0 };
            for (int i = 0; i < BOARD_LINE_SIZE; ++i) {
                conditions[0] += getMark(i, i);
                conditions[1] += getMark(i, (BOARD_LINE_SIZE - 1) - i);
            }
            for (int i = 0; i < 2; ++i) {
                gameResult = getWinnerByMarkCount(conditions[i]);
                if (gameResult != IGNORE_RESULT) {
                    break;
                }
            }
        }

        int getWinnerByMarkCount(int count) {
            if (abs(count) == BOARD_LINE_SIZE) {
                if (count < 0) {
                    return WINNER_X;
                } else if (count > 0) {
                    return WINNER_O;
                }
            }

            return IGNORE_RESULT;
        }

        int checkLine(int first, int second, int third) {
            int result = first + second + third;
            if (abs(result) == BOARD_LINE_SIZE) {
                if (result < 0) {
                    return MARK_X;
                } else if (result > 0) {
                    return MARK_O;
                }
            }
            return NO_MARK;
        }

        void processLogic() {
            timer.update();

            if (lastPosition != currentPosition) {
                showMarkDelay = MARK_DELAY;
                showMark = true;
            }

            showMarkDelay -= timer.getDelta();
            if (showMarkDelay < 0) {
                showMark = !showMark;
                showMarkDelay = MARK_DELAY;
            }
        }

        void draw() {
            SDL_RenderClear(renderer);
            drawBoard();
            drawMarks();
            drawCurrentMark();
            drawMessage();
            SDL_RenderPresent(renderer);
        }

        void drawBoard() {
            drawTexture(board.get(), CENTER(WINDOW_WIDTH), CENTER(WINDOW_HEIGHT));
        }

        void drawMarks() {
            for (int i = 0; i < BOARD_SIZE; ++i) {
                Texture* mark = NULL;
                if (gameBoard[i] == MARK_X) {
                    mark = markX.get();
                } else if (gameBoard[i] == MARK_O) {
                    mark = markO.get();
                }
                if (mark != NULL) {
                    drawTexture(mark, rects[i].x + CENTER(rects[i].w),
                                rects[i].y + CENTER(rects[i].h));
                }
            }
        }

        void drawCurrentMark() {
            Texture* mark = (currentPlayer == MARK_X) ? markX.get() : markO.get();
            if (showMark) {
                if (0 <= currentPosition && currentPosition < BOARD_SIZE) {
                    if (gameBoard[currentPosition] != NO_MARK) {
                        return;
                    }
                    drawTexture(mark, rects[currentPosition].x + CENTER(rects[currentPosition].w),
                                rects[currentPosition].y + CENTER(rects[currentPosition].h));

                }
            }
        }

        void drawMessage() {
            if (message != NO_MESSAGE) {
                drawTexture(messages[message].get(), CENTER(WINDOW_WIDTH), CENTER(WINDOW_HEIGHT));
            }
        }

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Event event;

        TexturePtr board;
        TexturePtr markX;
        TexturePtr markO;
        TexturePtr messages[MESSAGE_SIZE];
        Timer timer;

        int message;
        bool showMark;
        float showMarkDelay;
        int currentPosition;
        int lastPosition;
        int pressedRect;
        SDL_Rect rects[BOARD_SIZE];

        int countMarks;
        int currentPlayer;
        int gameBoard[BOARD_SIZE];
        int gameResult;

        stringstream stringbuf;
};

int main() {
    TicTacToe ttt;
    return ttt.run();
}
