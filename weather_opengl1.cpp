#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <random>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <sstream>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <map>
#include "json.hpp"

// stb_image (single header) for loading JPG/PNG.
// Download stb_image.h into the same folder before compiling.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using json = nlohmann::json;

// --- Config ---
const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 600;
const int FRAME_MS = 16;
const int WEATHER_REFRESH_MS = 60000; // 60s (unused if you don't want auto refresh)
const std::string API_KEY = "93c9d3d12ce3d18eefab28b5df598b67"; // keep if you want live weather

// --- Weather types ---
enum class WeatherType { SUNNY, RAINY, CLOUDY, SNOWY, CLEAR, UNKNOWN };

// --- Particle & Cloud ---
struct Particle { float x, y, vx, vy, size; };
struct Cloud { float x, y, speed, scale; };

// --- Page ---
enum class Page { INTRO, WEATHER };
Page currentPage = Page::INTRO;

// --- Globals ---
WeatherType currentWeather = WeatherType::UNKNOWN;
std::string currentCity = "Unknown";
std::vector<Particle> particles;
std::vector<Cloud> clouds;
int maxParticles = 300;
std::mt19937 rng((unsigned)time(nullptr));
int lastTimeMs = 0;
std::string typedCity = "";
bool isTyping = true;
std::atomic<bool> isUpdatingWeather(false);

// Weather details
float temperature = 0.0f; // Celsius
int humidity = 0;
float windSpeed = 0.0f;

// Team info
struct TeamMember { std::string name; std::string usn; };
std::vector<TeamMember> team = {
    {"Tejas Naik", "NNM23CS513"},
    {"Tejas", "NNM23CS512"},
    {"Shamanth Hegde", "NNM23CS511"},
    {"Ananya", "NNM23CS502"}
};

// Button bounds for "Next" (normalized coords)
float btnXMin = -0.2f, btnXMax = 0.2f, btnYMin = -0.8f, btnYMax = -0.7f;

// --- Map texture globals ---
GLuint mapTexture = 0;
bool mapLoaded = false;
std::string mapFilename = "map.jpg";
int mapImgW = 0, mapImgH = 0, mapImgChannels = 0;
std::mutex texMutex;

// --- City DB (lat, lon) for India (add more as needed) ---
struct CityInfo { float lat, lon; };
std::map<std::string, CityInfo> cities = {
    // --- Dakshina Kannada District ---
    {"mangalore", {12.9141f, 74.8560f}},
    {"managalore", {12.9141f, 74.8560f}}, // common misspell
    {"surathkal", {12.9800f, 74.8000f}},
    {"bajpe", {12.9612f, 74.8893f}},
    {"kinnigoli", {13.0924f, 74.8662f}},
    {"moodbidri", {13.0827f, 74.9941f}},
    {"bantwal", {12.8903f, 75.0347f}},
    {"puttur", {12.7590f, 75.2015f}},
    {"sullia", {12.5590f, 75.3900f}},
    {"belthangady", {13.9940f, 75.2865f}},
    {"venur", {13.0000f, 75.0500f}},
    {"kateel", {12.9768f, 74.8894f}},
    {"panemangalore", {12.8726f, 75.0038f}},
    {"ullal", {12.8056f, 74.8560f}},
    {"mulki", {13.0900f, 74.7900f}},
    {"thokottu", {12.8290f, 74.8642f}},

    // --- Udupi District ---
    {"udupi", {13.3409f, 74.7421f}},
    {"manipal", {13.3523f, 74.7850f}},
    {"nitte", {13.1790f, 74.9330f}},
    {"kundapura", {13.6269f, 74.6900f}},
    {"kaup", {13.2020f, 74.7390f}},
    {"karkala", {13.2082f, 74.9920f}},

    // --- Major Karnataka Cities ---
    {"bangalore", {12.9716f, 77.5946f}},
    {"bengaluru", {12.9716f, 77.5946f}},
    {"mysore", {12.2958f, 76.6394f}},
    {"tumkur", {13.3422f, 77.1010f}},
    {"hassan", {13.0072f, 76.0967f}},
    {"shimoga", {13.9299f, 75.5681f}},
    {"chikmagalur", {13.3150f, 75.7754f}},
    {"hubli", {15.3647f, 75.1240f}},
    {"dharwad", {15.4589f, 75.0078f}},
    {"belgaum", {15.8497f, 74.4977f}},
    {"bellary", {15.1394f, 76.9214f}},
    {"davangere", {14.4644f, 75.9218f}},
    {"bijapur", {16.8302f, 75.7100f}},
    {"raichur", {16.2120f, 77.3439f}},
    {"bidar", {17.9133f, 77.5301f}},
    {"karwar", {14.8136f, 74.1292f}},
    {"mandya", {12.5242f, 76.8958f}},
    {"kolar", {13.1360f, 78.1296f}},
    {"gadag", {15.4300f, 75.6200f}},
    {"bagalkot", {16.1720f, 75.6550f}},
    {"hospet", {15.2667f, 76.4000f}},
    {"madikeri", {12.4244f, 75.7382f}},
    {"sagara", {14.1689f, 75.0231f}},

    // --- South India (other states) ---
    {"hyderabad", {17.3850f, 78.4867f}},
    {"chennai", {13.0827f, 80.2707f}},
    {"kochi", {9.9312f, 76.2673f}},
    {"trivandrum", {8.5241f, 76.9366f}},
    {"coimbatore", {11.0168f, 76.9558f}},
    {"madurai", {9.9252f, 78.1198f}},
    {"visakhapatnam", {17.6868f, 83.2185f}},
    {"vijayawada", {16.5062f, 80.6480f}},

    // --- Western India ---
    {"mumbai", {19.0760f, 72.8777f}},
    {"pune", {18.5204f, 73.8567f}},
    {"ahmedabad", {23.0225f, 72.5714f}},
    {"vadodara", {22.3072f, 73.1812f}},
    {"surat", {21.1702f, 72.8311f}},
    {"rajkot", {22.3039f, 70.8022f}},
    {"nashik", {19.9975f, 73.7898f}},

    // --- Northern India ---
    {"delhi", {28.6139f, 77.2090f}},
    {"dehradun", {30.3165f, 78.0322f}},
    {"jaipur", {26.9124f, 75.7873f}},
    {"lucknow", {26.8467f, 80.9462f}},
    {"patna", {25.5941f, 85.1376f}},
    {"chandigarh", {30.7333f, 76.7794f}},
    {"amritsar", {31.6340f, 74.8723f}},
    {"bhopal", {23.2599f, 77.4126f}},
    {"indore", {22.7196f, 75.8577f}},
    {"varanasi", {25.3176f, 82.9739f}},
    {"kanpur", {26.4499f, 80.3319f}},
    {"agra", {27.1767f, 78.0081f}},
    {"meerut", {28.9845f, 77.7064f}},

    // --- Eastern India ---
    {"kolkata", {22.5726f, 88.3639f}},
    {"bhubaneswar", {20.2961f, 85.8245f}},
    {"ranchi", {23.3441f, 85.3096f}},
    {"guwahati", {26.1445f, 91.7362f}},

    // --- Central / Other ---
    {"nagpur", {21.1458f, 79.0882f}},
    {"gwalior", {26.2183f, 78.1828f}},
    {"jabalpur", {23.1815f, 79.9864f}}
};


// India map geographic bounds
const float INDIA_LAT_MIN = 6.0f;
const float INDIA_LAT_MAX = 36.0f;
const float INDIA_LON_MIN = 68.0f;
const float INDIA_LON_MAX = 98.0f;

// Pin state
bool pinVisible = false;
float pinLat = 0.0f, pinLon = 0.0f;

// --- Utility ---
float randf(float a, float b){ std::uniform_real_distribution<float> d(a,b); return d(rng); }
std::string toLower(const std::string &s){ std::string t=s; std::transform(s.begin(),s.end(),t.begin(),::tolower); return t; }
std::string urlEncode(const std::string &value){
    std::ostringstream escaped; escaped.fill('0'); escaped << std::hex;
    for(char c: value){
        if(isalnum((unsigned char)c)) escaped << c;
        else if(c==' ') escaped << "%20";
        else {
            escaped << '%' << std::setw(2) << std::uppercase << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }
    return escaped.str();
}

// Map weather string
WeatherType mapWeatherString(const std::string &weather){
    std::string w = toLower(weather);
    if(w.find("sunny") != std::string::npos || w.find("clear") != std::string::npos)
        return WeatherType::SUNNY;
    if(w.find("cloud") != std::string::npos || w.find("haze") != std::string::npos ||
       w.find("mist") != std::string::npos || w.find("smoke") != std::string::npos)
        return WeatherType::CLOUDY;
    if(w.find("rain") != std::string::npos || w.find("drizzle") != std::string::npos ||
       w.find("thunderstorm") != std::string::npos)
        return WeatherType::RAINY;
    if(w.find("snow") != std::string::npos)
        return WeatherType::SNOWY;
    return WeatherType::SUNNY; // fallback
}

// --- Image loader using stb_image ---
bool loadMapImageToTexture(const std::string &filename) {
    std::lock_guard<std::mutex> lk(texMutex);
    if (mapTexture) {
        glDeleteTextures(1, &mapTexture);
        mapTexture = 0;
    }

    // flip vertically so (0,0) is bottom-left for OpenGL texture coords
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &mapImgW, &mapImgH, &mapImgChannels, 0);
    if (!data) {
        std::cerr << "Failed to load map image '" << filename << "': " << stbi_failure_reason() << "\n";
        return false;
    }

    GLenum format = (mapImgChannels == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &mapTexture);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, mapImgW, mapImgH, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    mapLoaded = true;
    std::cerr << "Loaded map texture: " << filename << " (" << mapImgW << "x" << mapImgH << " ch=" << mapImgChannels << ")\n";
    return true;
}

// Convert latitude/longitude (India) to OpenGL coordinates (-1..1) matching full-screen textured quad
void latlonToGLXY(float lat, float lon, float &outX, float &outY) {
    // clamp within India bounds
    if (lat < INDIA_LAT_MIN) lat = INDIA_LAT_MIN;
    if (lat > INDIA_LAT_MAX) lat = INDIA_LAT_MAX;
    if (lon < INDIA_LON_MIN) lon = INDIA_LON_MIN;
    if (lon > INDIA_LON_MAX) lon = INDIA_LON_MAX;

    float nx = (lon - INDIA_LON_MIN) / (INDIA_LON_MAX - INDIA_LON_MIN); // 0..1
    float ny = (lat - INDIA_LAT_MIN) / (INDIA_LAT_MAX - INDIA_LAT_MIN); // 0..1

    // Convert to OpenGL -1..1 coordinates (note y increases upward)
    outX = -1.0f + nx * 2.0f;
    outY = -1.0f + ny * 2.0f;
}

// --- Scene update ---
void updateScene(int deltaMs){
    for(auto &p:particles){
        p.x += p.vx*deltaMs;
        p.y += p.vy*deltaMs;
        if(p.y<-1.2f){ p.y=randf(1.0f,1.6f); p.x=randf(-1.2f,1.2f);}
        if(p.x<-1.4f)p.x=1.4f; if(p.x>1.4f)p.x=-1.4f;
    }
    for(auto &c:clouds){ c.x += c.speed*deltaMs; if(c.x-0.5f*c.scale>1.5f)c.x=-1.6f-0.5f*c.scale;}
}

// --- Drawing helpers ---
void drawFilledCircle(float cx,float cy,float r,int segments=32){
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
    for(int i=0;i<=segments;i++){
        float theta = 2.0f * 3.14159265358979323846f * i / segments;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}

void drawPin(float lat, float lon){
    float x, y;
    latlonToGLXY(lat, lon, x, y);
    float r = 0.03f; // pin radius (adjust)
    // white outline
    glColor3f(1.0f,1.0f,1.0f);
    drawFilledCircle(x, y, r + 0.008f);
    // red center
    glColor3f(0.85f, 0.12f, 0.12f);
    drawFilledCircle(x, y, r);
    // small black dot center
    glColor3f(0,0,0);
    drawFilledCircle(x, y, r*0.3f);
}

// Sun / clouds / particles same as before
void drawSun(float cx,float cy,float r,int elapsedMs){
    glColor3f(1.0f,0.92f,0.2f);
    drawFilledCircle(cx,cy,r);
    glLineWidth(2);
    glBegin(GL_LINES);
    for(int i=0;i<12;i++){
        float angle = i*30.0f*3.14159f/180.0f + elapsedMs*0.002f;
        glVertex2f(cx + r*cosf(angle), cy + r*sinf(angle));
        glVertex2f(cx + (r+0.08f)*cosf(angle), cy + (r+0.08f)*sinf(angle));
    }
    glEnd();
}
void drawCloud(float cx,float cy,float s){
    glColor4f(1.0f,1.0f,1.0f,0.85f);
    float offsets[6][2] = {{0.0f,0.0f},{0.06f,0.02f},{-0.06f,0.02f},{0.03f,0.04f},{-0.03f,0.04f},{0.0f,0.06f}};
    float sizes[6] = {0.08f,0.07f,0.07f,0.06f,0.06f,0.05f};
    for(int i=0;i<6;i++) drawFilledCircle(cx+offsets[i][0]*s*2, cy+offsets[i][1]*s*2, sizes[i]*s);
}
void drawClouds(int elapsedMs){
    for(size_t i=0;i<clouds.size();i++){
        float bx = clouds[i].x;
        float by = clouds[i].y + 0.03f * sinf(elapsedMs*0.002f + float(i)*1.2f);
        float s = clouds[i].scale;
        drawCloud(bx, by, s);
    }
}
void drawParticles(){
    if(currentWeather==WeatherType::RAINY){
        glLineWidth(1); glBegin(GL_LINES);
        for(auto &p:particles){ glColor3f(0.6f,0.7f,0.85f); glVertex2f(p.x,p.y); glVertex2f(p.x+p.vx*6.0f,p.y+p.vy*6.0f); }
        glEnd();
    } else if(currentWeather==WeatherType::SNOWY){
        for(auto &p:particles){ glColor3f(1,1,1); drawFilledCircle(p.x,p.y,p.size); }
    }
}

// --- Draw Intro Page ---
void drawIntroPage(int elapsedMs){
    updateScene(elapsedMs);
    glClearColor(0.6f,0.8f,1.0f,1.0f);
    drawClouds(elapsedMs);
    if(currentWeather==WeatherType::SUNNY) drawSun(0.7f,0.8f,0.12f,elapsedMs);
    if(currentWeather==WeatherType::RAINY||currentWeather==WeatherType::SNOWY) drawParticles();

    std::string title = "Project: Weather Animation (Offline Map)";
    glColor3f(0, 0, 0);
    glRasterPos2f(-0.6f, 0.85f);
    for(char c : title) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    // Table & button (same as before)
    float tableXMin = -0.5f, tableXMax = 0.5f;
    float tableYTop = 0.6f, rowHeight = 0.12f;
    int rows = team.size() + 1;

    glColor3f(0.2f,0.6f,0.8f);
    glBegin(GL_QUADS);
        glVertex2f(tableXMin, tableYTop);
        glVertex2f(tableXMax, tableYTop);
        glVertex2f(tableXMax, tableYTop - rowHeight);
        glVertex2f(tableXMin, tableYTop - rowHeight);
    glEnd();

    glColor3f(1,1,1);
    glRasterPos2f(tableXMin + 0.05f, tableYTop - 0.08f);
    for(char c: "Name") glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);
    glRasterPos2f((tableXMin + tableXMax)/2 + 0.05f, tableYTop - 0.08f);
    for(char c: "USN") glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);

    for(int i=0;i<team.size();i++){
        float yTop = tableYTop - rowHeight*(i+1);
        float yBottom = yTop - rowHeight;

        if(i%2==0) glColor3f(0.9f,0.9f,0.95f);
        else glColor3f(0.85f,0.85f,0.9f);
        glBegin(GL_QUADS);
            glVertex2f(tableXMin, yTop);
            glVertex2f(tableXMax, yTop);
            glVertex2f(tableXMax, yBottom);
            glVertex2f(tableXMin, yBottom);
        glEnd();

        glColor3f(0,0,0);
        glRasterPos2f(tableXMin + 0.05f, yTop - 0.08f);
        for(char c: team[i].name) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        glRasterPos2f((tableXMin + tableXMax)/2 + 0.05f, yTop - 0.08f);
        for(char c: team[i].usn) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

        glColor3f(0,0,0);
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(tableXMin, yTop);
            glVertex2f(tableXMax, yTop);
            glVertex2f(tableXMax, yBottom);
            glVertex2f(tableXMin, yBottom);
        glEnd();
    }

    glColor3f(0,0,0);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(tableXMin, tableYTop);
        glVertex2f(tableXMax, tableYTop);
        glVertex2f(tableXMax, tableYTop - rowHeight*rows);
        glVertex2f(tableXMin, tableYTop - rowHeight*rows);
    glEnd();

    // Next button
    glColor3f(0.2f,0.6f,0.8f);
    glBegin(GL_QUADS);
        glVertex2f(btnXMin,btnYMin);
        glVertex2f(btnXMax,btnYMin);
        glVertex2f(btnXMax,btnYMax);
        glVertex2f(btnXMin,btnYMax);
    glEnd();
    glColor3f(1,1,1);
    std::string btnText = "Next";
    float tx = (btnXMin + btnXMax)/2 - 0.03f;
    float ty = (btnYMin + btnYMax)/2 - 0.01f;
    glRasterPos2f(tx, ty);
    for(char c:btnText) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);
}

// --- Mouse click ---
void mouseClick(int button, int state, int x, int y){
    if(button==GLUT_LEFT_BUTTON && state==GLUT_DOWN){
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        float nx = (float(x)/w)*2-1;
        float ny = 1 - (float(y)/h)*2;
        if(currentPage==Page::INTRO){
            if(nx>=btnXMin && nx<=btnXMax && ny>=btnYMin && ny<=btnYMax){
                currentPage = Page::WEATHER;
                isTyping = true;
            }
        }
    }
}

// --- Display ---
void display(){
    int now=glutGet(GLUT_ELAPSED_TIME);
    int delta=now-lastTimeMs; if(delta<0) delta=0;
    glClear(GL_COLOR_BUFFER_BIT);

    if(currentPage==Page::INTRO){
        drawIntroPage(now);
    } else {
        // WEATHER PAGE
        // Draw map background if loaded
        if (mapLoaded && mapTexture) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mapTexture);
            glColor3f(1.0f,1.0f,1.0f);
            glBegin(GL_QUADS);
                // texture coords (0,0) at bottom-left thanks to stbi_set_flip_vertically_on_load(true)
                glTexCoord2f(0, 0); glVertex2f(-1.0f, -1.0f);
                glTexCoord2f(1, 0); glVertex2f( 1.0f, -1.0f);
                glTexCoord2f(1, 1); glVertex2f( 1.0f,  1.0f);
                glTexCoord2f(0, 1); glVertex2f(-1.0f,  1.0f);
            glEnd();
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        } else {
            // fallback: plain color
            switch(currentWeather){
                case WeatherType::SUNNY: glClearColor(0.53f,0.81f,0.92f,1.0f); break;
                case WeatherType::CLEAR: glClearColor(0.7f,0.9f,1.0f,1.0f); break;
                case WeatherType::CLOUDY: glClearColor(0.7f,0.75f,0.78f,1.0f); break;
                case WeatherType::RAINY: glClearColor(0.45f,0.55f,0.65f,1.0f); break;
                case WeatherType::SNOWY: glClearColor(0.8f,0.9f,0.98f,1.0f); break;
                default: glClearColor(0.6f,0.8f,1.0f,1.0f); break;
            }
            glClear(GL_COLOR_BUFFER_BIT);
        }

        // On top of background map, draw ground band and weather elements
        updateScene(delta);

        glColor3f(0.16f,0.6f,0.16f);
        glBegin(GL_QUADS); glVertex2f(-1,-1); glVertex2f(1,-1); glVertex2f(1,-0.6f); glVertex2f(-1,-0.6f); glEnd();

        if(currentWeather==WeatherType::SUNNY) drawSun(0.7f,0.8f,0.12f,now);
        drawClouds(now);
        if(currentWeather==WeatherType::RAINY||currentWeather==WeatherType::SNOWY) drawParticles();

        // draw pin if visible
        if (pinVisible) drawPin(pinLat, pinLon);

        // HUD
        glColor3f(0,0,0);
        std::ostringstream hud;
        hud << "City: " << currentCity;
        if(currentWeather==WeatherType::UNKNOWN) hud << " (Invalid city)";
        else{
            hud << " | Weather: ";
            switch(currentWeather){
                case WeatherType::SUNNY: hud<<"Sunny"; break;
                case WeatherType::CLEAR: hud<<"Clear"; break;
                case WeatherType::CLOUDY: hud<<"Cloudy"; break;
                case WeatherType::RAINY: hud<<"Rainy"; break;
                case WeatherType::SNOWY: hud<<"Snowy"; break;
                default: hud<<"Clear"; break;
            }
            hud << " | Temp: " << temperature << "°C | Humidity: " << humidity << "% | Wind: " << windSpeed << " m/s";
        }
        glRasterPos2f(-0.98f,0.92f); std::string hudStr=hud.str();
        for(char c:hudStr) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);

        // Typing input
        if(isTyping){
            bool showCursor = (now/500)%2==0;
            std::string msg = "Enter City: " + typedCity + (showCursor ? "_" : " ");
            glRasterPos2f(-0.98f,0.85f);
            for(char c:msg) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);
        }
        if(isUpdatingWeather.load()){ std::string upd="Updating weather..."; glRasterPos2f(-0.98f,0.78f); for(char c:upd) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);}
    }

    glutSwapBuffers();
    lastTimeMs=now;
}

// --- Timer ---
void timerFunc(int v){ glutPostRedisplay(); glutTimerFunc(FRAME_MS,timerFunc,0); }
void refreshWeatherTimer(int v){ if(!currentCity.empty()) {
    // optionally refresh weather here
} glutTimerFunc(WEATHER_REFRESH_MS,refreshWeatherTimer,0); }

// --- Weather fetch (kept but optional) ---
std::string fetchWeatherJson(const std::string &city){
    std::string urlCity = urlEncode(city + ",IN");
    std::string cmd = "curl -s \"http://api.openweathermap.org/data/2.5/weather?q=" + urlCity + "&appid=" + API_KEY + "&units=metric\" > temp.json";
    system(cmd.c_str());
    std::ifstream file("temp.json");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

void updateWeatherFromAPI() {
    if (currentCity.empty()) return;
    if (isUpdatingWeather.load()) return;
    isUpdatingWeather.store(true);

    std::thread([=]() {
        try {
            std::string jsonStr = fetchWeatherJson(currentCity);
            auto j = json::parse(jsonStr);

            if (j.contains("cod") && ( (j["cod"].is_number() && j["cod"] != 200) || (j["cod"].is_string() && j["cod"] != "200") )) {
                currentWeather = WeatherType::UNKNOWN;
                temperature = 0.0f; humidity = 0; windSpeed = 0.0f;
            } else {
                std::string weatherDesc = "";
                if (j.contains("weather") && j["weather"].is_array() && !j["weather"].empty()) {
                    weatherDesc = j["weather"][0]["description"].get<std::string>();
                    currentWeather = mapWeatherString(weatherDesc);
                } else {
                    currentWeather = WeatherType::CLEAR;
                }

                if (j.contains("main")) {
                    if (j["main"].contains("temp")) temperature = j["main"]["temp"].get<float>();
                    if (j["main"].contains("humidity")) humidity = j["main"]["humidity"].get<int>();
                }
                if (j.contains("wind") && j["wind"].contains("speed")) windSpeed = j["wind"]["speed"].get<float>();
            }

            clouds.clear();
            particles.clear();
            for (int i = 0; i < 8; i++)
                clouds.push_back({ randf(-1.5f, 1.5f), randf(0.5f, 0.9f), randf(0.0002f, 0.0006f), randf(0.35f, 0.6f) });

            if (currentWeather == WeatherType::RAINY) {
                maxParticles = 400;
                for (int i = 0; i < maxParticles; i++)
                    particles.push_back({ randf(-1.2f, 1.2f), randf(-0.2f, 1.2f),
                        randf(-0.002f, 0.002f), randf(-0.02f, -0.05f), randf(0.007f, 0.015f) });
            }
            else if (currentWeather == WeatherType::SNOWY) {
                maxParticles = 250;
                for (int i = 0; i < maxParticles; i++)
                    particles.push_back({ randf(-1.2f, 1.2f), randf(-0.2f, 1.2f),
                        randf(-0.002f, 0.002f), randf(-0.003f, -0.01f), randf(0.01f, 0.025f) });
            }
            else maxParticles = 0;

        } catch (...) {
            currentWeather = WeatherType::UNKNOWN;
            temperature = 0.0f; humidity = 0; windSpeed = 0.0f;
        }
        isUpdatingWeather.store(false);
    }).detach();
}

// --- Keyboard ---
void keyboard(unsigned char key,int x,int y){
    if(currentPage==Page::WEATHER && isTyping){
        if(key==13){ // Enter
            if(!typedCity.empty()){
                // find city (case-insensitive)
                std::string name = toLower(typedCity);
                currentCity = typedCity;
                // locate city in DB
                auto it = cities.find(name);
                if (it != cities.end()) {
                    pinLat = it->second.lat;
                    pinLon = it->second.lon;
                    pinVisible = true;
                } else {
                    pinVisible = false; // unknown city
                }
                // optionally fetch weather if API key available
                updateWeatherFromAPI();
            }
            typedCity.clear();
            isTyping = false;
        }
        else if(key==8 || key==127){ // Backspace
            if(!typedCity.empty()) typedCity.pop_back();
        }
        else if(key>=32 && key<=126){
            typedCity.push_back(key);
        }
    } else {
        if(key==27 || key=='q' || key=='Q') exit(0);
        else if(key=='t' || key=='T') isTyping=true;
    }
}

// --- Reshape ---
void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    if(w<=h) glOrtho(-1,1,-float(h)/float(w),float(h)/float(w),-1,1);
    else glOrtho(-float(w)/float(h),float(w)/float(h),-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

// --- Main ---
int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
    glutCreateWindow("OpenGL Weather + Offline India Map");

    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseClick);
    glutTimerFunc(FRAME_MS,timerFunc,0);
    glutTimerFunc(WEATHER_REFRESH_MS,refreshWeatherTimer,0);

    // Try to load local map.jpg (offline)
    if (!loadMapImageToTexture(mapFilename)) {
        std::cerr << "Could not load " << mapFilename << ". Place an India map named '" << mapFilename << "' in the program folder.\n";
        mapLoaded = false;
    }

    glutMainLoop();
    return 0;
}
