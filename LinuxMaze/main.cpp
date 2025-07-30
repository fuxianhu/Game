/*

迷宫游戏 Maze Game

项目主文件，若要启动游戏，请运行launcher。
启动器代码在launcher.cpp中。

*/

#define GAME_VERSION "0.1.0.20250730_Alpha"


#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <ctime>
#include <utility>
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>


using namespace std;


inline void clear() {
    // \033[2J 清除整个屏幕
    // \033[H 将光标移动到左上角
    cout << "\033[2J\033[H";
    cout.flush();  // 确保立即输出
}

int getRandomNumber(int min, int max) {
	random_device rd; // 用于获取随机数种子
	mt19937 gen(rd()); // 以随机数种子初始化随机数生成器
	uniform_int_distribution<> dis(min, max); // 创建一个生成指定范围内整数的均匀分布
	return dis(gen); // 生成并返回随机数
}


char getch() {
    struct termios oldt, newt;
    char ch;
    
    // 保存当前终端设置
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    
    // 禁用行缓冲和回显
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // 读取单个字符
    ch = getchar();
    
    // 恢复原始终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return ch;
}

// 迷宫尺寸（可修改，建议使用奇数）
int width = 21;
int height = 21;

// 迷宫单元格状态
// C++11 引入的 enum class（强类型枚举）
// 传统枚举存在一些问题（如隐式转换为整型，作用域污染等）
enum class Cell {
    WALL = 0,
    PATH = 1,
    START = 2,
    END = 3,
    NIGHT_VISION_POTION = 4, // 夜视药水
    SWIFTNESS_POTION = 5, // 迅捷药水
    TNT = 6
};




class Maze {
    // 迷宫类


    // -------------------- 变量与常量 --------------------
public:
    int playerNumber = 1;            // 玩家数量
private:
    
    const int dx[4] = {0, 1, 0, -1}; // X 方向数组
    const int dy[4] = {-1, 0, 1, 0}; // Y 方向数组
    vector<int> visibility;          // range of visibility 能见度范围 视野 视距
    int TNTEquivalent = 2;           // TNT当量（可以炸毁此半径的正方形）
    int maxVisibility = 5;           // 最大视野距离
    vector<int> swifthessTime;       // 迅捷效果剩余时间
    vector<int> tntNumber;           // 玩家拥有的TNT数量
    vector<int> playerX;
    vector<int> playerY;             // 玩家坐标（X、Y轴的值）
    int swifthessPotionTime = 20;    // 迅捷药水作用时间
    vector<vector<Cell>> grid;       // 地图
    pair<int, int> start;            // 起点坐标
    pair<int, int> end;              // 终点坐标
    chrono::minutes level_time_limit = chrono::minutes(10);      // 时间限制功能的参数
    chrono::time_point<chrono::system_clock> startTime, endTime; // 游戏开始时间和必须要结束的时间（不是实际结束的世界）

    // -------------------- 函数 --------------------
public:
    bool inBounds(int x, int y) {
        // 检查坐标是否在迷宫范围内
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    // 获取相邻的墙
    vector<pair<int, int>> getAdjacentWalls(int x, int y) {
        vector<pair<int, int>> walls;
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (inBounds(nx, ny) && grid[ny][nx] == Cell::WALL) {
                walls.emplace_back(nx, ny);
            }
        }
        return walls;
    }


    // Maze() : grid(height, vector<Cell>(width, Cell::WALL)), visibility(2, 1), tntNumber(2, 0), swifthessTime(2, 0) {
        
    // }
    // name("Default"), 
    // Maze() : grid(height, vector<Cell>(width, Cell::WALL)) {}
    Maze() : grid(height, vector<Cell>(width, Cell::WALL)) {
        visibility = vector<int>(2, 1);      // 默认2个玩家，初始视野1
        tntNumber = vector<int>(2, 0);       // 默认2个玩家，初始TNT数量0
        swifthessTime = vector<int>(2, 0);   // 默认2个玩家，初始迅捷时间0
        playerX = vector<int>(2, 0);         // 默认2个玩家
        playerY = vector<int>(2, 0);         // 默认2个玩家
    }

    // 生成迷宫
    void generate() {
        visibility = {1, 1};
        tntNumber = {0, 0};
        swifthessTime = {0, 0};



        // 初始化随机数生成器
        mt19937 rng(time(nullptr));

        // 选择随机起点（确保在奇数位置）
        uniform_int_distribution<int> distX(1, (width - 1) / 2);
        uniform_int_distribution<int> distY(1, (height - 1) / 2);
        start.first = distX(rng) * 2 - 1;
        start.second = distY(rng) * 2 - 1;

        grid[start.second][start.first] = Cell::PATH;

        // 使用队列存储边界墙
        queue<pair<int, int>> walls;
        for (auto wall : getAdjacentWalls(start.first, start.second)) {
            walls.push(wall);
        }

        while (!walls.empty()) {
            // 随机选择一个墙
            int randomIndex = uniform_int_distribution<int>(0, walls.size()-1)(rng);
            queue<pair<int, int>> temp;
            for (int i = 0; i < randomIndex; ++i) {
                temp.push(walls.front());
                walls.pop();
            }
            pair<int, int> wall = walls.front();
            walls.pop();
            while (!temp.empty()) {
                walls.push(temp.front());
                temp.pop();
            }

            int wx = wall.first, wy = wall.second;putchar(' '), putchar(' ');

            // 检查是否可以打通这面墙
            int pathCount = 0;
            pair<int, int> oppositeCell;
            for (int i = 0; i < 4; ++i) {
                int nx = wx + dx[i];
                int ny = wy + dy[i];
                if (inBounds(nx, ny) && grid[ny][nx] == Cell::PATH) {
                    pathCount++;
                    oppositeCell = make_pair(wx - dx[i], wy - dy[i]);
                }
            }

            // 如果只有一条路径相邻，则打通
            if (pathCount == 1) {
                grid[wy][wx] = Cell::PATH;
                if (inBounds(oppositeCell.first, oppositeCell.second)) {
                    grid[oppositeCell.second][oppositeCell.first] = Cell::PATH;
                    // 添加新的墙到边界
                    for (auto newWall : getAdjacentWalls(oppositeCell.first, oppositeCell.second)) {
                        walls.push(newWall);
                    }
                }
            }
        }

        // 设置起点
        grid[start.second][start.first] = Cell::START;
        for (int i = 0; i < playerNumber; i++) {
            playerY[i] = start.second, playerX[i] = start.first;
        }
        
        // 找到离起点最远的点作为终点
        vector<vector<int>> distances(height, vector<int>(width, -1));
        queue<pair<int, int>> q;
        q.push(start);
        distances[start.second][start.first] = 0;
        pair<int, int> farthest = start;

        while (!q.empty()) {
            auto current = q.front();
            q.pop();

            for (int i = 0; i < 4; ++i) {
                int nx = current.first + dx[i];
                int ny = current.second + dy[i];
                if (inBounds(nx, ny) && grid[ny][nx] != Cell::WALL && distances[ny][nx] == -1) {
                    distances[ny][nx] = distances[current.second][current.first] + 1;
                    q.emplace(nx, ny);
                    if (distances[ny][nx] > distances[farthest.second][farthest.first]) {
                        farthest = make_pair(nx, ny);
                    }
                }
            }
        }

        end = farthest;
        grid[end.second][end.first] = Cell::END;
    }

    void getItem(int y, int x, int player) {
        if (grid[y][x] == Cell::NIGHT_VISION_POTION) {
            grid[y][x] = Cell::PATH;
            visibility[player] = min(visibility[player] + 1, maxVisibility);
        }
        else if (grid[y][x] == Cell::SWIFTNESS_POTION) {
            grid[y][x] = Cell::PATH;
            swifthessTime[player] = swifthessPotionTime;
        }
        else if (grid[y][x] == Cell::TNT) {
            grid[y][x] = Cell::PATH;
            tntNumber[player]++;
        }
    }

    // 打印迷宫
    void print() {
        clear();

        // 随机刷新夜视药水
        // 大地图玩家很难找到药水，所以概率要更高
        if (getRandomNumber(1, (width + height) / 5) == 1) {
            int y = getRandomNumber(0, height - 1);
            int x = getRandomNumber(0, width - 1);
            if (grid[y][x] == Cell::PATH) {
                grid[y][x] = Cell::NIGHT_VISION_POTION;
            }
        }

        if (getRandomNumber(1, (width + height) / 1.5) == 1) {
            int y = getRandomNumber(0, height - 1);
            int x = getRandomNumber(0, width - 1);
            if (grid[y][x] == Cell::PATH) {
                grid[y][x] = Cell::SWIFTNESS_POTION;
            }
        }

        if (getRandomNumber(1, width + height) == 1) {
            int y = getRandomNumber(0, height - 1);
            int x = getRandomNumber(0, width - 1);
            if (grid[y][x] == Cell::PATH) {
                grid[y][x] = Cell::TNT;
            }
        }
        bool tag, printTag;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                tag = true;
                printTag = false;
                for (int k = 0; k < playerNumber; k++) {
                    if (printTag) {
                        if (y == playerY[k] && x == playerX[k] && grid[y][x] != Cell::START) {
                            putchar(k + '1'), putchar(' ');
                        }
                        continue;
                    }
                    
                    if (playerY[k] - visibility[k] <= y && playerY[k] + visibility[k] >= y && playerX[k] - visibility[k] <= x && playerX[k] + visibility[k] >= x) {
                        tag = false;
                        printTag = true;
                        if (y == playerY[k] && x == playerX[k]) {
                            int cnt = 0;
                            for (int l = k; l < playerNumber; l++) {
                                if (y == playerY[l] && x == playerX[l]) {
                                    cnt++;
                                    if (cnt >= 2) {
                                        break;
                                    }
                                }
                            }
                            if (cnt >= 2 || playerNumber == 1) {
                                putchar('%'), putchar(' ');
                            }
                            else {
                                putchar(k + '1'), putchar(' ');
                            }
                            getItem(y, x, k);
                            continue;
                        }
                        switch (grid[y][x]) {
                            case Cell::WALL:
                                printf("██");
                                break;
                            case Cell::PATH:
                                putchar(' '), putchar(' ');
                                break;
                            case Cell::START:
                                putchar('S'), putchar(' ');
                                break;
                            case Cell::END:
                                putchar('E'), putchar(' ');
                                break;
                            case Cell::NIGHT_VISION_POTION:
                                if (visibility[k] < maxVisibility) {
                                    putchar('N'), putchar(' ');
                                }
                                else {
                                    grid[y][x] = Cell::PATH;
                                    putchar(' '), putchar(' ');
                                }
                                break;
                            case Cell::SWIFTNESS_POTION:
                                putchar('W'), putchar(' ');
                                break;
                            case Cell::TNT:
                                putchar('T'), putchar(' ');
                                break;
                        }
                    }
                }
                if (tag) {
                    putchar('?'), putchar('?');
                }
                bool rn = getRandomNumber(1, 20 * (((width + height) / 2) / 5)) == 1;
                if (grid[y][x] == Cell::NIGHT_VISION_POTION && rn) {
                    grid[y][x] = Cell::PATH;
                }
                else if (grid[y][x] == Cell::SWIFTNESS_POTION && rn) {
                    grid[y][x] = Cell::PATH;
                }
                else if (grid[y][x] == Cell::TNT && rn) {
                    grid[y][x] = Cell::PATH;
                }
            }
            putchar('\n');
        }

        auto remaining = endTime - chrono::system_clock::now();

        // 分别提取小时、分钟、秒
        auto hours = chrono::duration_cast<chrono::hours>(remaining);
        remaining -= hours;
        auto minutes = chrono::duration_cast<chrono::minutes>(remaining);
        remaining -= minutes;
        auto seconds = chrono::duration_cast<chrono::seconds>(remaining);

        // 格式化输出为HH:MM:SS
        cout << "剩余时间: " 
            << setfill('0') << setw(2) << hours.count() << ":"
            << setw(2) << minutes.count() << ":" 
            << setw(2) << seconds.count() << '\n';
        
        for (int i = 0; i < playerNumber; i++) {
            cout << " === 玩家" << i + 1 << " ===\n";
            cout << "视野等级：" << visibility[i] << "/5\n";
            if (swifthessTime[i] != 0) {
                cout << "迅捷：" << swifthessTime[i] << endl;
            }
            if (tntNumber[i] != 0) {
                cout << "TNT：" << tntNumber[i] << endl;
            }
            putchar('\n');
        }
    }

    inline void playerMove(int d, int player) {
        /*
        处理玩家移动的逻辑
        d 代表按键 方向
        0    w    上
        1    a    左
        2    s    下
        3    d    右
        */
        const bool NOT_WALL = grid[playerY[player] + dy[d]][playerX[player] + dx[d]] != Cell::WALL;

        if (inBounds(playerX[player] + dx[d], playerY[player] + dy[d])) {
            if (swifthessTime[player] && inBounds(playerX[player] + (dx[d] * 2), playerY[player] + (dy[d] * 2)) && getRandomNumber(1, 2) == 1) {
                if (grid[playerY[player] + (dy[d] * 2)][playerX[player] + (dx[d] * 2)] != Cell::WALL && NOT_WALL) {
                    playerX[player] += dx[d] * 2, playerY[player] += dy[d] * 2;
                }
                else if (NOT_WALL){
                    playerX[player] += dx[d], playerY[player] += dy[d];
                }
                swifthessTime[player]--;
            } else {
                if (NOT_WALL) {
                    playerX[player] += dx[d], playerY[player] += dy[d];
                }
            }
        }
    }

    void DetonateTNT(int player) {
        if (tntNumber[player] != 0) {
            tntNumber[player]--;
            for (int i = max(playerX[player] - 2, 0); i <= min(playerX[player] + 2, width - 1); i++) {
                for (int j = max(playerY[player] - 2, 0); j <= min(playerY[player] + 2, height - 1); j++) {
                    if (grid[j][i] != Cell::START && grid[j][i] != Cell::END) {
                        grid[j][i] = Cell::PATH;
                    }
                }
            }
        }
    }

    void game() {
        // 获取当前时间点
        startTime = chrono::system_clock::now();
        level_time_limit = chrono::minutes((width + height) / 4);
        endTime = startTime + level_time_limit;

        while (true) {
            if (chrono::system_clock::now() >= endTime) {
                cout << "\nTLE！超出时间限制！逃离失败！" << endl;
                (void)system(("firefox " + (string)(filesystem::current_path() / "TLE.png")).c_str());
                cout << "按任意键继续..." << endl;
                (void)getch();
                break;
            }
            print();
            bool tag = false;
            for (int i = 0; i < playerNumber; i++) {
                if (playerY[i] == end.second && playerX[i] == end.first) {
                    cout << "\n恭喜玩家" << i + 1 << "成功逃出迷宫！" << endl;
                    cout << "按任意键继续..." << endl;
                    (void)system(("firefox " + (string)(filesystem::current_path() / "gongxi.jpeg")).c_str());
                    (void)getch();
                    tag = true;
                    break;
                }
            }
            if (tag) {
                break;
            }
            char ch = getch();
            switch(ch) {
                case 'w':
                    playerMove(0, 0);
                    break;
                case 'd':
                    playerMove(1, 0);
                    break;
                case 's':
                    playerMove(2, 0);
                    break;
                case 'a':
                    playerMove(3, 0);
                    break;
                case 'i':
                    playerMove(0, 1);
                    break;
                case 'l':
                    playerMove(1, 1);
                    break;
                case 'k':
                    playerMove(2, 1);
                    break;
                case 'j':
                    playerMove(3, 1);
                    break;
                case 'q':
                    DetonateTNT(0);
                    break;
                case 'u':
                    DetonateTNT(1);
                    break;
            }
        }
    }
};

void settings() {
    while (true) {
        clear();
        int x;
        cout << " ---设置--- " << endl << endl;
        cout << "[0] 宽度：" << width << endl;
        cout << "[1] 高度：" << height << endl;
        cout << "[Esc] 返回" << endl;
        char ch = getch();
        if (ch == 27) {
            return;
        }
        else if (ch == '0') {
            clear();
            cout << "请输入新的宽度（自动调整为奇数）：";
            cin >> x;
            if (x < 10 || x > 100) {
                cout << "数值必须大于等于10且小于等于100！按任意键返回...";
                (void)getch();
                (void)getch();
                continue;
            }
            if (x % 2 == 0) {
                x++;
            }
            width = x;
        }
        else if (ch == '1') {
            clear();
            cout << "请输入新的高度（自动调整为奇数）：";
            cin >> x;
            if (x < 10 || x > 100) {
                cout << "数值必须大于等于10且小于等于100！按任意键返回...";
                (void)getch();
                (void)getch();
                continue;
            }
            if (x % 2 == 0) {
                x++;
            }
            height = x;
        }
    }
}

int main() {
    while (true) {
        clear();
        cout << "欢迎游玩迷宫！" << endl;
        cout << "迷宫游戏版本：" << GAME_VERSION << endl << endl;

        std::string filePath = "doc.txt";
        std::ifstream inputFile(filePath);
        if (!inputFile) {
            std::cerr << "无法打开文件: " << filePath << std::endl;
            return 1;
        }
        std::stringstream buffer;  // 一次性读取整个文件
        buffer << inputFile.rdbuf();
        std::cout << buffer.str(); // 输出文件内容
        inputFile.close();
        cout << endl << "------------------------------" << endl << endl;
        cout << "等待您的按键...";
        char ch = getch();
        if (ch == ' ') {
            Maze maze;
            maze.playerNumber = 1;
            maze.generate();
            maze.game();
        }
        else if (ch == '2') {
            Maze maze;
            maze.playerNumber = 2;
            maze.generate();
            maze.game();
        }
        else if (ch == 't') {
            settings();
        }
        else if (ch == 27) {
            putchar('\n');
            break;
        }
    }
    return 0;
}