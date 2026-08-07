#include <windows.h>
#include <mmsystem.h>
#include <iostream>

int main() {
    char buf[128];
    long r1 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\day-birds.wav\" type mpegvideo alias test1", NULL, 0, NULL);
    mciGetErrorStringA(r1, buf, 128); std::cout << "day-birds.wav: " << r1 << " - " << buf << std::endl;

    long r2 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\morning-birds.wav\" type mpegvideo alias test2", NULL, 0, NULL);
    mciGetErrorStringA(r2, buf, 128); std::cout << "morning-birds.wav: " << r2 << " - " << buf << std::endl;

    long r3 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\home-page.wav\" type mpegvideo alias test3", NULL, 0, NULL);
    mciGetErrorStringA(r3, buf, 128); std::cout << "home-page.wav: " << r3 << " - " << buf << std::endl;
    
    long r4 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\day-birds.wav\" alias test4", NULL, 0, NULL);
    mciGetErrorStringA(r4, buf, 128); std::cout << "day-birds.wav (no type): " << r4 << " - " << buf << std::endl;
    return 0;
}
