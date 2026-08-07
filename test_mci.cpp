#include <windows.h>
#include <mmsystem.h>
#include <iostream>

int main() {
    char buf[128];
    long r1 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" type mpegvideo alias test1", NULL, 0, NULL);
    mciGetErrorStringA(r1, buf, 128); std::cout << "jump.wav: " << r1 << " - " << buf << std::endl;

    long r2 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\game-over.wav\" type mpegvideo alias test2", NULL, 0, NULL);
    mciGetErrorStringA(r2, buf, 128); std::cout << "game-over.wav: " << r2 << " - " << buf << std::endl;

    long r3 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\rain.wav\" type mpegvideo alias test3", NULL, 0, NULL);
    mciGetErrorStringA(r3, buf, 128); std::cout << "rain.wav: " << r3 << " - " << buf << std::endl;

    long r4 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\night-crickets.wav\" type mpegvideo alias test4", NULL, 0, NULL);
    mciGetErrorStringA(r4, buf, 128); std::cout << "night-crickets.wav: " << r4 << " - " << buf << std::endl;

    long r5 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" alias test5", NULL, 0, NULL);
    mciGetErrorStringA(r5, buf, 128); std::cout << "jump.wav (no type): " << r5 << " - " << buf << std::endl;

    return 0;
}
