#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int screen = 0; // 0 = main menu, 1 = create account, 2 = log in
const char* username = "test";

int mainmenuselection = 1;
void drawMainMenu(u64 kDown) {
    if (kDown & HidNpadButton_Down) {
        mainmenuselection--;
    }
    if (kDown & HidNpadButton_Up) {
        mainmenuselection++;
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
void drawLogIn(u64 kDown) {
    if (kDown & HidNpadButton_Down) {
        loginselection--;
    }
    if (kDown & HidNpadButton_Up) {
        loginselection++;
    }
    if (kDown & HidNpadButton_A) {
        if (loginselection == 1) {
            Result rc = 0;
            SwkbdConfig kbd;
            char result[256];
            
            rc = swkbdCreate(&kbd, 0);
            if (R_SUCCEEDED(rc)) {
                swkbdConfigMakePresetDefault(&kbd);
                swkbdConfigSetInitialText(&kbd, "");
                swkbdConfigSetGuideText(&kbd, "Enter your username");
                swkbdConfigSetStringLenMax(&kbd, 255);
                rc = swkbdShow(&kbd, username, sizeof(username));
                if (R_SUCCEEDED(rc)) {
                    username = result;
                }
                swkbdClose(&kbd);
            }
        } else if (loginselection == 2) {
            // TODO: Implement log in
        }
    }
    if (kDown & HidNpadButton_B) {
        screen = 0;
    }
    if (loginselection == 3) {
        loginselection = 1;
    } else if (loginselection == 0) {
        loginselection = 2;
    }

    drawRect(0, 0, 1280, 40, COL_HEADER);
    drawText(10, 28, "Log In", COL_WHITE, 22);

    drawRect(0, 40, 1280, 40, loginselection == 1 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+40, "Username", COL_WHITE, 22);

    drawRect(0, 82, 1280, 40, loginselection == 2 ? COL_HOVER : COL_PANEL);
    drawText(10, 28+82, "Password", COL_WHITE, 22);
}

void drawError(const char* message, const char* error_code) {
    drawText(10, 48, "oops, something went wrong :/", COL_RED, 50);
    drawText(10, 114, message, COL_WHITE, 35);
    drawText(10, 710, error_code, COL_WHITE, 22);
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
            // drawCreateAccount();
        } else if (screen == 2) {
            drawLogIn(kDown);
        } else {
            drawError("Error: Invalid screen value", "SCR_VAL_INV");
        }

        framebufferEnd(&fb);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    framebufferClose(&fb);
    romfsExit();
    return 0;
}