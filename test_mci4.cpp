#include <windows.h>
#include <mmsystem.h>
#include <iostream>

int main() {
    char buf[128];
    long r1 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" alias flap0", NULL, 0, NULL);
    mciGetErrorStringA(r1, buf, 128); std::cout << "flap0 open: " << r1 << " - " << buf << std::endl;

    long r2 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" alias flap1", NULL, 0, NULL);
    mciGetErrorStringA(r2, buf, 128); std::cout << "flap1 open: " << r2 << " - " << buf << std::endl;

    long r3 = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\jump.wav\" alias flap2", NULL, 0, NULL);
    mciGetErrorStringA(r3, buf, 128); std::cout << "flap2 open: " << r3 << " - " << buf << std::endl;

    long p1 = mciSendStringA("play flap0", NULL, 0, NULL);
    mciGetErrorStringA(p1, buf, 128); std::cout << "flap0 play: " << p1 << " - " << buf << std::endl;
    
    Sleep(2000);
    return 0;
}
