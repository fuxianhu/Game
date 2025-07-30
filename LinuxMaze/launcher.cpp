#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <filesystem>
using namespace std;

inline void clear() {
    // \033[2J 清除整个屏幕
    // \033[H 将光标移动到左上角
    cout << "\033[2J\033[H";
    cout.flush();  // 确保立即输出
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

int main() {

    
    char ch;
    while (true) {
        clear();
        std::cout << "=== 游戏启动器 ===" << std::endl;
        std::cout << "【空格】启动游戏" << std::endl;
        std::cout << "【Esc】退出" << std::endl;
        ch = getch();
        
        if (ch == ' ') {
            std::cout << "正在启动游戏..." << std::endl;
            
            // 使用system()启动新终端并等待游戏结束
            int status = system("gnome-terminal -- bash -c './main'");
            
            // 检查状态
            if (status == -1) {
                (void)system(("firefox " + (string)(filesystem::current_path() / "UKE.png")).c_str());
                std::cout << "无法启动游戏" << std::endl;
                getch();
            } else {
            }
        }
        else if (ch == 27) {
            return 0;
        }
    }
    
    return 0;
}