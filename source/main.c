#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define RGBA(r,g,b,a) (((a) << 24) | ((b) << 16) | ((g) << 8) | (r))
#define COL_BG      RGBA(0x1A, 0x1A, 0x2E, 0xFF)
#define COL_PANEL   RGBA(0x16, 0x21, 0x3E, 0xFF)
#define COL_HOVER   RGBA(0x2A, 0x3A, 0x5E, 0xFF)
#define COL_HEADER  RGBA(0x0F, 0x34, 0x60, 0xFF)
#define COL_WHITE   RGBA(0xFF, 0xFF, 0xFF, 0xFF)
#define COL_RED     RGBA(0xFF, 0x00, 0x00, 0xFF)
#define COL_BLACK   RGBA(0x00, 0x00, 0x00, 0xFF)

static u32* framebuf;
static u32 framebuf_width;
static FT_Library ft;
static FT_Face face;

struct MemoryStruct {
    char *memory;
    size_t size;
};

void drawPixel(int x, int y, u32 color) {
    if (x >= 0 && x < 1280 && y >= 0 && y < 720)
        framebuf[y * framebuf_width + x] = color;
}

void drawRect(int x, int y, int w, int h, u32 color) {
    for (int row = y; row < y + h && row < 720; row++)
        for (int col = x; col < x + w && col < 1280; col++)
            if (row >= 0 && col >= 0)
                framebuf[row * framebuf_width + col] = color;
}

void clearScreen(u32 color) {
    for (int y = 0; y < 720; y++)
        for (int x = 0; x < 1280; x++)
            framebuf[y * framebuf_width + x] = color;
}

void drawGlyph(int x, int y, FT_Bitmap* bmp, u32 color) {
    u8 cr = (color >>  0) & 0xFF;
    u8 cg = (color >>  8) & 0xFF;
    u8 cb = (color >> 16) & 0xFF;

    for (int row = 0; row < (int)bmp->rows; row++) {
        for (int col = 0; col < (int)bmp->width; col++) {
            int px = x + col;
            int py = y + row;
            if (px < 0 || px >= 1280 || py < 0 || py >= 720) continue;

            u8 alpha = bmp->buffer[row * bmp->pitch + col];
            if (alpha == 0) continue;

            u32 existing = framebuf[py * framebuf_width + px];
            u8 er = (existing >>  0) & 0xFF;
            u8 eg = (existing >>  8) & 0xFF;
            u8 eb = (existing >> 16) & 0xFF;

            u8 a = alpha;
            u8 r = (cr * a + er * (255 - a)) / 255;
            u8 g = (cg * a + eg * (255 - a)) / 255;
            u8 b = (cb * a + eb * (255 - a)) / 255;

            framebuf[py * framebuf_width + px] = RGBA(r, g, b, 0xFF);
        }
    }
}

void drawText(int x, int y, const char* text, u32 color, int size) {
    FT_Set_Pixel_Sizes(face, 0, size);

    int cx = x;
    int cy = y;

    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            cx = x;
            cy += size + 4;
            continue;
        }

        if (FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;

        FT_GlyphSlot g = face->glyph;
        int bx = cx + g->bitmap_left;
        int by = cy - g->bitmap_top;

        drawGlyph(bx, by, &g->bitmap, color);

        cx += g->advance.x >> 6;
    }
}

char* openKeyboard(int maxlen, const char* guideText) {
    Result rc = 0;
    SwkbdConfig kbd;
    char* result = malloc(256);
    
    if (!result) return NULL;
    
    rc = swkbdCreate(&kbd, 0);
    if (R_SUCCEEDED(rc)) {
        swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetInitialText(&kbd, "");
        swkbdConfigSetGuideText(&kbd, guideText);
        swkbdConfigSetStringLenMax(&kbd, maxlen);
        rc = swkbdShow(&kbd, result, 256);
        swkbdClose(&kbd);
        
        if (R_SUCCEEDED(rc)) {
            return result;
        }
    }
    
    free(result);
    return NULL;
}

const char* errmsg = "";
const char* errcode = "";

void drawError(const char* message, const char* error_code) {
    drawText(10, 48, "oops, something went wrong :/", COL_RED, 50);
    drawText(10, 114, message, COL_WHITE, 35);
    drawText(10, 710, error_code, COL_WHITE, 22);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        drawError("Not enough memory", "REALLOC_NULL");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

void network_request(const char* url, char** result, const char* method) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "aurorachat-switch/6.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        consoleUpdate(NULL);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            drawError("curl_easy_perform() failed", curl_easy_strerror(res));
            free(chunk.memory);
            *result = NULL;
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            *result = chunk.memory;
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

int screen = 0; // 0 = main menu, 1 = create account, 2 = log in, 3 = error screen, 4 = room selection
const char* username = "";
const char* password = "";

int mainmenuselection = 1;
void drawMainMenu(u64 kDown) {
    if (kDown & HidNpadButton_Down) {
        mainmenuselection++;
    }
    if (kDown & HidNpadButton_Up) {
        mainmenuselection--;
    }
    if (kDown & HidNpadButton_A) {
        screen = mainmenuselection;
    }
    if (mainmenuselection == 3) {
        mainmenuselection = 1;
    } else if (mainmenuselection == 0) {
        mainmenuselection = 2;
    }

    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "AuroraChat Switch", COL_WHITE, 22);

    drawRect(0, 40, 1280, 40, mainmenuselection == 1 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+40, "Create Account", COL_WHITE, 22);

    drawRect(0, 82, 1280, 40, mainmenuselection == 2 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+82, "Log In", COL_WHITE, 22);
}

int loginselection = 1;
bool loginAttempted = false;
char* roomresult = NULL;
int roomselection = 1;
char** rooms = NULL;
int roomcount = 0;
void drawLogIn(u64 kDown) {
    if (kDown & HidNpadButton_Down) {
        loginselection++;
    }
    if (kDown & HidNpadButton_Up) {
        loginselection--;
    }
    if (kDown & HidNpadButton_A) {
        if (loginselection == 1) {
            char* result = openKeyboard(255, "Enter your username");
            if (result) {
                username = result;
            }
        } else if (loginselection == 2) {
            char* result = openKeyboard(255, "Enter your password");
            if (result) {
                password = result;
            }
        } else if (loginselection == 3) {
            if (strlen(username) > 0 && strlen(password) > 0) {
                screen = 4;
                if (!loginAttempted) {
                    char* result = NULL;
                    loginAttempted = true;
                    network_request("http://104.236.25.60:6767/api/rooms", &result, "POST");
                    roomresult = result;
                }
            } else {
                errmsg = "Error: Invalid username or password";
                errcode = "INV_AUTH";
                screen = 3;
            }
        }
    }
    if (kDown & HidNpadButton_B) {
        screen = 0;
    }
    if (loginselection == 4) {
        loginselection = 1;
    } else if (loginselection == 0) {
        loginselection = 3;
    }

    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "Log In", COL_WHITE, 22);

    drawRect(0, 40, 1280, 40, loginselection == 1 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+40, "Username", COL_WHITE, 22);

    drawRect(0, 82, 1280, 40, loginselection == 2 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+82, "Password", COL_WHITE, 22);

    drawRect(0, 124, 1280, 40, loginselection == 3 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+124, "Log In", COL_WHITE, 22);
}

void parseRooms(const char* roomdata) {
    if (!roomdata) return;
    
    if (rooms) {
        for (int i = 0; i < roomcount; i++) {
            free(rooms[i]);
        }
        free(rooms);
        rooms = NULL;
        roomcount = 0;
    }
    char* data = strdup(roomdata);
    char* token = strtok(data, "|");
    
    if (token) {
        roomcount = atoi(token);
        if (roomcount > 0) {
            rooms = malloc(roomcount * sizeof(char*));
            
            for (int i = 0; i < roomcount; i++) {
                token = strtok(NULL, "|");
                if (token) {
                    rooms[i] = strdup(token);
                } else {
                    rooms[i] = strdup("Unknown Room");
                }
            }
        }
    }
    
    free(data);
    roomselection = 1;
}

void drawRoomSelection(u64 kDown) {
    if (roomresult && !rooms) {
        parseRooms(roomresult);
    }
    if (kDown & HidNpadButton_Down) {
        roomselection++;
        if (roomselection > roomcount) roomselection = 1;
    }
    if (kDown & HidNpadButton_Up) {
        roomselection--;
        if (roomselection < 1) roomselection = roomcount;
    }
    if (kDown & HidNpadButton_A) {
        // TODO: Join selected room
        screen = 2;
    }
    if (kDown & HidNpadButton_B) {
        screen = 2;
    }
    
    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "Room Selection", COL_WHITE, 22);

    if (rooms && roomcount > 0) {
        for (int i = 0; i < roomcount; i++) {
            int y_pos = 40 + (i * 42);
            drawRect(0, y_pos, 1280, 40, (i + 1 == roomselection) ? COL_HOVER : COL_PANEL);
            drawText(10, y_pos + 28, rooms[i], COL_WHITE, 22);
        }
    } else {
        drawError("Failed to load rooms", "ROOM_FETCH_FAIL");
    }
}

int main(int argc, char* argv[]) {
    romfsInit();

    FT_Init_FreeType(&ft);
    FT_New_Face(ft, "romfs:/fonts/OpenSans-Regular.ttf", 0, &face);

    NWindow* win = nwindowGetDefault();
    Framebuffer fb;
    framebufferCreate(&fb, win, 1280, 720, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;

        u32 stride;
        framebuf = (u32*)framebufferBegin(&fb, &stride);
        framebuf_width = stride / sizeof(u32);
        clearScreen(COL_BG);
        
        if (screen == 0) {
            drawMainMenu(kDown);
        } else if (screen == 1) {
            drawError("Screen Work in progress", "SCR_WIP");
            //drawCreateAccount(kDown);
        } else if (screen == 2) {
            drawLogIn(kDown);
        } else if (screen == 3) {
            drawError(errmsg, errcode);
        } else if (screen == 4) {
            drawRoomSelection(kDown);
        } else {
            drawError("Error: Invalid screen value", "SCR_VAL_INV");
        }

        framebufferEnd(&fb);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    framebufferClose(&fb);
    socketExit();
    romfsExit();
    return 0;
}