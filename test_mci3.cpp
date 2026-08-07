#include <windows.h>
#include <mmsystem.h>
#include <iostream>

int main() {
    char buf[128];
    long r1 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" type mpegvideo alias flap0", NULL, 0, NULL);
    mciGetErrorStringA(r1, buf, 128); std::cout << "flap0 open: " << r1 << " - " << buf << std::endl;

    long r2 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" type mpegvideo alias flap1", NULL, 0, NULL);
    mciGetErrorStringA(r2, buf, 128); std::cout << "flap1 open: " << r2 << " - " << buf << std::endl;

    long r3 = mciSendStringA("play flap0", NULL, 0, NULL);
    mciGetErrorStringA(r3, buf, 128); std::cout << "flap0 play: " << r3 << " - " << buf << std::endl;
    
    long rh = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\home-page.wav\" type mpegvideo alias home", NULL, 0, NULL);
    mciGetErrorStringA(rh, buf, 128); std::cout << "home open: " << rh << " - " << buf << std::endl;

    long rhp = mciSendStringA("play home repeat", NULL, 0, NULL);
    mciGetErrorStringA(rhp, buf, 128); std::cout << "home play: " << rhp << " - " << buf << std::endl;

    Sleep(2000);
    return 0;
}
