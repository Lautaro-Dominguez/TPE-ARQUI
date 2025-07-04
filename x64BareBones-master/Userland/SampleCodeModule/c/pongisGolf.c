#include <stdint.h>
#include <stddef.h>
#include "pongisGolf.h"
#include "userlib.h"

#define PLAYER_SIZE 30
#define PLAYER_SPEED 13
#define BALL_RADIUS 5
#define HOLE_RADIUS 10
#define MAX_LEVELS 5

#define COLOR_PLAYER1 0xFF0000 
#define COLOR_PLAYER2 0x0000FF 
#define COLOR_BALL 0xFFFFFF    
#define COLOR_HOLE 0x000000    
#define COLOR_BG 0x7FFFD4     

#define SCORE_AREA_X 0
#define SCORE_AREA_Y 0
#define SCORE_AREA_WIDTH_1P 140
#define SCORE_AREA_WIDTH_2P 300
#define SCORE_AREA_HEIGHT 70
#define SCORE_AREA_COLOR 0x7FFFD4

extern void syscall(uint64_t rax, uint64_t rbx, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9);
void waitNSeconds(uint8_t secondsToWait);

Level levels[MAX_LEVELS];
Player *lastHitter = NULL;
int currentLevel = 0;

// Estructuras para judadores, pelota, hoyo y obstáculos
Player p1 = {100, 300, COLOR_PLAYER1, 0, 'w', 's', 'a', 'd'};
Player p2 = {900, 300, COLOR_PLAYER2, 0, 'i', 'k', 'j', 'l'};
Ball ball;
Hole hole;
ObstacleRect scoreArea = {SCORE_AREA_X, SCORE_AREA_Y, SCORE_AREA_WIDTH_2P, SCORE_AREA_HEIGHT, SCORE_AREA_COLOR};

int numPlayers = 2;
int prev_ball_x = 0, prev_ball_y = 0;
int prev_p1_x = 0, prev_p1_y = 0;
int prev_p2_x = 0, prev_p2_y = 0;

// Llamada a syscall para ancho de pantalla
int getWidth() {
    int width;
    syscall(20, &width, 0, 0, 0, 0);
    return width;
}

// Llamada a syscall para alto de pantalla
int getHeight() {
    int height;
    syscall(21, &height,0 ,0, 0, 0);
    return height;
}

// Llamada a syscall para dibujar circulos
void drawCircle(int x, int y, int radius, uint32_t color) {
   syscall(11, x, y, radius, color, 0);
}

// Llamada a syscall para dibujar rectangulos
void drawRectangle(int x, int y, int width, int height, uint32_t color) {
    syscall(12, x, y, width, height, color);
}

// Llamada a syscall para borrar la pantalla
void clearScreen(uint32_t color) {
    syscall(13, color, 0, 0, 0, 0);
}

// Llamada a syscall para establecer el cursor en una posición
void setCursor(int x, int y) {
    syscall(14, x, y, 0, 0, 0);
}

// Llamada a syscall para ocultar el cursor
void hideCursor() {
    syscall(18, 0, 0, 0, 0, 0);
}

// Utilidades
void gameObjects() {
    ball.x = getWidth() / 2;
    ball.y = getHeight() / 2;
    ball.dx = 0;
    ball.dy = 0;
    ball.inMotion = 0;
    ball.color = COLOR_BALL;

    hole.x = getWidth() / 2;
    hole.y = 100;
    hole.radius = HOLE_RADIUS;
    hole.color = COLOR_HOLE;
}

// Funcion que dibuja el area de puntajes
void drawScoreArea() {
    if(numPlayers == 1){
        scoreArea.width = SCORE_AREA_WIDTH_1P;
    }
    drawRectangle(scoreArea.x, scoreArea.y, scoreArea.width, scoreArea.height, scoreArea.color);
}

// Funcion que dibuja los puntajes de los jugadores
void drawScores() {
    drawScoreArea();
    setCursor(10, 10);
    printf("P1: ");
    putChar('0' + p1.score);
    setCursor(10, 40);
    printf("Rojo (WASD)");
    if(numPlayers == 2){
        setCursor(200, 10);
        printf("P2: ");
        putChar('0' + p2.score); 
        setCursor(200, 40);
        printf("Azul (IJKL)");
    }
}

// Funcion que dibuja un jugador
void drawPlayer(Player *p) {
    drawCircle(p->x + PLAYER_SIZE / 2, p->y + PLAYER_SIZE / 2, PLAYER_SIZE / 2, p->color);

    int eye_radius = PLAYER_SIZE / 8;
    int eye_offset_x = PLAYER_SIZE / 4;
    int eye_offset_y = PLAYER_SIZE / 4;
    drawCircle(p->x + eye_offset_x, p->y + eye_offset_y, eye_radius, 0x000000);
    drawCircle(p->x + PLAYER_SIZE - eye_offset_x, p->y + eye_offset_y, eye_radius, 0x000000);

    int mouth_width = PLAYER_SIZE / 2;
    int mouth_height = PLAYER_SIZE / 8;
    int mouth_x = p->x + PLAYER_SIZE / 4;
    int mouth_y = p->y + (PLAYER_SIZE * 3) / 4;
    drawRectangle(mouth_x, mouth_y, mouth_width, mouth_height, 0x000000);
}

// Funcion que borra un jugador
void clearPlayer(int x, int y) {
    drawCircle(x + PLAYER_SIZE / 2, y + PLAYER_SIZE / 2, PLAYER_SIZE / 2, COLOR_BG);
}

// Funcion que dibuja la pelota
void drawBall(Ball *b) {
    drawCircle(b->x, b->y, BALL_RADIUS, b->color);
}

// Funcion que borra la pelota
void clearBall(int x, int y) {
    drawCircle(x, y, BALL_RADIUS, COLOR_BG);
}

// Funcion que dibuja el hoyo
void drawHole(Hole *h) {
    drawCircle(h->x, h->y, h->radius, h->color);
}

// Funcion que borra el hoyo
void clearHole(int x, int y) {
    drawCircle(x, y, HOLE_RADIUS, COLOR_BG);
}

// Funcion que limpia la pantalla
void clear() {
    clearScreen(COLOR_BG);
}

// Funcion que se encarga de reiniciar la pelota al pasar de nivel
void resetBall() {
    Level *lvl = &levels[currentLevel];
    int found = 0;
    int try_x = getWidth() / 2;
    int try_y = getHeight() / 2;

    for (int offset = 0; offset < getWidth() && !found; offset += 20) {
        for (int dx = -offset; dx <= offset && !found; dx += 20) {
            for (int dy = -offset; dy <= offset && !found; dy += 20) {
                int x = getWidth() / 2 + dx;
                int y = getHeight() / 2 + dy;

                if (x < BALL_RADIUS) x = BALL_RADIUS;
                if (x > getWidth() - BALL_RADIUS) x = getWidth() - BALL_RADIUS;
                if (y < BALL_RADIUS) y = BALL_RADIUS;
                if (y > getHeight() - BALL_RADIUS) y = getHeight() - BALL_RADIUS;

                Ball temp = {x, y, 0, 0, 0, COLOR_BALL};
                found = 1;
                for (int i = 0; i < lvl->numRects; i++)
                    if (ballHitsRect(&temp, &lvl->rects[i])) found = 0;
                for (int i = 0; i < lvl->numCircles; i++)
                    if (ballHitsCircle(&temp, &lvl->circles[i])) found = 0;
                if (found) {
                    try_x = x;
                    try_y = y;
                }
            }
        }
    }

    ball.x = try_x;
    ball.y = try_y;
    ball.dx = 0;
    ball.dy = 0;
    ball.inMotion = 0;
}

// Funcion que se encarga de reiniciar un jugador al pasar de nivel
void resetPlayer(Player *p, int start_x, int start_y) {
    Level *lvl = &levels[currentLevel];
    int found = 0, try_x, try_y;
    for (int tries = 0; tries < 100 && !found; tries++) {
        try_x = start_x;
        try_y = start_y;
        found = 1;
        for (int i = 0; i < lvl->numRects; i++)
            if (playerHitsRect(p, &lvl->rects[i])) found = 0;
        for (int i = 0; i < lvl->numCircles; i++)
            if (playerHitsCircle(p, &lvl->circles[i])) found = 0;
    }
    p->x = try_x;
    p->y = try_y;
}

// Funcion que retorna 1 si la pelota esta dentro del hoyo
int isInHole(Ball *b, Hole *h) {
    int dx = b->x - h->x;
    int dy = b->y - h->y;
    return (dx * dx + dy * dy <= h->radius * h->radius);
}

// Funcion que calcula el valor absoluto de un entero (no se podia usar math.h)
static int my_abs(int x) {
    return x < 0 ? -x : x;
}

// Funcion que retorna 1 si el jugador golpea la pelota
int playerHitsBall(Player *p, Ball *b) {
    int player_cx = p->x + PLAYER_SIZE / 2;
    int player_cy = p->y + PLAYER_SIZE / 2;
    int dx = b->x - player_cx;
    int dy = b->y - player_cy;
    int distance2 = dx * dx + dy * dy;
    int minDist = (PLAYER_SIZE / 2) + BALL_RADIUS;
    return distance2 <= (minDist * minDist);
}

// Funcion que retorna 1 si dos jugadores colisionan
int playersCollide(Player *p1, Player *p2) {
    int px1 = p1->x, px2 = p1->x + PLAYER_SIZE;
    int py1 = p1->y, py2 = p1->y + PLAYER_SIZE;
    int qx1 = p2->x, qx2 = p2->x + PLAYER_SIZE;
    int qy1 = p2->y, qy2 = p2->y + PLAYER_SIZE;
    return !(px2 < qx1 || px1 > qx2 || py2 < qy1 || py1 > qy2);
}

//Funcion que mueve al jugador segun sus coordenadas, velocidad y tamaño
void movePlayer(Player *p, char key, int *prev_x, int *prev_y) {
    int new_x = p->x;
    int new_y = p->y;

    if (key == p->up && p->y > 0)
        new_y -= PLAYER_SPEED;
    else if (key == p->down && p->y + PLAYER_SIZE < getHeight())
        new_y += PLAYER_SPEED;
    else if (key == p->left && p->x > 0)
        new_x -= PLAYER_SPEED;
    else if (key == p->right && p->x + PLAYER_SIZE < getWidth())
        new_x += PLAYER_SPEED;

    if (new_x == p->x && new_y == p->y) return;

    Level *lvl = &levels[currentLevel];
    Player temp = *p;
    temp.x = new_x;
    temp.y = new_y;

    for (int i = 0; i < lvl->numRects; i++)
        if (playerHitsRect(&temp, &lvl->rects[i])) return;
    for (int i = 0; i < lvl->numCircles; i++)
        if (playerHitsCircle(&temp, &lvl->circles[i])) return;
    if (playerHitsRect(&temp, &scoreArea)) return;

    if (numPlayers == 2) {
        Player *other = (p == &p1) ? &p2 : &p1;
        if (playersCollide(&temp, other)) return;
    }

    if (ball.inMotion) {
        int player_cx = new_x + PLAYER_SIZE / 2;
        int player_cy = new_y + PLAYER_SIZE / 2;
        int dx = ball.x - player_cx;
        int dy = ball.y - player_cy;
        int distance2 = dx * dx + dy * dy;
        int minDist = (PLAYER_SIZE / 2) + BALL_RADIUS;
        if (distance2 <= (minDist * minDist)) return;
    }

    clearPlayer(*prev_x, *prev_y);
    redrawObstaclesInArea(*prev_x, *prev_y, PLAYER_SIZE, PLAYER_SIZE);

    p->x = new_x;
    p->y = new_y;

    drawPlayer(p);

    *prev_x = new_x;
    *prev_y = new_y;

    if (!ball.inMotion && playerHitsBall(p, &ball)) {
        int player_cx = p->x + PLAYER_SIZE / 2;
        int player_cy = p->y + PLAYER_SIZE / 2;
        int dx = ball.x - player_cx;
        int dy = ball.y - player_cy;
        int mag2 = dx*dx + dy*dy;
        if (mag2 == 0) {
            dx = 0;
            dy = -1;
        }
        int abs_dx = my_abs(dx);
        int abs_dy = my_abs(dy);
        int max_abs = abs_dx > abs_dy ? abs_dx : abs_dy;
        if (max_abs == 0) max_abs = 1;
        int speed = 6;
        ball.dx = dx * speed / max_abs;
        ball.dy = dy * speed / max_abs;
        ball.inMotion = 1;
        lastHitter = p;
    }
}

// Funcion que mueve a la pelota segun sus coordenadas, velocidad de impacto y tamaño
void moveBall() {
    if (!ball.inMotion) return;

    int next_x = ball.x + ball.dx;
    int next_y = ball.y + ball.dy;
    Level *lvl = &levels[currentLevel];

    for (int i = 0; i < lvl->numRects; i++) {
        ObstacleRect *rect = &lvl->rects[i];
        Ball tempX = {next_x, ball.y, 0, 0, 0, COLOR_BALL};
        Ball tempY = {ball.x, next_y, 0, 0, 0, COLOR_BALL};
        Ball tempXY = {next_x, next_y, 0, 0, 0, COLOR_BALL};

        int hitX = ballHitsRect(&tempX, rect);
        int hitY = ballHitsRect(&tempY, rect);
        int hitXY = ballHitsRect(&tempXY, rect);

        if (hitXY) {
            if (hitX && !hitY) {
                ball.dx = -ball.dx;
            } else if (!hitX && hitY) {
                ball.dy = -2*ball.dy;
            } else {
                ball.dx = -ball.dx;
                ball.dy = -2*ball.dy;
            }
            return;
        }
    }

    if (ballHitsRect(&(Ball){next_x, next_y, 0, 0, 0, COLOR_BALL}, &scoreArea)) {
        if (ballHitsRect(&(Ball){ball.x, next_y, 0, 0, 0, COLOR_BALL}, &scoreArea)){
            ball.dy = -ball.dy;
        }
        if (ballHitsRect(&(Ball){next_x, ball.y, 0, 0, 0, COLOR_BALL}, &scoreArea)){
            ball.dx = -ball.dx;
        }
        return;
    }
    
    for (int i = 0; i < lvl->numCircles; i++) {
        ObstacleCircle *c = &lvl->circles[i];
        Ball temp = {next_x, next_y, 0, 0, 0, COLOR_BALL};
        if (ballHitsCircle(&temp, c)) {
            int cx = c->x;
            int cy = c->y;
            int bx = ball.x;
            int by = ball.y;
            int nx = bx - cx;
            int ny = by - cy;
            int nlen2 = nx*nx + ny*ny;
            if (nlen2 == 0){
                nlen2 = 1;
            } 

            int dot = ball.dx * nx + ball.dy * ny;

            ball.dx = 2 * (ball.dx - 2 * dot * nx / nlen2);
            ball.dy = 2 * (ball.dy - 2 * dot * ny / nlen2);
            return;
        }
    }

    clearBall(prev_ball_x, prev_ball_y);
    redrawObstaclesInArea(prev_ball_x - BALL_RADIUS, prev_ball_y - BALL_RADIUS, BALL_RADIUS * 2, BALL_RADIUS * 2);

    ball.x = next_x;
    ball.y = next_y;

    if (ball.x - BALL_RADIUS < 0) {
        ball.x = BALL_RADIUS;
        ball.dx = -ball.dx;
    }
    if (ball.x + BALL_RADIUS > getWidth()) {
        ball.x = getWidth() - BALL_RADIUS;
        ball.dx = -ball.dx;
    }
    if (ball.y - BALL_RADIUS < 0) {
        ball.y = BALL_RADIUS;
        ball.dy = -ball.dy;
    }
    if (ball.y + BALL_RADIUS > getHeight()) {
        ball.y = getHeight() - BALL_RADIUS;
        ball.dy = -ball.dy;
    }

    drawBall(&ball);

    int ball_area_x = prev_ball_x - BALL_RADIUS;
    int ball_area_y = prev_ball_y - BALL_RADIUS;
    int ball_area_size = BALL_RADIUS * 2;
    
    if (!(ball_area_x > p1.x + PLAYER_SIZE || ball_area_x + ball_area_size < p1.x || 
          ball_area_y > p1.y + PLAYER_SIZE || ball_area_y + ball_area_size < p1.y)) {
        drawPlayer(&p1);
    }
    
    if (numPlayers == 2) {
        if (!(ball_area_x > p2.x + PLAYER_SIZE || ball_area_x + ball_area_size < p2.x || 
              ball_area_y > p2.y + PLAYER_SIZE || ball_area_y + ball_area_size < p2.y)) {
            drawPlayer(&p2);
        }
    }

    prev_ball_x = ball.x;
    prev_ball_y = ball.y;

    ball.dx *= 0.95;
    ball.dy *= 0.95;

    if (my_abs(ball.dx) < 1 && my_abs(ball.dy) < 1) {
        ball.inMotion = 0;
    }
}

// Funcion que obtiene un caracter del teclado
char getCharFromKeyboard() {
    return getChar();
}

// Funcion que permite seleccionar la cantidad de jugadores
void selectPlayers() {
    clearScreen(COLOR_BG);
    setCursor(300, 300);
    printf("Presione '1' para 1 jugador o '2' para 2 jugadores:");
    setCursor(350, 320);
    printf("1. Jugador 1 (WASD)");
    setCursor(350, 340);
    printf("2. Jugador 2 (IJKL)");
    char c = 0;
    while (c != '1' && c != '2') {
        c = getCharFromKeyboard();
    }
    numPlayers = (c == '1') ? 1 : 2;
}

// Funcion que dibuja los obstaculos del nivel actual
void drawObstacles() {
    drawRectangle(scoreArea.x, scoreArea.y, scoreArea.width, scoreArea.height, scoreArea.color);
    Level *lvl = &levels[currentLevel];
    for (int i = 0; i < lvl->numRects; i++) {
        ObstacleRect *r = &lvl->rects[i];
        drawRectangle(r->x, r->y, r->width, r->height, r->color);
    }
    for (int i = 0; i < lvl->numCircles; i++) {
        ObstacleCircle *c = &lvl->circles[i];
        drawCircle(c->x, c->y, c->radius, c->color);
    }
}

// Funcion que redibuja los obstaculos en un area especifica
void redrawObstaclesInArea(int x, int y, int width, int height) {
    Level *lvl = &levels[currentLevel];
    
    for (int i = 0; i < lvl->numRects; i++) {
        ObstacleRect *r = &lvl->rects[i];
        if (!(x > r->x + r->width || x + width < r->x || 
              y > r->y + r->height || y + height < r->y)) {
            drawRectangle(r->x, r->y, r->width, r->height, r->color);
        }
    }

    if (!(x > scoreArea.x + scoreArea.width || x + width < scoreArea.x || y > scoreArea.y + scoreArea.height || y + height < scoreArea.y)) {
        drawRectangle(scoreArea.x, scoreArea.y, scoreArea.width, scoreArea.height, scoreArea.color);
    }
    for (int i = 0; i < lvl->numCircles; i++) {
        ObstacleCircle *c = &lvl->circles[i];
        int dx = (x + width/2) - c->x;
        int dy = (y + height/2) - c->y;
        int maxDist = width/2 + height/2 + c->radius;
        if (dx*dx + dy*dy <= maxDist*maxDist) {
            drawCircle(c->x, c->y, c->radius, c->color);
        }
    }
    
    int hx = hole.x, hy = hole.y;
    if (!(x > hx + HOLE_RADIUS || x + width < hx - HOLE_RADIUS || y > hy + HOLE_RADIUS || y + height < hy - HOLE_RADIUS)) {
        drawHole(&hole);
    }
}

// Funcion que retorna 1 si la pelota colisiona con un rectangulo
int ballHitsRect(Ball *b, ObstacleRect *r) {
    int closestX = b->x < r->x ? r->x : (b->x > r->x + r->width ? r->x + r->width : b->x);
    int closestY = b->y < r->y ? r->y : (b->y > r->y + r->height ? r->y + r->height : b->y);
    int dx = b->x - closestX;
    int dy = b->y - closestY;
    return (dx * dx + dy * dy) <= (BALL_RADIUS * BALL_RADIUS);
}

// Funcion que retorna 1 si la pelota colisiona con un circulo
int ballHitsCircle(Ball *b, ObstacleCircle *c) {
    int dx = b->x - c->x;
    int dy = b->y - c->y;
    int minDist = BALL_RADIUS + c->radius;
    return (dx * dx + dy * dy) <= (minDist * minDist);
}

// Funcion que retorna 1 si el jugador colisiona con un rectangulo
int playerHitsRect(Player *p, ObstacleRect *r) {
    int px1 = p->x, px2 = p->x + PLAYER_SIZE;
    int py1 = p->y, py2 = p->y + PLAYER_SIZE;
    int rx1 = r->x, rx2 = r->x + r->width;
    int ry1 = r->y, ry2 = r->y + r->height;
    return !(px2 < rx1 || px1 > rx2 || py2 < ry1 || py1 > ry2);
}

// Funcion que retorna 1 si el jugador colisiona con un circulo
int playerHitsCircle(Player *p, ObstacleCircle *c) {
    int cx = p->x + PLAYER_SIZE / 2;
    int cy = p->y + PLAYER_SIZE / 2;
    int dx = cx - c->x;
    int dy = cy - c->y;
    int minDist = PLAYER_SIZE / 2 + c->radius;
    return (dx * dx + dy * dy) <= (minDist * minDist);
}

// Funcion que inicializa los niveles del juego (se pueden crear mas niveles dentro de ella)
void Levels() {
    // Nivel 1
    levels[0].hole_x = getWidth() / 2;
    levels[0].hole_y = 100;
    levels[0].numRects = 0;
    levels[0].numCircles = 0;

    // Nivel 2
    levels[1].hole_x = getWidth() / 4;
    levels[1].hole_y = 150;
    levels[1].numRects = 1;
    levels[1].rects[0] = (ObstacleRect){getWidth()/2 - 50, getHeight()/2 - 20, 100, 40, 0x964B00};
    levels[1].numCircles = 0;

    // Nivel 3
    levels[2].hole_x = getWidth() * 3 / 4;
    levels[2].hole_y = 200;
    levels[2].numRects = 0;
    levels[2].numCircles = 1;
    levels[2].circles[0] = (ObstacleCircle){getWidth()/2, getHeight()/2, 30, 0x228B22};

    // Nivel 4
    levels[3].hole_x = getWidth() / 2;
    levels[3].hole_y = getHeight() - 100;
    levels[3].numRects = 2;
    levels[3].rects[0] = (ObstacleRect){200, 300, 80, 30, 0x964B00};
    levels[3].rects[1] = (ObstacleRect){700, 400, 60, 60, 0x964B00};
    levels[3].numCircles = 1;
    levels[3].circles[0] = (ObstacleCircle){getWidth()/2, 500, 40, 0x228B22};

    // Nivel 5
    levels[4].hole_x = getWidth() / 2 + 50;
    levels[4].hole_y = getHeight() /2 + 250;
    levels[4].numRects = 2;
    levels[4].rects[0] = (ObstacleRect){400, 200, 120, 30, 0x964B00};
    levels[4].rects[1] = (ObstacleRect){getWidth()/2, getHeight()/2 +100, 80, 80, 0x964B00};
    levels[4].numCircles = 2;
    levels[4].circles[0] = (ObstacleCircle){300, 400, 30, 0x228B22};
    levels[4].circles[1] = (ObstacleCircle){800, 300, 50, 0x228B22};
}

// Funcion que se encarga de pasar de nivel. Esto implica reiniciar la pelota y los jugadores
int nextLevel() {
    currentLevel++;
    if (currentLevel >= MAX_LEVELS) {
        return 0; 
    }
    hole.x = levels[currentLevel].hole_x;
    hole.y = levels[currentLevel].hole_y;
    resetBall();
    resetPlayer(&p1, 100, 300);
    if (numPlayers == 2){
        resetPlayer(&p2, 900, 300);
    } 
    resetBall();
    return 1;
}

// Funcion que muestra al ganador en pantalla al finalizar el juego
void showWinner() {
    clearScreen(COLOR_BG);
    setCursor(400, 300);
    if (p1.score > p2.score) {
        printf("Winner: player 1");
    } else if (p2.score > p1.score) {
        printf("Winner: player 2");
    } else {
        printf("Empate!");
    }
    setCursor(400, 320);
    printf("Returning shell...");
    waitNSeconds(5);
}

// Funcion principal. Se encarga de inicializar el juego dibujando a los jugadores, la pelota, el hoyo y los 
// obstaculos. Ademas, se encarga de manejar toda la logica del juego, como el movimiento de los jugadores
// y la pelota, las colisiones entre ellos y los cambios de nivel al meter la pelota.
// Tambien se encarga de mostrar al ganador una vez que se hayan completado todos los niveles.
void pongisGolfMain() {
    gameObjects();
    Levels();
    currentLevel = 0;
    hole.x = levels[currentLevel].hole_x;
    hole.y = levels[currentLevel].hole_y;
    selectPlayers();
    
    clear();
    drawHole(&hole);
    drawObstacles();
    drawPlayer(&p1);
    if (numPlayers == 2) {
        drawPlayer(&p2);
    }
    drawBall(&ball);
    drawScores();
    hideCursor();
    
    prev_ball_x = ball.x;
    prev_ball_y = ball.y;
    prev_p1_x = p1.x;
    prev_p1_y = p1.y;
    prev_p2_x = p2.x;
    prev_p2_y = p2.y;
     while (1) {
        char key = getCharFromKeyboard();

        if (key) {
            movePlayer(&p1, key, &prev_p1_x, &prev_p1_y);
            if (numPlayers == 2) {
                movePlayer(&p2, key, &prev_p2_x, &prev_p2_y);
            }
			syscall(5,0,0,0,0,0);
			hideCursor();
        }

        moveBall();

        if (isInHole(&ball, &hole)) {
            syscall(6, 0, 0, 0, 0, 0);
            if (lastHitter != NULL) {
                lastHitter->score++;
            }
            lastHitter = NULL; 
            if (!nextLevel()) {
                showWinner();
                clearScreen(0x000000);
                return;
            }

            clear();
            drawHole(&hole);
            drawObstacles();
            drawPlayer(&p1);
            if (numPlayers == 2) {
                drawPlayer(&p2);
            }
            drawBall(&ball);
            drawScores();
            
            prev_ball_x = ball.x;
            prev_ball_y = ball.y;
            prev_p1_x = p1.x;
            prev_p1_y = p1.y;
            prev_p2_x = p2.x;
            prev_p2_y = p2.y;
        }
    }
}