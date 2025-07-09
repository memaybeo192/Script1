#include <iostream>     // Thư viện cho nhập/xuất dữ liệu console (cout, cin)
#include <windows.h>    // Thư viện chính cho các hàm Windows API (GetPixel, PostMessage, Sleep, GetAsyncKeyState, v.v.)
#include <vector>       // Thư viện cho std::vector để lưu trữ cấu hình cột
#include <string>       // Thư viện cho std::string để lưu tên cột
#include <random>       // Thư viện cho việc tạo số ngẫu nhiên (dùng cho độ trễ click)
#include <chrono>       // Thư viện để lấy thời gian hiện tại, dùng làm seed cho bộ tạo số ngẫu nhiên

/*
 * ==============================================================================
 * OSU! MANIA 4K AUTO CLICKER SCRIPT
 * ==============================================================================
 * Mô tả:
 * Script này hoạt động như một Auto Clicker đơn giản được thiết kế đặc biệt cho
 * trò chơi osu!mania ở chế độ 4 phím (4K). Nó hoạt động dựa trên việc đọc màu
 * của các pixel tại những vị trí cụ thể trên màn hình (targetPoint) để phát hiện
 * sự xuất hiện của các nốt nhạc.
 *
 * Cách thức hoạt động:
 * 1.  Định nghĩa các "điểm mục tiêu" (targetPoint) cho mỗi cột nốt nhạc (S, D, J, K)
 * tại vị trí mà nốt nhạc sẽ đi qua vạch phán đoán (judgement line).
 * 2.  Trong một vòng lặp liên tục, script sẽ đọc màu của pixel tại mỗi targetPoint.
 * 3.  Nếu màu pixel đọc được khớp với "màu mục tiêu" (thường là màu trắng của nốt
 * bấm hoặc đầu nốt giữ) trong một "ngưỡng sai số" (colorTolerance) cho phép,
 * script sẽ tự động gửi lệnh nhấn phím tương ứng (S, D, J hoặc K) đến trò chơi.
 * 4.  Các phím F8 và F9 được sử dụng để BẮT ĐẦU và DỪNG auto clicker.
 * 5.  Độ trễ ngẫu nhiên được thêm vào sau mỗi lần bấm phím để mô phỏng hành vi
 * của con người và tránh over-clicking.
 *
 * Yêu cầu:
 * -   osu!mania phải chạy ở chế độ "Borderless Fullscreen" (Toàn màn hình không viền)
 * hoặc "Windowed" (Chế độ cửa sổ) để hàm GetPixel hoạt động chính xác.
 * -   Các tọa độ targetPoint (X, Y) cần được đo đạc cực kỳ chính xác dựa trên
 * độ phân giải màn hình và vị trí vạch phán đoán trong game của bạn.
 *
 * Khắc phục lỗi:
 * -   Nếu auto bỏ lỡ nốt: Rất có thể tọa độ targetPoint đang bị sai lệch.
 * Hãy đo lại chúng bằng cách chụp màn hình F2 trong game và dùng công cụ
 * chỉnh sửa ảnh để xác định pixel chính xác nhất tại vạch phán đoán.
 * -   Nếu auto bấm nhầm: colorTolerance có thể quá cao hoặc có vật thể khác
 * (ví dụ: máu thanh, hiệu ứng) trùng màu tại targetPoint.
 * ==============================================================================
 */

// Cấu trúc (struct) để lưu trữ các thành phần màu RGB (Red, Green, Blue)
struct RGB {
    BYTE red;   // Giá trị màu đỏ (0-255)
    BYTE green; // Giá trị màu xanh lá (0-255)
    BYTE blue;  // Giá trị màu xanh dương (0-255)

    // Hàm tạo (constructor) để khởi tạo các giá trị RGB dễ dàng hơn
    RGB(BYTE r, BYTE g, BYTE b) : red(r), green(g), blue(b) {}
};

// Cấu trúc để định nghĩa cấu hình cho mỗi cột nốt nhạc
struct ColumnConfig {
    POINT targetPoint; // Tọa độ X, Y trên màn hình nơi script sẽ kiểm tra màu pixel
                       // Đây là điểm mà nốt nhạc sẽ đi qua vạch phán đoán.
    RGB targetColor;   // Màu sắc mong đợi của nốt nhạc tại targetPoint.
                       // Thường là màu trắng (255,255,255) cho nốt bấm và đầu nốt giữ.
    char key;          // Phím keyboard tương ứng với cột này (ví dụ: 'S', 'D', 'J', 'K')
    std::string name;  // Tên của cột (ví dụ: "Column S") dùng cho việc in thông báo debug

    // Hàm tạo (constructor) cho ColumnConfig.
    // Hàm này giúp khởi tạo một đối tượng ColumnConfig với tất cả các thuộc tính của nó.
    // Nó được thêm vào để giải quyết lỗi biên dịch "no instance of constructor..."
    ColumnConfig(POINT p, RGB c, char k, std::string n)
        : targetPoint(p), targetColor(c), key(k), name(n) {}
};

// Hàm: Lấy màu pixel tại một tọa độ cụ thể trên màn hình
// Tham số:
//   - x: Tọa độ X của pixel trên màn hình
//   - y: Tọa độ Y của pixel trên màn hình
// Trả về: Một đối tượng RGB chứa màu của pixel đó.
RGB GetPixelFromScreen(int x, int y) {
    // Lấy Device Context (DC) của toàn bộ màn hình
    HDC hdcScreen = GetDC(NULL);
    // Lấy giá trị màu COLORREF của pixel tại tọa độ (x, y)
    COLORREF color = GetPixel(hdcScreen, x, y);
    // Giải phóng Device Context sau khi sử dụng
    ReleaseDC(NULL, hdcScreen);
    // Chuyển đổi COLORREF sang cấu trúc RGB và trả về
    // Ép kiểu rõ ràng các giá trị trả về sang BYTE
    return RGB(static_cast<BYTE>(GetRValue(color)),
               static_cast<BYTE>(GetGValue(color)),
               static_cast<BYTE>(GetBValue(color)));
}

// Hàm: Kiểm tra xem hai màu có khớp nhau trong một ngưỡng cho phép hay không
// Tham số:
//   - color1: Màu pixel đọc được từ màn hình
//   - color2: Màu mục tiêu (màu của nốt nhạc)
//   - tolerance: Ngưỡng sai số cho phép. Ví dụ, tolerance = 100 nghĩa là
//     mỗi thành phần R, G, B có thể lệch tối đa 100 đơn vị so với màu mục tiêu
//     mà vẫn được coi là khớp. Điều này giúp nhận diện các nốt có sắc thái màu
//     khác nhau (ví dụ: nốt giữ màu xám) hoặc ảnh hưởng từ anti-aliasing.
// Trả về: true nếu màu khớp, false nếu không khớp.
bool isColorMatch(RGB color1, RGB color2, int tolerance) {
    return abs(color1.red - color2.red) <= tolerance &&
           abs(color1.green - color2.green) <= tolerance &&
           abs(color1.blue - color2.blue) <= tolerance;
}

// Hàm: Mô phỏng hành động nhấn và nhả phím (dùng cho nốt bấm)
// Tham số:
//   - key: Ký tự của phím cần bấm (ví dụ: 'S', 'D').
void PressKey(char key) {
    // Chuyển đổi ký tự sang mã phím ảo (Virtual Key Code)
    SHORT vkCode = VkKeyScanA(key);
    if (vkCode == -1) {
        std::cerr << "[ERROR] Invalid key: " << key << std::endl;
        return;
    }

    // Gửi thông điệp nhấn phím xuống (key down)
    PostMessage(NULL, WM_KEYDOWN, vkCode, 0);
    // Gửi thông điệp nhả phím lên (key up) ngay sau đó để hoàn tất một lần bấm.
    // Độ trễ giữa keydown và keyup là rất nhỏ, đủ để game nhận diện.
    PostMessage(NULL, WM_KEYUP, vkCode, 0);
}

// Hàm: Mô phỏng hành động nhấn phím xuống (dùng cho nốt giữ, nếu cần phát triển thêm)
// Tham số:
//   - key: Ký tự của phím cần nhấn xuống.
void KeyDown(char key) {
    SHORT vkCode = VkKeyScanA(key);
    if (vkCode == -1) {
        std::cerr << "[ERROR] Invalid key for KeyDown: " << key << std::endl;
        return;
    }
    PostMessage(NULL, WM_KEYDOWN, vkCode, 0);
}

// Hàm: Mô phỏng hành động nhả phím lên (dùng cho nốt giữ, nếu cần phát triển thêm)
// Tham số:
//   - key: Ký tự của phím cần nhả lên.
void KeyUp(char key) {
    SHORT vkCode = VkKeyScanA(key);
    if (vkCode == -1) {
        std::cerr << "[ERROR] Invalid key for KeyUp: " << key << std::endl;
        return;
    }
    PostMessage(NULL, WM_KEYUP, vkCode, 0);
}


int main() {
    // Hàm này giúp đảm bảo rằng các hàm Windows API liên quan đến màn hình
    // (như GetPixel) hoạt động chính xác trên các hệ thống có cài đặt DPI cao.
    SetProcessDPIAware();

    // Định nghĩa cấu hình cho từng cột nốt nhạc (S, D, J, K)
    // Các tọa độ (X, Y) này là CỰC KỲ QUAN TRỌNG và phải được đo chính xác
    // tại vị trí vạch phán đoán trong osu!mania trên màn hình của bạn.
    // POINT{X, Y}: Tọa độ X, Y của pixel mà chương trình sẽ kiểm tra.
    // RGB(255, 255, 255): Màu mục tiêu là trắng tinh khiết (nốt bấm, đầu nốt giữ).
    // 'S', 'D', 'J', 'K': Phím tương ứng.
    // "Column S", v.v.: Tên để hiển thị trong console.
    std::vector<ColumnConfig> columns = {
        // Cột S: Tọa độ bạn cung cấp
        {{743, 959}, RGB(255, 255, 255), 'S', "Column S"}, // Sửa đổi cách khởi tạo POINT
        // Cột D: Tọa độ bạn cung cấp
        {{891, 962}, RGB(255, 255, 255), 'D', "Column D"}, // Sửa đổi cách khởi tạo POINT
        // Cột J: Tọa độ bạn cung cấp
        {{1033, 944}, RGB(255, 255, 255), 'J', "Column J"}, // Sửa đổi cách khởi tạo POINT
        // Cột K: Tọa độ bạn cung cấp
        {{1180, 954}, RGB(255, 255, 255), 'K', "Column K"}  // Sửa đổi cách khởi tạo POINT
    };

    // Ngưỡng sai số màu (Color Tolerance):
    // Cho phép màu đọc được khác biệt với màu mục tiêu một chút nhưng vẫn được coi là khớp.
    // Ví dụ: Với targetColor là trắng (255,255,255) và tolerance = 100,
    // màu xám (186,186,186) sẽ được nhận diện là khớp (vì 255-186 = 69 <= 100).
    // Điều này hữu ích để nhận diện các sắc thái màu khác nhau của nốt (như nốt giữ màu xám)
    // hoặc ảnh hưởng từ hiệu ứng đồ họa/anti-aliasing của game.
    int colorTolerance = 100;

    // Khoảng trễ ngẫu nhiên (Delay Range) sau mỗi lần bấm phím:
    // Thêm một độ trễ nhỏ, ngẫu nhiên sau mỗi lần bấm để:
    // 1. Tránh việc bấm quá nhanh gây ra hiện tượng over-clicking hoặc bỏ lỡ nốt.
    // 2. Mô phỏng hành vi bấm phím của con người, làm cho auto trông tự nhiên hơn.
    int delayMin = 1; // Độ trễ tối thiểu (ms)
    int delayMax = 5; // Độ trễ tối đa (ms)

    // Khởi tạo bộ tạo số ngẫu nhiên cho độ trễ
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(delayMin, delayMax);

    // In ra cấu hình hiện tại của auto clicker để người dùng dễ dàng kiểm tra
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "       OSU! MANIA 4K AUTO CLICKER CONFIG       " << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    for (const auto& config : columns) {
        std::cout << "  " << config.name << " (Phím: " << config.key << "):" << std::endl;
        std::cout << "    Màu mục tiêu (R,G,B): (" << (int)config.targetColor.red << ", "
                  << (int)config.targetColor.green << ", " << (int)config.targetColor.blue << ")" << std::endl;
        std::cout << "    Điểm mục tiêu (X,Y): (" << config.targetPoint.x << ", " << config.targetPoint.y << ")" << std::endl;
    }
    std::cout << "  Ngưỡng sai số màu: " << colorTolerance << std::endl;
    std::cout << "  Khoảng trễ sau click (Tối thiểu, Tối đa): (" << delayMin << ", " << delayMax << ") ms" << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Nhấn F8 để BẮT ĐẦU, F9 để DỪNG." << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    bool running = false; // Biến trạng thái để kiểm soát việc auto có đang chạy hay không

    // Vòng lặp chính của chương trình, chạy liên tục
    while (true) {
        // Kiểm tra phím F8 để bắt đầu auto
        // GetAsyncKeyState(VK_F8) & 0x8000 kiểm tra xem phím F8 có đang được nhấn không
        if (GetAsyncKeyState(VK_F8) & 0x8000) {
            if (!running) { // Nếu auto chưa chạy thì bắt đầu
                running = true;
                std::cout << "[INFO] Auto Clicker ĐÃ BẮT ĐẦU." << std::endl;
                Sleep(200); // Tạm dừng một chút để tránh nhận diện nhiều lần cùng một lúc
            }
        }
        // Kiểm tra phím F9 để dừng auto
        if (GetAsyncKeyState(VK_F9) & 0x8000) {
            if (running) { // Nếu auto đang chạy thì dừng
                running = false;
                std::cout << "[INFO] Auto Clicker ĐÃ DỪNG." << std::endl;
                // Nhả tất cả các phím đang được giữ (quan trọng để tránh kẹt phím trong game)
                for (const auto& config : columns) {
                    KeyUp(config.key);
                }
                Sleep(220); // Tạm dừng để tránh nhận diện nhiều lần và đảm bảo phím được nhả
            }
        }

        if (running) { // Nếu auto đang chạy
            // Lặp qua từng cột đã cấu hình
            for (auto& config : columns) {
                // Lấy màu pixel hiện tại tại targetPoint của cột
                RGB currentPixelColor = GetPixelFromScreen(config.targetPoint.x, config.targetPoint.y);

                // Kiểm tra xem màu hiện tại có khớp với màu mục tiêu trong ngưỡng tolerance không
                if (isColorMatch(currentPixelColor, config.targetColor, colorTolerance)) {
                    // Nếu khớp màu, tức là phát hiện nốt nhạc
                    // Uncomment dòng dưới đây để xem thông báo DEBUG chi tiết trong console
                    // Mỗi khi auto bấm phím. Hữu ích cho việc khắc phục lỗi.
                    /*
                    std::cout << "[DEBUG] " << config.name << " ĐÃ KHỚP. Màu hiện tại: RGB("
                              << (int)currentPixelColor.red << "," << (int)currentPixelColor.green << ","
                              << (int)currentPixelColor.blue << ") - Đang bấm phím " << config.key << std::endl;
                    */
                    PressKey(config.key); // Thực hiện bấm phím tương ứng
                    Sleep(dist(rng));     // Chờ một khoảng thời gian ngẫu nhiên để mô phỏng độ trễ của người
                } else {
                    // Nếu không khớp màu, tức là không có nốt nhạc hoặc là màu nền
                    // Uncomment dòng dưới đây để xem thông báo DEBUG chi tiết trong console
                    // Mỗi khi auto không bấm phím. Rất hữu ích khi nốt bị bỏ lỡ.
                    /*
                    std::cout << "[DEBUG] " << config.name << " KHÔNG KHỚP. Màu hiện tại: RGB("
                              << (int)currentPixelColor.red << "," << (int)currentPixelColor.green << ","
                              << (int)currentPixelColor.blue << ")" << std::endl;
                    */
                }
            }
            // Sleep(0) cho phép CPU chuyển sang các tác vụ khác ngay lập tức nếu có,
            // giúp chương trình phản ứng nhanh nhất có thể với sự thay đổi pixel.
            // Điều chỉnh giá trị này (ví dụ: Sleep(1)) nếu gặp vấn đề về CPU usage quá cao.
            Sleep(0);
        } else {
            // Khi auto không chạy, tạm dừng chương trình một chút để giảm tải CPU
            Sleep(50);
        }
    }

    return 0; // Kết thúc chương trình
}