#define _USE_MATH_DEFINES
#include <Windows.h>
#include <iostream>
#include <cmath>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
namespace constants
{
    constexpr std::pair<int, int> screenSize(160, 80);
    constexpr std::pair<int, int> mapSize(24, 24);
    constexpr int keyW = 0x57;
    constexpr int keyA = 0x41;
    constexpr int keyS = 0x53;
    constexpr int keyD = 0x44;
    constexpr double rotateSpeed = 0.15 * M_PI / 180.0;//radians per millisecond
    constexpr double movementSpeedForward = 0.005;//units per millisecond
    constexpr double movementSpeedBackward = 0.005;//units per millisecond
    constexpr double projectileSpeed = 0.01;//units per milisecond
    constexpr double fov = 90.0 * M_PI / 180.0;
    constexpr wchar_t fullShade = 0x2588;
    constexpr wchar_t darkShade = 0x2593;
    constexpr wchar_t mediumShade = 0x2592;
    constexpr wchar_t lightShade = 0x2591;
}
namespace player
{
    std::pair<double, double> pos(3.0, 3.0);
    double yaw = 0.0;//the center of the player's vision in radians
    std::string command{};
}
class Projectile
{
public:
    std::pair<double, double> pos;
    double yaw;
    Projectile()
        : pos(player::pos.first, player::pos.second), yaw(player::yaw)
        {}
};
using namespace constants;
using namespace player;
int main()
{
    wchar_t* screen = new wchar_t[screenSize.first * screenSize.second + 1];//creates a char array for drawing the game screen
    HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);//gets a handle to the console
    SetConsoleActiveScreenBuffer(console);//sets the `console` handle to the active handle
    DWORD bytesWritten = 0;//just something that the Windows API requires to write to the console buffer
    std::wstring map;//a unicode string representing the game map
    map += L"########################";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......##########......#";
    map += L"#......##########......#";
    map += L"#......##########......#";
    map += L"#......##########......#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"#......................#";
    map += L"########################";
    std::vector<Projectile> projectiles;
    auto currentTime = std::chrono::system_clock::now();//this is set to the system time when the game loop begins
    auto lastFrameTime = currentTime;//this records the system time when the last game loop began
    while (1)
    {
        currentTime = std::chrono::system_clock::now();//updates to the current system time
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime).count();//gets how much time have elapsed in milliseconds
        lastFrameTime = currentTime;//copies the current system time for calculating the elapsed time when the next loop begins
        if (GetAsyncKeyState(VK_LEFT))
        {
            yaw -= rotateSpeed * elapsedTime;//rotates counterclockwise //multiplied by `elapsedTime` to balance the amount that the player rotates
        }
        if (GetAsyncKeyState(VK_RIGHT))
        {
            yaw += rotateSpeed * elapsedTime;//rotates clockwise
        }
        if (GetAsyncKeyState(keyW))
        {
            std::pair<double, double> unitVector(cos(yaw), sin(yaw));
            pos.first += unitVector.first * movementSpeedForward * elapsedTime;//move forward
            pos.second += unitVector.second * movementSpeedForward * elapsedTime;
            if (map[(int)pos.second * mapSize.first + (int)pos.first] == '#')//if moving into a wall
            {
                pos.first -= unitVector.first * movementSpeedForward * elapsedTime;
                pos.second -= unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(keyS))
        {
            std::pair<double, double> unitVector(cos(yaw), sin(yaw));
            pos.first -= unitVector.first * movementSpeedBackward * elapsedTime;//move backward
            pos.second -= unitVector.second * movementSpeedBackward * elapsedTime;
            if (map[(int)pos.second * mapSize.first + (int)pos.first] == '#')//if moving into a wall
            {
                pos.first += unitVector.first * movementSpeedForward * elapsedTime;
                pos.second += unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(keyA))
        {
            std::pair<double, double> unitVector(cos(yaw - (90.0 * M_PI / 180.0)), sin(yaw - (90.0 * M_PI / 180.0)));
            pos.first += unitVector.first * movementSpeedForward * elapsedTime;//move forward
            pos.second += unitVector.second * movementSpeedForward * elapsedTime;
            if (map[(int)pos.second * mapSize.first + (int)pos.first] == '#')//if moving into a wall
            {
                pos.first -= unitVector.first * movementSpeedForward * elapsedTime;
                pos.second -= unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(keyD))
        {
            std::pair<double, double> unitVector(cos(yaw + (90.0 * M_PI / 180.0)), sin(yaw + (90.0 * M_PI / 180.0)));
            pos.first += unitVector.first * movementSpeedForward * elapsedTime;//move forward
            pos.second += unitVector.second * movementSpeedForward * elapsedTime;
            if (map[(int)pos.second * mapSize.first + (int)pos.first] == '#')//if moving into a wall
            {
                pos.first -= unitVector.first * movementSpeedForward * elapsedTime;
                pos.second -= unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(VK_SPACE))
        {
            projectiles.push_back(Projectile());//adds a new projectile
        }
        if (GetAsyncKeyState(VK_OEM_3))//the ` key
        {
            std::getline(std::cin, command);
        }
        for (std::vector<Projectile>::iterator it = projectiles.begin(); it != projectiles.end(); it++)//move all existing projectiles forward
        {
            std::pair<double, double> unitVector(cos(it->yaw), sin(it->yaw));
            it->pos.first += unitVector.first * projectileSpeed * elapsedTime;
            it->pos.second += unitVector.second * projectileSpeed * elapsedTime;
            /*
            if (map.at((int)it->pos.second * mapSize.first + (int)it->pos.first) == '#')
            {
                projectiles.erase(it);
            }
            else
            {
                it++;
            }
            */
        }
        projectiles.erase(std::remove_if(projectiles.begin(),
            projectiles.end(),
            [&](auto x) {return map.at((int)x.pos.second * mapSize.first + (int)x.pos.first) == '#'; }),
            projectiles.end());//remove all projectiles inside walls
        for (int x = 0; x < screenSize.first; x++)//scans horizontally
        {
            double rayYaw = (yaw - fov / 2) + ((double)x / (double)screenSize.first) * fov;//(the leftmost part of the player's vision) + (x/160 of the player's fov) //this basically casts 160 rays which scan across the player's vision
            double distance = 0.0;//distance to wall
            std::pair<double, double> unitVector(cos(rayYaw), sin(rayYaw));//gets the unit vector of the center of the player's vision
            while (1)//loop until the ray hits a wall or the depth limit has been reached
            {
                distance += 0.1;
                std::pair<int, int> testPoint((int)(pos.first + unitVector.first * distance), (int)(pos.second + unitVector.second * distance));//this point extends until it is inside a wall
                if (testPoint.first < 0 ||
                    testPoint.first >= mapSize.first ||
                    testPoint.second < 0 ||
                    testPoint.second >= mapSize.second)//if the testPoint is out of bound
                {
                    break;
                }
                if (map.at(testPoint.second * mapSize.first + testPoint.first) == '#')
                {
                    break;
                }
            }
            int ceilingEndPos = (int)((screenSize.second / 2) - screenSize.second / distance);//the ceiling ends at `ceilingEndPos` - 1 //if the wall is 1 unit away there will be no ceiling at all
            if (ceilingEndPos < 0)
            {
                ceilingEndPos = 0;
            }
            int floorStartPos = screenSize.second - ceilingEndPos;//just the mirror of the ceiling
            for (int y = 0; y < screenSize.second; y++)//scans top to bottom on this column(column x)
            {
                if (y < ceilingEndPos)//if the ceiling needs to be drawn
                {
                    screen[y * screenSize.first + x] = ' ';//draws a ceiling cell on row y column x
                }
                else if (y >= ceilingEndPos && y < floorStartPos)//if the wall needs to be drawn
                {
                    if (distance <= 3)
                    {
                        screen[y * screenSize.first + x] = fullShade;
                    }
                    else if (3 < distance && distance <= 5)
                    {
                        screen[y * screenSize.first + x] = darkShade;
                    }
                    else if (5 < distance && distance <= 8)
                    {
                        screen[y * screenSize.first + x] = darkShade;
                    }
                    else
                    {
                        screen[y * screenSize.first + x] = lightShade;
                    }
                }
                else//if the floor needs to be drawn
                {
                    if (45 < (screenSize.second - y) && (screenSize.second - y) <= screenSize.second / 2)
                    {
                        screen[y * screenSize.first + x] = ' ';
                    }
                    else if (30 < (screenSize.second - y) && (screenSize.second - y) <= 45)
                    {
                        screen[y * screenSize.first + x] = '.';
                    }
                    else if (15 < (screenSize.second - y) && (screenSize.second - y) <= 30)
                    {
                        screen[y * screenSize.first + x] = '-';
                    }
                    else
                    {
                        screen[y * screenSize.first + x] = 'X';
                    }
                }
            }
            for (std::vector<Projectile>::iterator it = projectiles.begin(); it != projectiles.end(); it++)
            {
                double projectileYaw = it->yaw - player::yaw;//how much the projectile deviates from the center of the player's vision
                double projectileDistance = sqrt(pow((player::pos.first - it->pos.first), 2) + pow((player::pos.second - it->pos.second), 2));
                if (projectileDistance < 1)
                {
                    projectileDistance = 1;
                }
                if (projectileDistance < 0)
                {
                    continue;
                }
                double projectileRadius = screenSize.second / 2 / projectileDistance;//the size of the projectile projection on the screen
                int screenCenter = screenSize.first * screenSize.second / 2 + screenSize.second / 2;//the index of the center point of the screen
                if (!(projectileYaw * 180 / M_PI > fov * 180 / M_PI / 2 || projectileYaw * 180 / M_PI < -(fov * 180 / M_PI / 2)))//if not out of the player's vision
                {
                    //screen[screenSize.first * (screenSize.second / 2) + (screenSize.first / 2)] = '+';
                    #if 0
                    for (int x = 0; x < screenSize.first; x++)
                    {
                        for (int y = 0; y < screenSize.second; y++)
                        {
                            if ((double)sqrt(pow(x - screenCenter, 2)) + pow(y - screenCenter, 2) <= projectileRadius)
                            {
                                screen[y * screenSize.first + x] = '+';
                            }
                        }
                    }
                    #endif
                    if (projectileDistance < 4)//draw a 3*3 square
                    {
                        for (int i = x - 1; i < x + 1; i++)
                        {
                            for (int j = screenSize.second / 2 - 1; j < screenSize.second / 2 + 1; j++)
                            {
                                if (!(i < 0 || i >= screenSize.first || j < 0 || j >= screenSize.second))//if not out of bound
                                {
                                    screen[i * screenSize.first + j] = '+';
                                }
                            }
                        }
                    }
                    else if (4 <= projectileDistance && projectileDistance < 8)
                    {

                    }
                    else
                    {

                    }
                }
            }
        }
        screen[screenSize.first * screenSize.second] = 0;//null terminating
        WriteConsoleOutputCharacterW(console, screen, screenSize.first * screenSize.second + 1, { 0, 0 }, &bytesWritten);//writes to console buffer
    }
}
