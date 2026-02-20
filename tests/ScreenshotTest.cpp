#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <raylib.h>

// Simple test to verify screenshot functionality
// This test initializes raylib, takes a screenshot, and verifies the file exists

int main() {
    std::cout << "Screenshot functionality test\n";
    std::cout << "============================\n\n";

    // Initialize raylib window (required for TakeScreenshot)
    InitWindow(800, 600, "Screenshot Test");
    if (!IsWindowReady()) {
        std::cerr << "ERROR: Failed to initialize window\n";
        return 1;
    }

    // Clear to a test color
    BeginDrawing();
    ClearBackground(BLUE);
    DrawText("Screenshot Test", 10, 10, 20, WHITE);
    DrawText("If you see this, the screenshot worked!", 10, 40, 16, WHITE);
    EndDrawing();

    // Generate test filename
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char filename[256];
    std::snprintf(filename, sizeof(filename), "test_screenshot_%04d%02d%02d_%02d%02d%02d.png",
                 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);

    std::cout << "Taking screenshot: " << filename << "\n";
    
    // Take screenshot
    TakeScreenshot(filename);

    // Wait a moment for file to be written
    WaitTime(0.1f);

    // Verify file exists
    std::ifstream file(filename, std::ios::binary);
    if (!file.good()) {
        std::cerr << "ERROR: Screenshot file not found: " << filename << "\n";
        CloseWindow();
        return 1;
    }

    // Check file size (should be > 0 for a valid PNG)
    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    file.close();

    if (size <= 0) {
        std::cerr << "ERROR: Screenshot file is empty or invalid\n";
        CloseWindow();
        return 1;
    }

    std::cout << "SUCCESS: Screenshot saved successfully!\n";
    std::cout << "  File: " << filename << "\n";
    std::cout << "  Size: " << size << " bytes\n";

    // Verify PNG header (should start with PNG signature)
    file.open(filename, std::ios::binary);
    unsigned char header[8];
    file.read(reinterpret_cast<char*>(header), 8);
    file.close();

    // PNG signature: 89 50 4E 47 0D 0A 1A 0A
    if (header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4E && header[3] == 0x47) {
        std::cout << "  Format: Valid PNG file\n";
    } else {
        std::cout << "  WARNING: File may not be a valid PNG (header check failed)\n";
    }

    CloseWindow();
    return 0;
}
