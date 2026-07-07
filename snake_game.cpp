#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
using namespace std;

const int W = 20, H = 16;
const int START_SPEED = 150;

enum Dir { UP, DOWN, LEFT, RIGHT };
struct Point { int x, y; };

// ========================= 控制台辅助 =========================
HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);

void Goto(int x, int y) { COORD c = { (SHORT)x, (SHORT)y }; SetConsoleCursorPosition(hCon, c); }
void Color(int fg) { SetConsoleTextAttribute(hCon, fg); }
void HideCursor() { CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(hCon, &ci); ci.bVisible = FALSE; SetConsoleCursorInfo(hCon, &ci); }
void ShowCursor() { CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(hCon, &ci); ci.bVisible = TRUE; SetConsoleCursorInfo(hCon, &ci); }

// ========================= 游戏类 =========================
class Snake {
    vector<Point> body;      // body[0] 是头
    Dir dir;
    Point food;
    int score, speed;
    bool alive, paused, quitToMenu;
    int foodEaten;

public:
    Snake() : score(0), speed(START_SPEED), alive(false), paused(false), quitToMenu(false), foodEaten(0) {
        HideCursor();
        srand((unsigned)time(nullptr));
    }

    void init() {
        body.clear();
        body.push_back({ W/2, H/2 });
        body.push_back({ W/2 - 1, H/2 });
        body.push_back({ W/2 - 2, H/2 });
        dir = RIGHT;
        score = 0;
        speed = START_SPEED;
        foodEaten = 0;
        alive = true;
        paused = false;
        quitToMenu = false;
        placeFood();
    }

private:
    bool onSnake(int x, int y) const {
        for (const auto& p : body)
            if (p.x == x && p.y == y) return true;
        return false;
    }

    void placeFood() {
        if ((int)body.size() >= W * H) { alive = false; return; } // 胜利（实际不会）
        do {
            food.x = rand() % W;
            food.y = rand() % H;
        } while (onSnake(food.x, food.y));
    }

    // ========== 绘制 ==========
    void drawAll() {
        system("cls");
        // 顶边
        Goto(0, 0); Color(7);
        cout << "  +"; for (int i = 0; i < W; i++) cout << "--"; cout << "+";

        // 地图
        for (int y = 0; y < H; y++) {
            Goto(0, y + 1); Color(7); cout << "  |";
            for (int x = 0; x < W; x++) {
                if (food.x == x && food.y == y) {
                    Color(12); cout << "()";
                } else if (onSnake(x, y)) {
                    if (body[0].x == x && body[0].y == y) {
                        Color(10);
                        if (dir == UP)       cout << "/\\";
                        else if (dir == DOWN) cout << "\\/";
                        else if (dir == LEFT) cout << "< ";
                        else                  cout << " >";
                    } else {
                        Color(2); cout << "[]";
                    }
                } else {
                    cout << "  ";
                }
            }
            Goto(2 + W*2, y + 1); Color(7); cout << "|";
        }

        // 底边
        Goto(0, H + 1); Color(7);
        cout << "  +"; for (int i = 0; i < W; i++) cout << "--"; cout << "+";

        // 状态栏
        Goto(0, H + 3); Color(11);   cout << "  Score: " << score;
        Goto(22, H + 3); Color(7);  cout << "Length: " << body.size();
        Goto(38, H + 3); Color(7);  cout << "Speed: " << (200 - speed) * 100 / 200 << "%  ";
        Goto(0, H + 4); Color(8);   cout << "  WASD/Arrows: move  P: pause  R: restart  ESC: menu";
    }

    // ========== 输入（非阻塞） ==========
    void input() {
        while (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) {
                int key = _getch();
                if (key == 72 && dir != DOWN)  dir = UP;
                if (key == 80 && dir != UP)    dir = DOWN;
                if (key == 75 && dir != RIGHT) dir = LEFT;
                if (key == 77 && dir != LEFT)  dir = RIGHT;
            } else {
                if ((ch == 'w' || ch == 'W') && dir != DOWN)  dir = UP;
                if ((ch == 's' || ch == 'S') && dir != UP)    dir = DOWN;
                if ((ch == 'a' || ch == 'A') && dir != RIGHT) dir = LEFT;
                if ((ch == 'd' || ch == 'D') && dir != LEFT)  dir = RIGHT;
                if (ch == 'p' || ch == 'P') paused = !paused;      // 暂停
                if (ch == 'r' || ch == 'R') { alive = false; }      // 重新开始（死亡标识）
                if (ch == 27) { alive = false; quitToMenu = true; } // ESC：回菜单
            }
        }
    }

    // ========== 更新 ==========
    void update() {
        if (!alive || paused) return;

        // 新头
        Point head = body[0];
        if (dir == UP) head.y--;
        else if (dir == DOWN) head.y++;
        else if (dir == LEFT) head.x--;
        else head.x++;

        bool ate = (head.x == food.x && head.y == food.y);

        // 撞墙
        if (head.x < 0 || head.x >= W || head.y < 0 || head.y >= H) { alive = false; return; }
        // 撞自身（排除尾巴，因为没吃到食物时尾巴会消失）
        for (size_t i = 0; i < body.size() - (ate ? 0 : 1); i++)
            if (body[i].x == head.x && body[i].y == head.y) { alive = false; return; }

        // 插入新头
        body.insert(body.begin(), head);
        if (ate) {
            score += 10;
            foodEaten++;
            if (foodEaten % 5 == 0 && speed > 60) speed -= 10;
            placeFood();
        } else {
            body.pop_back();
        }
    }

    // ========== 游戏结束画面 ==========
    void gameOverScreen() {
        system("cls");
        Color(15);
        cout << "\n\n";
        cout << "    +============================+\n";
        cout << "    |                            |\n";
        cout << "    |        ";
        Color(12); cout << "G A M E   O V E R";
        Color(15); cout << "        |\n";
        cout << "    |                            |\n";
        Color(11);
        printf("    |    Final Score: %-10d   |\n", score);
        printf("    |    Snake Length: %-9d   |\n", (int)body.size());
        Color(15);
        cout << "    |                            |\n";
        cout << "    +============================+\n\n";
        Color(14);
        cout << "      R = Restart      ESC = Menu\n";
        Color(7);
    }

    // ========== 主菜单 ==========
    void mainMenu() {
        system("cls");
        Color(11);
        cout << "\n\n";
        cout << "    +=============================+\n";
        cout << "    |                             |\n";
        Color(14);
        cout << "    |       SNAKE  GAME           |\n";
        Color(11);
        cout << "    |                             |\n";
        cout << "    +=============================+\n\n";
        Color(15);
        cout << "    1.  Start Game\n";
        cout << "    2.  Help\n";
        cout << "    0.  Quit\n\n";
        Color(7);
        cout << "    Choice: ";
    }

    void helpScreen() {
        system("cls");
        Color(15);
        cout << "\n    HELP\n\n";
        cout << "  Arrow keys / WASD  =  Move\n";
        cout << "  P                   =  Pause\n";
        cout << "  R                   =  Restart\n";
        cout << "  ESC                 =  Menu\n\n";
        cout << "  Eat red () to grow.\n";
        cout << "  Avoid walls and your own body.\n\n";
        Color(7);
        cout << "  Press any key to return...";
        _getch();
    }

public:
    void run() {
        system("title Snake Game");
        system("mode con cols=55 lines=27");

        while (true) {
            mainMenu();
            char choice = _getch();
            if (choice == '1') {
                init();
                while (alive) {
                    drawAll();
                    input();
                    update();
                    if (paused) { Sleep(100); continue; }
                    Sleep(speed);
                }
                if (quitToMenu) { quitToMenu = false; continue; } // ESC 回菜单
                gameOverScreen();
                while (true) {
                    int c = _getch();
                    if (c == 'r' || c == 'R') break;
                    if (c == 27) break;
                }
            } else if (choice == '2') {
                helpScreen();
            } else if (choice == '0') {
                system("cls");
                Color(7);
                cout << "\n\n  Goodbye!\n\n";
                break;
            }
        }
        ShowCursor();
    }
};

int main() {
    Snake snake;
    snake.run();
    return 0;
}
