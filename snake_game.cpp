#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <cstdio>
using namespace std;

// ========================= 常量 =========================
const int CELL      = 28;        // 每格像素
const int COLS      = 20;        // 列
const int ROWS      = 16;        // 行
const int MARGIN_X  = 40;        // 左边距
const int MARGIN_Y  = 60;        // 上边距
const int INFO_W    = 180;       // 右侧信息栏宽度
const int WIN_W     = MARGIN_X * 2 + COLS * CELL + INFO_W;
const int WIN_H     = MARGIN_Y * 2 + ROWS * CELL;

const int BASE_SPEED[] = { 180, 100, 50 };  // 简单/普通/困难 (ms)

enum Dir { UP, DOWN, LEFT, RIGHT };
struct Point { int x, y; };

// ========================= 全局 =========================
HWND   g_hwnd;
int    g_timerID = 1;
int    g_highScore = 0;

// ========================= 游戏类 =========================
class SnakeGame {
    vector<Point> body;
    Dir dir, nextDir;
    Point food;
    int  score, speed, level;
    bool alive, paused, quitToMenu, justAte;
    int  foodEaten;
    bool gameOver;

public:
    SnakeGame() { srand((unsigned)time(NULL)); loadHighScore(); }

    void init(int difficulty) {
        body.clear();
        body.push_back({ COLS/2,     ROWS/2 });
        body.push_back({ COLS/2 - 1, ROWS/2 });
        body.push_back({ COLS/2 - 2, ROWS/2 });
        dir = RIGHT;  nextDir = RIGHT;
        score = 0;    foodEaten = 0;
        level = difficulty;
        speed = BASE_SPEED[difficulty];
        alive = true; paused = false;
        quitToMenu = false; justAte = false;
        gameOver = false;
        placeFood();
    }

    int  getSpeed()    const { return speed; }
    int  getScore()    const { return score; }
    int  getLength()   const { return (int)body.size(); }
    int  getLevel()    const { return level; }
    bool isPaused()    const { return paused; }
    bool isAlive()     const { return alive; }
    bool isGameOver()  const { return gameOver; }

    void togglePause() { if (alive && !gameOver) paused = !paused; }

    void setDir(Dir d) {
        if ((d == UP    && dir != DOWN)  ||
            (d == DOWN  && dir != UP)    ||
            (d == LEFT  && dir != RIGHT) ||
            (d == RIGHT && dir != LEFT))
            nextDir = d;
    }

    void restart()        { alive = false; gameOver = false; quitToMenu = false; }
    void backToMenu()     { alive = false; gameOver = false; quitToMenu = true; }
    bool shouldQuitToMenu() const { return quitToMenu; }

    void update() {
        justAte = false;
        if (!alive || paused || gameOver) return;

        dir = nextDir;

        Point head = body[0];
        switch (dir) {
        case UP:    head.y--; break;
        case DOWN:  head.y++; break;
        case LEFT:  head.x--; break;
        case RIGHT: head.x++; break;
        }

        if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
            alive = false; gameOver = true;
            if (score > g_highScore) { g_highScore = score; saveHighScore(); }
            return;
        }

        bool ate = (head.x == food.x && head.y == food.y);

        int skip = ate ? 0 : 1;
        for (int i = 0; i < (int)body.size() - skip; i++) {
            if (body[i].x == head.x && body[i].y == head.y) {
                alive = false; gameOver = true;
                if (score > g_highScore) { g_highScore = score; saveHighScore(); }
                return;
            }
        }

        body.insert(body.begin(), head);
        if (ate) {
            score += 10;
            foodEaten++;
            justAte = true;
            if (foodEaten % 5 == 0 && speed > 30) speed -= 8;
            placeFood();
        } else {
            body.pop_back();
        }
    }

    void draw(HDC hdc, RECT& client) {
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
        HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

        // 背景
        HBRUSH bg = CreateSolidBrush(RGB(18, 22, 28));
        FillRect(memDC, &client, bg);
        DeleteObject(bg);

        // ===== 标题 =====
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(255, 200, 50));
        HFONT titleFont = CreateFont(32, 0,0,0, FW_BOLD, 0,0,0,0,0,0,0,0, "Consolas");
        HFONT oldFont = (HFONT)SelectObject(memDC, titleFont);
        RECT tr = { MARGIN_X, 12, MARGIN_X + 400, 60 };
        DrawText(memDC, "SNAKE GAME", -1, &tr, DT_LEFT);
        SelectObject(memDC, oldFont);
        DeleteObject(titleFont);

        // ===== 游戏区域背景 =====
        int gx = MARGIN_X, gy = MARGIN_Y;
        int gw = COLS * CELL, gh = ROWS * CELL;

        HBRUSH boardBg = CreateSolidBrush(RGB(28, 34, 42));
        HPEN boardPen = CreatePen(PS_SOLID, 2, RGB(60, 70, 85));
        HBRUSH oldBr = (HBRUSH)SelectObject(memDC, boardBg);
        HPEN  oldPn = (HPEN)SelectObject(memDC, boardPen);
        Rectangle(memDC, gx - 2, gy - 2, gx + gw + 2, gy + gh + 2);
        SelectObject(memDC, oldBr);
        SelectObject(memDC, oldPn);
        DeleteObject(boardBg);
        DeleteObject(boardPen);

        // ===== 网格线 =====
        HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(36, 42, 52));
        oldPn = (HPEN)SelectObject(memDC, gridPen);
        for (int x = 0; x <= COLS; x++) {
            int px = gx + x * CELL;
            MoveToEx(memDC, px, gy, NULL); LineTo(memDC, px, gy + gh);
        }
        for (int y = 0; y <= ROWS; y++) {
            int py = gy + y * CELL;
            MoveToEx(memDC, gx, py, NULL); LineTo(memDC, gx + gw, py);
        }
        SelectObject(memDC, oldPn);
        DeleteObject(gridPen);

        // ===== 食物 =====
        int fx = gx + food.x * CELL, fy = gy + food.y * CELL;
        HBRUSH foodBr = justAte
            ? CreateSolidBrush(RGB(255, 255, 80))
            : CreateSolidBrush(RGB(240, 50, 50));
        oldBr = (HBRUSH)SelectObject(memDC, foodBr);
        HPEN noPen = (HPEN)GetStockObject(NULL_PEN);
        oldPn = (HPEN)SelectObject(memDC, noPen);
        RoundRect(memDC, fx + 3, fy + 3, fx + CELL - 3, fy + CELL - 3, 10, 10);
        SelectObject(memDC, oldBr);
        SelectObject(memDC, oldPn);
        DeleteObject(foodBr);

        // ===== 蛇身 =====
        for (int i = 0; i < (int)body.size(); i++) {
            int sx = gx + body[i].x * CELL;
            int sy = gy + body[i].y * CELL;
            int pad = 2;

            if (i == 0) {
                // 蛇头
                HBRUSH hBr = CreateSolidBrush(RGB(60, 230, 80));
                oldBr = (HBRUSH)SelectObject(memDC, hBr);
                oldPn = (HPEN)SelectObject(memDC, noPen);
                RoundRect(memDC, sx + 1, sy + 1, sx + CELL - 1, sy + CELL - 1, 10, 10);

                // 眼睛
                HPEN eyePen = CreatePen(PS_SOLID, 2, RGB(18, 22, 28));
                SelectObject(memDC, eyePen);
                int ex1, ey1, ex2, ey2;
                switch (dir) {
                case UP:    ex1 = sx + CELL/2 - 6; ey1 = sy + 8;
                            ex2 = sx + CELL/2 + 6; ey2 = sy + 8;  break;
                case DOWN:  ex1 = sx + CELL/2 - 6; ey1 = sy + CELL - 8;
                            ex2 = sx + CELL/2 + 6; ey2 = sy + CELL - 8; break;
                case LEFT:  ex1 = sx + 8; ey1 = sy + CELL/2 - 6;
                            ex2 = sx + 8; ey2 = sy + CELL/2 + 6; break;
                case RIGHT: ex1 = sx + CELL - 8; ey1 = sy + CELL/2 - 6;
                            ex2 = sx + CELL - 8; ey2 = sy + CELL/2 + 6; break;
                }
                Ellipse(memDC, ex1 - 3, ey1 - 3, ex1 + 3, ey1 + 3);
                Ellipse(memDC, ex2 - 3, ey2 - 3, ex2 + 3, ey2 + 3);
                DeleteObject(eyePen);

                SelectObject(memDC, oldBr);
                SelectObject(memDC, oldPn);
                DeleteObject(hBr);
            } else {
                // 蛇身渐变色
                float t = 1.0f - (float)i / body.size();
                int gv = 120 + (int)(110 * t);
                HBRUSH bBr = CreateSolidBrush(RGB(30, gv, 50));
                oldBr = (HBRUSH)SelectObject(memDC, bBr);
                oldPn = (HPEN)SelectObject(memDC, noPen);
                RoundRect(memDC, sx + pad, sy + pad, sx + CELL - pad, sy + CELL - pad, 6, 6);
                SelectObject(memDC, oldBr);
                SelectObject(memDC, oldPn);
                DeleteObject(bBr);
            }
        }

        // ===== 右侧信息栏 =====
        int ix = gx + gw + 30;
        SetBkMode(memDC, TRANSPARENT);

        RECT infoBg = { ix - 10, gy, ix + 150, gy + 180 };
        HBRUSH infoBr = CreateSolidBrush(RGB(24, 28, 34));
        HPEN  infoPn = CreatePen(PS_SOLID, 1, RGB(60, 70, 85));
        oldBr = (HBRUSH)SelectObject(memDC, infoBr);
        oldPn = (HPEN)SelectObject(memDC, infoPn);
        RoundRect(memDC, infoBg.left, infoBg.top, infoBg.right, infoBg.bottom, 8, 8);
        SelectObject(memDC, oldBr);
        SelectObject(memDC, oldPn);
        DeleteObject(infoBr);
        DeleteObject(infoPn);

        HFONT infoFont = CreateFont(18, 0,0,0, FW_NORMAL, 0,0,0,0,0,0,0,0, "Consolas");
        HFONT boldFont = CreateFont(20, 0,0,0, FW_BOLD,   0,0,0,0,0,0,0,0, "Consolas");
        oldFont = (HFONT)SelectObject(memDC, infoFont);

        int iy = gy + 15;
        char buf[64];

        // 分数
        SelectObject(memDC, boldFont);
        SetTextColor(memDC, RGB(100, 200, 255));
        RECT r = { ix, iy, ix + 140, iy + 30 };
        DrawText(memDC, "SCORE", -1, &r, DT_LEFT);
        SelectObject(memDC, infoFont);
        sprintf(buf, "%d", score);
        r.top += 30; r.bottom += 30; DrawText(memDC, buf, -1, &r, DT_LEFT);

        // 长度
        iy += 65;
        SelectObject(memDC, boldFont);
        SetTextColor(memDC, RGB(120, 220, 100));
        r.top = iy; r.bottom = iy + 30;
        DrawText(memDC, "LENGTH", -1, &r, DT_LEFT);
        SelectObject(memDC, infoFont);
        sprintf(buf, "%d", (int)body.size());
        r.top += 30; r.bottom += 30; DrawText(memDC, buf, -1, &r, DT_LEFT);

        // 速度
        iy += 65;
        SelectObject(memDC, boldFont);
        SetTextColor(memDC, RGB(255, 180, 60));
        r.top = iy; r.bottom = iy + 30;
        DrawText(memDC, "SPEED", -1, &r, DT_LEFT);
        SelectObject(memDC, infoFont);
        int spdPct = (200 - speed) * 100 / 200;
        sprintf(buf, "%d%%", spdPct);
        r.top += 30; r.bottom += 30; DrawText(memDC, buf, -1, &r, DT_LEFT);

        // 最高分
        iy += 70;
        SetTextColor(memDC, RGB(160, 170, 185));
        r.top = iy; r.bottom = iy + 25;
        DrawText(memDC, "High Score", -1, &r, DT_LEFT);
        SetTextColor(memDC, RGB(255, 215, 80));
        sprintf(buf, "%d", g_highScore);
        r.top += 22; r.bottom += 22;
        DrawText(memDC, buf, -1, &r, DT_LEFT);

        SelectObject(memDC, oldFont);
        DeleteObject(infoFont);
        DeleteObject(boldFont);

        // ===== 暂停遮罩 =====
        if (paused && alive) {
            HPEN maskPen = CreatePen(PS_SOLID, 4, RGB(255, 200, 50));
            oldPn = (HPEN)SelectObject(memDC, maskPen);
            oldBr = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
            RECT pr = { gx + gw/2 - 90, gy + gh/2 - 30, gx + gw/2 + 90, gy + gh/2 + 30 };
            RoundRect(memDC, pr.left, pr.top, pr.right, pr.bottom, 8, 8);

            HBRUSH fillBr = CreateSolidBrush(RGB(20, 25, 32));
            SelectObject(memDC, fillBr);
            Rectangle(memDC, pr.left + 2, pr.top + 2, pr.right - 2, pr.bottom - 2);
            SelectObject(memDC, oldBr);
            DeleteObject(fillBr);

            SetTextColor(memDC, RGB(255, 220, 80));
            HFONT pauseFont = CreateFont(24, 0,0,0, FW_BOLD, 0,0,0,0,0,0,0,0, "Consolas");
            oldFont = (HFONT)SelectObject(memDC, pauseFont);
            SetBkMode(memDC, TRANSPARENT);
            DrawText(memDC, "PAUSED", -1, &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(memDC, oldFont);
            DeleteObject(pauseFont);

            SelectObject(memDC, oldPn);
            DeleteObject(maskPen);
        }

        // ===== 游戏结束遮罩 =====
        if (gameOver) {
            int boxW = 300, boxH = 200;
            int bx = gx + (gw - boxW) / 2, by = gy + (gh - boxH) / 2;

            HBRUSH goMask = CreateSolidBrush(RGB(12, 16, 22));
            oldBr = (HBRUSH)SelectObject(memDC, goMask);
            oldPn = (HPEN)SelectObject(memDC, noPen);
            Rectangle(memDC, bx, by, bx + boxW, by + boxH);
            SelectObject(memDC, oldBr);
            SelectObject(memDC, oldPn);
            DeleteObject(goMask);

            HPEN goPen = CreatePen(PS_SOLID, 3, RGB(240, 70, 70));
            oldPn = (HPEN)SelectObject(memDC, goPen);
            oldBr = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
            RoundRect(memDC, bx, by, bx + boxW, by + boxH, 12, 12);
            SelectObject(memDC, oldBr);
            SelectObject(memDC, oldPn);
            DeleteObject(goPen);

            SetBkMode(memDC, TRANSPARENT);

            HFONT goFont = CreateFont(36, 0,0,0, FW_BOLD, 0,0,0,0,0,0,0,0, "Consolas");
            oldFont = (HFONT)SelectObject(memDC, goFont);
            SetTextColor(memDC, RGB(255, 60, 60));
            RECT gor = { bx, by + 10, bx + boxW, by + 55 };
            DrawText(memDC, "GAME OVER", -1, &gor, DT_CENTER);

            HFONT scoreFont = CreateFont(18, 0,0,0, FW_NORMAL, 0,0,0,0,0,0,0,0, "Consolas");
            SelectObject(memDC, scoreFont);
            SetTextColor(memDC, RGB(100, 200, 255));
            RECT sr = { bx, by + 65, bx + boxW, by + 90 };
            char sbuf[64];
            sprintf(sbuf, "Score: %d   Length: %d", score, (int)body.size());
            DrawText(memDC, sbuf, -1, &sr, DT_CENTER);

            if (score >= g_highScore && score > 0) {
                SetTextColor(memDC, RGB(255, 215, 50));
                RECT hr = { bx, by + 100, bx + boxW, by + 125 };
                DrawText(memDC, "*** NEW HIGH SCORE! ***", -1, &hr, DT_CENTER);
            }

            SetTextColor(memDC, RGB(200, 200, 200));
            RECT rr = { bx, by + 140, bx + boxW, by + 170 };
            DrawText(memDC, "R = Restart   ESC = Menu", -1, &rr, DT_CENTER);

            SelectObject(memDC, oldFont);
            DeleteObject(goFont);
            DeleteObject(scoreFont);
        }

        // ===== 底部操作提示 =====
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(120, 130, 145));
        HFONT hintFont = CreateFont(15, 0,0,0, FW_NORMAL, 0,0,0,0,0,0,0,0, "Consolas");
        oldFont = (HFONT)SelectObject(memDC, hintFont);
        RECT hintRect = { MARGIN_X, gy + gh + 12, MARGIN_X + 600, gy + gh + 35 };
        DrawText(memDC, "Arrow Keys / WASD: Move   P: Pause   R: Restart   ESC: Menu", -1, &hintRect, DT_LEFT);
        SelectObject(memDC, oldFont);
        DeleteObject(hintFont);

        // ===== 输出到屏幕 =====
        BitBlt(hdc, 0, 0, WIN_W, WIN_H, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);
    }

private:
    bool onSnake(int x, int y) const {
        for (size_t i = 0; i < body.size(); i++)
            if (body[i].x == x && body[i].y == y) return true;
        return false;
    }

    void placeFood() {
        if ((int)body.size() >= COLS * ROWS) { alive = false; gameOver = true; return; }
        vector<Point> empty;
        empty.reserve(COLS * ROWS - body.size());
        for (int y = 0; y < ROWS; y++)
            for (int x = 0; x < COLS; x++)
                if (!onSnake(x, y))
                    empty.push_back({ x, y });
        if (empty.empty()) { alive = false; gameOver = true; return; }
        food = empty[rand() % empty.size()];
    }

    void loadHighScore() {
        ifstream f("snake_highscore.dat");
        if (f) f >> g_highScore;
        f.close();
    }

    void saveHighScore() {
        ofstream f("snake_highscore.dat");
        if (f) f << g_highScore;
        f.close();
    }
};

// ========================= 全局游戏实例 =========================
SnakeGame* gpGame = NULL;

// ========================= 菜单画面 =========================
void DrawMenu(HDC hdc, RECT& client) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
    HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

    HBRUSH bg = CreateSolidBrush(RGB(18, 22, 28));
    FillRect(memDC, &client, bg);
    DeleteObject(bg);

    SetBkMode(memDC, TRANSPARENT);

    HFONT titleFont = CreateFont(42, 0,0,0, FW_BOLD, 0,0,0,0,0,0,0,0, "Consolas");
    HFONT oldFont = (HFONT)SelectObject(memDC, titleFont);
    SetTextColor(memDC, RGB(255, 200, 50));
    RECT tr = { 0, 50, WIN_W, 110 };
    DrawText(memDC, "SNAKE GAME", -1, &tr, DT_CENTER);
    SelectObject(memDC, oldFont);
    DeleteObject(titleFont);

    const char* items[] = {
        "1.  Start Game  ( Easy )",
        "2.  Start Game  ( Normal )",
        "3.  Start Game  ( Hard )",
        "4.  Help",
        "0.  Quit"
    };
    COLORREF colors[] = {
        RGB(100, 220, 120),
        RGB(255, 200, 60),
        RGB(255, 90, 80),
        RGB(160, 180, 200),
        RGB(150, 150, 150)
    };

    HFONT menuFont = CreateFont(20, 0,0,0, FW_NORMAL, 0,0,0,0,0,0,0,0, "Consolas");
    SelectObject(memDC, menuFont);

    for (int i = 0; i < 5; i++) {
        SetTextColor(memDC, colors[i]);
        RECT r = { 0, 180 + i * 40, WIN_W, 180 + (i + 1) * 40 };
        DrawText(memDC, items[i], -1, &r, DT_CENTER);
    }

    HFONT hsFont = CreateFont(16, 0,0,0, FW_NORMAL, 0,0,0,0,0,0,0,0, "Consolas");
    SelectObject(memDC, hsFont);
    SetTextColor(memDC, RGB(140, 150, 165));
    char hsbuf[64];
    sprintf(hsbuf, "High Score: %d", g_highScore);
    RECT hr = { 0, 420, WIN_W, 450 };
    DrawText(memDC, hsbuf, -1, &hr, DT_CENTER);

    SelectObject(memDC, oldFont);
    DeleteObject(menuFont);
    DeleteObject(hsFont);

    BitBlt(hdc, 0, 0, WIN_W, WIN_H, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

void DrawHelp(HDC hdc, RECT& client) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
    HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

    HBRUSH bg = CreateSolidBrush(RGB(18, 22, 28));
    FillRect(memDC, &client, bg);
    DeleteObject(bg);

    SetBkMode(memDC, TRANSPARENT);

    HFONT titleFont = CreateFont(28, 0,0,0, FW_BOLD, 0,0,0,0,0,0,0,0, "Consolas");
    HFONT bodyFont  = CreateFont(17, 0,0,0, FW_NORMAL, 0,0,0,0,0,0,0,0, "Consolas");
    HFONT oldFont   = (HFONT)SelectObject(memDC, titleFont);

    SetTextColor(memDC, RGB(255, 200, 50));
    RECT tr = { 0, 40, WIN_W, 80 };
    DrawText(memDC, "HELP", -1, &tr, DT_CENTER);

    SelectObject(memDC, bodyFont);
    SetTextColor(memDC, RGB(200, 210, 220));

    const char* lines[] = {
        "",
        "[ Controls ]",
        "  Arrow Keys / WASD  =  Change Direction",
        "  P                   =  Pause / Resume",
        "  R                   =  Restart",
        "  ESC                 =  Back to Menu",
        "",
        "[ Rules ]",
        "  Eat the red apple to grow and earn points.",
        "  Don't hit the walls or your own body.",
        "  Speed increases every 5 apples eaten.",
        "",
        "[ Difficulty ]",
        "  Easy   =  Slow pace, great for beginners.",
        "  Normal =  Moderate speed.",
        "  Hard   =  Fast from the start!",
        "",
        "Press any key to return ...",
    };
    for (int i = 0; i < 17; i++) {
        RECT r = { 80, 100 + i * 26, WIN_W - 80, 100 + (i + 1) * 26 };
        DrawText(memDC, lines[i], -1, &r, DT_LEFT);
    }

    SelectObject(memDC, oldFont);
    DeleteObject(titleFont);
    DeleteObject(bodyFont);

    BitBlt(hdc, 0, 0, WIN_W, WIN_H, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

// ========================= 窗口过程 =========================
enum GameState { STATE_MENU, STATE_HELP, STATE_PLAYING };
GameState g_state = STATE_MENU;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        gpGame = new SnakeGame();
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, g_timerID);
        delete gpGame;
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 1;   // 禁止系统擦背景，避免闪烁

    case WM_KEYDOWN: {
        if (g_state == STATE_MENU) {
            if      (wp == '1') { g_state = STATE_PLAYING; gpGame->init(0); SetTimer(hwnd, g_timerID, gpGame->getSpeed(), NULL); }
            else if (wp == '2') { g_state = STATE_PLAYING; gpGame->init(1); SetTimer(hwnd, g_timerID, gpGame->getSpeed(), NULL); }
            else if (wp == '3') { g_state = STATE_PLAYING; gpGame->init(2); SetTimer(hwnd, g_timerID, gpGame->getSpeed(), NULL); }
            else if (wp == '4') { g_state = STATE_HELP; }
            else if (wp == '0') { DestroyWindow(hwnd); return 0; }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_state == STATE_HELP) {
            g_state = STATE_MENU;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_state == STATE_PLAYING) {
            if (wp == 'P' || wp == 'p') {
                gpGame->togglePause();
                if (gpGame->isPaused()) KillTimer(hwnd, g_timerID);
                else SetTimer(hwnd, g_timerID, gpGame->getSpeed(), NULL);
            }
            else if (wp == 'R' || wp == 'r') {
                KillTimer(hwnd, g_timerID);
                if (gpGame->isGameOver()) {
                    gpGame->init(gpGame->getLevel());
                    SetTimer(hwnd, g_timerID, gpGame->getSpeed(), NULL);
                } else {
                    gpGame->restart();
                }
            }
            else if (wp == VK_ESCAPE) {
                KillTimer(hwnd, g_timerID);
                if (gpGame->isGameOver()) {
                    g_state = STATE_MENU;
                } else {
                    gpGame->backToMenu();
                }
            }
            else if (wp == VK_UP    || wp == 'W' || wp == 'w') gpGame->setDir(UP);
            else if (wp == VK_DOWN  || wp == 'S' || wp == 's') gpGame->setDir(DOWN);
            else if (wp == VK_LEFT  || wp == 'A' || wp == 'a') gpGame->setDir(LEFT);
            else if (wp == VK_RIGHT || wp == 'D' || wp == 'd') gpGame->setDir(RIGHT);

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        return 0;
    }

    case WM_TIMER: {
        if (g_state != STATE_PLAYING) return 0;
        if (gpGame->isPaused()) return 0;

        gpGame->update();

        if (gpGame->shouldQuitToMenu()) {
            KillTimer(hwnd, g_timerID);
            g_state = STATE_MENU;
        }
        else if (!gpGame->isAlive() && gpGame->isGameOver()) {
            KillTimer(hwnd, g_timerID);
        }
        else if (gpGame->isAlive()) {
            KillTimer(hwnd, g_timerID);
            SetTimer(hwnd, g_timerID, gpGame->getSpeed(), NULL);
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);

        if (g_state == STATE_MENU)       DrawMenu(hdc, client);
        else if (g_state == STATE_HELP)  DrawHelp(hdc, client);
        else if (g_state == STATE_PLAYING && gpGame)
            gpGame->draw(hdc, client);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ========================= 入口 =========================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmd) {
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "SnakeGameWnd";
    RegisterClassEx(&wc);

    RECT rect = { 0, 0, WIN_W, WIN_H };
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    int adjW = rect.right - rect.left, adjH = rect.bottom - rect.top;

    int sx = GetSystemMetrics(SM_CXSCREEN), sy = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(0, "SnakeGameWnd", "Snake Game",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (sx - adjW) / 2, (sy - adjH) / 2, adjW, adjH,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmd);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
