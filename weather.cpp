#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

#define BUFFER_SIZE 256
#define SEND_DATA_LEN 30

// socket文件描述符
int sockfd;
struct sockaddr_in servaddr;

// 发送/接收缓冲区
char sendbuffer[BUFFER_SIZE], recvbuffer[BUFFER_SIZE]; 

// 询问城市的名字
string city_name;

// 发送报文的格式
struct SendPacket {
    char page, query_type; // 分别表示位于哪一个page, 询问类型
    char data[SEND_DATA_LEN]; // 传输询问的城市名称
    char day_num; // 传输询问的日期
} __attribute__((aligned(1)));

// 接收报文的格式
struct RecvPacket {
    char retval, reply_type; // 返回值，返回的询问类型
    char data[SEND_DATA_LEN]; // 传输城市名称
    char date[4]; // 传输当前日期
    char day_num; // 传输日期数量
    char weather[6]; // 传输天气/温度信息
    char useless[84]; // 占位
} __attribute__((aligned(1)));

// 用包的指针指向缓冲区，可以直接修改缓冲区中对应偏移量的内容
SendPacket *sndpkt = (SendPacket *)sendbuffer;
RecvPacket *rcvpkt = (RecvPacket *)recvbuffer;

// 初始化
void init() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        cerr << "Error: Failed to create socket." << endl;
        exit(0);
    }
    // 设置server address: 47.105.85.28:4321
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(4321);
    servaddr.sin_addr.s_addr = inet_addr("47.105.85.28");
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
        cerr << "Error: Failed to connect to server." << endl;
        exit(0);
    }
}

// 发送包
int sendPacket(char page, char query_type, const char *data, char day_num) {
    // 为对应字段赋值
    sndpkt->page = page;
    sndpkt->query_type = query_type;
    strncpy(sndpkt->data, data, sizeof(sndpkt->data) - 1);
    sndpkt->data[SEND_DATA_LEN] = 0x00;
    sndpkt->day_num = day_num;
    return send(sockfd, sendbuffer, sizeof(SendPacket), 0);
}

// 接收包
int recvPacket(){
    if (recv(sockfd, recvbuffer, sizeof(RecvPacket), 0) <= 0) {
        return false;
    }
    return true;
}

// enum Weather {
//     overcast = 0,
//     sunny = 1,
//     cloudy = 2,
//     rain = 3,
//     fog = 4,
//     rainstorm = 5, 
//     thunderstorm = 6,
//     breeze = 7,
//     sandstorm = 8
// };

// 天气映射到字符串
string weather_name[] = {
    "overcast", 
    "sunny", 
    "cloudy",
    "rain",
    "fog",
    "rainstorm",
    "thunderstorm",
    "breeze",
    "Please guess: ZmxhZ3tzYW5kX3N0MHJtfQ==" // sand_st0rm
};

// 给参数，打印一行天气信息
void print_weather(int day_num, int weather, int temperature, bool is_TypeA = true) {
    if (day_num == 1 && is_TypeA) cout << "Today's Weather is: ";
    else cout << "The " << day_num << "th day's Weather is: ";
    cout << weather_name[weather] << ";  Temp:" << setw(2) << setfill('0') << temperature << endl;
}

// 打印缓冲区中全部天气信息
void display_weather() {
    // date[4] 中前两个字节表示年，后两个字节分别表示月，日
    int year = (unsigned)rcvpkt->date[0] * 256 + (unsigned char)rcvpkt->date[1];
    int month = rcvpkt->date[2];
    int day = rcvpkt->date[3];
    cout << "City: " << city_name << "  Today is: ";
    cout << year << "/" << setw(2) << setfill('0') << month << "/" << setw(2) << setfill('0') << day;
    cout << "  Weather information is as follows:" << endl;
    // reply_type A 只有一天天气信息， B 有三天天气信息
    if (rcvpkt->reply_type == 'A') {
        print_weather(rcvpkt->day_num, rcvpkt->weather[0], rcvpkt->weather[1], true);
    }
    else {
        for (char i = 0x01; i <= rcvpkt->day_num; i++) {
            print_weather(i, rcvpkt->weather[2 * i - 2], rcvpkt->weather[2 * i - 1], false);
        }
    }
}

// 读取 query() 中自定义天气日期
int get_day() {
    cout << "Please enter the day number(below 10, e.g. 1 means today):";
    string str;
    cin >> str;
    if (str.size() == 1) {
        char ch = str[0];
        if ('1' <= ch && ch <= '9') return ch - '0';
    }
    return -1;
}

// 读取一行，获取其中第一个字符串
string get_input() {
    string line, city_name;
    getline(cin, line);
    stringstream ss(line);
    ss >> city_name;
    return city_name;
}

// 第二个界面发生在从server处成功获取城市名称后
int query() {
    system("clear");
    cout << "Please enter the given number to query" << endl;
    cout << "1.today" << endl;
    cout << "2.three days from today" << endl;
    cout << "3.custom day by yourself" << endl;
    cout << "(r)back,(c)cls,(#)exit" << endl;
    cout << "===================================================" << endl;
    while (1) {
        string str = get_input();
        if (str == "c") {
            system("clear");
            cout << "Please enter the given number to query" << endl;
            cout << "1.today" << endl;
            cout << "2.three days from today" << endl;
            cout << "3.custom day by yourself" << endl;
            cout << "(r)back,(c)cls,(#)exit" << endl;
            cout << "===================================================" << endl;
        }
        else if (str == "r") { // 返回main, 自动进入另一个界面
            return 0;
        }
        else if (str == "#") {
            close(sockfd);
            exit(0);
        }
        else if (str == "1" || str == "2" || str == "3") {
            char op = str[0];
            // 发包
            switch (op) {
                case '1': {
                    sendPacket(0x02, 0x01, city_name.c_str(), 0x01);
                } break;
                case '2': {
                    sendPacket(0x02, 0x02, city_name.c_str(), 0x03);
                } break;
                case '3': {
                    while (int day = get_day()) {
                        if (day == -1) {
                            cout << "input error!" << endl;
                        }
                        else {
                            sendPacket(0x02, 0x01, city_name.c_str(), day);
                            break;
                        }
                    }
                } break;
                default: break;
            }
            // 收包
            while (!recvPacket()) ;
            if (rcvpkt->retval == 0x01) {
                return 0;
            }
            else if (rcvpkt->retval == 0x03) {
                display_weather();
            }
            else if (rcvpkt->retval == 0x04) {
                cout << "Sorry, no given day's weather information for city " << city_name << "!" << endl;
            }
        }
        else if (str.size() > 0) {
            cout << "input error!" << endl;
        }
    }

}

// 初始界面，读取一个城市的名字，发给server
int hello() {
    system("clear");
    cout << "Welcome to NJUCS Weather Forecast Demo Program!" << endl;
    cout << "Please input City Name in Chinese pinyin(e.g. nanjing or beijing)" << endl;
    cout << "(c)cls,(#)exit" << endl;
    while (1) {
        // 从键盘读取输入
        city_name = get_input();
        if (city_name == "c") {
            system("clear");
            cout << "Welcome to NJUCS Weather Forecast Demo Program!" << endl;
            cout << "Please input City Name in Chinese pinyin(e.g. nanjing or beijing)" << endl;
            cout << "(c)cls,(#)exit" << endl;
        }
        else if (city_name == "#") {
            close(sockfd);
            exit(0);
        }
        else if (city_name.size() > 0) {
            // 发包
            sendPacket(0x01, 0x00, city_name.c_str(), 0x00);
            memset(recvbuffer, 0, sizeof(recvbuffer));
            // 收包
            while (!recvPacket());
            if (rcvpkt->retval == 0x01) {
                return 0;
            }
            else if (rcvpkt->retval == 0x02) {
                cout << "Sorry, Server does not have weather information for city " << rcvpkt->data << "!" << endl;
                cout << "Welcome to NJUCS Weather Forecast Demo Program!" << endl;
                cout << "Please input City Name in Chinese pinyin(e.g. nanjing or beijing)" << endl;
                cout << "(c)cls,(#)exit" << endl;
            }
        }
    }
    return 0;
}

int main() {
    init();
    // 轮流进入hello(), query()状态
    while (true) {
        hello();
        query();
    }
    return 0;
}