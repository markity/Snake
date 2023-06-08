#include <ncurses.h>
#include <list>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <ctime>

constexpr int N_FOOD = 5;            // 蛇蛋数目
constexpr int N_BODY_INIT = 6;       // 起始的蛇身长度
constexpr int MIN_HEIGHT = 10;       // 窗口最小的高度
constexpr int MIN_WIDTH = 10;        // 矿口最小的宽度
constexpr int UPDATE_INTERVAL = 100; // 定时器时间, 单位ms, 更新蛇的位置
constexpr int UPDATE_INTERVAL_MIN = 50;

// 头 <- 身子 <- 身子
// 蛇身或尾巴(吃蛋的时候可能发生)碰到墙壁都会狗带

class SnakeNode
{
public:
    SnakeNode(int y_, int x_) : y(y_), x(x_) {}
    virtual void update() = 0;
    int y, x;
};

// 蛇的身体节点
class SnakeBodyNode final : public SnakeNode
{
public:
    SnakeBodyNode(int y_, int x_, SnakeNode *before_) : SnakeNode(y_, x_), before(before_) {}
    // 蛇身节点: 继承上下一级对象的位置
    void update() override
    {
        this->y = before->y;
        this->x = before->x;
    }
    SnakeNode *before;
};

// 蛇的头节点
class SnakeHeadNode final : public SnakeNode
{
public:
    SnakeHeadNode(int y_, int x_, char direction_) : SnakeNode(y_, x_), direction(direction_) {}
    void update() override
    {
        switch (direction)
        {
        case 'w':
            y--;
            break;
        case 's':
            y++;
            break;
        case 'a':
            x--;
            break;
        case 'd':
            x++;
            break;
        }
    }
    // w表示上, s代表下, a代表左, d代表右
    char direction;
};

class Snake
{
public:
    Snake(WINDOW *w, int y_, int x_, char direction, int nBodys) : nodes(), win(w)
    {
        nodes.push_back(new SnakeHeadNode(y_, x_, direction));
        switch (direction)
        {
        case 'w':
            for (size_t i = 0; i < nBodys; i++)
            {
                y_--;
                nodes.push_back(new SnakeBodyNode(y_, x_, nodes.back()));
            }
            break;
        case 's':
            for (size_t i = 0; i < nBodys; i++)
            {
                y_++;
                nodes.push_back(new SnakeBodyNode(y_, x_, nodes.back()));
            }
            break;
        case 'a':
            for (size_t i = 0; i < nBodys; i++)
            {
                x_++;
                nodes.push_back(new SnakeBodyNode(y_, x_, nodes.back()));
            }
            break;
        case 'd':
            for (size_t i = 0; i < nBodys; i++)
            {
                x_--;
                nodes.push_back(new SnakeBodyNode(y_, x_, nodes.back()));
            }
            break;
        }

        // 初始化蛇蛋 3个
        for (size_t i = 0; i < N_FOOD; i++)
        {
            int y, x;
            while (1)
            {
                y = rand() % win->_maxy;
                x = rand() % win->_maxx;
                // 不与节点冲突
                for (auto &i : nodes)
                {
                    if (i->y == y && i->x == x)
                    {
                        continue;
                    }
                }
                // 不与之前的蛋冲突
                for (auto &i : eggs)
                {
                    if (i.first == y && i.second == x)
                    {
                        continue;
                    }
                }
                eggs.push_back({y, x});
                break;
            }
        }
    };

    // 每隔一段事件, 调用定时器, 非阻塞
    void run()
    {
        std::thread([&]()
                    {
                        while (1)
                        {
                            // 绘图
                            wclear(win);

                            // 画蛇蛋
                            for (auto &i : eggs)
                            {
                                move(i.first, i.second);
                                waddch(win, 'O');
                            }
                            
                            nodesLock.lock();

                            auto head = *nodes.begin();
                            for (auto b = nodes.begin(), e = nodes.end(); b != e; b++)
                            {
                                // // 检查是否触碰边界
                                // if((*b)->y > LINES-1 || (*b)->y < 0  || (*b)->x > COLS-1 || (*b)->x < 0) {
                                //     nodesLock.unlock();
                                //     wclear(win);
                                //     wprintw(win, "Game over!");
                                //     wrefresh(win);
                                //     return;
                                // }

                                if ((*b)->y > LINES-1) {
                                    (*b)->y = 0;
                                }
                                if ((*b)->y < 0) {
                                    (*b)->y = LINES - 1;
                                }
                                if ((*b)->x < 0) {
                                    (*b)->x = COLS - 1;
                                }
                                if ((*b)->x > COLS-1) {
                                    (*b)->x = 0;
                                }

                                move((*b)->y, (*b)->x);

                                if (b == nodes.begin())
                                {
                                    // 检查是否触碰到蛇蛋
                                    bool insert = false;
                                    int insertY, insertX;
                                    for (auto beg = eggs.begin();beg != eggs.end();beg++)
                                    {
                                        if(head->y == (*beg).first && head->x == (*beg).second) {
                                            // 移除蛇蛋
                                            eggs.erase(beg);
                                            int y, x;
                                            while(1) {
                                                y = rand() % win->_maxy;
                                                x = rand() % win->_maxx;
                                                // 不与节点冲突
                                                for (auto &i : nodes)
                                                {
                                                    if (i->y == y && i->x == x)
                                                    {
                                                        continue;
                                                    }
                                                }
                                                // 不与之前的蛋冲突
                                                for (auto &i : eggs)
                                                {
                                                    if (i.first == y && i.second == x)
                                                    {
                                                        continue;
                                                    }
                                                }
                                                eggs.push_back({y, x});
                                                break;
                                            }
                                            insert = true;
                                            break;
                                        }
                                    }
                                    if (insert)
                                    {
                                        // 最后一项
                                        auto last = nodes.end(); last --;
                                        // 倒数第二项
                                        auto last_2 = nodes.end();last_2 --; last_2--;
                                        if((*last)->y - (*last_2)->y == 1) {
                                            insertX = (*last)->x;
                                            insertY = (*last)->y + 1;
                                        }
                                        if((*last)->y - (*last_2)->y == -1) {
                                            insertX = (*last)->x;
                                            insertY = (*last)->y - 1;
                                        }
                                        if((*last)->x - (*last_2)->x == 1) {
                                            insertY = (*last)->y;
                                            insertX = (*last)->x + 1;
                                        }
                                        if((*last)->x - (*last_2)->x == -1) {
                                            insertY = (*last)->y;
                                            insertX = (*last)->x - 1;
                                        }
                                        nodes.push_back(new SnakeBodyNode(insertY, insertX, *last));
                                    }
                                    
                                    waddch(win, 'O');
                                } else {
                                    // 检查是否触碰蛇身
                                    if((*b)->y == head->y && (*b)->x == head->x) {
                                        nodesLock.unlock();
                                        wclear(win);
                                        wprintw(win, "Game over!");
                                        wrefresh(win);
                                        return;
                                    }

                                    waddch(win, '#');
                                }
                            }
                            nodesLock.unlock();
                            wrefresh(win);


                            while (1) {
                                nodesLock.lock();
                                auto f = fast;
                                auto s = stop;
                                nodesLock.unlock();

                                // 休息
                                if (!f) {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL));
                                } else {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL_MIN));
                                }

                                if (s) {
                                    continue;
                                }
                                break;
                            }

                            // 更新下一帧的链表数据
                            nodesLock.lock();
                            for (auto b = nodes.rbegin(), e = nodes.rend(); b != e; b++)
                            {
                                (*b)->update();
                            } 
                            nodesLock.unlock(); 
                        } }).detach();
    }

    void changeDirection(char direction)
    {
        nodesLock.lock();
        auto head = dynamic_cast<SnakeHeadNode *>(nodes.front());

        if (direction == 'w' && head->direction != 's')
        {
            head->direction = 'w';
        }
        if (direction == 's' && head->direction != 'w')
        {
            head->direction = 's';
        }
        if (direction == 'a' && head->direction != 'd')
        {
            head->direction = 'a';
        }
        if (direction == 'd' && head->direction != 'a')
        {
            head->direction = 'd';
        }
        nodesLock.unlock();
    }

    void stopOrStart() {
        nodesLock.lock();
        stop = !stop;
        nodesLock.unlock();
    }

    void changeSpeed() {
        nodesLock.lock();
        fast = !fast;
        nodesLock.unlock();
    }

    ~Snake()
    {
        for (auto &i : nodes)
        {
            delete i;
        }
    }

    WINDOW *win;

    std::mutex nodesLock;
    std::list<SnakeNode *> nodes;

    std::vector<std::pair<int, int>> eggs;
    bool fast = false;
    bool stop = false;
};

int main()
{
    auto main_win = initscr();
    wrefresh(main_win);
    noecho();
    raw();
    curs_set(0);
    srand(unsigned(time(NULL)));

    // 检查窗口大小是否合适
    if (LINES < MIN_HEIGHT || COLS < MIN_WIDTH)
    {
        endwin();
        printf("screen size too small, less than %d in height or %d in width\n", MIN_HEIGHT, MIN_WIDTH);
        return -1;
    }

    Snake s(main_win, LINES / 2, COLS / 2, 'd', N_BODY_INIT);

    // refresh只由run里面创建的线程调用, 对绘图库线程安全
    s.run();

    // 主线程负责监听键盘事件
    keypad(main_win, TRUE);
    int tempC;
    while (1)
    {
        tempC = getch();
        switch (tempC)
        {
        case KEY_UP:
        case 'w':
        case 'W':
            s.changeDirection('w');
            break;
        case KEY_DOWN:
        case 's':
        case 'S':
            s.changeDirection('s');
            break;
        case KEY_LEFT:
        case 'a':
        case 'A':
            s.changeDirection('a');
            break;
        case KEY_RIGHT:
        case 'd':
        case 'D':
            s.changeDirection('d');
            break;
        case 'q':
        case 'Q':
            goto end;
            break;
        case 'p':
        case 'P':
            s.stopOrStart();
            break;
        case 32:
            s.changeSpeed();
        }
    }

end:
    endwin();
}
