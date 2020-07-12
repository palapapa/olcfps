#define _USE_MATH_DEFINES
#define NOMINMAX
#include <Windows.h>
#include <iostream>
#include <cmath>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <limits>
namespace constants
{
    constexpr std::pair<int, int> screenSize(160, 80); 
    constexpr int keyW = 0x57;
    constexpr int keyA = 0x41;
    constexpr int keyS = 0x53;
    constexpr int keyD = 0x44;
    constexpr double rotateSpeed = 0.15 * M_PI / 180.0;//radians per millisecond
    constexpr double movementSpeedForward = 0.005;//units per millisecond
    constexpr double movementSpeedBackward = 0.005;//units per millisecond
    constexpr double projectileSpeed = 0.01;//units per millisecond
    constexpr double fov = 90.0 * M_PI / 180.0;
    constexpr wchar_t fullShade = 0x2588;
    constexpr wchar_t darkShade = 0x2593;
    constexpr wchar_t mediumShade = 0x2592;
    constexpr wchar_t lightShade = 0x2591;
}
class game_info
{
public:
    std::pair<double, double> playerPos;
    double playerYaw;//the center of the player's vision in radians
    std::pair<int, int> mapSize;
    std::string command;
    bool isConsoleOpen;
    game_info() :
        playerPos(0.0, 0.0),
        playerYaw(0.0),
        mapSize(0, 0),
        isConsoleOpen(true)
    {}
};
class projectile
{
public:
    std::pair<double, double> pos;
    double yaw;
    projectile(const std::pair<double, double> pos, const double yaw) :
        pos(pos),
        yaw(yaw)
    {}
};
class editor_state
{
public:
    std::wstring map;
    std::pair<int, int> mapSize;
    std::pair<double, double> startPos;
    std::wstring message;
    editor_state(const std::wstring map, const std::pair<int, int> mapSize, const std::pair<double, double> startPos, const std::wstring message) :
        map(map),
        mapSize(mapSize),
        startPos(startPos),
        message(message)
    {}
};
enum class commands
{
    wall,
    undo,
    redo,
    help,
    save,
    size,
    start,
    space,
    unknown
};
commands get_command_enum(std::wstring command)
{
    static const std::map<std::wstring, commands> commandsMap
    {
        {L"wall", commands::wall},
        {L"undo", commands::undo},
        {L"redo", commands::redo},
        {L"help", commands::help},
        {L"save", commands::save},
        {L"size", commands::size},
        {L"start", commands::start},
        {L"space", commands::space}
    };
    auto it = commandsMap.find(command);
    if (it != commandsMap.end())
    {
        return it->second;
    }
    else
    {
        return commands::unknown;
    }
}
using namespace constants;
void gotoxy(short x, short y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
void start_game()
{
    wchar_t *screen = new wchar_t[screenSize.first * screenSize.second + 1];//creates a char array for drawing the game screen
    HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);//gets a handle to the console
    SetConsoleActiveScreenBuffer(console);//sets the `console` handle to the active handle
    DWORD bytesWritten = 0;//just something that the Windows API requires to write to the console buffer
    std::wifstream mapStream;
    game_info gameInfo;
    mapStream.open("map.txt", std::ios::in);
    std::wstring map;//a unicode string representing the game map
    if (!mapStream)//if the map file fails to open
    {
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
    }
    else
    {
        std::wstring line;
        while (std::getline(mapStream, line))//reads line by line
        {
            std::wistringstream wiss(line);
            std::wstring token;
            std::vector<std::wstring> tokens;
            while (wiss >> token)//reads tokens for each line
            {
                tokens.push_back(token);
            }
            if (tokens.size() == 0)
            {
                continue;
            }
            if (tokens.front() == L"MapSize")
            {
                if (tokens.size() != 3)
                {
                    throw "MapSize must have 2 arguments";
                }
                gameInfo.mapSize.first = std::stoi(tokens.at(1));
                gameInfo.mapSize.second = std::stoi(tokens.at(2));
            }
            else if (tokens.front() == L"StartPos")
            {
                if (tokens.size() != 3)
                {
                    throw "StartPos must have 2 arguments";
                }
                gameInfo.playerPos.first = std::stod(tokens.at(1));
                gameInfo.playerPos.second = std::stod(tokens.at(2));
            }
            else
            {
                map += line;
            }
        }
    }
    std::vector<projectile> projectiles;
    auto currentTime = std::chrono::system_clock::now();//this is set to the system time when the game loop begins
    auto lastFrameTime = currentTime;//this records the system time when the last game loop began
    while (1)
    {
        if (gameInfo.isConsoleOpen)
        {
            std::getline(std::cin, gameInfo.command);
            gameInfo.isConsoleOpen = false;
        }
        static int frameCount;
        frameCount++;
        currentTime = std::chrono::system_clock::now();//updates to the current system time
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime).count();//gets how much time have elapsed in milliseconds
        lastFrameTime = currentTime;//copies the current system time for calculating the elapsed time when the next loop begins
        if (frameCount % 10 == 0)
        {
            std::ostringstream oss;
            oss << "palapapa's FPS " << "X=" << gameInfo.playerPos.first << " Y=" << gameInfo.playerPos.second << " FPS=" << (int)(1000.0 / elapsedTime) << " Frame=" << frameCount;
            SetConsoleTitleA(oss.str().c_str());
        }
        if (GetAsyncKeyState(VK_LEFT))
        {
            gameInfo.playerYaw -= rotateSpeed * elapsedTime;//rotates counterclockwise //multiplied by `elapsedTime` to balance the amount that the player rotates
        }
        if (GetAsyncKeyState(VK_RIGHT))
        {
            gameInfo.playerYaw += rotateSpeed * elapsedTime;//rotates clockwise
        }
        if (GetAsyncKeyState(keyW))
        {
            std::pair<double, double> unitVector(cos(gameInfo.playerYaw), sin(gameInfo.playerYaw));
            gameInfo.playerPos.first += unitVector.first * movementSpeedForward * elapsedTime;//move forward
            gameInfo.playerPos.second += unitVector.second * movementSpeedForward * elapsedTime;
            if (map[(int)gameInfo.playerPos.second * gameInfo.mapSize.first + (int)gameInfo.playerPos.first] == '#')//if moving into a wall
            {
                gameInfo.playerPos.first -= unitVector.first * movementSpeedForward * elapsedTime;
                gameInfo.playerPos.second -= unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(keyS))
        {
            std::pair<double, double> unitVector(cos(gameInfo.playerYaw), sin(gameInfo.playerYaw));
            gameInfo.playerPos.first -= unitVector.first * movementSpeedBackward * elapsedTime;//move backward
            gameInfo.playerPos.second -= unitVector.second * movementSpeedBackward * elapsedTime;
            if (map[(int)gameInfo.playerPos.second * gameInfo.mapSize.first + (int)gameInfo.playerPos.first] == '#')//if moving into a wall
            {
                gameInfo.playerPos.first += unitVector.first * movementSpeedForward * elapsedTime;
                gameInfo.playerPos.second += unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(keyA))
        {
            std::pair<double, double> unitVector(cos(gameInfo.playerYaw - (90.0 * M_PI / 180.0)), sin(gameInfo.playerYaw - (90.0 * M_PI / 180.0)));
            gameInfo.playerPos.first += unitVector.first * movementSpeedForward * elapsedTime;//move forward
            gameInfo.playerPos.second += unitVector.second * movementSpeedForward * elapsedTime;
            if (map[(int)gameInfo.playerPos.second * gameInfo.mapSize.first + (int)gameInfo.playerPos.first] == '#')//if moving into a wall
            {
                gameInfo.playerPos.first -= unitVector.first * movementSpeedForward * elapsedTime;
                gameInfo.playerPos.second -= unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(keyD))
        {
            std::pair<double, double> unitVector(cos(gameInfo.playerYaw + (90.0 * M_PI / 180.0)), sin(gameInfo.playerYaw + (90.0 * M_PI / 180.0)));
            gameInfo.playerPos.first += unitVector.first * movementSpeedForward * elapsedTime;//move forward
            gameInfo.playerPos.second += unitVector.second * movementSpeedForward * elapsedTime;
            if (map[(int)gameInfo.playerPos.second * gameInfo.mapSize.first + (int)gameInfo.playerPos.first] == '#')//if moving into a wall
            {
                gameInfo.playerPos.first -= unitVector.first * movementSpeedForward * elapsedTime;
                gameInfo.playerPos.second -= unitVector.second * movementSpeedForward * elapsedTime;
            }
        }
        if (GetAsyncKeyState(VK_SPACE))
        {
            projectiles.push_back(projectile(gameInfo.playerPos, gameInfo.playerYaw));//adds a new projectile
        }
        if (GetAsyncKeyState(VK_OEM_3))//the ` key
        {
            gameInfo.isConsoleOpen = true;
        }
        for (std::vector<projectile>::iterator it = projectiles.begin(); it != projectiles.end(); it++)//move all existing projectiles forward
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
            [&](auto x) {return map.at((int)x.pos.second * gameInfo.mapSize.first + (int)x.pos.first) == '#';}),
            projectiles.end());//remove all projectiles inside walls
        for (int x = 0; x < screenSize.first; x++)//scans horizontally
        {
            double rayYaw = (gameInfo.playerYaw - fov / 2) + ((double)x / (double)screenSize.first) * fov;//(the leftmost part of the player's vision) + (x/160 of the player's fov) //this basically casts 160 rays which scan across the player's vision
            double distance = 0.0;//distance to wall
            std::pair<double, double> unitVector(cos(rayYaw), sin(rayYaw));//gets the unit vector of the center of the player's vision
            while (1)//loop until the ray hits a wall or the depth limit has been reached
            {
                distance += 0.1;
                std::pair<int, int> testPoint((int)(gameInfo.playerPos.first + unitVector.first * distance), (int)(gameInfo.playerPos.second + unitVector.second * distance));//this point extends until it is inside a wall
                if (testPoint.first < 0 ||
                    testPoint.first >= gameInfo.mapSize.first ||
                    testPoint.second < 0 ||
                    testPoint.second >= gameInfo.mapSize.second)//if the testPoint is out of bound
                {
                    break;
                }
                if (map.at(testPoint.second * gameInfo.mapSize.first + testPoint.first) == '#')
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
            for (auto it = projectiles.begin(); it != projectiles.end(); it++)
            {
                double projectileYaw = it->yaw - gameInfo.playerYaw;//how much the projectile deviates from the center of the player's vision
                double projectileDistance = sqrt(pow((gameInfo.playerPos.first - it->pos.first), 2) + pow((gameInfo.playerPos.second - it->pos.second), 2));
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
void start_editor()
{
    system("cls");
    std::wstring input;
    std::vector<editor_state> editorStates;//used for undo and redo
    std::wfstream mapStream;
    std::pair<int, int> mapSize(0, 0);
    std::pair<double, double> startPos(0, 0);
    std::wstring map;//temp map string used for editing
    std::wstring message;
    mapStream.open("map.txt");
    if (!mapStream)//if map.txt doesn't exist
    {
        std::wofstream newMapStream("map.txt");//create a new map.txt
        mapStream.open("map.txt");
        std::wstring defaultMap;
        defaultMap += L"MapSize 24 24\n";
        defaultMap += L"StartPos 3 3\n";
        defaultMap += L"########################";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......##########......#";
        defaultMap += L"#......##########......#";
        defaultMap += L"#......##########......#";
        defaultMap += L"#......##########......#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"#......................#";
        defaultMap += L"########################";
        mapStream << defaultMap;
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
        mapSize = std::make_pair(24, 24);
        startPos = std::make_pair(3, 3);
    }
    else
    {
        std::wstring line;
        while (std::getline(mapStream, line))//reads line by line
        {
            std::wistringstream wiss(line);
            std::wstring token;
            std::vector<std::wstring> tokens;//splits a line into tokens
            while (wiss >> token)//reads tokens for each line
            {
                tokens.push_back(token);
            }
            if (tokens.front() == L"MapSize")
            {
                if (tokens.size() != 3)
                {
                    throw "MapSize must have 2 arguments";
                }
                mapSize.first = std::stoi(tokens.at(1));
                mapSize.second = std::stoi(tokens.at(2));
            }
            else if (tokens.front() == L"StartPos")
            {
                if (tokens.size() != 3)
                {
                    throw "StartPos must have 2 arguments";
                }
                startPos.first = std::stod(tokens.at(1));
                startPos.second = std::stod(tokens.at(2));
            }
            else
            {
                map += line;
            }
        }
    }
    while (1)
    {
        system("cls");
        for (int y = 0; y < mapSize.first; y++)//prints the map
        {
            for (int x = 0; x < mapSize.second; x++)
            {
                std::wcout << map.at(y * mapSize.first + x);
            }
            std::cout << "\n";
        }
        std::cout << "Map Size = (" << mapSize.first << ", " << mapSize.second << ")\n";
        std::cout << "Starting Position = (" << startPos.first << ", " << startPos.second << ")\n";
        std::wcout << message;
        message.clear();
        std::wcin.ignore(std::wcin.rdbuf()->in_avail(), '\n');//flushes the input buffer
        std::getline(std::wcin, input);
        if (std::wcin.eof())
        {
            std::wcin.clear();
            continue;
        }
        editorStates.push_back(editor_state(map, mapSize, startPos, message));
        std::wistringstream inputWiss(input);
        std::wstring token;
        std::vector<std::wstring> tokens;
        while (inputWiss >> token)//separates an input into tokens
        {
            tokens.push_back(token);
        }
        if (tokens.size() == 0)
        {
            continue;
        }
        else
        {
            switch (get_command_enum(tokens.front()))
            {
            case commands::wall :
                if (tokens.size() == 1)//no arguments
                {
                    //enclose the map with walls
                    for (int x = 0; x < mapSize.first; x++)
                    {
                        map.at(x) = '#';//horizontal first row
                        map.at(mapSize.first * (mapSize.second - 1) + x) = '#';//horizontal last row
                        
                    }
                    for (int y = 0; y < mapSize.second; y++)
                    {
                        map.at(y * mapSize.first) = '#';//vertical first column
                        map.at((y + 1) * mapSize.first - 1) = '#';//vertical last column
                    }
                    std::wstringstream messageWss;
                    messageWss << "Enclosed the map\n";
                    message = messageWss.str();
                }
                else if (tokens.size() == 3)//point
                {
                    try
                    {
                        int x = std::stoi(tokens.at(1));
                        int y = std::stoi(tokens.at(2));
                        map.at(y *mapSize.first + x) = '#';
                        std::wstringstream messageWss;
                        messageWss << "Turned block at (" << x << ", " << y << ") into wall\n";
                        message = messageWss.str();
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::wstringstream messageWss;
                        messageWss << "Invalid argument: " << e.what() << "\n";
                        message = messageWss.str();
                        break;
                    }
                }
                else if (tokens.size() == 5)//area
                {
                    try
                    {
                        int x1 = std::stoi(tokens.at(1));
                        int y1 = std::stoi(tokens.at(2));
                        int x2 = std::stoi(tokens.at(3));
                        int y2 = std::stoi(tokens.at(4));
                        if (x1 > mapSize.first ||
                            x1 < 0 ||
                            x2 > mapSize.first ||
                            x2 < 0 ||
                            y1 > mapSize.second ||
                            y1 < 0 ||
                            y2 > mapSize.second ||
                            y2 < 0)//if coords are out of bounds
                        {
                            std::wstringstream messageWss;
                            messageWss << "Coordinates are out of bounds\n";
                            message = messageWss.str();
                            break;
                        }
                        if (x1 > x2)
                        {
                            std::swap(x1, x2);
                        }
                        if (y1 > y2)
                        {
                            std::swap(y1, y2);
                        }
                        for (int x = x1; x < x2; x++)
                        {
                            for (int y = y1; y < y2; y++)
                            {
                                map.at(y * mapSize.first + x) = '#';
                            }
                        }
                        std::wstringstream messageWss;
                        messageWss << "Turned " << (x2 - x1) * (y2 - y1) << " blocks into walls\n";
                        message = messageWss.str();
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::wstringstream messageWss;
                        messageWss << "Invalid argument: " << e.what() << "\n";
                        message = messageWss.str();
                        break;
                    }
                }
                else
                {
                    std::wstringstream messageWss;
                    messageWss << "\'wall\' only accepts 2, 4, or no arguments\n";
                    message = messageWss.str();
                }
                break;
            case commands::size :
                if (tokens.size() == 3)
                {
                    try
                    {
                        int previousX = mapSize.first;
                        int previousY = mapSize.second;
                        int newX = std::stoi(tokens.at(1));
                        int newY = std::stoi(tokens.at(2));
                        mapSize.first = newX;
                        mapSize.second = newY;
                        if (newX * newY > previousX * previousY)//if the altered map size is larger than the original one
                        {
                            std::wstring previousMap = map;//saves a copy of the original map
                            map.clear();
                            for (int i = 0; i < newX * newY; i++)//resets the map first
                            {
                                map += '.';
                            }
                            for (int x = 0; x < previousX; x++)//copy the original map into the new one
                            {
                                for (int y = 0; y < previousY; y++)
                                {
                                    map.at(y * newX + x) = previousMap.at(y * previousX + x);//problematic: doesn't support map widening or lengthening
                                }
                            }
                        }
                        else if (newX * newY < previousX * previousY)//if the altered map size is smaller than the original one
                        {

                        }
                        std::wstringstream messageWss;
                        messageWss << "Changed map size to (" << newX << ", " << newY << ")\n";
                        message = messageWss.str();
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::wstringstream messageWss;
                        messageWss << "Invalid argument: " << e.what() << "\n";
                        message = messageWss.str();
                        break;
                    }
                }
                else
                {
                    std::wstringstream messageWss;
                    messageWss << "\'size\' only accepts 2 arguments\n";
                    message = messageWss.str();
                }
                break;
            case commands::unknown :
                std::wstringstream messageWss;
                messageWss << "Bruh not a valid command(enter \'help\' for help)\n";
                message = messageWss.str();
                break;
            }
        }
    }
}
void start_visual_editor()
{

}
int main()
{
    std::cout << "Select Mode:\n1: Start the Game\n2: Start Map Editor\n3: Start Visual Map Editor\n";
    std::wstring choice;
    while (1)//select mode
    {
        std::wcin >> choice;
        if (choice == L"1")
        {
            start_game();
            return 0;
        }
        else if (choice == L"2")
        {
            start_editor();
            return 0;
        }
        else if (choice == L"3")
        {
            start_visual_editor();
            return 0;
        }
        else
        {
            std::cout << "Bruh\n";
            continue;
        }
    }
}
