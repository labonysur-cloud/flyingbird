#include <windows.h>
#include <mmsystem.h>
#include <iostream>

int main() {
    char buf[128];
    long ro = mciSendStringA("open \"D:\\bird_game\\asset\\sound\\rain.wav\" alias test", NULL, 0, NULL);
    mciGetErrorStringA(ro, buf, 128); std::cout << "open rain (no type): " << ro << " - " << buf << std::endl;

    long rp = mciSendStringA("play test repeat", NULL, 0, NULL);
    mciGetErrorStringA(rp, buf, 128); std::cout << "play rain repeat: " << rp << " - " << buf << std::endl;

    long rp2 = mciSendStringA("play test", NULL, 0, NULL);
    mciGetErrorStringA(rp2, buf, 128); std::cout << "play rain: " << rp2 << " - " << buf << std::endl;
    return 0;
}
