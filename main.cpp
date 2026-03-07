#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <map>
#include <conio.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>

// Networking Libraries
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h> // For SIO_UDP_CONNRESET
#include <natupnp.h>
#include <mutex>
#include "mongoose.h"
#include "index.html.h"
#include <iphlpapi.h>

// Windows Audio (WASAPI) Libraries
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <psapi.h>
#include <wrl.h>
#include <wrl/implements.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "mmdevapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "user32.lib")

using namespace Microsoft::WRL;

// COLOR THEMES (ANSI ESCAPE CODES)
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

void EnableColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

const char* ASCII_LOGO = 
    "\033[38;2;255;20;147m" "       |                _)         |        _)      |             \n"
    "\033[38;2;200;40;180m" "  _ \\    \\    _ \\    \\   |   _|     _ \\   _| |   _` |   _` |   -_)\n"
    "\033[38;2;150;50;210m" " .__/ _| _| \\___/ _| _| _| \\__|   _.__/ _|  _| \\__,_| \\__, | \\___|\n"
    "\033[38;2;100;60;240m" "_|                                                    ____/       \n\n"
    "\033[38;2;0;120;255m"   "          >>> [ Secure Audio Streaming & Network Capture Module ] <<<\n"
    "                                                                  "
    "\033[38;2;255;0;0m""m"
    "\033[38;2;235;0;0m""a"
    "\033[38;2;215;0;0m""d"
    "\033[38;2;195;0;0m""e"
    " "
    "\033[38;2;175;0;0m""b"
    "\033[38;2;155;0;0m""y"
    " "
    "\033[38;2;135;0;0m""x"
    "\033[38;2;115;0;0m""x"
    "\033[38;2;95;0;0m""l"
    "\033[38;2;75;0;0m""r"
    "\033[38;2;55;0;0m""n\n" RESET;

// ======================================================================
// CONFIG & PROFILE MANAGER (.xxlrn)
// ======================================================================
struct AppConfig {
    std::string ip = "0.0.0.0";
    int port = 0000;
    uint32_t pin = 0000;
    int muteKey = 'M';
    DWORD pid = 0;
    std::string username = "";
    bool enableWebStream = false;
    int webPort = 8080;
};
AppConfig g_Config;
bool g_AllowAllWebClients = false;
std::vector<std::string> g_AuthorizedWebIPs;
std::map<std::string, std::string> g_CustomDeviceNames;
std::string g_ActiveProfileName = "";

void loadConfig(const std::string& profile) {
    g_AuthorizedWebIPs.clear();
    g_CustomDeviceNames.clear();
    g_AllowAllWebClients = false;
    
    if (profile.empty()) return;
    g_ActiveProfileName = profile;
    
    std::ifstream f(profile + ".xxlrn");
    if (f.is_open()) {
        std::string line;
        while(std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.find("IP=") == 0) g_Config.ip = line.substr(3);
            else if (line.find("PORT=") == 0) g_Config.port = std::stoi(line.substr(5));
            else if (line.find("PIN=") == 0) g_Config.pin = std::stoul(line.substr(4));
            else if (line.find("MUTEKEY=") == 0) g_Config.muteKey = std::stoi(line.substr(8));
            else if (line.find("PID=") == 0) g_Config.pid = std::stoul(line.substr(4));
            else if (line.find("USERNAME=") == 0) g_Config.username = line.substr(9);
            else if (line.find("WEBSTREAM=") == 0) g_Config.enableWebStream = (line.find("1", 10) != std::string::npos);
            else if (line.find("WEBPORT=") == 0) g_Config.webPort = std::stoi(line.substr(8));
            else if (line.find("ALLOW_ALL_WEB=") == 0) g_AllowAllWebClients = (line.find("1", 14) != std::string::npos);
            else if (line.find("AUTH_WEB_IP=") == 0) g_AuthorizedWebIPs.push_back(line.substr(12));
            else if (line.find("WEB_ALIAS=") == 0) {
                std::string data = line.substr(10);
                size_t sep = data.find('|');
                if(sep != std::string::npos) {
                    g_CustomDeviceNames[data.substr(0, sep)] = data.substr(sep + 1);
                }
            }
        }
    }
}

void saveConfig(const std::string& profile) {
    if (profile.empty()) return;
    std::ofstream f(profile + ".xxlrn");
    f << "IP=" << g_Config.ip << "\n";
    f << "PORT=" << g_Config.port << "\n";
    f << "PIN=" << g_Config.pin << "\n";
    f << "MUTEKEY=" << g_Config.muteKey << "\n";
    f << "PID=" << g_Config.pid << "\n";
    f << "USERNAME=" << g_Config.username << "\n";
    f << "WEBSTREAM=" << (g_Config.enableWebStream ? "1" : "0") << "\n";
    f << "WEBPORT=" << g_Config.webPort << "\n";
    f << "ALLOW_ALL_WEB=" << (g_AllowAllWebClients ? "1" : "0") << "\n";
    for(const auto& authIp : g_AuthorizedWebIPs) f << "AUTH_WEB_IP=" << authIp << "\n";
    for(const auto& alias : g_CustomDeviceNames) f << "WEB_ALIAS=" << alias.first << "|" << alias.second << "\n";
}

// GLOBAL VARIABLES
std::atomic<bool> g_ExitReceiver(false);
std::atomic<bool> g_ExitSender(false);
std::atomic<bool> g_ExitHost(false);
bool g_TargetChangeRequested = false;
std::atomic<bool> g_SenderMuted(false);
std::atomic<bool> g_ReceiverMuted(false);
std::atomic<bool> g_HostMuted(false);
std::atomic<bool> g_WebSocketsActive(false);
std::atomic<bool> g_SignalWebShutdown(false);
bool g_EnableWebStream = false;
float g_Volume = 1.0f; // Default Volume 100%
std::string g_NetworkError = "";
bool g_ShowHelpTexts = true; // Global flag to show/hide helpful tooltips across the UI
bool g_IsModernModeActive = false; // Flag to trace environment input styles (Legacy vs Modern)
int g_LastWebPort = 0; // Tracks live port to aggressively drop stale websockets
std::atomic<bool> g_WebServerThreadSpawned(false);
bool g_EnableLoopback = false;

// WEBSOCKET SERVER PROTOTYPES (Phase 15 - Isolated)
void startWebServerThread();
std::atomic<bool> g_WebThreadActive(false);
void broadcastToWebClients(const char* data, size_t len);
void broadcastTextToWebClients(const std::string& text);
void promptForWebClients(bool isModernMode = true, int muteKey = 0);

// PACKET PROTOCOL
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic;      // 0x50484F4E ('PHON')
    uint8_t type;        // 0=RAW_AUDIO, 1=KEEPALIVE_RECV, 2=KEEPALIVE_SEND, 3=16BIT_AUDIO, 4=KICK, 5=BAN, 6=NAME_REJECTED, 7=HOST_MUTE, 8=HOST_UNMUTE
    uint32_t roomPin;    // e.g. 9999
    char username[25];   // 24 real chars + 1 null terminator
};
#pragma pack(pop)

#define CHECK_HR(hr) \
    if (FAILED(hr)) { \
        std::cout << RED << "[!] ERROR: Line " << __LINE__ << " | Code: 0x" << std::hex << hr << RESET << std::endl; \
        system("pause"); \
        return; \
    }

// ======================================================================
// UPNP (AUTOMATIC PORT FORWARDING) MODULE
// ======================================================================
std::string getLocalIP() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) return "127.0.0.1";
    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) return "127.0.0.1";
    
    for (int i = 0; host->h_addr_list[i] != 0; ++i) {
        struct in_addr addr;
        addr.s_addr = *(u_long*)host->h_addr_list[i];
        std::string ip = inet_ntoa(addr);
        if (ip.find("192.") == 0 || ip.find("10.") == 0 || ip.find("172.") == 0) return ip;
    }
    return "127.0.0.1";
}

bool UPnP_ForwardPort(int port, const std::string& localIP) {
    IUPnPNAT *nat = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(UPnPNAT), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUPnPNAT), (void**)&nat);
    if (FAILED(hr) || !nat) return false;

    IStaticPortMappingCollection *mappings = nullptr;
    hr = nat->get_StaticPortMappingCollection(&mappings);
    if (FAILED(hr) || !mappings) { nat->Release(); return false; }

    BSTR bstrProtocol = SysAllocString(L"UDP");
    
    int len = MultiByteToWideChar(CP_UTF8, 0, localIP.c_str(), -1, NULL, 0);
    wchar_t* wstrInternalClient = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, localIP.c_str(), -1, wstrInternalClient, len);
    BSTR bstrInternalClient = SysAllocString(wstrInternalClient);
    delete[] wstrInternalClient;

    BSTR bstrDescription = SysAllocString(L"PhonicBridge_V1");
    IStaticPortMapping *mapping = nullptr;
    
    mappings->Remove(port, bstrProtocol);
    hr = mappings->Add(port, bstrProtocol, port, bstrInternalClient, VARIANT_TRUE, bstrDescription, &mapping);

    SysFreeString(bstrProtocol);
    SysFreeString(bstrInternalClient);
    SysFreeString(bstrDescription);

    bool success = SUCCEEDED(hr) && mapping != nullptr;
    if (mapping) mapping->Release();
    mappings->Release();
    nat->Release();
    return success;
}

// ======================================================================
// KEYBOARD LISTENER THREADS
// ======================================================================
void printVolumeBar() {
    int percent = (int)std::round(g_Volume * 100.0f);
    int bars = percent / 10;
    if (bars > 20) bars = 20; // Cap visual rendering strictly to 200% (20 chunks)
    std::cout << "\r\033[K" << GREEN << "[~] SYS_VOL: [" << CYAN;
    for(int i=0; i<20; i++) std::cout << ((i < bars) ? "#" : " ");
    std::cout << GREEN << "] " << YELLOW << percent << "%   " << RESET << std::flush;
}

void volumeControlThread(int muteKey) {
    bool wasPressed = false;
    std::cout << GREEN << "\n[+] Audio Control Interface Activated." << RESET << std::endl;
    std::cout << YELLOW << "[>] Hotkeys: [MUTE Bind: " << muteKey << "] Toggle Mute | [RIGHT] Vol Up | [LEFT] Vol Down | [ESC] Disconnect\n" << RESET;
    if (g_EnableWebStream) std::cout << CYAN << "[-] Web Stream Link: \033[97mhttp://" << getLocalIP() << ":" << g_Config.webPort << "\033[0m  " << MAGENTA << "(Press [W] to Manage Web Clients)\n";
    std::cout << GREEN << "[+] MUTE OFF - Audio playback active.\n" << RESET;
    printVolumeBar();
    
    while(!g_ExitReceiver) {
        if (GetAsyncKeyState(muteKey) & 0x8000) {
            if (!wasPressed) {
                g_ReceiverMuted = !g_ReceiverMuted;
                std::cout << "\r\033[A\033[K" << (g_ReceiverMuted ? "\033[31m[-] MUTE ON  - Audio playback suspended.\033[0m" : "\033[32m[+] MUTE OFF - Audio playback active.\033[0m") << "\n";
                printVolumeBar();
                wasPressed = true;
            }
        } else {
            wasPressed = false;
        }

        if (_kbhit()) {
            int ch = _getch();
            if (ch == 27) { // ESC
                std::cout << "\n\n";
                g_ExitReceiver = true;
                break;
            } else if (ch == 224) { // Arrow keys
                int arrow = _getch();
                if (arrow == 77) { // RIGHT
                    g_Volume += 0.05f; if (g_Volume > 2.0f) g_Volume = 2.0f; // Max volume 200%
                    printVolumeBar();
                } else if (arrow == 75) { // LEFT
                    g_Volume -= 0.05f; if (g_Volume < 0.0f) g_Volume = 0.0f;
                    printVolumeBar();
                }
            } else if ((ch == 'W' || ch == 'w') && g_EnableWebStream) {
                promptForWebClients(g_IsModernModeActive, muteKey);
                std::cout << "\033[2J\033[H"; // Clear screen and reset cursor
                std::cout << YELLOW << "\n[*] Joining HOST: " << g_Config.ip << ":" << g_Config.port << " [ROOM " << g_Config.pin << "]" << RESET << std::endl;
                std::cout << GREEN << "\n[+] Audio Control Interface Activated." << RESET << std::endl;
                std::cout << YELLOW << "[>] Hotkeys: [MUTE Bind: " << muteKey << "] Toggle Mute | [RIGHT] Vol Up | [LEFT] Vol Down | [ESC] Disconnect\n" << RESET;
                if (g_EnableWebStream) std::cout << CYAN << "[-] Web Stream Link: \033[97mhttp://" << getLocalIP() << ":" << g_Config.webPort << "\033[0m  " << MAGENTA << "(Press [W] to Manage Web Clients)\n";
                std::cout << (g_ReceiverMuted ? "\033[31m[-] MUTE ON  - Audio playback suspended.\033[0m\n" : "\033[32m[+] MUTE OFF - Audio playback active.\033[0m\n");
                printVolumeBar();
            }
        }
        Sleep(50);
    }
}

void senderMuteThread(int muteKey) {
    bool wasPressed = false;
    std::cout << YELLOW << "\n[>] Hotkeys: [MUTE Bind: " << muteKey << "] Toggle Mute | [P] Change Target PID | [ESC] Disconnect\n" << RESET;
    std::cout << GREEN << "[+] MUTE OFF - Transmitter active." << RESET;
    while(!g_ExitSender) {
        if (GetAsyncKeyState(muteKey) & 0x8000) {
            if (!wasPressed) {
                g_SenderMuted = !g_SenderMuted;
                if (g_SenderMuted) std::cout << "\r\033[K" << RED << "[-] MUTE ON  - Transmitter suspended (KeepAlives only)." << RESET;
                else std::cout << "\r\033[K" << GREEN << "[+] MUTE OFF - Transmitter active." << RESET;
                wasPressed = true;
            }
        } else {
            wasPressed = false;
        }
        
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 27) { // 27 = VK_ESCAPE
                std::cout << "\n\n";
                g_ExitSender = true;
                break;
            } else if (ch == 'P' || ch == 'p') {
                g_TargetChangeRequested = true;
                g_ExitSender = true;
                break;
            }
        }
        Sleep(50);
    }
}

// ======================================================================
// WINDOW DISCOVERY
// ======================================================================
struct TargetProcessInfo {
    DWORD pid;
    std::string name;
    std::string title;
};

std::vector<TargetProcessInfo> g_ActiveWindows;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (IsWindowVisible(hwnd)) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        if (strlen(title) > 0) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            char processName[256] = "<unknown>";
            if (hProcess) {
                GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName));
                CloseHandle(hProcess);
            }
            if (strcmp(processName, "<unknown>") != 0) {
                g_ActiveWindows.push_back({pid, std::string(processName), std::string(title)});
            }
        }
    }
    return TRUE;
}

TargetProcessInfo selectWindowByClick() {
    std::cout << "\n" << MAGENTA << "[ TARGET MODE ] " << YELLOW << "Please click on the window you want to capture, or press ESC to cancel..." << RESET << "\n";
    
    // Get current console cursor position to restore it later
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hOut, &csbi);
    COORD originalCursorPos = csbi.dwCursorPosition;

    // Wait for the mouse button to be released first (in case it was held down)
    while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) { Sleep(10); }

    HWND hwnd = NULL;
    DWORD pid = 0;
    char processName[256] = "<unknown>";
    
    while (true) {
        // Check for ESC key press to cancel
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            // Restore cursor position and clear line
            SetConsoleCursorPosition(hOut, originalCursorPos);
            std::cout << "\033[K" << YELLOW << "[ TARGET MODE ] Selection cancelled." << RESET << "\n";
            return {0, "", ""}; // Return empty/invalid info
        }

        // Wait for a new click
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            hwnd = GetForegroundWindow();
            GetWindowThreadProcessId(hwnd, &pid);

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (hProcess) {
                GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName));
                CloseHandle(hProcess);
            }
            
            // Wait for release again so we don't spam
            while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) { Sleep(10); }
            break; // Click detected and processed, exit loop
        }
        Sleep(10);
    }

    // Restore cursor position and clear line
    SetConsoleCursorPosition(hOut, originalCursorPos);
    std::cout << "\033[K" << GREEN << "[ TARGET MODE ] Window selected: " << YELLOW << processName << RESET << "\n";

    return {pid, std::string(processName), ""};
}

class AudioInterfaceCompletionHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>, FtmBase, IActivateAudioInterfaceCompletionHandler>
{
public:
    HANDLE m_hEvent;
    ComPtr<IAudioClient> m_AudioClient;

    AudioInterfaceCompletionHandler() { m_hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr); }
    ~AudioInterfaceCompletionHandler() { if (m_hEvent) CloseHandle(m_hEvent); }

    STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation *operation) override {
        HRESULT hr;
        ComPtr<IUnknown> punkAudioInterface;
        operation->GetActivateResult(&hr, &punkAudioInterface);
        if (SUCCEEDED(hr) && punkAudioInterface) { punkAudioInterface.As(&m_AudioClient); }
        SetEvent(m_hEvent);
        return S_OK;
    }
};

// ======================================================================
// NETWORKING GLOBALS
// ======================================================================
// ======================================================================
// [3] HOST MODULE (UDP Relay & Live Tracker)
// ======================================================================
struct ClientInfo {
    sockaddr_in addr;
    DWORD lastSeen;
    int type; // 1=Receiver, 2=Sender
    std::string username;
    std::string ipStr;
    int port;
    bool muted = false;
};

struct WebClient {
    struct mg_connection* conn;
    std::string ip;
    bool is_authorized;
};

std::vector<WebClient> ws_clients;
std::mutex ws_mutex;

// Thread-safe broadcast queues. Audio/text data is pushed here from any thread,
// then drained exclusively by the Mongoose thread inside MG_EV_POLL, eliminating race conditions.
std::mutex g_wsBroadcastMutex;
std::vector<std::vector<char>> g_wsAudioQueue;
std::vector<std::string> g_wsTextQueue;

std::map<std::string, std::string> g_DeviceNames;
std::mutex g_DeviceNamesMutex;

void resolveDeviceName(std::string ip) {
    if (ip == "127.0.0.1" || ip == "0.0.0.0") return;
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip.c_str());
    char hostname[NI_MAXHOST];
    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD) == 0) {
        std::lock_guard<std::mutex> lock(g_DeviceNamesMutex);
        g_DeviceNames[ip] = hostname;
    } else {
        std::lock_guard<std::mutex> lock(g_DeviceNamesMutex);
        g_DeviceNames[ip] = "Unknown";
    }
}

void startHost(int listenPort) {
    auto safeStoul = [](const std::string& s, uint32_t def) -> uint32_t {
        try { return std::stoul(s); } catch(...) { return def; }
    };
    g_ExitHost = false;
    SOCKET hostSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    sockaddr_in hostAddr;
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(listenPort);
    hostAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(hostSocket, (SOCKADDR*)&hostAddr, sizeof(hostAddr)) == SOCKET_ERROR) {
        std::cout << RED << "\n[!] CRITICAL ERROR: Port " << listenPort << " is already in use by another Host or application!\n" << RESET;
        std::cout << YELLOW << "[!] Please select a different port to launch your server on.\n\n" << RESET;
        std::cout << "Press ENTER to return to the main menu...\n";
        std::cin.get();
        return;
    }

#ifndef SIO_UDP_CONNRESET
    #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    #endif
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(hostSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);

    std::map<uint32_t, std::vector<ClientInfo>> activeRooms;
    struct BannedUser { std::string ip; std::string name; };
    std::vector<BannedUser> bannedList;
    std::vector<std::string> mutedIPs;
    char buffer[65535];

    std::vector<std::string> hostLogs;
    std::string tempError = "";

    auto addLog = [&](const std::string& msg) {
        time_t t = time(0);
        struct tm* now = localtime(&t);
        char buf[80]; strftime(buf, sizeof(buf), "[\033[90m%H:%M:%S\033[0m] ", now);
        
        std::string coloredMsg = msg;
        if (msg.find("joined!") != std::string::npos) coloredMsg = "\033[32m" + msg + "\033[0m"; // GREEN
        else if (msg.find("left!") != std::string::npos) coloredMsg = "\033[31m" + msg + "\033[0m"; // RED
        else if (msg.find("opened") != std::string::npos || msg.find("cleared") != std::string::npos || 
                 msg.find("Exported") != std::string::npos || msg.find("Kicked") != std::string::npos ||
                 msg.find("Banned") != std::string::npos || msg.find("unban") != std::string::npos || msg.find("Unbanned") != std::string::npos ||
                 msg.find("Muted") != std::string::npos || msg.find("Unmuted") != std::string::npos ||
                 msg.find("Shutting") != std::string::npos || msg.find("admin") != std::string::npos ||
                 msg.find("ACTIVE ON") != std::string::npos || msg.find("closed") != std::string::npos) {
            coloredMsg = "\033[33m" + msg + "\033[0m"; // YELLOW
        }
        
        hostLogs.insert(hostLogs.begin(), std::string(buf) + coloredMsg);
    };

    system("cls");
    addLog("HOST SERVER ACTIVE ON LOCAL PORT " + std::to_string(listenPort));
    addLog("Type 'help' for admin commands.");

    fd_set readfds;
    timeval tv;

    DWORD lastStatusPrint = GetTickCount();
    int lastSenders = -1, lastReceivers = -1;
    
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode;
    GetConsoleMode(hIn, &oldMode);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT);
    std::string adminCmd = "";
    
    bool selectingUser = false;
    int selectedIndex = -1;
    int selectedAction = 0; // 0=Kick, 1=Ban, 2=Mute, 3=Unmute
    std::vector<ClientInfo*> flatClients;
    bool isBannedList = false;
    bool flashGreen = false;
    std::atomic<bool> inHelpMenu(false);

    struct OngoingKick {
        bool active = false;
        DWORD startTime = 0;
        DWORD lastPacketTime = 0;
        std::string ip;
        std::string name;
        uint32_t roomPin;
        ClientInfo data;
    };
    OngoingKick ongoingKick;

    auto redrawHostList = [&]() {
        if (inHelpMenu) return;
        flatClients.clear();
        DWORD now = GetTickCount();
        for (auto& rp : activeRooms) {
            for (auto& c : rp.second) { if (now - c.lastSeen < 10000) flatClients.push_back(&c); }
        }

        std::cout << "\033[H\033[0J"; // Home and clear screen
        
        std::sort(flatClients.begin(), flatClients.end(), [](ClientInfo* a, ClientInfo* b) {
            bool aIsSender = (a->type != 1);
            bool bIsSender = (b->type != 1);
            if (aIsSender != bIsSender) return aIsSender; // SENDERS come first
            std::string n1 = (a->username != "Guest") ? a->username : a->ipStr;
            std::string n2 = (b->username != "Guest") ? b->username : b->ipStr;
            return n1 < n2; // Alphabetical
        });

        std::cout << CYAN << "=== [ LIVE CONNECTIONS ] ===\n" << RESET;
        std::cout << "ID   " << std::left << std::setw(26) << "Username" 
                  << std::setw(23) << "IP:Port" 
                  << std::setw(12) << "Type" 
                  << std::setw(11) << "Status" 
                  << "Room\n" << std::string(80, '-') << "\n";
        
        int totalList = isBannedList ? bannedList.size() : flatClients.size();
        int maxItems = 7;
        int startIdx = 0;
        
        if (totalList > maxItems) {
            maxItems = 5;
            int selIdx = (selectingUser && selectedIndex > 0) ? selectedIndex - 1 : 0;
            if (selIdx >= maxItems / 2) startIdx = selIdx - maxItems / 2;
            if (startIdx + maxItems > totalList) startIdx = totalList - maxItems;
            if (startIdx < 0) startIdx = 0;
        }

        if (isBannedList) {
            if (totalList > maxItems && startIdx > 0) std::cout << CYAN << "   ... (" << startIdx << " more above) ...\033[K\n" << RESET;
            
            for (int i = startIdx; i < (std::min)((int)totalList, startIdx + maxItems); ++i) {
                if (selectingUser && selectedIndex == i + 1) {
                    if (selectedAction > -1) std::cout << (flashGreen ? "\033[42;30m" : "\033[44;97m"); // Flash or Blue
                }
                std::cout << std::left << std::setw(5) << (i + 1)
                          << std::setw(26) << bannedList[i].name
                          << std::setw(23) << bannedList[i].ip
                          << std::setw(12) << "BANNED" << std::setw(20) << "RESTRICTED\033[K" << RESET << "\n";
            }
            if (totalList == 0) std::cout << "No banned users.\n";
            
            if (totalList > maxItems && startIdx + maxItems < totalList) std::cout << CYAN << "   ... (" << totalList - (startIdx + maxItems) << " more below) ...\033[K\n" << RESET;
        } else {
            if (totalList > maxItems && startIdx > 0) std::cout << CYAN << "   ... (" << startIdx << " more above) ...\033[K\n" << RESET;
            
            for (int i = startIdx; i < (std::min)((int)totalList, startIdx + maxItems); ++i) {
                if (selectingUser && selectedIndex == i + 1) {
                    if (selectedAction > -1) std::cout << (flashGreen ? "\033[42;30m" : "\033[44;97m"); // Flash or Blue
                }
                uint32_t activeRoomPin = 0;
                for (auto& rpm : activeRooms) {
                    for (auto& cg : rpm.second) { if (cg.ipStr == flatClients[i]->ipStr && cg.port == flatClients[i]->port) activeRoomPin = rpm.first; }
                }
                
                std::cout << std::left << std::setw(5) << (i + 1)
                          << std::setw(26) << flatClients[i]->username
                          << std::setw(23) << (flatClients[i]->ipStr + ":" + std::to_string(flatClients[i]->port))
                          << std::setw(12) << (flatClients[i]->type != 1 ? "SENDER" : "RECEIVER")
                          << std::setw(20) << (flatClients[i]->muted ? "\033[31mMUTED\033[0m" : "\033[32mACTIVE\033[0m") 
                          << "[ " << std::setw(4) << activeRoomPin << " ]"
                          << "\033[K" << RESET << "\n";
            }
            if (totalList == 0) std::cout << "No active connections right now.\n";
            
            if (totalList > maxItems && startIdx + maxItems < totalList) std::cout << CYAN << "   ... (" << totalList - (startIdx + maxItems) << " more below) ...\033[K\n" << RESET;
        }
        
        if (selectingUser) {
            std::cout << "\nAction: ";
            if (isBannedList) {
                if (selectedAction == 0) std::cout << "\033[42;30m[UNBAN]\033[0m "; else std::cout << "[UNBAN] ";
            } else {
                if (selectedAction == 0) std::cout << "\033[42;30m[KICK]\033[0m "; else std::cout << "[KICK] ";
                if (selectedAction == 1) std::cout << "\033[42;30m[BAN]\033[0m "; else std::cout << "[BAN] ";
                if (selectedAction == 2) std::cout << "\033[42;30m[MUTE]\033[0m "; else std::cout << "[MUTE] ";
                if (selectedAction == 3) std::cout << "\033[42;30m[UNMUTE]\033[0m "; else std::cout << "[UNMUTE] ";
            }
            std::cout << "\n";
        }

        // Detailed instruction block shown only if Help Texts are toggled ON in Boot Menu
        if (g_ShowHelpTexts) {
            std::cout << "\n\033[92m[HELP] Press TAB to manage users with Kick/Ban/Mute. Type 'exit' to return to Menu.\n"
                      << "[HELP] Or type commands directly: 'kick <name>', 'ban <IP>', 'unban <IP>'... (Type 'help' for more information)\033[0m\n";
        }
        
        std::cout << CYAN << "============================\n" << RESET;
        
        if (!tempError.empty()) std::cout << RED << tempError << "\033[K" << RESET << "\n";
        else std::cout << "\033[K\n";
        
        std::cout << "\rHost> " << adminCmd << "\033[K\n\n";
        
        std::cout << CYAN << "--- EVENT LOGS ---\n" << RESET;
        int linesLeft = 10;
        int printedLogs = 0;
        for (const auto& log : hostLogs) {
            std::cout << log << "\033[K\n";
            printedLogs++;
            if (--linesLeft <= 0) break;
        }
        
        std::cout << "\033[" << (printedLogs + 3) << "A\r\033[" << (6 + adminCmd.length()) << "C";
        fflush(stdout);
    };

    redrawHostList();

    while (!g_ExitHost) {
        // NON-BLOCKING INPUT
        DWORD numEvents = 0;
        GetNumberOfConsoleInputEvents(hIn, &numEvents);
        if (numEvents > 0) {
            INPUT_RECORD ir[32];
            DWORD read;
            ReadConsoleInput(hIn, ir, 32, &read);
            for (DWORD i = 0; i < read; ++i) {
                if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                    WORD vKey = ir[i].Event.KeyEvent.wVirtualKeyCode;
                    char ch = ir[i].Event.KeyEvent.uChar.AsciiChar;
                    
                    if (vKey == VK_RETURN) {
                        if (selectingUser) {
                            std::string tgt = "";
                            if (selectedIndex > 0) {
                                if (isBannedList && selectedIndex <= bannedList.size()) {
                                    tgt = bannedList[selectedIndex - 1].ip;
                                } else if (!isBannedList && selectedIndex <= flatClients.size()) {
                                    tgt = (flatClients[selectedIndex - 1]->username != "Guest" ? flatClients[selectedIndex - 1]->username : flatClients[selectedIndex - 1]->ipStr);
                                    uint32_t activeRoomPin = 0;
                                    for (auto& rpm : activeRooms) {
                                        for (auto& cg : rpm.second) { if (cg.ipStr == flatClients[selectedIndex - 1]->ipStr && cg.port == flatClients[selectedIndex - 1]->port) activeRoomPin = rpm.first; }
                                    }
                                    if (activeRoomPin != 0 && selectedAction != 1) { // Not Ban
                                        tgt += " " + std::to_string(activeRoomPin);
                                    }
                                }
                            }
                            
                            if (!tgt.empty()) {
                                flashGreen = true;
                                redrawHostList();
                                Sleep(300);
                                flashGreen = false;
                                selectingUser = false;
                                
                                if (isBannedList) adminCmd = "unban " + tgt;
                                else {
                                    if (selectedAction == 0) adminCmd = "kick " + tgt;
                                    else if (selectedAction == 1) adminCmd = "ban " + tgt;
                                    else if (selectedAction == 2) adminCmd = "mute " + tgt;
                                    else if (selectedAction == 3) adminCmd = "unmute " + tgt;
                                }
                            } else {
                                selectingUser = false;
                                continue;
                            }
                        }
                        
                        std::string cmdStr = adminCmd; adminCmd = "";
                        
                        std::string raw = cmdStr;
                        size_t fnz = raw.find_first_not_of(" ");
                        if (fnz != std::string::npos) raw = raw.substr(fnz);
                        else raw = "";
                        
                        std::string cmd = "";
                        std::string targetPrm = "";
                        uint32_t targetRoom = 0;
                        
                        if (!raw.empty()) {
                            size_t trailingNonSpace = raw.find_last_not_of(" ");
                            if (trailingNonSpace != std::string::npos) {
                                size_t lastSpaceBeforeWord = raw.find_last_of(" ", trailingNonSpace);
                                std::string potentialRoom = "";
                                if (lastSpaceBeforeWord != std::string::npos) {
                                    potentialRoom = raw.substr(lastSpaceBeforeWord + 1, trailingNonSpace - lastSpaceBeforeWord);
                                } else {
                                    potentialRoom = raw.substr(0, trailingNonSpace + 1);
                                }
                                
                                if (!potentialRoom.empty() && std::all_of(potentialRoom.begin(), potentialRoom.end(), ::isdigit)) {
                                    targetRoom = safeStoul(potentialRoom, 0);
                                    if (lastSpaceBeforeWord != std::string::npos) raw = raw.substr(0, lastSpaceBeforeWord);
                                    else raw = "";
                                }
                            }
                            
                            if (!raw.empty()) {
                                size_t firstSpc = raw.find_first_of(" ");
                                size_t lastSpc = raw.find_last_of(" ");
                                
                                std::string w1 = (firstSpc != std::string::npos) ? raw.substr(0, firstSpc) : raw;
                                std::string wEnd = (lastSpc != std::string::npos) ? raw.substr(lastSpc + 1) : raw;
                                
                                std::string w1L = w1; std::transform(w1L.begin(), w1L.end(), w1L.begin(), ::tolower);
                                std::string wEndL = wEnd; std::transform(wEndL.begin(), wEndL.end(), wEndL.begin(), ::tolower);
                                
                                auto isAction = [](const std::string& s) { return s == "kick" || s == "ban" || s == "mute" || s == "unmute" || s == "unban" || s == "glog" || s == "clear" || s == "exit" || s == "quit" || s == "q" || s == "help" || s == "h" || s == "?"; };
                                
                                if (isAction(w1L)) {
                                    cmd = w1L;
                                    if (firstSpc != std::string::npos) targetPrm = raw.substr(firstSpc + 1);
                                } else if (isAction(wEndL)) {
                                    cmd = wEndL;
                                    if (lastSpc != std::string::npos) targetPrm = raw.substr(0, lastSpc);
                                } else {
                                    cmd = w1L;
                                }
                            }
                        }
                        
                        if (cmd == "") {
                            // Do nothing.
                            } else if (cmd == "help" || cmd == "h" || cmd == "?") {
                                inHelpMenu = true;
                                system("cls"); // Clear entire screen explicitly for legacy host
                                std::cout << CYAN << "=== HELP MENU ===\n" << RESET;
                                std::cout << YELLOW << "kick <name>   - Disconnect user\n";
                                std::cout << "ban <name>    - Ban user IP\n";
                                std::cout << "unban <ip>    - Remove IP from banlist\n";
                                std::cout << "mute <name>   - Stop routing user audio\n";
                                std::cout << "unmute <name> - Restore audio routing\n";
                                std::cout << "glog <file>   - Export logs to file\n";
                                std::cout << "clear         - Clear console logs\n";
                                std::cout << "exit          - Stop server safely\n" << RESET;
                                std::cout << CYAN << "-------------------------------------\n" << RESET;
                                std::cout << "\033[90mPress ESC to return to Host Screen.\033[0m" << std::flush;
                                
                                while (true) {
                                    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
                                    Sleep(50);
                                }
                                while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(10); // Wait for release
                                while (_kbhit()) _getch(); // Flush input buffer
                                
                                inHelpMenu = false;
                                system("cls"); // Clear screen again to wipe help menu
                                addLog("Help window closed.");
                                redrawHostList();
                            } else if (cmd == "exit" || cmd == "quit" || cmd == "q") {
                                addLog("Shutting down Host...");
                                SetConsoleTitleA("PhonicBridge");
                                g_ExitHost = true; break;
                            } else if (cmd == "glog") {
                                std::string lgName = targetPrm;
                                if (lgName.empty()) {
                                    time_t t = time(0); struct tm* now = localtime(&t);
                                    char buf[80]; strftime(buf, sizeof(buf), "HostLog_%Y%m%d_%H%M%S.txt", now);
                                    lgName = buf;
                                }
                                if (lgName.find(".txt") == std::string::npos) lgName += ".txt";
                                
                                std::string baseName = lgName.substr(0, lgName.find_last_of('.'));
                                std::string ext = ".txt";
                                int counter = 1;
                                std::string finalName = lgName;
                                while (std::ifstream(finalName).good()) {
                                    finalName = baseName + "(" + std::to_string(counter++) + ")" + ext;
                                }
                                
                                std::ofstream lg(finalName, std::ios::app);
                                if (lg.is_open()) {
                                    for (auto it = hostLogs.rbegin(); it != hostLogs.rend(); ++it) {
                                        std::string line = *it;
                                        size_t pos;
                                        while ((pos = line.find("\033[")) != std::string::npos) {
                                            size_t end = line.find('m', pos);
                                            if (end != std::string::npos) line.erase(pos, end - pos + 1);
                                            else break;
                                        }
                                        lg << line << "\n";
                                    }
                                    lg.close();
                                    addLog("[+] Exported logs to " + finalName + " successfully.");
                                } else {
                                    tempError = "Failed to create " + finalName;
                                }
                            } else if (cmd == "clear" || cmd == "c" || cmd == "cl") {
                                hostLogs.clear();
                                addLog("Logs cleared.");
                            } else if (cmd == "list" || cmd == "l") {
                                addLog("Live connections are already visible above.");
                            } else if (!targetPrm.empty()) {
                                if (cmd == "kick" || cmd == "ban" || cmd == "mute" || cmd == "unmute") {
                                    bool found = false;
                                    for (auto& roomPair : activeRooms) {
                                        if (targetRoom != 0 && roomPair.first != targetRoom) continue;
                                        for (auto it = roomPair.second.begin(); it != roomPair.second.end(); ) {
                                            if (it->username == targetPrm || it->ipStr == targetPrm) {
                                                found = true;
                                                if (cmd == "kick") {
                                                    PacketHeader kickHdr = {0x50484F4E, 4, roomPair.first};
                                                    sendto(hostSocket, (char*)&kickHdr, sizeof(kickHdr), 0, (SOCKADDR*)&it->addr, sizeof(sockaddr_in));
                                                    
                                                    ongoingKick.active = true;
                                                    ongoingKick.startTime = GetTickCount();
                                                    ongoingKick.lastPacketTime = 0;
                                                    ongoingKick.ip = it->ipStr;
                                                    ongoingKick.name = it->username;
                                                    ongoingKick.roomPin = roomPair.first;
                                                    ongoingKick.data = *it;
                                                    
                                                    tempError = "please wait. we are doing a safety check (Est: 2-3s)";
                                                    
                                                    it = roomPair.second.erase(it);
                                                    break; // Break inner loop after action
                                                } else if (cmd == "ban") {
                                                    PacketHeader banHdr = {0x50484F4E, 5, roomPair.first};
                                                    sendto(hostSocket, (char*)&banHdr, sizeof(banHdr), 0, (SOCKADDR*)&it->addr, sizeof(sockaddr_in));
                                                    addLog("[!] Banned: " + it->ipStr + " (" + it->username + ")");
                                                    bannedList.push_back({it->ipStr, it->username});
                                                    it = roomPair.second.erase(it);
                                                    break; // Break inner loop after action
                                                } else if (cmd == "mute") {
                                                    it->muted = true;
                                                    if (std::find(mutedIPs.begin(), mutedIPs.end(), it->ipStr) == mutedIPs.end()) mutedIPs.push_back(it->ipStr);
                                                    addLog("[-] Muted audio from " + it->username);
                                                    PacketHeader muteHdr = {0x50484F4E, 7, roomPair.first};
                                                    sendto(hostSocket, (char*)&muteHdr, sizeof(muteHdr), 0, (SOCKADDR*)&it->addr, sizeof(sockaddr_in));
                                                    break; // Break inner loop after action
                                                } else if (cmd == "unmute") {
                                                    it->muted = false;
                                                    mutedIPs.erase(std::remove(mutedIPs.begin(), mutedIPs.end(), it->ipStr), mutedIPs.end());
                                                    addLog("[+] Unmuted audio from " + it->username);
                                                    PacketHeader unmuteHdr = {0x50484F4E, 8, roomPair.first};
                                                    sendto(hostSocket, (char*)&unmuteHdr, sizeof(unmuteHdr), 0, (SOCKADDR*)&it->addr, sizeof(sockaddr_in));
                                                    break; // Break inner loop after action
                                                }
                                            } else {
                                                ++it;
                                            }
                                        }
                                        if (found) break; // Break outer loop once the user is found and actioned on
                                    }
                                    if (!found) tempError = "User '" + targetPrm + "' not found.";
                                } else if (cmd == "unban") {
                                    auto it = std::find_if(bannedList.begin(), bannedList.end(), [&](const BannedUser& u) { return u.ip == targetPrm || u.name == targetPrm; });
                                    if (it != bannedList.end()) {
                                        addLog("[+] " + it->name + " (" + it->ip + ") unbanned.");
                                        bannedList.erase(it);
                                        isBannedList = false;
                                        selectingUser = false;
                                        redrawHostList();
                                    } else {
                                        tempError = "IP/Name not in banlist.";
                                    }
                                }
                            } else {
                                tempError = "Missing target. E.g. '" + cmd + " username'";
                            }
                        
                        redrawHostList(); 
                        
                    } else if (vKey == VK_ESCAPE) {
                        if (selectingUser) {
                            selectingUser = false;
                            redrawHostList();
                        } else if (!adminCmd.empty()) {
                            adminCmd = "";
                            tempError = "";
                            redrawHostList();
                        }
                    } else if (vKey == VK_TAB) {
                        if (!selectingUser) {
                            selectingUser = true; selectedIndex = 1;
                            std::string cmdTest = adminCmd; std::transform(cmdTest.begin(), cmdTest.end(), cmdTest.begin(), ::tolower);
                            isBannedList = (cmdTest.find("unban") != std::string::npos);
                        } else {
                            selectingUser = false;
                        }
                        redrawHostList();
                    } else if (vKey == VK_UP && selectingUser) {
                        int maxItems = isBannedList ? bannedList.size() : flatClients.size();
                        if (selectedIndex <= 1) selectedIndex = maxItems; else selectedIndex--;
                        redrawHostList();
                    } else if (vKey == VK_DOWN && selectingUser) {
                        int maxItems = isBannedList ? bannedList.size() : flatClients.size();
                        if (selectedIndex >= maxItems) selectedIndex = 1; else selectedIndex++;
                        redrawHostList();
                    } else if (vKey == VK_LEFT && selectingUser) {
                        if (!isBannedList && selectedAction > 0) selectedAction--;
                        redrawHostList();
                    } else if (vKey == VK_RIGHT && selectingUser) {
                        if (!isBannedList && selectedAction < 3) selectedAction++;
                        redrawHostList();
                    } else if (vKey == VK_BACK) {
                        if (selectingUser) { selectingUser = false; }
                        else if (!adminCmd.empty()) { adminCmd.pop_back(); tempError = ""; }
                        redrawHostList();
                    } else if (ch >= 32 && ch <= 126) {
                        if (selectingUser) { selectingUser = false; }
                        adminCmd += ch; tempError = "";
                        redrawHostList();
                    }
                }
            }
        }
        if (g_ExitHost) break;

        FD_ZERO(&readfds);
        FD_SET(hostSocket, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 10000; 
        
        if (select(0, &readfds, NULL, NULL, &tv) > 0) {
            sockaddr_in clientAddr;
            int clientLen = sizeof(clientAddr);
            int bytes = recvfrom(hostSocket, buffer, sizeof(buffer), 0, (SOCKADDR*)&clientAddr, &clientLen);

            if (bytes > 0 && (size_t)bytes >= sizeof(PacketHeader)) {
                std::string c_ip = inet_ntoa(clientAddr.sin_addr);
                PacketHeader* hdr = (PacketHeader*)buffer;
                
                if (hdr->magic == 0x50484F4E && ongoingKick.active && c_ip == ongoingKick.ip && hdr->roomPin == ongoingKick.roomPin) {
                    ongoingKick.lastPacketTime = GetTickCount();
                    continue; // Skip processing this packet during safety check
                }

                auto it = std::find_if(bannedList.begin(), bannedList.end(), [&](const BannedUser& u) { return u.ip == c_ip; });
                if (it != bannedList.end()) {
                    if (hdr->magic == 0x50484F4E) {
                        PacketHeader rejHdr = {0}; rejHdr.magic = 0x50484F4E; rejHdr.type = 5; rejHdr.roomPin = hdr->roomPin;
                        sendto(hostSocket, (char*)&rejHdr, sizeof(rejHdr), 0, (SOCKADDR*)&clientAddr, clientLen);
                    }
                } else {
                    if (hdr->magic == 0x50484F4E) {
                        DWORD now = GetTickCount();
                        auto& roomClients = activeRooms[hdr->roomPin];
                        std::string c_user(hdr->username);
                        if (c_user.empty()) c_user = "Unknown";
                        
                        bool found = false;
                        bool isMuted = false;
                        for (auto it = roomClients.begin(); it != roomClients.end(); ++it) {
                            if (it->addr.sin_addr.s_addr == clientAddr.sin_addr.s_addr && it->addr.sin_port == clientAddr.sin_port) {
                                if (hdr->type == 9) { // DISCONNECT PACKET
                                    std::string tName = (it->type != 1 ? "SENDER" : "RECEIVER");
                                    addLog("[-] " + tName + " (" + c_user + " @ " + c_ip + ") left! [Room: " + std::to_string(hdr->roomPin) + "]");
                                    roomClients.erase(it);
                                    found = true;
                                    break;
                                }
                                it->lastSeen = now;
                                it->username = c_user; 
                                isMuted = it->muted;
                                found = true;
                                if (hdr->type == 1) it->type = 1;
                                else if (hdr->type == 2) it->type = 2;
                                break;
                            }
                        }
                        
                        if (hdr->type == 9) {
                            redrawHostList();
                            continue;
                        }

                        if (!found && c_user != "Guest" && c_user != "Unknown") {
                            bool nameTaken = false;
                            for (auto& c : roomClients) {
                                if (c.username == c_user) { nameTaken = true; break; }
                            }
                            if (nameTaken) {
                                PacketHeader rejHdr = {0}; rejHdr.magic = 0x50484F4E; rejHdr.type = 6; rejHdr.roomPin = hdr->roomPin;
                                sendto(hostSocket, (char*)&rejHdr, sizeof(rejHdr), 0, (SOCKADDR*)&clientAddr, clientLen);
                                continue;
                            }
                        }

                        if (!found) {
                            ClientInfo ni; ni.addr = clientAddr; ni.lastSeen = now; ni.type = (hdr->type == 1) ? 1 : 0;
                            ni.username = c_user; ni.ipStr = c_ip; ni.port = ntohs(clientAddr.sin_port);
                            
                            bool isMutedLocally = (std::find(mutedIPs.begin(), mutedIPs.end(), c_ip) != mutedIPs.end());
                            ni.muted = isMutedLocally;
                            
                            roomClients.push_back(ni);
                            std::string tName = (ni.type != 1 ? "SENDER" : "RECEIVER");
                            addLog("[+] " + tName + " (" + c_user + " @ " + c_ip + ") joined!" + (isMutedLocally ? " [MUTED]" : "") + " [Room: " + std::to_string(hdr->roomPin) + "]");
                            if (isMutedLocally) {
                                PacketHeader muteHdr = {0x50484F4E, 7, hdr->roomPin};
                                sendto(hostSocket, (char*)&muteHdr, sizeof(muteHdr), 0, (SOCKADDR*)&clientAddr, clientLen);
                            }
                            lastStatusPrint = 0; 
                        }

                        if (!isMuted && (hdr->type == 0 || hdr->type == 3)) { 
                            for (auto& c : roomClients) {
                                if (c.type == 1 && !c.muted) { 
                                    sendto(hostSocket, buffer, bytes, 0, (SOCKADDR*)&(c.addr), sizeof(sockaddr_in));
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if (ongoingKick.active) {
            DWORD now = GetTickCount();
            if (now - ongoingKick.startTime >= 2000) {
                ongoingKick.active = false;
                tempError = "";
                
                if (ongoingKick.lastPacketTime != 0 && now - ongoingKick.lastPacketTime < 1000) {
                    addLog("[!] Kick failed: " + ongoingKick.name + " (" + ongoingKick.ip + ") is still connected. [Room: " + std::to_string(ongoingKick.roomPin) + "]");
                    ongoingKick.data.lastSeen = now;
                    activeRooms[ongoingKick.roomPin].push_back(ongoingKick.data);
                } else {
                    addLog("[-] Kicked " + ongoingKick.name + " (" + ongoingKick.ip + ") [Room: " + std::to_string(ongoingKick.roomPin) + "]");
                }
                
                redrawHostList();
            }
        }
        
        DWORD now = GetTickCount();
        if (now - lastStatusPrint > 2000 || lastStatusPrint == 0) {
            int senders = 0, receivers = 0;
            for (auto& roomPair : activeRooms) {
                for (auto it = roomPair.second.begin(); it != roomPair.second.end(); ) {
                   if (now - it->lastSeen > 3000) { 
                       std::string tName = (it->type != 1 ? "SENDER" : "RECEIVER");
                       addLog("[-] " + tName + " (" + it->username + " @ " + it->ipStr + ") left! [Room: " + std::to_string(roomPair.first) + "]");
                       it = roomPair.second.erase(it);
                       lastStatusPrint = 0; 
                   } else {
                       if (it->type == 1) receivers++;
                       else if (it->type == 2) senders++;
                       ++it;
                   }
                }
            }
            if (senders != lastSenders || receivers != lastReceivers || lastStatusPrint == 0) {
                std::string hud = "PhonicBridge Host | " + std::to_string(senders) + " Sender(s) | " + std::to_string(receivers) + " Receiver(s) Active";
                SetConsoleTitleA(hud.c_str());
                lastSenders = senders;
                lastReceivers = receivers;
                redrawHostList();
            }
            lastStatusPrint = GetTickCount();
        }
    }
    SetConsoleMode(hIn, oldMode);
    closesocket(hostSocket);
}

// ======================================================================
// [1] SENDER MODULE
// ======================================================================
void startSender(const std::string& hostIP, int hostPort, uint32_t roomPin, DWORD targetPID, int muteKey, const std::string& username) {
    g_ExitSender = false;
    g_SenderMuted = false;
    g_HostMuted = false;
    g_TargetChangeRequested = false;
    
    // Track Mongoose Port Mutator natively
    if (g_LastWebPort != g_Config.webPort && g_WebServerThreadSpawned) {
        g_SignalWebShutdown = true;
        while(g_WebServerThreadSpawned) Sleep(50);
        g_SignalWebShutdown = false;
        g_WebThreadActive = false;
    }
    g_LastWebPort = g_Config.webPort;
    
    std::cout << YELLOW << "\n[*] Initializing Network Transmission..." << RESET << std::endl;

    SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in localAddr; localAddr.sin_family = AF_INET; localAddr.sin_port = htons(0); localAddr.sin_addr.s_addr = INADDR_ANY;
    bind(sendSocket, (SOCKADDR*)&localAddr, sizeof(localAddr));

    sockaddr_in hostAddr;
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(hostPort);
    hostAddr.sin_addr.s_addr = inet_addr(hostIP.c_str());

    CoInitialize(nullptr);
    IAudioClient *pAudioClient = nullptr;
    WAVEFORMATEX *pwfx = nullptr;

    if (targetPID == 0) {
        IMMDeviceEnumerator* pEnum = nullptr;
        CHECK_HR(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum));
        IMMDevice* pDev = nullptr;
        CHECK_HR(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev));
        CHECK_HR(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient));
        CHECK_HR(pAudioClient->GetMixFormat(&pwfx));
        pDev->Release(); pEnum->Release();
    } else {
        IMMDeviceEnumerator* pEnum = nullptr;
        CHECK_HR(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum));
        IMMDevice* pDev = nullptr;
        CHECK_HR(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev));
        IAudioClient* pDefClient = nullptr;
        CHECK_HR(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pDefClient));
        CHECK_HR(pDefClient->GetMixFormat(&pwfx));
        pDefClient->Release(); pDev->Release(); pEnum->Release();

        AUDIOCLIENT_ACTIVATION_PARAMS activateParams = {};
        activateParams.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
        activateParams.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
        activateParams.ProcessLoopbackParams.TargetProcessId = targetPID;

        PROPVARIANT propVar = {};  propVar.vt = VT_BLOB; propVar.blob.cbSize = sizeof(activateParams); propVar.blob.pBlobData = (BYTE*)&activateParams;
        ComPtr<AudioInterfaceCompletionHandler> completionHandler = Make<AudioInterfaceCompletionHandler>();
        ComPtr<IActivateAudioInterfaceAsyncOperation> asyncOp;

        HRESULT hr = ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK, __uuidof(IAudioClient), &propVar, completionHandler.Get(), &asyncOp);
        CHECK_HR(hr);
        WaitForSingleObject(completionHandler->m_hEvent, INFINITE);

        pAudioClient = completionHandler->m_AudioClient.Get();
        if (!pAudioClient) {
            std::cout << RED << "[!] Connection failed. Process silent or PID invalid.\n" << RESET;
            return;
        }
        pAudioClient->AddRef();
    }

    // Force universal 48000Hz 32-bit Stereo to prevent frequency pitch shifts
    WAVEFORMATEX wfxUniversal = {};
    wfxUniversal.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfxUniversal.nChannels = 2;
    wfxUniversal.nSamplesPerSec = 48000;
    wfxUniversal.wBitsPerSample = 32;
    wfxUniversal.nBlockAlign = 8;
    wfxUniversal.nAvgBytesPerSec = 384000;
    wfxUniversal.cbSize = 0;

    REFERENCE_TIME hnsReq = 10000000;
    HRESULT hrInit = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | 0x80000000 | 0x08000000, hnsReq, 0, &wfxUniversal, NULL);
    if (SUCCEEDED(hrInit)) {
        pwfx = &wfxUniversal; // Windows auto-conversion success
    } else {
        CHECK_HR(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsReq, 0, pwfx, NULL));
    }

    IAudioCaptureClient *pCaptureClient = nullptr;
    CHECK_HR(pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient));

    CHECK_HR(pAudioClient->Start());
    std::cout << GREEN << "\n[!] TRANSMISSION ACTIVE. Audio streaming to ROOM " << roomPin << RESET << std::endl;
    
    std::string processIndicator = "Global_System";
    if (targetPID != 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetPID);
        if (hProcess) {
            char processName[256] = "<unknown>";
            if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
                processIndicator = std::string(processName);
            } else {
                processIndicator = "PID " + std::to_string(targetPID);
            }
            CloseHandle(hProcess);
        } else {
            processIndicator = "PID " + std::to_string(targetPID);
        }
    }
    std::cout << CYAN << "[-] Target Capture Stream: \033[97m" << processIndicator << "\033[0m  " << MAGENTA << "(Press [P] to Manage Target PID)\n" << RESET;
    
    std::thread muteUi(senderMuteThread, muteKey);
    muteUi.detach();

    DWORD lastKeepAlive = 0;
    char recvBuffer[256];

    while (!g_ExitSender) {
        Sleep(10);
        DWORD now = GetTickCount();

        // Listen for dynamic Target PID 'P' toggle
        if (((GetAsyncKeyState('P') & 0x8000) || (GetAsyncKeyState('p') & 0x8000)) && GetForegroundWindow() == GetConsoleWindow()) {
            std::cout << "\n\n";
            g_TargetChangeRequested = true;
            g_ExitSender = true;
            break;
        }

        // TRUE HEARTBEAT logic (Independent from capture state)
        if (now - lastKeepAlive >= 1000) {
            PacketHeader hdr = {0};
            hdr.magic = 0x50484F4E; hdr.type = 2; hdr.roomPin = roomPin; // KEEPALIVE_SENDER
            strncpy_s(hdr.username, 25, username.c_str(), 24);
            sendto(sendSocket, (char*)&hdr, sizeof(hdr), 0, (SOCKADDR*)&hostAddr, sizeof(hostAddr));
            lastKeepAlive = now;
        }

        // BACKCHANNEL LISTENER (Check for Kicks/Bans)
        fd_set readfds; FD_ZERO(&readfds); FD_SET(sendSocket, &readfds);
        timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;
        if (select(0, &readfds, NULL, NULL, &tv) > 0) {
            int bytesReceived = recvfrom(sendSocket, recvBuffer, sizeof(recvBuffer), 0, NULL, NULL);
            if (bytesReceived > 0 && (size_t)bytesReceived >= sizeof(PacketHeader)) {
                PacketHeader* hdr = (PacketHeader*)recvBuffer;
                if (hdr->magic == 0x50484F4E && hdr->roomPin == roomPin) {
                    if (hdr->type == 4) { g_NetworkError = "KICKED"; std::cout << "\n\n"; g_ExitSender = true; break; }
                    else if (hdr->type == 5) { g_NetworkError = "BANNED"; std::cout << "\n\n"; g_ExitSender = true; break; }
                    else if (hdr->type == 6) { g_NetworkError = "Username is already taken or invalid."; std::cout << "\n\n"; g_ExitSender = true; break; }
                    else if (hdr->type == 7) { g_HostMuted = true; std::cout << "\n\033[K" << YELLOW << "[!] YOU ARE MUTED BY THE HOST. Transmitter suspended locally." << RESET << "\033[A"; }
                    else if (hdr->type == 8) { g_HostMuted = false; std::cout << "\n\033[K" << GREEN << "[+] MUTE LIFTED BY HOST. Transmitter active." << RESET << "\033[A"; }
                }
            }
        }

        UINT32 packetLength = 0;
        pCaptureClient->GetNextPacketSize(&packetLength);

        while (packetLength != 0) {
            BYTE *pData;
            UINT32 numFramesAvailable;
            DWORD flags;
            CHECK_HR(pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL));
            
            if (!g_SenderMuted && !g_HostMuted) {
                int bytesToSend = numFramesAvailable * pwfx->nBlockAlign;
                std::vector<char> sendBuf(sizeof(PacketHeader) + bytesToSend);
                PacketHeader* hdr = (PacketHeader*)sendBuf.data();
                memset(hdr, 0, sizeof(PacketHeader));
                hdr->magic = 0x50484F4E; hdr->type = 0; hdr->roomPin = roomPin;
                strncpy_s(hdr->username, 25, username.c_str(), 24);
                memcpy(sendBuf.data() + sizeof(PacketHeader), pData, bytesToSend);
                sendto(sendSocket, sendBuf.data(), sendBuf.size(), 0, (SOCKADDR*)&hostAddr, sizeof(hostAddr));
            }

            CHECK_HR(pCaptureClient->ReleaseBuffer(numFramesAvailable));
            pCaptureClient->GetNextPacketSize(&packetLength);
        }
    }

    pAudioClient->Stop();
    pAudioClient->Release();
    CoUninitialize();
    
    PacketHeader hdr = {0};
    hdr.magic = 0x50484F4E; hdr.type = 9; hdr.roomPin = roomPin; // DISCONNECT
    strncpy_s(hdr.username, 25, username.c_str(), 24);
    sendto(sendSocket, (char*)&hdr, sizeof(hdr), 0, (SOCKADDR*)&hostAddr, sizeof(hostAddr));
    
    closesocket(sendSocket);

    if (g_NetworkError == "KICKED" || g_NetworkError == "BANNED") {
        std::cout << "\n\n\033[K" << RED << " [!] ACCESS DENIED: " << (g_NetworkError == "KICKED" ? "YOU WERE KICKED BY THE HOST." : "YOU ARE BANNED FROM THIS HOST.") << RESET << "\n";
        std::cout << YELLOW << " Press \033[97mESC\033[33m to return to the Main Menu...\n" << RESET;
        while (_kbhit()) _getch(); // Flush
        while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(50);
        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) Sleep(50);
        while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(50);
    }
}

// ======================================================================
// [2] RECEIVER MODULE (Supports Reconnect & 16-bit uncompression)
// ======================================================================
void startReceiver(const std::string& hostIP, int hostPort, uint32_t roomPin, const std::string& username, int muteKey) {
    g_ExitReceiver = false;
    g_ReceiverMuted = false;
    g_HostMuted = false;
    g_NetworkError = "";
    g_TargetChangeRequested = false;
    std::cout << YELLOW << "\n[*] Joining HOST: " << hostIP << ":" << hostPort << " [ROOM " << roomPin << "]" << RESET << std::endl;
    
    std::thread muteUi(volumeControlThread, muteKey);
    muteUi.detach();
    
    // Track Mongoose Port Mutator natively
    if (g_LastWebPort != g_Config.webPort && g_WebServerThreadSpawned) {
        g_SignalWebShutdown = true;
        while(g_WebServerThreadSpawned) Sleep(50);
        g_SignalWebShutdown = false;
        g_WebThreadActive = false;
    }
    g_LastWebPort = g_Config.webPort;
    
    g_WebSocketsActive = g_EnableWebStream;
    if (!g_EnableWebStream && g_WebServerThreadSpawned) {
        g_SignalWebShutdown = true;
        while(g_WebServerThreadSpawned) Sleep(50);
        g_SignalWebShutdown = false;
        g_WebThreadActive = false; // Track thread active state
    }
    if (g_EnableWebStream && !g_WebServerThreadSpawned) {
        g_SignalWebShutdown = false;
        g_WebServerThreadSpawned = true;
        while(g_WebThreadActive) Sleep(10); // Wait for dead port
        std::thread ws(startWebServerThread);
        ws.detach();
        g_WebThreadActive = true; // Track thread active state
    }

    SOCKET recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in localAddr; localAddr.sin_family = AF_INET; localAddr.sin_port = htons(0); localAddr.sin_addr.s_addr = INADDR_ANY;
    bind(recvSocket, (SOCKADDR*)&localAddr, sizeof(localAddr));

    sockaddr_in hostAddr; hostAddr.sin_family = AF_INET; hostAddr.sin_port = htons(hostPort); hostAddr.sin_addr.s_addr = inet_addr(hostIP.c_str());

    CoInitialize(nullptr);
    IMMDeviceEnumerator *pEnumerator = nullptr;
    CHECK_HR(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator));
    IMMDevice *pDevice = nullptr;
    CHECK_HR(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice));
    IAudioClient *pAudioClient = nullptr;
    CHECK_HR(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient));
    WAVEFORMATEX *pwfx = nullptr;
    CHECK_HR(pAudioClient->GetMixFormat(&pwfx));

    // Force universal 48000Hz 32-bit Stereo for gapless audio sync
    WAVEFORMATEX wfxUniversal = {};
    wfxUniversal.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfxUniversal.nChannels = 2;
    wfxUniversal.nSamplesPerSec = 48000;
    wfxUniversal.wBitsPerSample = 32;
    wfxUniversal.nBlockAlign = 8;
    wfxUniversal.nAvgBytesPerSec = 384000;
    wfxUniversal.cbSize = 0;

    REFERENCE_TIME hnsReq = 10000000;
    HRESULT hrInit = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0x80000000 | 0x08000000, hnsReq, 0, &wfxUniversal, NULL);
    if (SUCCEEDED(hrInit)) {
        pwfx = &wfxUniversal;
    } else {
        CHECK_HR(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsReq, 0, pwfx, NULL));
    }
    
    IAudioRenderClient *pRenderClient = nullptr;
    CHECK_HR(pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient));
    CHECK_HR(pAudioClient->Start());

    char buffer[65535];
    UINT32 bufferFrameCount;
    pAudioClient->GetBufferSize(&bufferFrameCount);

    fd_set readfds;
    timeval tv;
    DWORD lastKeepAlive = 0;
    DWORD lastDataReceived = GetTickCount();
    bool isDisconnected = false;

    while (!g_ExitReceiver) {
        DWORD now = GetTickCount();
        if (now - lastKeepAlive >= 1000) {
            PacketHeader hdr = {0};
            hdr.magic = 0x50484F4E; hdr.type = 1; hdr.roomPin = roomPin; // KEEPALIVE_RECV
            strncpy_s(hdr.username, 25, username.c_str(), 24);
            sendto(recvSocket, (char*)&hdr, sizeof(hdr), 0, (SOCKADDR*)&hostAddr, sizeof(hostAddr));
            lastKeepAlive = now;
        }

        FD_ZERO(&readfds); FD_SET(recvSocket, &readfds);
        tv.tv_sec = 0; tv.tv_usec = 50000;

        if (select(0, &readfds, NULL, NULL, &tv) > 0) {
            int bytesReceived = recvfrom(recvSocket, buffer, sizeof(buffer), 0, NULL, NULL);
            if (bytesReceived > 0 && (size_t)bytesReceived >= sizeof(PacketHeader)) {
                PacketHeader* hdr = (PacketHeader*)buffer;
                if (hdr->magic == 0x50484F4E && hdr->roomPin == roomPin) {
                    if (hdr->type == 4) { 
                        g_NetworkError = "KICKED"; 
                        if (g_EnableWebStream) { broadcastTextToWebClients("MSG:KICKED"); Sleep(150); }
                        std::cout << "\n\n";
                        g_ExitReceiver = true; break; 
                    }
                    else if (hdr->type == 5) { 
                        g_NetworkError = "BANNED"; 
                        if (g_EnableWebStream) { broadcastTextToWebClients("MSG:BANNED"); Sleep(150); }
                        std::cout << "\n\n";
                        g_ExitReceiver = true; break; 
                    }
                    else if (hdr->type == 6) { 
                        g_NetworkError = "Username is already taken or invalid."; 
                        std::cout << "\n\n";
                        g_ExitReceiver = true; break; 
                    }
                    else if (hdr->type == 7) { 
                        g_HostMuted = true;
                        std::cout << "\n\033[K" << YELLOW << "[!] YOU ARE MUTED BY THE HOST. Receiver suspended locally." << RESET << "\033[A"; 
                        if (g_EnableWebStream) broadcastTextToWebClients("MSG:MUTED");
                    }
                    else if (hdr->type == 8) { 
                        g_HostMuted = false;
                        std::cout << "\n\033[K" << GREEN << "[+] MUTE LIFTED BY HOST. Receiver active." << RESET << "\033[A"; 
                        if (g_EnableWebStream) broadcastTextToWebClients("MSG:UNMUTED");
                    }

                    lastDataReceived = GetTickCount(); // Valid packet resets timeout

                    if (isDisconnected) {
                        std::cout << "\n\033[K" << GREEN << "[+] Reconnected!" << RESET << " (Music continues)\033[A";
                        if (g_EnableWebStream) broadcastTextToWebClients("MSG:RECONNECTED");
                        isDisconnected = false;
                    }

                    if (hdr->type == 0) {
                        int audioBytes = bytesReceived - sizeof(PacketHeader);
                        char* audioData = buffer + sizeof(PacketHeader);
                        
                        UINT32 numFrames = audioBytes / pwfx->nBlockAlign;

                        UINT32 padding; pAudioClient->GetCurrentPadding(&padding);
                        if (bufferFrameCount - padding >= numFrames) {
                            BYTE *pData;
                            CHECK_HR(pRenderClient->GetBuffer(numFrames, &pData));
                            
                            // Send raw audio to web clients completely unlinked from local PC Mute/Volume
                            if (g_EnableWebStream) broadcastToWebClients((const char*)audioData, audioBytes);

                            if (pwfx->wBitsPerSample == 32) {
                                float* dst = (float*)pData;
                                float* src = (float*)audioData;
                                int count = audioBytes / 4;
                                for(int i=0; i<count; i++) {
                                    float val = (g_ReceiverMuted || g_HostMuted) ? 0.0f : (src[i] * g_Volume);
                                    if (val > 1.0f) val = 1.0f; else if (val < -1.0f) val = -1.0f;
                                    dst[i] = val;
                                }
                            } else {
                                if (g_ReceiverMuted) memset(pData, 0, audioBytes);
                                else memcpy(pData, audioData, audioBytes); // Basic fallback
                            }
                            CHECK_HR(pRenderClient->ReleaseBuffer(numFrames, 0));
                        }
                    }
                }
            }
        } else {
            // TIMEOUT LOGIC
            if (GetTickCount() - lastDataReceived > 3000 && !isDisconnected && !g_ReceiverMuted) {
                isDisconnected = true;
                std::cout << "\n\033[K" << RED << "[-] Connection Lost... Waiting for Host..." << RESET << "\033[A";
                if (g_EnableWebStream) broadcastTextToWebClients("MSG:CONNECTION_LOST");
            }
        }
    }

    pAudioClient->Stop(); pAudioClient->Release(); CoUninitialize(); 
    
    PacketHeader hdr = {0};
    hdr.magic = 0x50484F4E; hdr.type = 9; hdr.roomPin = roomPin; // DISCONNECT
    strncpy_s(hdr.username, 25, username.c_str(), 24);
    sendto(recvSocket, (char*)&hdr, sizeof(hdr), 0, (SOCKADDR*)&hostAddr, sizeof(hostAddr));
    
    closesocket(recvSocket);

    if (g_NetworkError == "KICKED" || g_NetworkError == "BANNED") {
        std::cout << "\n\n\033[K" << RED << " [!] ACCESS DENIED: " << (g_NetworkError == "KICKED" ? "YOU WERE KICKED BY THE HOST." : "YOU ARE BANNED FROM THIS HOST.") << RESET << "\n";
        std::cout << YELLOW << " Press \033[97mESC\033[33m to return to the Main Menu...\n" << RESET;
        while (_kbhit()) _getch(); // Flush
        while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(50);
        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) Sleep(50);
        while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(50);
    }
}

// ======================================================================
// DYNAMIC CONSOLE SCALER
// Definitively stops terminal buffer wrap-around stacking glitches.
// Calculates if the required UI lines exceed the current screen buffer height,
// and symmetrically extends the Y-axis just enough to prevent wrapping.
// ======================================================================
void ensureConsoleHeight(int requiredRows) {
    requiredRows += 2; // Pad vertically precisely to prevent native Windows scrollbars
    if (requiredRows < 30) requiredRows = 30; // 30 is the minimum absolute boundary for the UI
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        int winHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        // ONLY resize if the window is currently smaller than the REQUIRED minimum or width gets broken.
        // This allows users to manually drag-maximize the window without the app forcefully shrinking it back.
        if (winHeight < requiredRows || csbi.dwSize.X != 105) {
            std::string cmd = "mode con cols=105 lines=" + std::to_string(requiredRows);
            system(cmd.c_str());
            Sleep(30); 
        }
    }
}

// ======================================================================
// MAIN ENTRY
// ======================================================================
auto safeStoi = [](const std::string& s, int def) -> int {
    try { return std::stoi(s); } catch(...) { return def; }
};
auto safeStoul = [](const std::string& s, uint32_t def) -> uint32_t {
    try { return std::stoul(s); } catch(...) { return def; }
};

void animateTitleThread() {
    std::string text = "Phonic Bridge By XXLRN";
    std::string current = "";
    for (char c : text) {
        current += c;
        SetConsoleTitleA(current.c_str());
        Sleep(80);
    }
}

uint32_t promptForPid(bool& cancelled, bool isModernMode = true) {
    if (!isModernMode) std::cout << "\n"; // Padding explicitly before spawning Legacy PID chooser
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    // Purge phantom keystrokes (like the 'P' hotkey) bleeding into the UI buffer organically
    FlushConsoleInputBuffer(hIn);
    Sleep(50);
    while (_kbhit()) _getch();
    
    int printedLines = 0;
    bool pidConfirmed = false;
    uint32_t finalPid = g_Config.pid;
    cancelled = false;

    while (!pidConfirmed) {
        g_ActiveWindows.clear();
        EnumWindows(EnumWindowsProc, 0);
        
        DWORD oldMode;
        GetConsoleMode(hIn, &oldMode);
        if (isModernMode) SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
        else SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT);
        
        std::string inputBuf = "";
        bool inputComplete = false;
        int selectedIndex = -1; // -1 means typing
        int confirmedIndex = -1;
        int baseRow = 0;
        
        auto redrawList = [&]() {
            int totalItems = g_ActiveWindows.size() + 1;
            if (isModernMode) {
                ensureConsoleHeight(30); 
                SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {0, 0});
                std::cout << "\033[0J";
                SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
            }
            else { if (printedLines > 0) std::cout << "\r\033[" << printedLines << "A\033[0J"; }
            int curLines = 0;
            
            if (isModernMode) {
                std::istringstream logoStream(ASCII_LOGO); std::string l;
                while(std::getline(logoStream, l)) { std::cout << l << "\033[K\n"; }
                std::cout << MAGENTA << "=== SELECT TARGET PID ===\033[K\n\n" << RESET;
            }
            
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            baseRow = csbi.dwCursorPosition.Y;
            
            int maxItems = 11; // Strict 11 bounds
            int startIdx = 0;
            
            if (selectedIndex != -1) {
                if (selectedIndex >= maxItems / 2) startIdx = selectedIndex - maxItems / 2;
                if (startIdx + maxItems > totalItems) startIdx = totalItems - maxItems;
                if (startIdx < 0) startIdx = 0;
            }
            
            int endIdx = startIdx + maxItems;
            if (endIdx > totalItems) endIdx = totalItems;
            
            if (startIdx > 0) {
                std::cout << CYAN << "   ... (" << startIdx << " more above) ...\033[K" << RESET << "\n";
                curLines++;
            }
            
            for (int i = startIdx; i < endIdx; ++i) {
                if (i == 0) {
                    std::string bg0 = (confirmedIndex == 0) ? "\033[42;30m" : (selectedIndex == 0 ? "\033[44;97m" : "");
                    if (!bg0.empty()) std::cout << bg0;
                    std::cout << CYAN << "   [PID: 0]\t" << RESET << bg0 << "Global_System \t- Capture Entire Computer Audio\033[K" << RESET << "\n";
                } else {
                    int wIdx = i - 1;
                    std::string bgi = (confirmedIndex == i) ? "\033[42;30m" : (selectedIndex == i ? "\033[44;97m" : "");
                    if (!bgi.empty()) std::cout << bgi;
                    
                    std::string tName = g_ActiveWindows[wIdx].name;
                    std::string tTitle = g_ActiveWindows[wIdx].title;
                    if (tName.length() > 25) tName = tName.substr(0, 22) + "...";
                    if (tTitle.length() > 40) tTitle = tTitle.substr(0, 37) + "...";
                    
                    std::cout << CYAN << "   [PID: " << g_ActiveWindows[wIdx].pid << "]\t" << RESET 
                              << bgi << tName << " \t- " << tTitle << "\033[K" << RESET << "\n";
                }
                curLines++;
            }
            
            if (endIdx < totalItems) {
                std::cout << CYAN << "   ... (" << totalItems - endIdx << " more below) ...\033[K" << RESET << "\n";
                curLines++;
            }
            
            std::cout << "\nInput Target PID [" << finalPid << "], 'T' to Target Window, 'R' to Reload list: " << inputBuf;
            curLines += 1;
            printedLines = curLines;
        };
        
        redrawList();
        
        while (!inputComplete) {
            INPUT_RECORD ir[32];
            DWORD read;
            ReadConsoleInput(hIn, ir, 32, &read);
            for (DWORD i = 0; i < read; ++i) {
                if (isModernMode && ir[i].EventType == MOUSE_EVENT && ir[i].Event.MouseEvent.dwEventFlags == 0 && (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                    int my = ir[i].Event.MouseEvent.dwMousePosition.Y;
                    int listOffset = my - baseRow;
                    int startIdx = 0;
                    int totalItems = g_ActiveWindows.size() + 1;
                    
                    int maxItems = 11;
                    
                    if (selectedIndex != -1) {
                        if (selectedIndex >= maxItems / 2) startIdx = selectedIndex - maxItems / 2;
                        if (startIdx + maxItems > totalItems) startIdx = totalItems - maxItems;
                        if (startIdx < 0) startIdx = 0;
                    }
                    if (startIdx > 0) listOffset--; // Adjust for "more above" line
                    
                    int clickedGlobalIdx = startIdx + listOffset;
                    if (clickedGlobalIdx >= 0 && clickedGlobalIdx <= totalItems) {
                        selectedIndex = clickedGlobalIdx;
                        inputBuf = (selectedIndex == 0) ? "0" : std::to_string(g_ActiveWindows[selectedIndex - 1].pid);
                        confirmedIndex = selectedIndex;
                        selectedIndex = -1;
                        system("cls"); redrawList();
                        Sleep(300);
                        confirmedIndex = -1;
                        redrawList();
                        inputComplete = true; break;
                    }
                } else if (isModernMode && ir[i].EventType == MOUSE_EVENT && ir[i].Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
                    int32_t zDelta = ir[i].Event.MouseEvent.dwButtonState;
                    if (zDelta > 0) {
                        if (selectedIndex <= 0) selectedIndex = g_ActiveWindows.size();
                        else selectedIndex--;
                        redrawList();
                    } else {
                        if (selectedIndex == -1) selectedIndex = 0;
                        else selectedIndex = (selectedIndex + 1) % (g_ActiveWindows.size() + 1);
                        redrawList();
                    }
                } else if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                    WORD vKey = ir[i].Event.KeyEvent.wVirtualKeyCode;
                    char ch = ir[i].Event.KeyEvent.uChar.AsciiChar;
                    
                    if (vKey == VK_RETURN) {
                        if (selectedIndex != -1) {
                            if (selectedIndex == 0) inputBuf = "0";
                            else inputBuf = std::to_string(g_ActiveWindows[selectedIndex - 1].pid);
                            confirmedIndex = selectedIndex;
                            selectedIndex = -1;
                            redrawList();
                            Sleep(300); // Visual flair for green highlight
                            confirmedIndex = -1;
                            redrawList();
                        }
                        inputComplete = true; break;
                    } else if (vKey == VK_TAB) {
                        if (selectedIndex == -1) selectedIndex = 0;
                        else selectedIndex = (selectedIndex + 1) % (g_ActiveWindows.size() + 1);
                        redrawList();
                    } else if (vKey == VK_UP) {
                        if (selectedIndex <= 0) selectedIndex = g_ActiveWindows.size();
                        else selectedIndex--;
                        redrawList();
                    } else if (vKey == VK_DOWN) {
                        if (selectedIndex == -1) selectedIndex = 0;
                        else selectedIndex = (selectedIndex + 1) % (g_ActiveWindows.size() + 1);
                        redrawList();
                    } else if (vKey == VK_ESCAPE || ch == 'q' || ch == 'Q') {
                        if (selectedIndex != -1) {
                            selectedIndex = -1;
                            redrawList();
                        } else if (!inputBuf.empty()) {
                            inputBuf = "";
                            redrawList();
                        } else {
                            inputBuf = "ESC_EXIT";
                            inputComplete = true; break;
                        }
                    } else if (vKey == VK_BACK) {
                        if (selectedIndex != -1) { selectedIndex = -1; redrawList(); }
                        else if (!inputBuf.empty()) {
                            inputBuf.pop_back();
                            std::cout << "\b \b";
                        }
                    } else if (ch == 'R' || ch == 'r') {
                        inputBuf = "R";
                        inputComplete = true; break;
                    } else if (ch == 'T' || ch == 't') {
                        inputBuf = "T";
                        inputComplete = true; break;
                    } else if (ch >= '0' && ch <= '9') {
                        if (selectedIndex != -1) { selectedIndex = -1; redrawList(); }
                        inputBuf += ch;
                        std::cout << ch;
                    }
                }
            }
        }
        SetConsoleMode(hIn, oldMode);
        std::cout << std::endl; 
        
        std::string in = inputBuf;
        if (in == "R" || in == "r") {
            if (printedLines > 0) std::cout << "\r\033[" << (printedLines + 1) << "A\033[0J";
            printedLines = 0;
            continue; 
        } else if (in == "T" || in == "t") {
            std::cout << "\n" << MAGENTA << "[ TARGET MODE ] " << YELLOW << "Please click on the window you want to capture... (Press ESC to cancel)" << RESET << "\n";
            
            // Wait for release
            while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) { Sleep(10); } 
            
            bool targetAborted = false;
            while (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) { 
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { targetAborted = true; break; }
                Sleep(10); 
            }
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(10);
            FlushConsoleInputBuffer(hIn);
            
            if (targetAborted) {
                std::cout << YELLOW << "[-] Target Window Mode CANCELLED.\n\n" << RESET;
                if (printedLines > 0) std::cout << "\r\033[" << (printedLines + 5) << "A\033[0J";
                printedLines = 0;
            } else {
                HWND hwnd = GetForegroundWindow();
                DWORD pid = 0;
                GetWindowThreadProcessId(hwnd, &pid);
                char processName[256] = "<unknown>";
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                if (hProcess) { GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName)); CloseHandle(hProcess); }

                std::cout << GREEN << "[+] Selected Window: " << processName << " (PID: " << pid << ")\n" << RESET;
                std::cout << "Press ENTER to Confirm, or ESC to Cancel and return to list...\n";
                
                while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) Sleep(10); 
                
                bool confAborted = false;
                while (true) {
                    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { confAborted = true; break; }
                    if (GetAsyncKeyState(VK_RETURN) & 0x8000) { break; }
                    Sleep(10);
                }
                while (GetAsyncKeyState(VK_ESCAPE) & 0x8000 || GetAsyncKeyState(VK_RETURN) & 0x8000) Sleep(10);
                FlushConsoleInputBuffer(hIn);
                
                if (!confAborted) {
                    finalPid = pid;
                    pidConfirmed = true;
                } else {
                    if (printedLines > 0) std::cout << "\r\033[" << (printedLines + 5) << "A\033[0J";
                    printedLines = 0;
                }
            }
        } else if (in == "ESC_EXIT") {
            cancelled = true;
            pidConfirmed = true; 
            continue;
        } else {
            if (!in.empty()) {
                try { finalPid = std::stoul(in); } catch (...) { std::cout << RED << "[!] Invalid input. Using previous PID: " << finalPid << RESET << "\n"; }
            }
            pidConfirmed = true;
        }
    }
    return finalPid;
}

void promptForWebClients(bool isModernMode, int muteKey) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    DWORD oldModeIn;
    GetConsoleMode(hIn, &oldModeIn);
    
    if (isModernMode) {
        system("cls");
        // Flush any stale CRT input left over from _getch(), then set console to mouse mode.
        // Without this, the first 'W' press may fail to activate mouse/scroll events.
        while (_kbhit()) _getch();
        Sleep(50);
        FlushConsoleInputBuffer(hIn);
        SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
        FlushConsoleInputBuffer(hIn);
    } else {
        std::cout << "\n";
    }

    bool inputComplete = false;
    int selectedIndex = 0;
    int maxItems = 15;
    int startIdx = 0;
    
    std::vector<std::string> lanDevices;
    lanDevices.push_back(getLocalIP() + " (This PC)");
    
    PMIB_IPNETTABLE pIpNetTable = NULL; DWORD dwSize = 0;
    if (GetIpNetTable(pIpNetTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIpNetTable = (MIB_IPNETTABLE *)malloc(dwSize);
        if (pIpNetTable != NULL) {
            if (GetIpNetTable(pIpNetTable, &dwSize, FALSE) == NO_ERROR) {
                for (int i = 0; i < (int)pIpNetTable->dwNumEntries; i++) {
                    if (pIpNetTable->table[i].dwType == 3 || pIpNetTable->table[i].dwType == 4) {
                        struct in_addr rA; rA.s_addr = pIpNetTable->table[i].dwAddr;
                        std::string mip = inet_ntoa(rA);
                        if (mip != "127.0.0.1" && mip != "0.0.0.0" && mip.find("255") == std::string::npos) {
                            if (std::find(lanDevices.begin(), lanDevices.end(), mip) == lanDevices.end()) {
                                lanDevices.push_back(mip);
                                std::lock_guard<std::mutex> lock(g_DeviceNamesMutex);
                                if (g_CustomDeviceNames.find(mip) != g_CustomDeviceNames.end()) {
                                    g_DeviceNames[mip] = g_CustomDeviceNames[mip];
                                } else if (g_DeviceNames.find(mip) == g_DeviceNames.end() || g_DeviceNames[mip] == "Resolving...") {
                                    g_DeviceNames[mip] = "Resolving...";
                                    std::thread(resolveDeviceName, mip).detach();
                                }
                            }
                        }
                    }
                }
            }
            free(pIpNetTable);
        }
    }
    
    // Sort descending IPv4 numerically
    std::sort(lanDevices.begin(), lanDevices.end(), [](const std::string& a, const std::string& b) {
        // Extract IP part, ignoring "(This PC)"
        std::string ip_a = a.substr(0, a.find(" (This PC)"));
        std::string ip_b = b.substr(0, b.find(" (This PC)"));

        // Convert IP strings to in_addr for numerical comparison
        unsigned long addr_a = inet_addr(ip_a.c_str());
        unsigned long addr_b = inet_addr(ip_b.c_str());

        // Sort in descending order
        return addr_a > addr_b;
    });
    
    int total = lanDevices.size();
    int printedLines = 0;
    int Y_AllowAll = 0;
    int Y_ListStart = 0;

    auto drawClients = [&]() {
        if (!isModernMode) {
            if (printedLines > 0) std::cout << "\r\033[" << printedLines << "A\033[0J";
            else std::cout << "\n";
            printedLines = 0;
        } else {
            ensureConsoleHeight(30); 
            SetConsoleCursorPosition(hOut, {0, 0});
            std::cout << "\033[0J";
        }
        
        maxItems = 11; // Hardcoded strict 11-item list
        startIdx = 0;
        
        int selList = selectedIndex - 1; 
        if (selList >= maxItems / 2) startIdx = selList - maxItems / 2;
        if (startIdx + maxItems > total) startIdx = total - maxItems;
        if (startIdx < 0) startIdx = 0;
        
        int listStartRow = 0;
        if (isModernMode) {
            std::istringstream logoStream(ASCII_LOGO); std::string l;
            while(std::getline(logoStream, l)) { std::cout << l << "\033[K\n"; listStartRow++; }
        }
        
        std::cout << MAGENTA << "=== MANAGE NETWORK CLIENTS (WAITING ROOM) ===\033[K\n\n" << RESET;
        listStartRow += 2;
        if (!isModernMode) printedLines += 2;
        
        std::cout << "Select Local Area Network (LAN) ARP Addresses to Authorize Audio streaming.\n\n";
        listStartRow += 2;
        if (!isModernMode) printedLines += 2;
        
        // Pinned ALLOW ALL anchor
        Y_AllowAll = listStartRow;
        std::string bgAll = (selectedIndex == 0) ? "\033[44;97m" : "";
        std::cout << "  " << bgAll << (g_AllowAllWebClients ? "\033[92m[X]  " : "\033[91m[ ]  ") 
                  << "[ ALLOW ALL (Bypass Access Control) ]" << RESET << "\033[K\n";
        listStartRow++;
        if (!isModernMode) printedLines++;
        
        if (startIdx > 0) {
            std::cout << CYAN << "   ... (" << startIdx << " more above) ...\033[K\n" << RESET;
            listStartRow++;
            if (!isModernMode) printedLines++;
        }

        Y_ListStart = listStartRow;
        for (int i = startIdx; i < (std::min)((int)total, startIdx + maxItems); ++i) {
            std::string sip = lanDevices[i];
            
            bool isAuth = false;
            std::string dName = "";

            if (sip.find(" (This PC)") != std::string::npos) sip = sip.substr(0, sip.find(" (This PC)"));
            
            std::lock_guard<std::mutex> lock(g_DeviceNamesMutex);
            auto it = g_DeviceNames.find(sip);
            if (it == g_DeviceNames.end() && g_CustomDeviceNames.find(sip) != g_CustomDeviceNames.end()) {
                g_DeviceNames[sip] = g_CustomDeviceNames[sip];
            }
            it = g_DeviceNames.find(sip);
            if (it != g_DeviceNames.end() && it->second != "Unknown" && it->second != "Resolving...") {
                dName = " \033[90m(" + it->second + ")\033[0m";
            } else if (it != g_DeviceNames.end() && it->second == "Resolving...") {
                dName = " \033[90m(Resolving...)\033[0m";
            }
            isAuth = g_AllowAllWebClients || (std::find(g_AuthorizedWebIPs.begin(), g_AuthorizedWebIPs.end(), sip) != g_AuthorizedWebIPs.end());
            
            std::string bg = (selectedIndex == i + 1) ? "\033[44;97m" : "";
            std::cout << "  " << bg << (isAuth ? "\033[92m[X]  " : "\033[91m[ ]  ") 
                      << lanDevices[i] << dName << RESET << "\033[K\n";
            listStartRow++;
            if (!isModernMode) printedLines++;
        }
        
        if (startIdx + maxItems < total) {
            std::cout << CYAN << "   ... (" << total - (startIdx + maxItems) << " more below) ...\033[K\n" << RESET;
            listStartRow++;
            if (!isModernMode) printedLines++;
        }

        std::cout << "\n\033[90m[UP/DOWN] Move  [ENTER] Toggle Auth  [A] Set Alias  [D] Delete Alias  [R] Reload\n"
                  << "[S] Save Current Preset  [N] Save New Preset  [ESC/Q] Exit\033[0m\033[K\n";
        
        if (!isModernMode) printedLines += 3;
    };
    drawClients();
    
    // Throttle mute checks locally to stop key-bounce
    static DWORD lastMuteToggle = 0;

    while (!inputComplete) {
        if (muteKey > 0 && (GetAsyncKeyState(muteKey) & 0x8000)) {
            DWORD now = GetTickCount();
            if (now - lastMuteToggle > 300) {
                g_ReceiverMuted = !g_ReceiverMuted;
                lastMuteToggle = now;
            }
        }
        
        DWORD dwEvents = 0;
        GetNumberOfConsoleInputEvents(hIn, &dwEvents);
        if (dwEvents == 0) { Sleep(10); continue; }
        
        INPUT_RECORD ir[32]; DWORD read;
        ReadConsoleInput(hIn, ir, 32, &read);
        for (DWORD i = 0; i < read; ++i) {
            if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                WORD vK = ir[i].Event.KeyEvent.wVirtualKeyCode;
                char ch = ir[i].Event.KeyEvent.uChar.AsciiChar;
                int numItems = total + 1;
                if (vK == VK_UP && numItems > 0) { selectedIndex = (selectedIndex - 1 + numItems) % numItems; drawClients(); }
                else if (vK == VK_DOWN && numItems > 0) { selectedIndex = (selectedIndex + 1) % numItems; drawClients(); }
                else if (vK == VK_RETURN) { 
                    if (selectedIndex == 0) {
                        g_AllowAllWebClients = !g_AllowAllWebClients;
                        if (!g_AllowAllWebClients) {
                            std::lock_guard<std::mutex> lock(ws_mutex);
                            for (auto& wc : ws_clients) {
                                if (std::find(g_AuthorizedWebIPs.begin(), g_AuthorizedWebIPs.end(), wc.ip) == g_AuthorizedWebIPs.end()) {
                                    wc.is_authorized = false;
                                }
                            }
                            // Thread-safe: let Mongoose thread deliver the revocation
                            broadcastTextToWebClients("MSG:REVOKED");
                        } else {
                            std::lock_guard<std::mutex> lock(ws_mutex);
                            for (auto& wc : ws_clients) wc.is_authorized = true;
                        }
                    } else {
                        std::string sip = lanDevices[selectedIndex - 1];
                        if (sip.find(" (This PC)") != std::string::npos) sip = sip.substr(0, sip.find(" (This PC)"));
                        auto it = std::find(g_AuthorizedWebIPs.begin(), g_AuthorizedWebIPs.end(), sip);
                        bool isNowAuth = true;
                        if (it != g_AuthorizedWebIPs.end()) { 
                            g_AuthorizedWebIPs.erase(it); 
                            isNowAuth = false; 
                            g_AllowAllWebClients = false; // Disable global ALL if specific dropping happens
                        }
                        else { g_AuthorizedWebIPs.push_back(sip); }
                        
                        {
                            std::lock_guard<std::mutex> lock(ws_mutex);
                            for (auto& wc : ws_clients) {
                                if (wc.ip == sip) {
                                    wc.is_authorized = (g_AllowAllWebClients || isNowAuth);
                                }
                            }
                        }
                        // Thread-safe: let Mongoose thread deliver the revocation
                        if (!isNowAuth) broadcastTextToWebClients("MSG:REVOKED");
                    }
                    drawClients(); 
                }
                else if (vK == 'A' || ch == 'a') {
                    if (selectedIndex > 0) {
                        std::string sip = lanDevices[selectedIndex - 1];
                        if (sip.find(" (This PC)") != std::string::npos) sip = sip.substr(0, sip.find(" (This PC)"));
                        std::cout << "\r\033[K" << YELLOW << "Enter Alias for " << sip << " [Max 24]: " << RESET;
                        std::string alias = "";
                        bool cancelled = false;
                        while (true) {
                            int c = _getch();
                            if (c == 27) { cancelled = true; break; } // ESC handling
                            if (c == '\r' || c == '\n') break;
                            if (c == '\b') { if (!alias.empty()) { alias.pop_back(); std::cout << "\b \b"; } }
                            else if (alias.length() < 24 && (isalnum(c) || c == '_' || c == ' ' || c == '-')) { alias += (char)c; std::cout << (char)c; }
                        }
                        if (!alias.empty() && !cancelled) {
                            std::lock_guard<std::mutex> lock(g_DeviceNamesMutex);
                            g_CustomDeviceNames[sip] = alias;
                            g_DeviceNames[sip] = alias;
                            if (!g_ActiveProfileName.empty()) saveConfig(g_ActiveProfileName); // Automatically save alias to profile
                        }
                        drawClients();
                    }
                }
                else if (vK == 'D' || ch == 'd') {
                    if (selectedIndex > 0) {
                        std::string sip = lanDevices[selectedIndex - 1];
                        if (sip.find(" (This PC)") != std::string::npos) sip = sip.substr(0, sip.find(" (This PC)"));
                        {
                            std::lock_guard<std::mutex> lock(g_DeviceNamesMutex);
                            if (g_CustomDeviceNames.find(sip) != g_CustomDeviceNames.end()) {
                                g_CustomDeviceNames.erase(sip);
                                g_DeviceNames.erase(sip);
                                if (!g_ActiveProfileName.empty()) saveConfig(g_ActiveProfileName);
                            }
                        }
                        drawClients();
                    }
                }
                else if (vK == 'R' || ch == 'r') {
                    std::cout << "\r\033[K" << YELLOW << "Refreshing LAN Devices..." << RESET;
                    lanDevices.clear();
                    lanDevices.push_back(getLocalIP() + " (This PC)");
                    PMIB_IPNETTABLE pIpNetTable = NULL; DWORD dwSize = 0;
                    if (GetIpNetTable(pIpNetTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
                        pIpNetTable = (MIB_IPNETTABLE *)malloc(dwSize);
                        if (pIpNetTable != NULL) {
                            if (GetIpNetTable(pIpNetTable, &dwSize, FALSE) == NO_ERROR) {
                                for (int k = 0; k < (int)pIpNetTable->dwNumEntries; k++) {
                                    if (pIpNetTable->table[k].dwType == 3 || pIpNetTable->table[k].dwType == 4) {
                                        struct in_addr rA; rA.s_addr = pIpNetTable->table[k].dwAddr;
                                        std::string mip = inet_ntoa(rA);
                                        if (mip != "127.0.0.1" && mip != "0.0.0.0" && mip.find("255") == std::string::npos) {
                                            if (std::find(lanDevices.begin(), lanDevices.end(), mip) == lanDevices.end()) lanDevices.push_back(mip);
                                        }
                                    }
                                }
                            }
                            free(pIpNetTable);
                        }
                    }
                    // Sort descending IPv4 numerically
                    std::sort(lanDevices.begin(), lanDevices.end(), [](const std::string& a, const std::string& b) {
                        std::string ip_a = a.substr(0, a.find(" (This PC)"));
                        std::string ip_b = b.substr(0, b.find(" (This PC)"));
                        unsigned long addr_a = inet_addr(ip_a.c_str());
                        unsigned long addr_b = inet_addr(ip_b.c_str());
                        return addr_a > addr_b;
                    });
                    total = lanDevices.size();
                    startIdx = 0;
                    drawClients();
                }
                else if (vK == 'S' || ch == 's') {
                    if (!g_ActiveProfileName.empty()) {
                        saveConfig(g_ActiveProfileName);
                        std::cout << "\r\033[A\033[K" << GREEN << "Saved to preset: " << g_ActiveProfileName << ".xxlrn\n\n\033[0m";
                        Sleep(1000);
                        drawClients();
                    } else { ch = '+'; }
                }
                else if (vK == 'N' || ch == 'n' || ch == '+') {
                    std::cout << "\r\033[K" << YELLOW << "Enter new preset name: \033[0m";
                    std::string pname = "";
                    bool cancelled = false;
                    while (true) {
                        int c = _getch();
                        if (c == 27) { cancelled = true; break; } // ESC handling
                        if (c == '\r' || c == '\n') break;
                        if (c == '\b') { if (!pname.empty()) { pname.pop_back(); std::cout << "\b \b"; } }
                        else if (pname.length() < 24 && (isalnum(c) || c == '_' || c == '-')) { pname += (char)c; std::cout << (char)c; }
                    }
                    if (!pname.empty() && !cancelled) {
                        g_ActiveProfileName = pname;
                        saveConfig(pname);
                    }
                    std::cout << "\n\033[A\033[K\n\033[A"; // Clear natively returning upwards naturally
                    drawClients();
                }
                else if (vK == VK_ESCAPE || ch == 'q' || ch == 'Q') { inputComplete = true; break; }
            } else if (isModernMode && ir[i].EventType == MOUSE_EVENT && ir[i].Event.MouseEvent.dwEventFlags == 0 && (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                int my = ir[i].Event.MouseEvent.dwMousePosition.Y;
                if (my == Y_AllowAll) {
                    selectedIndex = 0;
                    g_AllowAllWebClients = !g_AllowAllWebClients;
                    if (!g_AllowAllWebClients) {
                        std::lock_guard<std::mutex> lock(ws_mutex);
                        for (auto& wc : ws_clients) {
                            if (std::find(g_AuthorizedWebIPs.begin(), g_AuthorizedWebIPs.end(), wc.ip) == g_AuthorizedWebIPs.end()) {
                                wc.is_authorized = false;
                            }
                        }
                        // Thread-safe: let Mongoose thread deliver the revocation
                        broadcastTextToWebClients("MSG:REVOKED");
                    } else {
                        std::lock_guard<std::mutex> lock(ws_mutex);
                        for (auto& wc : ws_clients) wc.is_authorized = true;
                    }
                    drawClients();
                } else if (my >= Y_ListStart && my < Y_ListStart + (std::min)((int)total, maxItems)) {
                    int clickedIdx = startIdx + (my - Y_ListStart);
                    if (clickedIdx >= 0 && clickedIdx < total) {
                        selectedIndex = clickedIdx + 1; // +1 because 0 is ALLOW ALL
                        std::string sip = lanDevices[clickedIdx];
                        if (sip.find(" (This PC)") != std::string::npos) sip = sip.substr(0, sip.find(" (This PC)"));
                        auto it = std::find(g_AuthorizedWebIPs.begin(), g_AuthorizedWebIPs.end(), sip);
                        bool isNowAuth = true;
                        if (it != g_AuthorizedWebIPs.end()) { g_AuthorizedWebIPs.erase(it); isNowAuth = false; }
                        else g_AuthorizedWebIPs.push_back(sip);
                        // I wonder how many people are gonna read this comment lol
                        {
                            std::lock_guard<std::mutex> lock(ws_mutex);
                            for (auto& wc : ws_clients) {
                                if (wc.ip == sip) {
                                    wc.is_authorized = (g_AllowAllWebClients || isNowAuth);
                                }
                            }
                        }
                        // Thread-safe: let Mongoose thread deliver the revocation
                        if (!isNowAuth) broadcastTextToWebClients("MSG:REVOKED");
                        drawClients();
                    }
                }
            } else if (isModernMode && ir[i].EventType == MOUSE_EVENT && ir[i].Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
                int numItems = total + 1;
                // High word of dwButtonState contains the scroll delta
                short zDelta = HIWORD(ir[i].Event.MouseEvent.dwButtonState);
                if (zDelta > 0 && numItems > 0) { selectedIndex = (selectedIndex - 1 + numItems) % numItems; drawClients(); }
                else if (zDelta < 0 && numItems > 0) { selectedIndex = (selectedIndex + 1) % numItems; drawClients(); }
            }
        }
    }
    
    if (isModernMode) {
        system("cls");
    } else {
        if (printedLines > 0) std::cout << "\r\033[" << (printedLines + 2) << "A\033[0J";
    }
    SetConsoleMode(hIn, oldModeIn);
}

bool safeGetLineGlobalEsc(std::string& outStr, int limit = 255, bool numbersOnly = false, bool allowDots = false, const std::string& allowedChars = "", bool enforcePortMax = false) {
    outStr = "";
    while (true) {
        if (_kbhit()) {
            int c = _getch();
            if (c == 27) { // ESCAPE
                return false; 
            } else if (c == '\r' || c == '\n') {
                std::cout << "\n";
                break;
            } else if (c == '\b') {
                if (!outStr.empty()) {
                    outStr.pop_back();
                    std::cout << "\b \b";
                }
            } else if (c >= 32 && c <= 126) {
                if (outStr.length() >= limit) continue;
                if (numbersOnly) { if (!isdigit(c) && (!allowDots || c != '.')) continue; }
                if (!allowedChars.empty() && allowedChars.find((char)c) == std::string::npos) continue; // Deny disallowed chars silently
                
                if (enforcePortMax) {
                    std::string testStr = outStr + (char)c;
                    long long testVal = std::stoll(testStr);
                    if (testVal > 65535) continue; // Physically deny the keystroke from reaching the console
                }
                
                outStr += (char)c;
                std::cout << (char)c;
            }
        }
        Sleep(10);
    }
    return true;
}

void runLegacyMode() {
    g_IsModernModeActive = false;
    EnableColors();
    SetConsoleOutputCP(CP_UTF8);
    
    std::thread titleAnim(animateTitleThread);
    titleAnim.detach();

    while (true) {
        system("cls");
        if (!g_NetworkError.empty()) {
            std::cout << RED << " [!] SYSTEM MESSAGE: " << g_NetworkError << RESET << "\n\n";
            g_NetworkError = "";
        }
        
        auto getPrompt = [&]() {
            std::string promptName = "phonic";
            if (!g_Config.username.empty() && g_Config.username != "Guest" && g_Config.username != "Unknown") {
                promptName = g_Config.username.substr(0, 6);
            }
            return "\033[35mroot:" + promptName + ":~# \033[0m";
        };

        std::cout << ASCII_LOGO << std::flush;
        std::cout << GREEN << "  1. [ SENDER ] Transmit Audio to a Host Room" << RESET << std::endl;
        std::cout << GREEN << "  2. [RECEIVER] Join a Host Room & Listen" << RESET << std::endl;
        std::cout << CYAN  << "  3. [  HOST  ] Create a Central Relay Server" << RESET << std::endl;
        std::cout << YELLOW << "  4. [  EXIT  ] Terminate Application" << RESET << std::endl;
        
        if (g_ShowHelpTexts) {
            std::cout << "\n\033[92m[HELP] Enter the number of the mode you wish to use, e.g. '1', then press ENTER.\033[0m";
        }
        
        std::cout << "\n" << getPrompt() << "Select mode: ";
        
        std::string inputStr;

        if (!safeGetLineGlobalEsc(inputStr, 1, true, false)) { std::cout << "\n" << YELLOW << "[-] Returning to Main Menu...\n" << RESET; continue; }
        
        if (inputStr.empty()) continue;
        int choice = safeStoi(inputStr, 0);

        if (choice >= 1 && choice <= 3) {
            std::string inBuf = "";
            while (true) {
                std::cout << "\r\033[K" << getPrompt() << "Enter Profile Name (Only english characters) [" << inBuf.length() << "/24]: " << inBuf << std::flush;
                int c = _getch();
                if (c == 27) { inBuf = "ESC_EXIT"; break; } // ESC handling
                if (c == '\r' || c == '\n') break;
                if (c == '\b') {
                    if (!inBuf.empty()) inBuf.pop_back();
                } else if (inBuf.length() < 24 && (isalnum(c) || c == '_' || c == '-')) {
                    inBuf += (char)c;
                }
            }
            std::cout << "\n";
            if (inBuf == "ESC_EXIT") { std::cout << YELLOW << "[-] Cancelled.\n" << RESET; continue; }

            if (!inBuf.empty()) {
                g_ActiveProfileName = inBuf;
                loadConfig(g_ActiveProfileName);
                std::cout << GREEN << "[+] Profile Loaded! IP: " << g_Config.ip << " Port: " << g_Config.port << " PIN: " << g_Config.pin << RESET << "\n";
            }
            
            if (choice == 1 || choice == 2) { 
                std::string in;
                
                std::cout << getPrompt() << "Enter Username [" << g_Config.username << "]: ";
                if (!safeGetLineGlobalEsc(in, 24, false, false)) { std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; continue; }
                if(!in.empty()) g_Config.username = in;
                // Pre-sanitize whitespace-only names
                if (std::all_of(g_Config.username.begin(), g_Config.username.end(), ::isspace)) g_Config.username = "Guest";
                
                std::cout << getPrompt() << "Enter HOST IP [" << g_Config.ip << "]: ";
                if (!safeGetLineGlobalEsc(in, 24, false, true)) { std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; continue; }
                if(!in.empty()) g_Config.ip = in;
                
                std::cout << getPrompt() << "Enter HOST Port [" << g_Config.port << "]: ";
                if (!safeGetLineGlobalEsc(in, 5, true, false, "", true)) { std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; continue; }
                if(!in.empty()) g_Config.port = safeStoi(in, g_Config.port);
                
                std::cout << getPrompt() << "Enter ROOM PIN (Only use numbers) [" << g_Config.pin << "]: ";
                if (!safeGetLineGlobalEsc(in, 10, true, false)) { std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; continue; }
                if(!in.empty()) g_Config.pin = safeStoul(in, g_Config.pin);
                
                std::cout << getPrompt() << "Press [ENTER] to keep current MUTE Key (" << g_Config.muteKey << "),\nor PRESS ANY NEW KEY (Mouse 4, Numpad, etc.) to bind now... (Press ESC to cancel)\n";
                while (true) { bool anyPressed = false; for (int i = 1; i < 255; i++) { if (GetAsyncKeyState(i) & 0x8000) anyPressed = true; } if (!anyPressed) break; Sleep(10); } // Wait for release
                
                int newMuteKey = 0;
                bool escPressed = false;
                while (newMuteKey == 0) {
                    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { escPressed = true; break; }
                    if (GetAsyncKeyState(VK_RETURN) & 0x8000) { newMuteKey = g_Config.muteKey; break; }
                    for (int i = 1; i < 255; i++) {
                        if (i == VK_RETURN || i == VK_ESCAPE || i == VK_LBUTTON || i == VK_RBUTTON) continue;
                        if (GetAsyncKeyState(i) & 0x8000) { newMuteKey = i; break; }
                    }
                    Sleep(10);
                }
                if (!escPressed) {
                    g_Config.muteKey = newMuteKey;
                    std::cout << GREEN << "[+] Mute Key Confirmed (Virtual Key Code: " << newMuteKey << ")\n" << RESET;
                } else {
                    std::cout << YELLOW << "[-] Mute Key binding cancelled. Keeping previous key (" << g_Config.muteKey << ").\n" << RESET;
                }
                
                Sleep(200); // Wait for physical key release
                while (_kbhit()) _getch(); // Flush literal keyboard buffer so it doesn't bleed into the next cin
                
            if (choice == 1) { // SENDER
                bool pCanceled = false;
                std::cout << "\n"; // Prevent overlap line bug
                g_Config.pid = promptForPid(pCanceled, false);
                
                if (pCanceled) { std::cout << "\n" << YELLOW << "[-] Escaping to Main Menu...\n" << RESET; continue; }

                saveConfig(g_ActiveProfileName);
                bool runSenderLoop = true;
                while (runSenderLoop) {
                    runSenderLoop = false;
                    startSender(g_Config.ip, g_Config.port, g_Config.pin, g_Config.pid, g_Config.muteKey, g_Config.username);
                    if (g_TargetChangeRequested) {
                        uint32_t previousPid = g_Config.pid;
                        std::cout << "\n\n"; // Padding gap for Legacy UX so promptForPid drops below
                        g_Config.pid = promptForPid(pCanceled, false); // FALSE for legacy mode visually
                        if (pCanceled) { 
                            g_Config.pid = previousPid; 
                            std::cout << "\n" << YELLOW << "[-] Resuming active stream...\n" << RESET; 
                        }
                        runSenderLoop = true;    // Re-loop immediately with the new PID
                    }
                }
            } else if (choice == 2) { // RECEIVER
                
                std::string defWeb = g_Config.enableWebStream ? "Y" : "N";
                std::cout << getPrompt() << "Enable Mobile Web Server for this room? (Y/N) [" << defWeb << "]: ";
                
                while (true) {
                    if (!safeGetLineGlobalEsc(in, 1, false, false, "yYnN")) { in = ""; std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; break; }
                    if (in.empty()) { in = g_Config.enableWebStream ? "Y" : "N"; break; }
                    if (in == "Y" || in == "y" || in == "N" || in == "n") { break; }
                }
                
                if (in.empty()) continue; // ESC hit
                g_EnableWebStream = (in == "Y" || in == "y");
                g_Config.enableWebStream = g_EnableWebStream;
                if (g_EnableWebStream) {
                    std::string defWp = std::to_string(g_Config.webPort); if (defWp == "0") defWp = "8080";
                    std::cout << getPrompt() << "Mobile Web Server Port [" << defWp << "]: ";
                    if (!safeGetLineGlobalEsc(in, 8, true, false)) { std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; continue; }
                    if (in.empty()) in = defWp;
                    g_Config.webPort = safeStoi(in, 8080);
                }
                
                std::cout << GREEN << "\n[*] Joining room as Receiver...\n" << RESET;
                saveConfig(g_ActiveProfileName);
                startReceiver(g_Config.ip, g_Config.port, g_Config.pin, g_Config.username, g_Config.muteKey);
            }
            } else if (choice == 3) { // HOST
                std::string in;
                std::cout << getPrompt() << "Enter HOST Listen Port [" << g_Config.port << "]: ";
                if (!safeGetLineGlobalEsc(in, 5, true, false, "", true)) { std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; continue; }
                if(!in.empty()) g_Config.port = safeStoi(in, g_Config.port);
                
                std::cout << GREEN << " 1. [ SECURE ] VPN / Local Area Mode\n" << RESET;
                std::cout << RED << " 2. [INSECURE] UPnP Auto-Port Forward Mode\n" << RESET;
                std::cout << getPrompt() << "Select Mode [1]: ";
                
                while (true) {
                    if (!safeGetLineGlobalEsc(in, 1, false, false, "12")) { in = ""; std::cout << "\n" << YELLOW << "[-] Cancelled.\n" << RESET; break; }
                    if (in.empty() || in == "1" || in == "2") { if (in.empty()) in = "1"; break; }
                }
                if (in.empty()) continue; // ESC hit
                int secMode = safeStoi(in, 1);

                saveConfig(g_ActiveProfileName);

                if (secMode == 2) {
                    std::string ip = getLocalIP();
                    std::cout << YELLOW << "[*] Instructing router to open port " << g_Config.port << " -> " << ip << " ...\n" << RESET;
                    if (UPnP_ForwardPort(g_Config.port, ip)) {
                        std::cout << GREEN << "[+] UPnP SUCCESS! Port is open.\n" << RESET;
                    } else {
                        std::cout << RED << "[-] UPnP FAILED.\n" << RESET;
                    }
                }
                startHost(g_Config.port);
            }
        }
        else if (choice == 4) { break; }
        
        g_Config = AppConfig(); // Clear preset on return to menu
    }
}

// ======================================================================
// MODERN UI ENGINE
// This function operates the ANSI Grid Dashboard. It binds Mouse X/Y coordinates
// to an array index (activeBox) which controls highlighting and input capture.
// ======================================================================
void runModernMode() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    // Automatically force the console window to be wide/tall enough to prevent line-wrapping glitch
    COORD newSize = { 100, 48 };
    SMALL_RECT rect = { 0, 0, 99, 47 };
    SetConsoleScreenBufferSize(hOut, newSize);
    SetConsoleWindowInfo(hOut, TRUE, &rect);
    
    std::thread titleAnim(animateTitleThread);
    titleAnim.detach();

    std::string ip = g_Config.ip; if(ip.empty()) ip = "127.0.0.1";
    std::string port = std::to_string(g_Config.port); if(port=="0") port = "4444";
    std::string pin = std::to_string(g_Config.pin); if(pin=="0") pin = "1234";
    std::string pidStr = std::to_string(g_Config.pid);
    std::string uname = g_Config.username; if(uname.empty()) uname = "Guest";
    int muteKey = g_Config.muteKey;
    int secMode = 1;
    bool enableWeb = g_Config.enableWebStream;
    int rowWeb = 0;
    
    struct TextHistory { std::vector<std::string> undo; std::vector<std::string> redo; };
    TextHistory histIp, histPort, histPin, histPidStr, histUname;
    int activeMuteKey = muteKey;
    
    enum class Role { Sender, Receiver, Host };
    Role myRole = Role::Sender;

    int activeBox = 6; // Start at Role
    bool exitDashboard = false;
    bool goConnect = false;

    auto getFocusList = [&]() -> std::vector<int> {
        if (myRole == Role::Sender) return {6, 9, 5, 0, 1, 2, 3, 4, 7, 8};
        if (myRole == Role::Receiver) {
            std::vector<int> lst = {6, 9, 5, 0, 1, 2, 4, 11};
            lst.insert(lst.end(), {7, 8});
            return lst;
        }
        return {6, 1, 10, 7, 8}; // Host
    };
    
    auto nextBox = [&](int dir) {
        auto list = getFocusList();
        auto it = std::find(list.begin(), list.end(), activeBox);
        if (it == list.end()) { activeBox = list[0]; return; }
        int idx = std::distance(list.begin(), it) + dir;
        if (idx < 0) idx = list.size() - 1;
        if (idx >= list.size()) idx = 0;
        activeBox = list[idx];
    };

    auto sanitizeInput = [&](std::string& str, TextHistory& hist, WORD vK, char ch, bool numbersOnly, bool allowDots, bool ctrl) {
        if (ctrl && ch == 26) { // Ctrl+Z
            if (!hist.undo.empty()) { hist.redo.push_back(str); str = hist.undo.back(); hist.undo.pop_back(); } return;
        }
        if (ctrl && ch == 25) { // Ctrl+Y
            if (!hist.redo.empty()) { hist.undo.push_back(str); str = hist.redo.back(); hist.redo.pop_back(); } return;
        }
        if (ctrl && vK == VK_BACK) { hist.undo.push_back(str); hist.redo.clear(); str = ""; return; }
        if (vK == VK_BACK && !str.empty()) {
            hist.undo.push_back(str); hist.redo.clear(); str.pop_back();
        } else if (!ctrl && ch >= 32 && ch <= 126 && str.length() < 24) {
            bool allow = true;
            if (numbersOnly) { if (!isdigit(ch) && (!allowDots || ch != '.')) allow = false; } 
            
            // Phase 77 Physical Range Lockout
            if (allow && activeBox == 1) { 
                long long testVal = std::stoll(str + ch);
                if (testVal > 65535) allow = false;
            }
            
            if (allow) { hist.undo.push_back(str); hist.redo.clear(); str += ch; }
        }
    };

    auto handlePresetsMenu = [&]() {
        std::vector<std::string> options;
        options.push_back("[ + CREATE NEW PRESET ]");
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA("*.xxlrn", &fd);
        if(hFind != INVALID_HANDLE_VALUE) {
            do { options.push_back(fd.cFileName); } while(FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
        if(options.empty()) return;
        
        // Base terminal bounds guarantees safe minimal loading 
        ensureConsoleHeight(30);
        
        int selId = 0;
        bool menuExit = false;
        int confirmedIndex = -1;
        int startIdx = 0;
        
        auto renderMenu = [&]() -> int {
            SetConsoleCursorPosition(hOut, {0, 0});
            std::cout << "\033[0J";
            SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
            int baseRow = 0;
            std::istringstream logoStream(ASCII_LOGO); std::string l;
            while(std::getline(logoStream, l)) { std::cout << l << "\033[K\n"; baseRow++; }
            
            std::cout << MAGENTA << "=== LOAD OR SAVE PROFILE ===\033[K\n\n" << RESET;
            baseRow += 2;
            
            std::cout << "  " << (0 == confirmedIndex ? "\033[42;97m" : (0 == selId ? "\033[44;97m" : "")) << "[ + CREATE NEW PRESET ]\033[K" << RESET << "\n";
            baseRow += 1;
            
            int maxOptions = 5; // Hardcoded strictly to 5 visible limit
            
            startIdx = 1;
            if (selId >= 1) {
                if (selId >= 1 + maxOptions / 2) startIdx = selId - maxOptions / 2;
                if (startIdx + maxOptions > options.size()) startIdx = options.size() - maxOptions;
                if (startIdx < 1) startIdx = 1;
            }
            
            if (startIdx > 1) {
                std::cout << CYAN << "   ... (" << startIdx - 1 << " more above) ...\033[K\n" << RESET;
            } else {
                std::cout << "\033[K\n";
            }
            
            for(size_t i = startIdx; i < (std::min)((size_t)options.size(), (size_t)(startIdx + maxOptions)); i++) {
                std::cout << "  " << (i == confirmedIndex ? "\033[42;97m" : (i == selId ? "\033[44;97m" : "")) << "[ " << options[i] << " ]\033[K" << RESET << "\n";
            }
            
            // Pad empty rows if list is smaller than 5
            for(size_t i = options.size() - 1; i < maxOptions; i++) {
                std::cout << "\033[K\n";
            }
            
            if (startIdx + maxOptions < options.size()) {
                std::cout << CYAN << "   ... (" << options.size() - (startIdx + maxOptions) << " more below) ...\033[K\n" << RESET;
            } else {
                std::cout << "\033[K\n";
            }
            
            std::cout << "\n\033[90mTip: Arrow Keys to move, ENTER to load. ESC to cancel.\033[K\033[0m\n";
            return baseRow;
        };

        system("cls"); // Initial full wipe for transition
        while(!menuExit) {
            int baseRow = renderMenu();
            
            AppConfig originalCfg = g_Config;
            
            if (selId == 0) {
                std::cout << "\n" << CYAN << "--- PREVIEW ---\033[K\n" << RESET;
                std::cout << "Create a new .xxlrn configuration file based on the current Modern Dashboard settings.\n";
            } else {
                loadConfig(options[selId].substr(0, options[selId].length() - 6));
                AppConfig previewCfg = g_Config;
                g_Config = originalCfg;
                
                std::cout << "\n" << CYAN << "--- PREVIEW ---\033[K\n" << RESET;
                std::cout << "Username: " << previewCfg.username << "\nIP: " << previewCfg.ip << "\nPORT: " << previewCfg.port 
                          << "\nPIN: " << previewCfg.pin << "\nTARGET PID: " << previewCfg.pid << "\nMUTE KEY: " << previewCfg.muteKey 
                          << "\nWEB STREAM: " << (previewCfg.enableWebStream ? "ON" : "OFF") << "\n";
            }
            
            FlushConsoleInputBuffer(hIn);
            INPUT_RECORD ir[128];
            bool actionDone = false;
            bool triggerCreate = false;
            while(!actionDone) {
                DWORD read;
                GetNumberOfConsoleInputEvents(hIn, &read);
                if (read > 0) {
                    ReadConsoleInput(hIn, ir, 128, &read);
                    for (DWORD i = 0; i < read; i++) {
                        if (ir[i].EventType == MOUSE_EVENT && ir[i].Event.MouseEvent.dwEventFlags == 0 && (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                            int my = ir[i].Event.MouseEvent.dwMousePosition.Y;
                            int listOffset = my - baseRow;
                            int clickedIdx = -1;
                            
                            if (listOffset == -1) { // CREATE NEW
                                clickedIdx = 0;
                            } else if (listOffset == 0) { // UP BUTTON
                                selId--; if (selId < 1) selId = options.size() - 1; actionDone = true; break;
                            } else if (listOffset >= 1 && listOffset <= 5) { // 5 ITEMS
                                clickedIdx = startIdx + (listOffset - 1);
                            } else if (listOffset == 6) { // DOWN BUTTON
                                selId++; if (selId >= options.size()) selId = 1; actionDone = true; break;
                            }
                            
                            if (clickedIdx >= 0 && clickedIdx < options.size()) {
                                selId = clickedIdx;
                                if (selId == 0) {
                                    triggerCreate = true; actionDone = true; break;
                                } else {
                                    loadConfig(options[selId].substr(0, options[selId].length() - 6));
                                    AppConfig clickedCfg = g_Config;
                                    g_Config = originalCfg; // Restore global
                                    
                                    confirmedIndex = selId;
                                    renderMenu(); Sleep(300); confirmedIndex = -1;
                                    
                                    ip = clickedCfg.ip; port = std::to_string(clickedCfg.port); pin = std::to_string(clickedCfg.pin); 
                                    uname = clickedCfg.username; muteKey = clickedCfg.muteKey; pidStr = std::to_string(clickedCfg.pid);
                                    enableWeb = clickedCfg.enableWebStream;
                                    menuExit = true; actionDone = true; activeBox = -1; break;
                                }
                            }
                        } else if (ir[i].EventType == MOUSE_EVENT && ir[i].Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
                            int32_t zDelta = ir[i].Event.MouseEvent.dwButtonState;
                            if (zDelta > 0) {
                                selId--; if(selId < 0) selId = options.size()-1; actionDone=true; break;
                            } else {
                                selId++; if(selId >= options.size()) selId = 0; actionDone=true; break;
                            }
                        } else if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                            WORD vK = ir[i].Event.KeyEvent.wVirtualKeyCode;
                            if (vK == VK_UP) { selId--; if(selId < 0) selId = options.size()-1; actionDone=true; break; }
                            if (vK == VK_DOWN) { selId++; if(selId >= options.size()) selId = 0; actionDone=true; break; }
                            if (vK == VK_ESCAPE) { menuExit = true; actionDone = true; break; }
                            if (vK == VK_RETURN) { 
                                if (selId == 0) {
                                    triggerCreate = true; actionDone = true; break;
                                } else {
                                    loadConfig(options[selId].substr(0, options[selId].length() - 6));
                                    AppConfig previewCfg = g_Config;
                                    g_Config = originalCfg;
                                    confirmedIndex = selId;
                                    renderMenu(); Sleep(300); confirmedIndex = -1;
                                    
                                    ip = previewCfg.ip; port = std::to_string(previewCfg.port); pin = std::to_string(previewCfg.pin); 
                                    uname = previewCfg.username; muteKey = previewCfg.muteKey; pidStr = std::to_string(previewCfg.pid);
                                    enableWeb = previewCfg.enableWebStream;
                                    activeMuteKey = muteKey;
                                    menuExit = true; actionDone = true; activeBox = -1; break;
                                }
                            }
                        }
                    }
                } else { Sleep(10); }
            }
            
            if (triggerCreate) {
                std::string newName = "";
                bool createCancel = false;
                while (true) {
                    system("cls");
                    std::cout << MAGENTA << "=== CREATE NEW PRESET ===\n\n" << RESET;
                    std::cout << "Enter Profile Name (Press ESC to cancel)\n\n> " << newName << "\033[90m.xxlrn\033[0m\n";
                    char c = _getch();
                    if (c == 27) { createCancel = true; break; } // ESCAPE
                    if (c == '\r' || c == '\n') break;
                    if (c == '\b') { if (!newName.empty()) newName.pop_back(); }
                    else if (isalnum(c) || c == '_' || c == '-' || c == ' ') {
                        if (newName.length() < 24) newName += c;
                    }
                }
                if (createCancel) { menuExit = true; activeBox = -1; }
                else if (!newName.empty()) {
                    g_Config.ip = ip; g_Config.port = safeStoi(port, 4444); g_Config.pin = safeStoul(pin, 1234);
                    g_Config.username = uname; g_Config.muteKey = activeMuteKey; g_Config.pid = safeStoul(pidStr, 0);
                    g_Config.enableWebStream = enableWeb;
                    saveConfig(newName);
                    menuExit = true; activeBox = -1;
                }
            }
        }
        FlushConsoleInputBuffer(hIn);
    };

    auto handlePidList = [&]() {
        bool cancelled = false;
        system("cls"); // Full clear once to transition smoothly to the large PID menu 
        uint32_t selected = promptForPid(cancelled);
        if (!cancelled) {
            pidStr = std::to_string(selected);
            g_Config.pid = selected; // update global sync
        }
        activeBox = -1;
        system("cls"); // Full wipe once returning to dashboard to reset the cursor correctly
    };

    auto handleWebClients = [&]() {
        promptForWebClients(true);
        ensureConsoleHeight(30);
        activeBox = -1;
    };

    while (!exitDashboard && !goConnect) {
        DWORD oldModeIn, oldModeOut;
        GetConsoleMode(hIn, &oldModeIn);
        GetConsoleMode(hOut, &oldModeOut);
        
        SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
        SetConsoleMode(hOut, oldModeOut | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

        int rowRole = -1, rowPresets = -1, rowUname = -1, rowIp = -1, rowPort = -1, rowPin = -1, rowPid = -1, rowMute = -1, rowSec = -1, rowWeb = -1, rowWebClients = -1, rowConn = -1, rowExit = -1;

        // Renders the entire graphical UI grid.
        auto printDashboard = [&]() {
            SetConsoleCursorPosition(hOut, {0, 0});
            std::cout << "\033[0J";
            SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
            int currentY = 0;
            
            std::istringstream logoStream(ASCII_LOGO);
            std::string line;
            while(std::getline(logoStream, line)) { std::cout << line << "\033[K\n"; currentY++; }
            std::cout << "\033[K\n"; currentY++;

            if (!g_NetworkError.empty()) {
                std::cout << RED << "[!] SYSTEM MESSAGE: " << g_NetworkError << RESET << "\033[K\n\033[K\n";
                currentY += 2;
                g_NetworkError = ""; // Consume error
            } else {
                // Ensure phantom errors don't stay on screen when updating
                std::cout << "\033[K\n\033[K\n";
                currentY += 2;
                std::cout << "\033[2A";
                currentY -= 2;
            }
            
            rowRole = currentY;
            std::string bgS = (myRole == Role::Sender) ? "\033[42;30m" : "";
            std::string bgR = (myRole == Role::Receiver) ? "\033[42;30m" : "";
            std::string bgH = (myRole == Role::Host) ? "\033[42;30m" : "";
            
            if (activeBox == 6) {
                if(myRole == Role::Sender) bgS = "\033[44;97m";
                else if(myRole == Role::Receiver) bgR = "\033[44;97m";
                else bgH = "\033[44;97m";
            }
            
            std::cout << "  ROLE:         " 
                      << "[" << bgS << (myRole==Role::Sender?"X":" ") << RESET << "] " << bgS << "SENDER" << RESET << "    " 
                      << "[" << bgR << (myRole==Role::Receiver?"X":" ") << RESET << "] " << bgR << "RECEIVER" << RESET << "    "
                      << "[" << bgH << (myRole==Role::Host?"X":" ") << RESET << "] " << bgH << "HOST" << RESET << "\033[K\n";
            currentY += 1;

            auto printField = [&](const std::string& label, const std::string& val, int id, int& rowTracker, bool highlightRed = false) {
                rowTracker = currentY;
                std::cout << "  " << std::left << std::setw(12) << label << ": ";
                std::string bg = "";
                if (activeBox == id) bg = (highlightRed ? "\033[41;97m" : "\033[44;97m");
                std::cout << "[ " << bg << std::left << std::setw(30) << val << RESET << " ]\033[K\n";
                currentY++;
            };

            if (myRole == Role::Sender || myRole == Role::Receiver) {
                printField("PRESETS", "LOAD PROFILE...", 9, rowPresets);
                printField("USERNAME", uname, 5, rowUname);
                printField("HOST IP", ip, 0, rowIp);
                printField("HOST PORT", port, 1, rowPort);
                printField("ROOM PIN", pin, 2, rowPin);
            }
            if (myRole == Role::Sender) {
                printField("TARGET PID", pidStr + " (0 = System)", 3, rowPid);
            }
            if (myRole == Role::Sender || myRole == Role::Receiver) {
                if (activeBox != 4) activeMuteKey = muteKey;
                int displayMute = (activeBox == 4) ? activeMuteKey : muteKey;
                std::string dispStr = (displayMute==0 ? "NONE" : std::to_string(displayMute));
                
                if (activeBox == 4 && activeMuteKey == muteKey) dispStr = "Waiting for input...";
                else if (activeBox == 4) dispStr += " (Press ENTER to Confirm)";
                
                printField("MUTE KEY", dispStr, 4, rowMute, true);
            }
            if (myRole == Role::Receiver) {
                rowWeb = currentY;
                std::cout << "  WEB STREAM  : ";
                std::string bg = (activeBox == 11) ? "\033[44;97m" : "";
                std::cout << "[" << bg << (enableWeb ? "X" : " ") << RESET << "] " << bg << (enableWeb ? "ON (Port: " + std::to_string(g_Config.webPort) + ")" : "OFF") << RESET << "\n";
                currentY++;
            }
            if (myRole == Role::Host) {
                printField("HOST PORT", port, 1, rowPort);
                rowSec = currentY;
                std::cout << "  SECURE MODE : ";
                std::string bg = (activeBox == 10) ? "\033[44;97m" : "";
                std::cout << "[" << bg << (secMode==1 ? "X" : " ") << RESET << "] " << bg << (secMode==1 ? "VPN / Local (Secure)" : "UPnP Auto-Forward (Insecure)") << RESET << "\n";
                currentY++;
            }
            
            while (currentY < rowRole + 9) { std::cout << "\033[K\n"; currentY++; }
            rowConn = currentY;
            std::cout << "                  [" << (activeBox==7?"\033[44;97m":"\033[42;30m") << "       CONNECT       " << RESET << "]\033[K\n";
            currentY += 1;
            
            while (currentY < rowRole + 11) { std::cout << "\033[K\n"; currentY++; }
            rowExit = currentY;
            std::cout << "                  [" << (activeBox==8?"\033[44;97m":"\033[41;97m") << " Exit to Boot Menu   " << RESET << "]\033[K\n";
            currentY += 1;
            
            while (currentY < rowRole + 13) { std::cout << "\033[K\n"; currentY++; }
            // Helpful tooltips controlled by the global toggle
            if (g_ShowHelpTexts) {
                std::cout << "\033[92m[HELP] Navigation: Up/Down Arrows or Mouse Scroll Wheel.\n"
                          << "[HELP] Action: Click regions to edit or press ENTER to confirm.\n"
                          << "[HELP] Shortcuts: Press ESC instantly to abort and exit to Boot Menu.\033[0m";
            }
            
            std::cout << std::flush;
        };

        printDashboard();

        INPUT_RECORD ir[128];
        bool eventTriggered = false;
        while (!exitDashboard && !goConnect && !eventTriggered) {
            DWORD read;
            GetNumberOfConsoleInputEvents(hIn, &read);
            if (read > 0) {
                ReadConsoleInput(hIn, ir, 128, &read);
                for (DWORD i = 0; i < read; i++) {
                    if (ir[i].EventType == MOUSE_EVENT) {
                        if (ir[i].Event.MouseEvent.dwEventFlags == 0 && (ir[i].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                            int mx = ir[i].Event.MouseEvent.dwMousePosition.X;
                            int my = ir[i].Event.MouseEvent.dwMousePosition.Y;
                            
                            if (my == rowRole) {
                                if (mx >= 16 && mx <= 26) myRole = Role::Sender;
                                else if (mx >= 30 && mx <= 42) myRole = Role::Receiver;
                                else if (mx >= 46 && mx <= 54) myRole = Role::Host;
                                activeBox = 6; // Force the box to remain on Role safely
                                eventTriggered = true; break;
                            }
                            else if (my == rowPresets && mx >= 16 && mx <= 48) { activeBox = 9; handlePresetsMenu(); eventTriggered = true; break; }
                            else if (my == rowUname && mx >= 16 && mx <= 48) { activeBox = 5; eventTriggered = true; break; }
                            else if (my == rowIp && mx >= 16 && mx <= 48) { activeBox = 0; eventTriggered = true; break; }
                            else if (my == rowPort && mx >= 16 && mx <= 48) { activeBox = 1; eventTriggered = true; break; }
                            else if (my == rowPin && mx >= 16 && mx <= 48) { activeBox = 2; eventTriggered = true; break; }
                            else if (my == rowPid && myRole == Role::Sender && mx >= 16 && mx <= 48) { activeBox = 3; handlePidList(); eventTriggered = true; break; }
                            else if (my == rowMute && mx >= 16 && mx <= 48) { activeBox = 4; eventTriggered = true; break; }
                            else if (my == rowWeb && myRole == Role::Receiver && mx >= 16 && mx <= 48) {
                                activeBox = 11;
                                enableWeb = !enableWeb;
                                if (enableWeb) {
                                    SetConsoleCursorPosition(hOut, {0, (short)(rowWeb+2)});
                                    std::cout << "\r\033[K>> Enter Web Server Port [" << g_Config.webPort << "]: ";
                                    std::string wp;
                                    if (safeGetLineGlobalEsc(wp, 5, true, false, "", true) && !wp.empty()) g_Config.webPort = safeStoi(wp, 8080);
                                }
                                eventTriggered = true; break;
                            }
                            else if (my == rowSec && myRole == Role::Host && mx >= 16 && mx <= 48) {
                                activeBox = 10;
                                secMode = (secMode == 1) ? 2 : 1;
                                eventTriggered = true; break;
                            }
                            else if (my == rowConn && mx >= 18 && mx <= 41) { activeBox = 7; goConnect = true; break; }
                            else if (my == rowExit && mx >= 18 && mx <= 41) { activeBox = 8; exitDashboard = true; break; }
                            else {
                                if (activeBox != -1) { activeBox = -1; eventTriggered = true; break; }
                            }
                        } else if (ir[i].Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
                            int32_t zDelta = ir[i].Event.MouseEvent.dwButtonState;
                            if (zDelta > 0) nextBox(-1); else nextBox(1);
                            eventTriggered = true; break;
                        }
                    } else if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                        WORD vK = ir[i].Event.KeyEvent.wVirtualKeyCode;
                        char ch = ir[i].Event.KeyEvent.uChar.AsciiChar;
                        
                        if (activeBox == 4) {
                            if (vK == VK_RETURN) { muteKey = activeMuteKey; activeBox = -1; eventTriggered = true; break; }
                            else if (vK == VK_ESCAPE) { activeMuteKey = muteKey; activeBox = -1; eventTriggered = true; break; }
                            else if (vK != VK_UP && vK != VK_DOWN && vK != VK_LEFT && vK != VK_RIGHT && vK != VK_TAB) {
                                if (activeMuteKey == muteKey) { activeMuteKey = vK; eventTriggered = true; break; }
                            }
                            if (activeMuteKey != muteKey) { eventTriggered = true; break; }
                        }
                        
                        if (vK == VK_UP) { nextBox(-1); eventTriggered = true; break; }
                        else if (vK == VK_DOWN || vK == VK_TAB) { nextBox(1); eventTriggered = true; break; }
                        else if (vK == VK_LEFT || vK == VK_RIGHT) {
                            if (activeBox == 6) {
                                if (vK == VK_RIGHT) {
                                    if(myRole == Role::Sender) myRole = Role::Receiver;
                                    else if(myRole == Role::Receiver) myRole = Role::Host;
                                    else myRole = Role::Sender;
                                } else {
                                    if(myRole == Role::Sender) myRole = Role::Host;
                                    else if(myRole == Role::Receiver) myRole = Role::Sender;
                                    else myRole = Role::Receiver;
                                }
                                activeBox = 6; // Explicitly reset focus to Role Switcher to prevent layout glitches on unseen indices
                                printDashboard(); 
                                eventTriggered = true; break;
                            } else if (activeBox == 10 && myRole == Role::Host) {
                                secMode = (secMode == 1) ? 2 : 1;
                                eventTriggered = true; break;
                            } else if (activeBox != -1) {
                                std::string* target = nullptr; bool no = false, ad = false; TextHistory* th = nullptr;
                                if (activeBox == 0) { target = &ip; no = true; ad = true; th = &histIp; }
                                else if (activeBox == 1) { target = &port; no = true; th = &histPort; }
                                else if (activeBox == 2) { target = &pin; no = true; th = &histPin; }
                                else if (activeBox == 3) { target = &pidStr; no = true; th = &histPidStr; }
                                else if (activeBox == 5) { target = &uname; th = &histUname; }
                                bool ctrl = (ir[i].Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
                                if (target && th) { sanitizeInput(*target, *th, vK, ch, no, ad, ctrl); eventTriggered = true; break; }
                            }
                        }
                        else if (vK == VK_ESCAPE) { exitDashboard = true; eventTriggered = true; break; }
                        else if (vK == VK_RETURN) {
                            if (activeBox == 7) { goConnect = true; break; }
                            else if (activeBox == 8) { exitDashboard = true; break; }
                            else if (activeBox == 3) { handlePidList(); eventTriggered = true; break; }
                            else if (activeBox == 9) { handlePresetsMenu(); eventTriggered = true; break; }
                            else if (activeBox == 10) { secMode = (secMode==1) ? 2 : 1; eventTriggered = true; break; }
                            else if (activeBox == 11) { 
                                if (myRole == Role::Receiver) {
                                    enableWeb = !enableWeb; 
                                    if (enableWeb) {
                                        SetConsoleCursorPosition(hOut, {0, (short)(rowWeb+2)});
                                        std::cout << "\r\033[K>> Enter Web Server Port [" << g_Config.webPort << "]: ";
                                        std::string wp;
                                        if (safeGetLineGlobalEsc(wp, 5, true, false, "", true) && !wp.empty()) g_Config.webPort = safeStoi(wp, 8080);
                                    }
                                }
                                eventTriggered = true; break; 
                            }
                            else if (activeBox == 12) { handleWebClients(); eventTriggered = true; break; }
                            else { nextBox(1); eventTriggered = true; break; }
                        }
                        else if (activeBox != -1) {
                            std::string* target = nullptr; bool no = false, ad = false; TextHistory* th = nullptr;
                            if (activeBox == 0) { target = &ip; no = true; ad = true; th = &histIp; }
                            else if (activeBox == 1) { target = &port; no = true; th = &histPort; }
                            else if (activeBox == 2) { target = &pin; no = true; th = &histPin; }
                            else if (activeBox == 3) { target = &pidStr; no = true; th = &histPidStr; }
                            else if (activeBox == 5) { target = &uname; th = &histUname; }
                            bool ctrl = (ir[i].Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
                            if (target && th) { sanitizeInput(*target, *th, vK, ch, no, ad, ctrl); eventTriggered = true; break; }
                        }
                    }
                }
            } else { Sleep(10); }
        }
        
        SetConsoleMode(hIn, oldModeIn);
    }
    
    if (goConnect) {
        system("cls");
        
        bool isBlank = std::all_of(uname.begin(), uname.end(), [](char c){ return std::isspace(static_cast<unsigned char>(c)); });
        if (uname.empty() || isBlank) uname = "Guest";
        
        int pt = safeStoi(port, 4444);
        uint32_t pn = safeStoul(pin, 1234);
        AppConfig cfg = g_Config; // Preserve
        cfg.ip = ip; cfg.port = pt; cfg.pin = pn; cfg.username = uname; cfg.muteKey = muteKey;
        cfg.enableWebStream = enableWeb;
        g_Config = cfg;
        saveConfig("modern_auto");
        
        if (myRole == Role::Sender) {
            uint32_t pd = safeStoul(pidStr, 0);
            
            bool runSender = true;
            bool pCanceled = false;
            while (runSender) {
                runSender = false;
                system("cls"); // CLS BEFORE SENDER Starts
                startSender(ip, pt, pn, pd, muteKey, uname);
                if (g_TargetChangeRequested) {
                    uint32_t previousPid = pd;
                    pd = promptForPid(pCanceled, true); // TRUE for Modern Native Mode Resizing
                    system("cls"); // CLS after returning from PID menu
                    if (pCanceled) { 
                        pd = previousPid;
                    }
                    std::cout << "\n" << YELLOW << "[-] Resuming active stream...\n" << RESET;
                    runSender = true;
                }
            }
        } else if (myRole == Role::Receiver) {
            system("cls"); // CLS BEFORE RECEIVER Starts
            g_EnableWebStream = enableWeb;
            startReceiver(ip, pt, pn, uname, muteKey);
        } else if (myRole == Role::Host) {
            if (secMode == 2) {
                std::string myIp = getLocalIP();
                std::cout << YELLOW << "[*] Instructing router to open port " << port << " -> " << myIp << " ...\n" << RESET;
                if (UPnP_ForwardPort(pt, myIp)) {
                    std::cout << GREEN << "[+] UPnP SUCCESS! Port is open.\n" << RESET;
                } else {
                    std::cout << RED << "[-] UPnP FAILED.\n" << RESET;
                }
            }
            startHost(pt);
        }
    }
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        return TRUE; // Ignore Ctrl+C
    }
    return FALSE;
}

std::vector<std::string> g_OpeningMessages = {
    "don't worry mom, the bridge is about to end",
    "sounds are cool!",
    "initiating packet loss matrix...",
    "clippy was here",
    "transmitting via temporal sub-frequencies",
    "phonic means sound!",
    "bandwidth is just a state of mind",
    "HOTFIX 0.66.6: clippy is deleted",
    "sponsored by q7.mtd",
    "worlds first hybrid supersonic sound... uhhh... yeah i forgot the rest",
    "xxlrn was here!"
};

int main() {
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    SetConsoleOutputCP(CP_UTF8);
    EnableColors();

    // Pick a random Opening Message once per application launch
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, g_OpeningMessages.size() - 1);
    std::string activeOpeningMsg = g_OpeningMessages[dist(gen)];
    
    WSADATA wsaData; 
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    while (true) {
        system("cls");
        std::cout << "\033[36m============================================================\033[0m\n";
        std::cout << "\033[35m                 PHONIC BRIDGE BOOT MANAGER                 \033[0m\n";
        // Center the active Opening Message
        int pad = (60 - activeOpeningMsg.length()) / 2;
        if (pad < 0) pad = 0;
        std::cout << "\033[90m" << std::string(pad, ' ') << activeOpeningMsg << "\033[0m\n";
        std::cout << "\033[36m============================================================\033[0m\n";
        std::cout << "  \033[32m[1] Phonic Bridge (Legacy Mode)\033[0m\n";
        std::cout << "  \033[32m[2] Phonic Bridge (Modern Mode)\033[0m\n";
        std::cout << "  \033[33m[3] Toggle Help Texts (Current: " << (g_ShowHelpTexts ? "ON)" : "OFF)") << "\033[0m\n";
        std::cout << "  \033[31m[4] Exit Application\033[0m\n\n";
        std::cout << "  Select boot mode: ";
        
        int choice = 0;
        while (true) {
            if (_kbhit()) {
                char c = _getch();
                if (c == '1') { choice = 1; break; }
                if (c == '2') { choice = 2; break; }
                if (c == '3') { g_ShowHelpTexts = !g_ShowHelpTexts; choice = 3; break; }
                if (c == '4') { choice = 4; break; }
            }
            Sleep(50);
        }
        
        if (choice == 3) continue; // Loop back to redraw boot menu with updated status
        
        if (choice == 1) {
            runLegacyMode();
        } else if (choice == 2) {
            g_IsModernModeActive = true;
            runModernMode();
        } else if (choice == 4) {
            break;
        }
    }
    
    WSACleanup();
    return 0;
}

// ======================================================================
// ISOLATED WEBSOCKET SERVER API (Mongoose Engine V8)
// ======================================================================

const std::string RESTRICTED_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ACCESS DENIED</title>
    <style>
        body {
            background-color: #050000;
            color: #ff0000;
            font-family: 'Courier New', Courier, monospace;
            text-align: center;
            overflow: hidden;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        .title {
            font-size: 2.2rem;
            font-weight: bold;
            color: #ff0000;
            margin-bottom: 20px;
            letter-spacing: 2px;
        }
        .msg {
            max-width: 80%;
            margin-top: 30px;
            font-size: 1rem;
            text-align: center;
            line-height: 1.5;
        }
        .ip-highlight {
            color: #ff0000;
            font-weight: bold;
            display: inline-block;
            margin-top: 20px;
            font-size: 1.2rem;
        }
        .desktop-text { display: inline; }
        .mobile-text { display: none; }
        @media (max-width: 600px) {
            .desktop-text { display: none; }
            .mobile-text { display: inline; }
        }
    </style>
</head>
<body>
    <div class="title"><span class="desktop-text">[ ACCESS DENIED ]</span><span class="mobile-text">[ACCESS DENIED]</span></div>
    <div class="msg">
        YOUR DEVICE HAS BEEN RESTRICTED OR BANNED.<br><br>
        PLEASE INSTRUCT THE HOST TO WHITELIST YOUR IP:<br>
        <span class="ip-highlight">{{IP}}</span>
    </div>
</body>
</html>
)=====";

const std::string OFFLINE_HTML = std::string(R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>[ RECEIVER OFFLINE ]</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap');
        body { background-color: #080808; color: #ffcc00; font-family: 'Share Tech Mono', monospace; margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; text-transform: uppercase; }
        .text { text-align: center; }
        .title { font-size: 2rem; margin-bottom: 20px; font-weight: bold; background: #ffcc00; color: #080808; padding: 5px 15px; }
        .desktop-text { display: inline; }
        .mobile-text { display: none; }
        @media (max-width: 600px) {
            .desktop-text { display: none; }
            .mobile-text { display: inline; }
        }
    </style>
</head>
<body>
    <div class="title"><span class="desktop-text">[ RECEIVER OFFLINE ]</span><span class="mobile-text">[RECEIVER OFFLINE]</span></div>
    <div class="text">THE AUDIO BRIDGE HAS BEEN TERMINATED OR SUSPENDED.</div>
    <div class="text" style="color: #666; margin-top: 20px;">PLEASE AWAIT INITIALIZATION.</div>
</body>
</html>
)=====");

static void webServerEventHandler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        
        char ip_buf[64];
        mg_snprintf(ip_buf, sizeof(ip_buf), "%M", mg_print_ip, &c->rem);
        std::string reqIP(ip_buf);
        if (reqIP.find("::ffff:") == 0) reqIP = reqIP.substr(7);
        
        bool isAuth = false;
        if (!g_WebSocketsActive) isAuth = false;
        else if (g_AllowAllWebClients || reqIP == "127.0.0.1") isAuth = true;
        else {
            auto it = std::find(g_AuthorizedWebIPs.begin(), g_AuthorizedWebIPs.end(), reqIP);
            isAuth = (it != g_AuthorizedWebIPs.end());
        }

        if (mg_match(hm->uri, mg_str("/stream"), NULL)) { // Upgrade to WebSocket
            mg_ws_upgrade(c, hm, NULL);
            // DO NOT WRITE TO c->data! Mongoose 7.x uses it internally for buffers.
            
            if (g_ExitReceiver) {
                std::string rm = "MSG:CONNECTION_LOST";
                if (g_NetworkError == "KICKED") rm = "MSG:KICKED";
                else if (g_NetworkError == "BANNED") rm = "MSG:BANNED";
                mg_ws_send(c, rm.c_str(), rm.length(), WEBSOCKET_OP_TEXT);
                return;
            }
            if (!isAuth) {
                std::string rm = "MSG:REVOKED";
                mg_ws_send(c, rm.c_str(), rm.length(), WEBSOCKET_OP_TEXT);
                return;
            }
            
            std::lock_guard<std::mutex> lock(ws_mutex);
            ws_clients.push_back({c, reqIP, true});
            
            if (g_HostMuted) {
                std::string rm = "MSG:MUTED";
                mg_ws_send(c, rm.c_str(), rm.length(), WEBSOCKET_OP_TEXT);
            } else {
                std::string rm = "MSG:UNMUTED_SILENT";
                mg_ws_send(c, rm.c_str(), rm.length(), WEBSOCKET_OP_TEXT);
            }
        } else if (mg_match(hm->uri, mg_str("/api/status"), NULL)) {
            if (g_ExitReceiver) mg_http_reply(c, 503, "", "Idle");
            else if (!isAuth) mg_http_reply(c, 403, "", "Denied");
            else mg_http_reply(c, 200, "", "Active");
        } else { // Serve the UI
            if (!isAuth) {
                if (!g_WebSocketsActive) {
                    mg_http_reply(c, 503, "Content-Type: text/plain\r\n", "HTTP 503 Service Unavailable: Web Stream is currently disabled by the Host.");
                } else {
                    std::string html = RESTRICTED_HTML;
                    size_t pos = html.find("{{IP}}");
                    if (pos != std::string::npos) html.replace(pos, 6, reqIP);
                    mg_http_reply(c, 403, "Content-Type: text/html\r\n", "%s", html.c_str());
                }
                return;
            }
            // Even if authorized, if the Receiver loop hasn't started natively we show an offline banner, NOT the banned page.
            if (!g_WebServerThreadSpawned || (g_ExitReceiver && ws_clients.empty())) {
                mg_http_reply(c, 503, "Content-Type: text/html\r\n", "%s", OFFLINE_HTML.c_str());
                return;
            }
            mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", WEB_CLIENT_HTML.c_str());
        }
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        if (wm->data.len == 4 && memcmp(wm->data.buf, "PING", 4) == 0) {
            mg_ws_send(c, "PONG", 4, WEBSOCKET_OP_TEXT);
        }
    } else if (ev == MG_EV_CLOSE) {
        std::lock_guard<std::mutex> lock(ws_mutex);
        ws_clients.erase(
            std::remove_if(ws_clients.begin(), ws_clients.end(),
                           [c](const WebClient& wc) { return wc.conn == c; }),
            ws_clients.end()
        );
    } else if (ev == MG_EV_POLL) {
        // Drain the thread-safe broadcast queues ON the Mongoose thread.
        // This is the ONLY place mg_ws_send is called for broadcast data, ensuring thread safety.
        std::vector<std::vector<char>> audioCopy;
        std::vector<std::string> textCopy;
        {
            std::lock_guard<std::mutex> lock(g_wsBroadcastMutex);
            audioCopy.swap(g_wsAudioQueue);
            textCopy.swap(g_wsTextQueue);
        }
        if (!audioCopy.empty() || !textCopy.empty()) {
            std::lock_guard<std::mutex> lock(ws_mutex);
            // Send text control messages first (KICK/BAN/MUTE have priority)
            for (const auto& txt : textCopy) {
                for (const WebClient& client : ws_clients) {
                    if (client.is_authorized && client.conn != nullptr) {
                        if (client.conn->send.len >= 131072) continue;
                        mg_ws_send(client.conn, txt.c_str(), txt.length(), WEBSOCKET_OP_TEXT);
                    }
                }
            }
            // Send binary audio packets
            for (const auto& buf : audioCopy) {
                for (const WebClient& client : ws_clients) {
                    if (client.is_authorized && client.conn != nullptr) {
                        if (client.conn->send.len >= 131072) continue;
                        mg_ws_send(client.conn, buf.data(), buf.size(), WEBSOCKET_OP_BINARY);
                    }
                }
            }
        }
    }
}

// Thread-safe producers. These push data into queues that the Mongoose thread drains.
// NEVER call mg_ws_send from these functions — that would be a cross-thread race condition.
void broadcastToWebClients(const char* data, size_t len) {
    if (!g_WebSocketsActive) return;
    std::lock_guard<std::mutex> lock(g_wsBroadcastMutex);
    // Drop if queue is already backed up (>50 packets ~ 500ms of audio) to prevent memory explosion
    if (g_wsAudioQueue.size() >= 50) return;
    g_wsAudioQueue.emplace_back(data, data + len);
}

void broadcastTextToWebClients(const std::string& text) {
    if (!g_WebSocketsActive) return;
    std::lock_guard<std::mutex> lock(g_wsBroadcastMutex);
    g_wsTextQueue.push_back(text);
}

void startWebServerThread() {
    g_WebThreadActive = true;
    mg_log_set(0); // Silence mongoose internal logs to prevent breaking the UI
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    std::string listenUrl = "http://0.0.0.0:" + std::to_string(g_Config.webPort);
    
    // Attempt binding multiple times if Windows is still slowly releasing the port from a hard crash
    bool bound = false;
    for(int attempt=0; attempt < 20; attempt++) {
        if (mg_http_listen(&mgr, listenUrl.c_str(), webServerEventHandler, NULL) != NULL) {
            bound = true; break;
        }
        Sleep(100);
    }
    
    if (!bound) {
        g_WebServerThreadSpawned = false;
        mg_mgr_free(&mgr);
        return;
    }
    
    // Run the Web Server permanently as a heartbeat throughout the Application's lifetime locally unless shutdown
    while (!g_SignalWebShutdown) {
        mg_mgr_poll(&mgr, 1); // 1ms tick: drain audio queue with near-zero latency
    }
    
    mg_mgr_free(&mgr);
    g_WebServerThreadSpawned = false;
}