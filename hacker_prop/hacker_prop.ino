/*
 * DARKNET TERMINAL — movie-hacker prop for ESP32-S3-Touch-LCD-1.69
 * Waveshare board: ST7789V2 240x280, CST816T touch, buzzer, battery
 */

#include <LovyanGFX.hpp>
#include <Wire.h>

// ── Display config (Waveshare ESP32-S3-Touch-LCD-1.69) ──────────
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;
    lgfx::Touch_CST816S _touch;
public:
    LGFX(void) {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000UL;
            cfg.freq_read = 16000000UL;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 6;
            cfg.pin_mosi = 7;
            cfg.pin_miso = -1;
            cfg.pin_dc = 4;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = 5;
            cfg.pin_rst = 8;
            cfg.pin_busy = -1;
            cfg.panel_width = 240;
            cfg.panel_height = 280;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 20;
            cfg.offset_rotation = 0;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.readable = false;
            cfg.bus_shared = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl = 15;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        {
            auto cfg = _touch.config();
            cfg.i2c_port = 0;
            cfg.i2c_addr = 0x15;
            cfg.pin_sda = 11;
            cfg.pin_scl = 10;
            cfg.pin_int = 14;
            cfg.pin_rst = 13;
            cfg.freq = 400000;
            cfg.x_min = 0;
            cfg.x_max = 239;
            cfg.y_min = 0;
            cfg.y_max = 279;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }
        setPanel(&_panel);
    }
};

LGFX lcd;

#define BUZZER_PIN 33
#define W 240
#define H 280
#define FONT_H 10
#define FONT_W 6
#define MAX_LINES (H / FONT_H)
#define MAX_COLS  (W / FONT_W)

enum Screen { BOOT_SEQ, MAIN_TERMINAL, TARGET_SELECT, ATTACK_SELECT, ATTACKING, BREACH };
Screen currentScreen = BOOT_SEQ;

// ── Fake data pools ─────────────────────────────────────────────
const char* ipTargets[] = {
    "192.168.1.1", "10.0.0.137", "172.16.42.88", "203.0.113.66",
    "198.51.100.23", "185.220.101.42", "91.219.237.10", "45.33.32.156",
    "104.16.249.249", "8.8.8.8", "1.1.1.1", "208.67.222.222",
    "159.203.176.91", "64.225.89.12", "142.250.80.46"
};
const char* hostnames[] = {
    "darknet-node-7.onion", "shadow-relay-03.tor",
    "vault.megacorp.internal", "gibson.mainframe.mil",
    "core-switch-alpha.nsa.ic", "blackice-fw.zaibatsu.jp",
    "honeypot-3.interpol.int", "proxy.silk-route.io",
    "backdoor.pentagon.smil", "zero-day-cache.fsb.ru",
    "sentinel.gchq.uk.gov", "neuromancer.sprawl.net"
};
const char* malwareNames[] = {
    "Stuxnet v3.7", "WannaCry-2.0", "NotPetya-RX", "EternalBlue-NG",
    "Pegasus-STEALTH", "Flame-REBORN", "DarkSide-4.1", "BlackEnergy-X",
    "Duqu-PHANTOM", "Shamoon-WIPER", "Triton-ICS", "SolarWinds-B",
    "Emotet-HYDRA", "TrickBot-MK4", "Cobalt-Strike-5", "Mimikatz-ZX"
};
const char* attackModes[] = {
    "BUFFER OVERFLOW", "SQL INJECTION", "ZERO-DAY EXPLOIT",
    "PRIVILEGE ESCALATION", "KERNEL ROOTKIT", "ARP POISONING",
    "DNS TUNNELING", "REVERSE SHELL", "FIRMWARE IMPLANT",
    "SIDE-CHANNEL ATTACK", "ROP CHAIN DEPLOY", "HEAP SPRAY"
};

// ── Procedural generation helpers ───────────────────────────────
// Two IP buffers so we can use two IPs in one printf
char _ip1[16], _ip2[16];
void makeIP(char* buf) {
    snprintf(buf, 16, "%d.%d.%d.%d",
        random(1,255), random(0,256), random(0,256), random(1,255));
}

int randPort() {
    const int p[] = {21,22,23,25,53,80,110,135,139,443,445,993,1433,1723,3306,3389,5432,5900,8080,8443,8888,9090,27017};
    return p[random(23)];
}

// random hex word
uint32_t rh() { return (uint32_t)esp_random(); }

// short random register names
const char* randReg() {
    const char* r[] = {"rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp","r8","r9","r10","r11","r12","r13","r14","r15","eax","ebx","ecx","edx","esi","edi"};
    return r[random(22)];
}
const char* randUser() {
    const char* u[] = {"root","admin","user","oracle","postgres","www-data","nobody","test","backup","mysql","git","nginx","deploy","ubuntu","centos","pi","ftpuser","daemon","syslog","operator"};
    return u[random(20)];
}
const char* randSvc() {
    const char* s[] = {"ssh","http","https","ftp","telnet","smb","mysql","rdp","vnc","smtp","pop3","dns","mssql","mongodb","proxy","redis","docker","kube","ldap","nfs"};
    return s[random(20)];
}
const char* randPath() {
    const char* p[] = {"/etc/shadow","/etc/passwd","/var/log/auth","/root/.ssh/id_rsa","/home/admin/.bash_history","/proc/self/mem","/dev/kmem","/tmp/.X11","/var/spool/cron","/opt/app/config.yml","/srv/db/creds.db","/boot/vmlinuz","/sys/kernel/core","/run/secrets/token"};
    return p[random(14)];
}
const char* randProc() {
    const char* p[] = {"sshd","httpd","mysqld","nginx","firewalld","snort","syslogd","crond","dockerd","kubelet","postgres","named","sendmail","vsftpd","ntpd","rsyslogd"};
    return p[random(16)];
}
const char* randProto() {
    const char* p[] = {"TCP","UDP","ICMP","SYN","ACK","RST","FIN","PSH"};
    return p[random(8)];
}

// ── Progress bar state ──────────────────────────────────────────
struct ProgressBar {
    bool active;
    int x, y, w;
    float progress;
    float speed;
    uint32_t color;
    char label[24];
    uint32_t startTime;
};
#define MAX_BARS 5
ProgressBar bars[MAX_BARS];

// ── Terminal state ──────────────────────────────────────────────
int cursorLine = 0;
uint32_t lastLineTime = 0;
bool screenFilled = false;
uint32_t speedChangeTime = 0;
int currentDelay = 30;
int burstRemaining = 0;

// ── Big message overlay state ───────────────────────────────────
bool msgActive = false;
uint32_t msgStartTime = 0;
uint32_t msgDuration = 0;
int msgY = 0;
uint32_t lastBigMsg = 0;
uint32_t nextBigMsgInterval = 5000;

// ── Selected target/attack ──────────────────────────────────────
int selectedTarget = -1;
int selectedAttack = -1;
char targetIP[20];
char targetHost[40];
char attackName[24];
char malwareName[24];
uint32_t attackStartTime = 0;
int attackPhase = 0;

// ── Utility ─────────────────────────────────────────────────────
void buzz(int freq, int ms) {
    tone(BUZZER_PIN, freq, ms);
}

uint16_t dimGreen(uint8_t brightness) {
    return lcd.color565(0, brightness, 0);
}
uint16_t hackerGreen() { return lcd.color565(0, 255, 0); }
uint16_t darkGreen()   { return lcd.color565(0, 100, 0); }
uint16_t termRed()     { return lcd.color565(255, 40, 40); }
uint16_t termYellow()  { return lcd.color565(255, 255, 0); }
uint16_t termCyan()    { return lcd.color565(0, 255, 255); }

// ── Boot sequence ───────────────────────────────────────────────
void drawBootSequence() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(1);

    const char* bootLines[] = {
        "BIOS v4.7.2 DARKNET SYSTEMS INC.",
        "CPU: AMD EPYC 9684X @ 3.8GHz",
        "RAM: 1024 GB DDR5-6400 ECC",
        "GPU: 8x NVIDIA H100 80GB",
        "",
        "Initializing secure enclave...",
        "Loading kernel modules...",
        "  [OK] crypto_engine",
        "  [OK] stealth_net",
        "  [OK] zero_trace",
        "  [OK] exploit_framework v7.2",
        "  [OK] payload_generator",
        "  [OK] proxy_chain (14 nodes)",
        "",
        "Connecting to darknet...",
        "  Route: TOR -> I2P -> MESH",
        "  Latency: 847ms (14 hops)",
        "  Encryption: AES-256-GCM",
        "",
        "[SYSTEM ONLINE]",
    };
    int nLines = sizeof(bootLines) / sizeof(bootLines[0]);

    for (int i = 0; i < nLines; i++) {
        int y = i * FONT_H + 2;
        if (y + FONT_H > H) break;

        const char* line = bootLines[i];
        if (strstr(line, "[OK]"))
            lcd.setTextColor(hackerGreen(), TFT_BLACK);
        else if (strstr(line, "[SYSTEM"))
            lcd.setTextColor(termCyan(), TFT_BLACK);
        else if (strstr(line, "Route:") || strstr(line, "Latency") || strstr(line, "Encryption"))
            lcd.setTextColor(termYellow(), TFT_BLACK);
        else
            lcd.setTextColor(darkGreen(), TFT_BLACK);

        lcd.setCursor(2, y);
        for (int c = 0; line[c]; c++) {
            lcd.print(line[c]);
            if (line[c] != ' ') delay(5 + random(8));
        }

        if (strstr(line, "[OK]")) buzz(2000, 15);
        if (strstr(line, "[SYSTEM")) {
            buzz(1500, 100); delay(200);
            buzz(2000, 100); delay(200);
            buzz(2500, 200);
        }
    }

    delay(800);
    currentScreen = MAIN_TERMINAL;
    lcd.fillScreen(TFT_BLACK);
    fillScreen();
}

// ── Fill the entire screen with text lines ──────────────────────
void fillScreen() {
    lcd.fillScreen(TFT_BLACK);
    for (int row = 0; row < MAX_LINES; row++) {
        drawTerminalLine(row * FONT_H);
    }
    cursorLine = 0;
    screenFilled = true;
}

// ── Draw one terminal line — fully procedural, never repeats ────
void drawTerminalLine(int y) {
    int r = random(1000);
    lcd.setCursor(2, y);

    if (r < 20) {
        // red alert — unique each time with random IPs/PIDs
        lcd.setTextColor(termRed(), TFT_BLACK);
        makeIP(_ip1);
        int v = random(8);
        if (v == 0) lcd.printf("[!] IDS ALERT %s:%d", _ip1, randPort());
        else if (v == 1) lcd.printf("[X] DROPPED %s > %s", _ip1, (makeIP(_ip2), _ip2));
        else if (v == 2) lcd.printf("[!] TRACE pid=%d FROM %s", random(1000,65535), _ip1);
        else if (v == 3) lcd.printf("[!] FIREWALL BLOCK %s", _ip1);
        else if (v == 4) lcd.printf("[!] INTRUSION %s port %d", _ip1, randPort());
        else if (v == 5) lcd.printf("SEGFAULT at 0x%08lX", rh());
        else if (v == 6) lcd.printf("[X] AUTH FAIL %s@%s", randUser(), _ip1);
        else lcd.printf("[!] OVERFLOW 0x%04lX +%d", rh()&0xFFFF, random(64,4096));
    }
    else if (r < 40) {
        // cyan success
        lcd.setTextColor(termCyan(), TFT_BLACK);
        makeIP(_ip1);
        int v = random(8);
        if (v == 0) lcd.printf("[+] ROOT SHELL %s:%d", _ip1, randPort());
        else if (v == 1) lcd.printf("[+] CREDS DUMPED %s", _ip1);
        else if (v == 2) lcd.printf("[+] PIVOT %s -> %s", _ip1, (makeIP(_ip2), _ip2));
        else if (v == 3) lcd.printf("[+] HASH CRACKED uid=%d", random(500,65535));
        else if (v == 4) lcd.printf("[+] TUNNEL %s:%d OPEN", _ip1, randPort());
        else if (v == 5) lcd.printf("[+] EXFIL %dKB via %s", random(12,9999), _ip1);
        else if (v == 6) lcd.printf("[+] PAYLOAD STAGED %s", _ip1);
        else lcd.printf("[+] %s EXPLOITED", _ip1);
        buzz(1800, 8);
    }
    else if (r < 120) {
        // nmap port scan — always unique IP+port combo
        lcd.setTextColor(dimGreen(100 + random(100)), TFT_BLACK);
        makeIP(_ip1);
        int p = randPort();
        int v = random(5);
        if (v == 0) lcd.printf("Nmap: %s:%d %s OPEN", _ip1, p, randSvc());
        else if (v == 1) lcd.printf("%s  %d/tcp open %s", _ip1, p, randSvc());
        else if (v == 2) lcd.printf("scan %s [%d/%d] %d open", _ip1, random(1,65535), 65535, random(1,24));
        else if (v == 3) lcd.printf("%s:%d FILTERED (%s)", _ip1, p, randProto());
        else lcd.printf("PORT %5d %s  %s", p, random(2)?"open":"closed", randSvc());
    }
    else if (r < 200) {
        // ssh/login attempts
        lcd.setTextColor(dimGreen(130 + random(80)), TFT_BLACK);
        makeIP(_ip1);
        int v = random(5);
        if (v == 0) lcd.printf("ssh %s@%s -p %d", randUser(), _ip1, random(2)?22:2222);
        else if (v == 1) lcd.printf("login: %s pass: ****%04x", randUser(), random(0xFFFF));
        else if (v == 2) lcd.printf("AUTH %s@%s [ATTEMPT %d]", randUser(), _ip1, random(1,999));
        else if (v == 3) lcd.printf("telnet %s %d...", _ip1, random(2)?23:8023);
        else lcd.printf("rdp %s\\%s -> %s", randUser(), randUser(), _ip1);
    }
    else if (r < 300) {
        // hex dump — procedurally generated, never same
        lcd.setTextColor(dimGreen(90 + random(70)), TFT_BLACK);
        lcd.printf("%04lX: ", rh() & 0xFFFF);
        for (int j = 0; j < 8; j++) lcd.printf("%02lX ", rh() & 0xFF);
    }
    else if (r < 380) {
        // packet capture with full detail
        lcd.setTextColor(dimGreen(120 + random(60)), TFT_BLACK);
        makeIP(_ip1); makeIP(_ip2);
        int v = random(5);
        if (v == 0) lcd.printf("%s:%d > %s:%d [%s]", _ip1, randPort(), _ip2, randPort(), randProto());
        else if (v == 1) lcd.printf("%s %s > %s len=%d", randProto(), _ip1, _ip2, random(40,1500));
        else if (v == 2) lcd.printf("tcpdump: %s.%d > %s.%d", _ip1, randPort(), _ip2, randPort());
        else if (v == 3) lcd.printf("%s seq=%lu win=%d", _ip1, rh(), random(1024,65535));
        else lcd.printf("ARP who-has %s tell %s", _ip1, _ip2);
    }
    else if (r < 440) {
        // assembly — randomized registers and addresses
        lcd.setTextColor(dimGreen(140 + random(80)), TFT_BLACK);
        int v = random(10);
        if (v == 0) lcd.printf("mov %s, [%s-0x%02x]", randReg(), randReg(), random(8,128));
        else if (v == 1) lcd.printf("xor %s, %s", randReg(), randReg());
        else if (v == 2) lcd.printf("call 0x%08lX", rh());
        else if (v == 3) lcd.printf("jmp loc_%06lX", rh() & 0xFFFFFF);
        else if (v == 4) lcd.printf("cmp %s, 0x%04lX", randReg(), rh() & 0xFFFF);
        else if (v == 5) lcd.printf("lea %s, [rip+0x%04lx]", randReg(), rh() & 0xFFFF);
        else if (v == 6) lcd.printf("push %s; sub rsp, 0x%02x", randReg(), random(16,256));
        else if (v == 7) lcd.printf("test %s, %s; jnz 0x%04lX", randReg(), randReg(), rh()&0xFFFF);
        else if (v == 8) lcd.printf("shr %s, 0x%02x", randReg(), random(1,32));
        else lcd.printf("and %s, 0x%08lX", randReg(), rh());
    }
    else if (r < 500) {
        // memory addresses / pointers
        lcd.setTextColor(termYellow(), TFT_BLACK);
        int v = random(4);
        if (v == 0) lcd.printf("0x%08lX -> 0x%08lX", rh(), rh());
        else if (v == 1) lcd.printf("[%08lX] = %08lX (%d)", rh(), rh(), random(-9999,9999));
        else if (v == 2) lcd.printf("alloc 0x%06lX sz=%d", rh()&0xFFFFFF, random(64,65536));
        else lcd.printf("free(0x%08lX) %d bytes", rh(), random(16,8192));
    }
    else if (r < 560) {
        // password/hash cracking
        lcd.setTextColor(dimGreen(160 + random(60)), TFT_BLACK);
        int v = random(5);
        if (v == 0) lcd.printf("$6$%08lx$...CRACKED", rh());
        else if (v == 1) lcd.printf("NTLM:%08lx%04lx MATCH", rh(), rh()&0xFFFF);
        else if (v == 2) lcd.printf("hashcat #%d: %08lx OK", random(1,9999), rh());
        else if (v == 3) lcd.printf("john: %s:%08lx FOUND", randUser(), rh());
        else lcd.printf("bcrypt r=%d %08lx...done", random(8,14), rh());
    }
    else if (r < 620) {
        // file ops with random paths
        lcd.setTextColor(dimGreen(150 + random(60)), TFT_BLACK);
        makeIP(_ip1);
        int v = random(6);
        if (v == 0) lcd.printf("cat %s >> /tmp/dump", randPath());
        else if (v == 1) lcd.printf("scp %s %s:/loot", randPath(), _ip1);
        else if (v == 2) lcd.printf("wget %s:%d/stager.sh", _ip1, randPort());
        else if (v == 3) lcd.printf("curl -s %s/shell.php", _ip1);
        else if (v == 4) lcd.printf("rm -rf /var/log/auth*");
        else lcd.printf("tar czf x.tgz %s", randPath());
    }
    else if (r < 680) {
        // process ops with random PIDs
        lcd.setTextColor(termRed(), TFT_BLACK);
        int pid = random(1000, 65535);
        int v = random(5);
        if (v == 0) lcd.printf("kill -9 %d (%s)", pid, randProc());
        else if (v == 1) lcd.printf("inject pid=%d %s", pid, randProc());
        else if (v == 2) lcd.printf("ptrace(%d) attached", pid);
        else if (v == 3) lcd.printf("hook %s at 0x%08lX", randProc(), rh());
        else lcd.printf("ps: %d %s %dMB", pid, randProc(), random(1,4096));
    }
    else if (r < 740) {
        // metasploit / exploit framework
        lcd.setTextColor(dimGreen(170 + random(50)), TFT_BLACK);
        makeIP(_ip1);
        int v = random(6);
        if (v == 0) lcd.printf("msf6> set RHOST %s", _ip1);
        else if (v == 1) lcd.printf("msf6> exploit -j [%d]", random(1,50));
        else if (v == 2) lcd.printf("[*] Meterpreter %s:%d", _ip1, randPort());
        else if (v == 3) lcd.printf("[*] Sending stage %dB", random(50000,200000));
        else if (v == 4) lcd.printf("msf6> sessions -i %d", random(1,20));
        else lcd.printf("[*] Handler %s:%d", _ip1, random(4440,4450));
    }
    else if (r < 790) {
        // SQL injection
        lcd.setTextColor(dimGreen(140 + random(40)), TFT_BLACK);
        int v = random(5);
        if (v == 0) lcd.printf("' OR 1=1;--uid=%d", random(1,9999));
        else if (v == 1) lcd.printf("UNION SELECT %d,%d,%d--", random(1,9), random(1,9), random(1,9));
        else if (v == 2) lcd.printf("sqlmap -u %s --dbs", (makeIP(_ip1), _ip1));
        else if (v == 3) lcd.printf("EXEC xp_cmdshell '%08lx'", rh());
        else lcd.printf("INSERT INTO log VALUES(%ld", rh());
    }
    else if (r < 840) {
        // crypto / TLS
        lcd.setTextColor(dimGreen(130 + random(60)), TFT_BLACK);
        int v = random(5);
        if (v == 0) lcd.printf("TLS %s:%d HANDSHAKE OK", (makeIP(_ip1), _ip1), randPort());
        else if (v == 1) lcd.printf("AES-256-GCM iv=%08lx", rh());
        else if (v == 2) lcd.printf("X25519 pub=%08lx%04lx", rh(), rh()&0xFFFF);
        else if (v == 3) lcd.printf("CERT CN=%s SPOOFED", (makeIP(_ip1), _ip1));
        else lcd.printf("HMAC-SHA384 %08lx VALID", rh());
    }
    else if (r < 890) {
        // network routing / interface
        lcd.setTextColor(dimGreen(110 + random(50)), TFT_BLACK);
        makeIP(_ip1); makeIP(_ip2);
        int v = random(4);
        if (v == 0) lcd.printf("eth%d: %s MTU 1500 UP", random(8), _ip1);
        else if (v == 1) lcd.printf("route add %s gw %s", _ip1, _ip2);
        else if (v == 2) lcd.printf("iptables -A INPUT -s %s", _ip1);
        else lcd.printf("tun0: %s <-> %s", _ip1, _ip2);
    }
    else if (r < 940) {
        // C exploit code with random values
        lcd.setTextColor(dimGreen(140 + random(80)), TFT_BLACK);
        int v = random(8);
        if (v == 0) lcd.printf("buf[%d]=shellcode[%d];", random(0,4096), random(0,4096));
        else if (v == 1) lcd.printf("memcpy(dst+0x%03x,src,%d)", random(0,4096), random(32,1024));
        else if (v == 2) lcd.printf("send(fd,payload,%d,0);", random(64,8192));
        else if (v == 3) lcd.printf("recv(fd,resp,%d,0);", random(64,8192));
        else if (v == 4) lcd.printf("mmap(0,%d,7,0x22,-1,0)", random(4096,65536));
        else if (v == 5) lcd.printf("for(i=0;i<%d;i++){", random(16,65536));
        else if (v == 6) lcd.printf("rc4_crypt(key,%d,buf);", random(128,4096));
        else lcd.printf("connect(fd,&sa,%d);", random(16,128));
    }
    else {
        // timestamps + misc system stuff
        lcd.setTextColor(dimGreen(100 + random(80)), TFT_BLACK);
        int v = random(5);
        if (v == 0) lcd.printf("%02d:%02d:%02d.%03d %s", random(24), random(60), random(60), random(1000), randProc());
        else if (v == 1) lcd.printf("[%ld] %s: %s", rh()%100000, randProc(), randSvc());
        else if (v == 2) lcd.printf("syslog: %s[%d] %08lx", randProc(), random(100,65535), rh());
        else if (v == 3) lcd.printf("kern: %s fault at %08lX", randProc(), rh());
        else lcd.printf("audit: %s uid=%d exe=%s", randUser(), random(0,65535), randProc());
    }
}

// ── Progress bars ───────────────────────────────────────────────
void spawnProgressBar() {
    for (int i = 0; i < MAX_BARS; i++) {
        if (!bars[i].active) {
            bars[i].active = true;
            bars[i].x = 4;
            bars[i].y = random(3, MAX_LINES - 3) * FONT_H;
            bars[i].w = W - 8;
            bars[i].progress = 0;
            bars[i].speed = 0.5 + (random(100) / 100.0) * 3.0;
            bars[i].startTime = millis();

            const char* labels[] = {
                "DECRYPT", "UPLOAD", "INJECT", "COMPILE",
                "EXTRACT", "CRACK", "BRUTE", "SCAN",
                "EXFIL", "PATCH", "DEPLOY", "ENCODE",
                "DUMP", "PWNED", "SNIFF", "PIVOT"
            };
            strncpy(bars[i].label, labels[random(16)], 23);

            uint16_t colors[] = {hackerGreen(), termCyan(), termYellow(), termRed()};
            bars[i].color = colors[random(4)];
            return;
        }
    }
}

void updateProgressBars() {
    for (int i = 0; i < MAX_BARS; i++) {
        if (!bars[i].active) continue;

        bars[i].progress += bars[i].speed;
        if (bars[i].progress >= 100.0) {
            bars[i].active = false;
            lcd.fillRect(bars[i].x - 2, bars[i].y - 2, bars[i].w + 4, FONT_H + 8, TFT_BLACK);
            buzz(2200, 15);
            continue;
        }

        int barY = bars[i].y;
        int barW = bars[i].w;
        int filled = (int)(barW * bars[i].progress / 100.0);

        lcd.fillRect(bars[i].x, barY, barW, 2, lcd.color565(30, 30, 30));
        lcd.fillRect(bars[i].x, barY, filled, 2, bars[i].color);

        lcd.setTextColor(bars[i].color, TFT_BLACK);
        lcd.setCursor(bars[i].x, barY + 4);
        lcd.printf("%s %d%%  ", bars[i].label, (int)bars[i].progress);
    }
}

// ── Target selection screen ─────────────────────────────────────
void drawTargetSelect() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(1);

    lcd.setTextColor(termCyan(), TFT_BLACK);
    lcd.setCursor(2, 2);
    lcd.print("=== SELECT TARGET ===");
    lcd.drawFastHLine(0, 14, W, lcd.color565(0, 80, 80));

    for (int i = 0; i < 8; i++) {
        int y = 20 + i * 32;
        if (y + 28 > H) break;

        lcd.setTextColor(hackerGreen(), TFT_BLACK);
        lcd.setCursor(6, y);
        lcd.printf("[%d] %s", i + 1, ipTargets[random(15)]);

        lcd.setTextColor(darkGreen(), TFT_BLACK);
        lcd.setCursor(12, y + 12);
        lcd.print(hostnames[random(12)]);

        lcd.drawFastHLine(4, y + 26, W - 8, lcd.color565(0, 40, 0));
    }

    lcd.setTextColor(termYellow(), TFT_BLACK);
    lcd.setCursor(2, H - 14);
    lcd.print("TAP TARGET TO SELECT");
}

// ── Attack mode selection ───────────────────────────────────────
void drawAttackSelect() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(1);

    lcd.setTextColor(termRed(), TFT_BLACK);
    lcd.setCursor(2, 2);
    lcd.printf("TARGET: %s", targetIP);
    lcd.setTextColor(darkGreen(), TFT_BLACK);
    lcd.setCursor(2, 14);
    lcd.print(targetHost);

    lcd.drawFastHLine(0, 26, W, lcd.color565(80, 0, 0));

    lcd.setTextColor(termCyan(), TFT_BLACK);
    lcd.setCursor(2, 32);
    lcd.print("=== SELECT ATTACK ===");

    for (int i = 0; i < 8; i++) {
        int y = 48 + i * 28;
        if (y + 24 > H) break;

        lcd.setTextColor(termRed(), TFT_BLACK);
        lcd.setCursor(6, y);
        lcd.printf("[%d] %s", i + 1, malwareNames[random(16)]);

        lcd.setTextColor(dimGreen(100), TFT_BLACK);
        lcd.setCursor(12, y + 12);
        lcd.print(attackModes[random(12)]);

        lcd.drawFastHLine(4, y + 22, W - 8, lcd.color565(40, 0, 0));
    }

    lcd.setTextColor(termYellow(), TFT_BLACK);
    lcd.setCursor(2, H - 14);
    lcd.print("TAP ATTACK TO DEPLOY");
}

// ── Attacking animation ─────────────────────────────────────────
void drawAttacking() {
    lcd.fillScreen(TFT_BLACK);
    attackStartTime = millis();
    attackPhase = 0;
}

void updateAttacking() {
    uint32_t elapsed = millis() - attackStartTime;

    if (attackPhase == 0) {
        lcd.setTextColor(termRed(), TFT_BLACK);
        lcd.setTextSize(1);
        lcd.setCursor(2, 2);
        lcd.printf("DEPLOYING: %s", malwareName);
        lcd.setCursor(2, 14);
        lcd.printf("TARGET: %s", targetIP);
        lcd.setCursor(2, 26);
        lcd.printf("METHOD: %s", attackName);
        lcd.drawFastHLine(0, 38, W, termRed());
        attackPhase = 1;
    }

    // blast lines fast
    static uint32_t lastAttackLine = 0;
    if (millis() - lastAttackLine > 15) {
        lastAttackLine = millis();
        for (int n = 0; n < 2; n++) {
            int y = 44 + random(18) * FONT_H;
            if (y < H - 30) {
                lcd.fillRect(0, y, W, FONT_H, TFT_BLACK);
                drawTerminalLine(y);
            }
        }
    }

    float mainProg = min(100.0f, elapsed / 80.0f);
    int barY = H - 24;
    int filled = (int)((W - 8) * mainProg / 100.0);
    lcd.fillRect(4, barY, W - 8, 4, lcd.color565(30, 30, 30));

    uint16_t barColor = mainProg < 60 ? termYellow() : (mainProg < 90 ? termCyan() : hackerGreen());
    lcd.fillRect(4, barY, filled, 4, barColor);

    lcd.setTextColor(barColor, TFT_BLACK);
    lcd.setCursor(4, barY + 8);
    lcd.printf("PROGRESS: %d%%  ", (int)mainProg);

    if (mainProg >= 100) {
        buzz(800, 100); delay(100);
        buzz(1200, 100); delay(100);
        buzz(1600, 200);
        currentScreen = BREACH;
        drawBreach();
    }
}

// ── Breach screen ───────────────────────────────────────────────
void drawBreach() {
    for (int i = 0; i < 3; i++) {
        lcd.fillScreen(hackerGreen());
        buzz(3000, 50);
        delay(80);
        lcd.fillScreen(TFT_BLACK);
        delay(80);
    }

    lcd.fillScreen(TFT_BLACK);

    lcd.setTextColor(termRed(), TFT_BLACK);
    lcd.setTextSize(1);
    const char* skull[] = {
        "     .---.     ",
        "    / o o \\    ",
        "   |   ^   |   ",
        "    \\ --- /    ",
        "     '---'     ",
    };
    for (int i = 0; i < 5; i++) {
        lcd.setCursor(50, 30 + i * FONT_H);
        lcd.print(skull[i]);
    }

    lcd.setTextSize(2);
    lcd.setTextColor(termRed(), TFT_BLACK);
    lcd.setCursor(24, 100);
    lcd.print("BREACHED");

    lcd.setTextSize(1);
    lcd.setTextColor(hackerGreen(), TFT_BLACK);
    lcd.setCursor(2, 130);
    lcd.printf("Host: %s", targetHost);
    lcd.setCursor(2, 144);
    lcd.printf("IP:   %s", targetIP);
    lcd.setCursor(2, 158);
    lcd.printf("Via:  %s", malwareName);
    lcd.setCursor(2, 172);
    lcd.printf("Mode: %s", attackName);

    lcd.setTextColor(termCyan(), TFT_BLACK);
    lcd.setCursor(2, 196);
    lcd.print("Root shell established.");
    lcd.setCursor(2, 210);
    lcd.print("All defenses bypassed.");
    lcd.setCursor(2, 224);
    lcd.print("Data exfiltration ready.");

    lcd.setTextColor(termYellow(), TFT_BLACK);
    lcd.setCursor(2, 250);
    lcd.print("[TAP] New target");
    lcd.setCursor(2, 264);
    lcd.print("[HOLD] Return to terminal");

    buzz(500, 300);
}

// ── Touch handling ──────────────────────────────────────────────
void handleTouch() {
    lgfx::touch_point_t tp;
    int count = lcd.getTouchRaw(&tp, 1);
    if (count == 0) return;

    int ty = tp.y;
    buzz(1500, 10);

    switch (currentScreen) {
        case MAIN_TERMINAL:
            if (ty < H / 3) {
                currentScreen = TARGET_SELECT;
                drawTargetSelect();
            } else if (ty < H * 2 / 3) {
                // tap middle = big burst
                burstRemaining = 8 + random(16);
                currentDelay = 2;
                speedChangeTime = millis() + 50;
            } else {
                spawnProgressBar();
            }
            break;
        case TARGET_SELECT: {
            int idx = (ty - 20) / 32;
            if (idx >= 0 && idx < 8) {
                strncpy(targetIP, ipTargets[random(15)], 19);
                strncpy(targetHost, hostnames[random(12)], 39);
                selectedTarget = idx;
                buzz(2000, 30);
                delay(200);
                currentScreen = ATTACK_SELECT;
                drawAttackSelect();
            }
            break;
        }
        case ATTACK_SELECT: {
            int idx = (ty - 48) / 28;
            if (idx >= 0 && idx < 8) {
                strncpy(malwareName, malwareNames[random(16)], 23);
                strncpy(attackName, attackModes[random(12)], 23);
                selectedAttack = idx;
                buzz(1000, 50); delay(100);
                buzz(2000, 50); delay(200);
                currentScreen = ATTACKING;
                drawAttacking();
            }
            break;
        }
        case BREACH: {
            delay(300);
            int still = lcd.getTouchRaw(&tp, 1);
            if (still > 0) {
                currentScreen = MAIN_TERMINAL;
                screenFilled = false;
            } else {
                currentScreen = TARGET_SELECT;
                drawTargetSelect();
            }
            break;
        }
        case ATTACKING:
            spawnProgressBar();
            break;
        default:
            break;
    }
    delay(200);
}

// ── Main ────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    pinMode(BUZZER_PIN, OUTPUT);

    lcd.init();
    lcd.setBrightness(200);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(1);

    randomSeed(esp_random());
    memset(bars, 0, sizeof(bars));

    drawBootSequence();
}

void loop() {
    handleTouch();

    switch (currentScreen) {
        case MAIN_TERMINAL: {
            if (!screenFilled) fillScreen();

            uint32_t now = millis();

            // pick new speed when timer expires
            if (now > speedChangeTime) {
                int pick = random(100);
                if (pick < 25) {
                    // burst — overwrite many lines instantly
                    burstRemaining = 4 + random(14);
                    currentDelay = 2;
                    speedChangeTime = now + 50;
                } else if (pick < 55) {
                    // fast
                    currentDelay = 10 + random(20);
                    speedChangeTime = now + 300 + random(800);
                } else if (pick < 85) {
                    // normal — readable
                    currentDelay = 40 + random(60);
                    speedChangeTime = now + 400 + random(1200);
                } else {
                    // brief pause
                    currentDelay = 200 + random(500);
                    speedChangeTime = now + currentDelay + 50;
                }
            }

            if (now - lastLineTime > (uint32_t)currentDelay) {
                lastLineTime = now;

                int y = cursorLine * FONT_H;
                // skip lines that overlap the message box
                if (!(msgActive && y >= msgY - 4 && y <= msgY + FONT_H * 5 + 4)) {
                    lcd.fillRect(0, y, W, FONT_H, TFT_BLACK);
                    drawTerminalLine(y);
                }
                cursorLine = (cursorLine + 1) % MAX_LINES;

                if (burstRemaining > 0) {
                    burstRemaining--;
                    if (burstRemaining == 0) {
                        currentDelay = 20 + random(40);
                        speedChangeTime = millis() + 200 + random(600);
                    }
                }
            }

            if (random(400) < 3) spawnProgressBar();

            // clear expired message overlay
            if (msgActive && now - msgStartTime > msgDuration) {
                msgActive = false;
                lcd.fillRect(0, msgY - 4, W, FONT_H * 5 + 8, TFT_BLACK);
                for (int row = (msgY - 4) / FONT_H; row <= (msgY + FONT_H * 5 + 4) / FONT_H && row < MAX_LINES; row++) {
                    if (row >= 0) drawTerminalLine(row * FONT_H);
                }
            }

            // spawn new message overlay (non-blocking)
            if (!msgActive && now - lastBigMsg > nextBigMsgInterval) {
                lastBigMsg = now;
                nextBigMsgInterval = 3000 + random(27000);
                msgActive = true;
                msgStartTime = now;
                msgDuration = 1500 + random(2500);
                msgY = (H / 2) - FONT_H * 2;

                lcd.fillRect(0, msgY - 4, W, FONT_H * 5 + 8, TFT_BLACK);
                lcd.drawRect(2, msgY - 2, W - 4, FONT_H * 5 + 4, dimGreen(60));

                makeIP(_ip1);
                int v = random(30);

                switch (v) {
                case 0: case 1: case 2: case 3: case 4: {
                    lcd.setTextSize(2);
                    lcd.setTextColor(hackerGreen(), TFT_BLACK);
                    const char* w[] = {"SUCCESS!","PWNED!","OWNED!","HACKED!","GOT ROOT!"};
                    lcd.setCursor((W - strlen(w[v])*12)/2, msgY+4);
                    lcd.print(w[v]);
                    lcd.setTextSize(1); lcd.setTextColor(dimGreen(160), TFT_BLACK);
                    lcd.setCursor(8, msgY+28); lcd.printf("Target %s compromised", _ip1);
                    buzz(2000, 60);
                    break;
                }
                case 5: case 6: case 7: {
                    lcd.setTextSize(2);
                    lcd.setTextColor(termRed(), TFT_BLACK);
                    const char* f[] = {"FAILED!","BLOCKED!","DENIED!"};
                    lcd.setCursor((W - strlen(f[v-5])*12)/2, msgY+4);
                    lcd.print(f[v-5]);
                    lcd.setTextSize(1); lcd.setTextColor(dimGreen(120), TFT_BLACK);
                    lcd.setCursor(8, msgY+28); lcd.printf("Host %s fought back", _ip1);
                    buzz(300, 80);
                    msgDuration = 1000 + random(1500);
                    break;
                }
                case 8: case 9:
                    lcd.setTextSize(1); lcd.setTextColor(termCyan(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.printf(">>> %d.%dTB EXFILTRATED <<<", random(1,999), random(1,99));
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Source: %s", _ip1);
                    lcd.setCursor(8, msgY+30); lcd.printf("Files: %d classified docs", random(100,50000));
                    buzz(1800, 30);
                    break;
                case 10: case 11:
                    lcd.setTextSize(1); lcd.setTextColor(termYellow(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.printf("$$$ %d BTC TRANSFERRED $$$", random(2,500));
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Wallet: %08lx...%04lx", rh(), rh()&0xFFFF);
                    lcd.setCursor(8, msgY+30); lcd.printf("Value: $%d,%03d,%03d USD", random(1,99), random(100,999), random(100,999));
                    buzz(2500, 40);
                    break;
                case 12:
                    lcd.setTextSize(1); lcd.setTextColor(hackerGreen(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.printf("[+] %d PASSWORDS CRACKED", random(500,99999));
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Accounts: %d compromised", random(100,25000));
                    lcd.setCursor(8, msgY+30); lcd.printf("Admin creds: %d found", random(1,50));
                    buzz(1800, 30);
                    break;
                case 13:
                    lcd.setTextSize(1); lcd.setTextColor(termCyan(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] NETWORK TAKEOVER");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Nodes owned: %d/%d", random(50,254), 254);
                    lcd.setCursor(8, msgY+30); lcd.printf("Gateway: %s hijacked", _ip1);
                    buzz(2000, 30);
                    break;
                case 14:
                    lcd.setTextSize(1); lcd.setTextColor(termYellow(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print(">>> DATABASE DUMPED <<<");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Tables: %d  Rows: %dM", random(20,500), random(1,999));
                    lcd.setCursor(8, msgY+30); lcd.printf("SSNs: %d  Cards: %d", random(10000,999999), random(1000,99999));
                    buzz(1500, 40);
                    break;
                case 15:
                    lcd.setTextSize(1); lcd.setTextColor(termRed(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[!] CCTV FEED HIJACKED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Cameras: %d streams live", random(4,128));
                    lcd.setCursor(8, msgY+30); lcd.printf("DVR: %s root access", _ip1);
                    buzz(1200, 40);
                    break;
                case 16:
                    lcd.setTextSize(1); lcd.setTextColor(termCyan(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("<<< SAT LINK ACQUIRED >>>");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Orbit: %dkm  Band: %cGHz", random(200,36000), "CXKS"[random(4)]);
                    lcd.setCursor(8, msgY+30); lcd.printf("Uplink: %dMbps encrypted", random(50,900));
                    buzz(2800, 30);
                    break;
                case 17:
                    lcd.setTextSize(1); lcd.setTextColor(hackerGreen(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] FIREWALL BYPASSED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Rules evaded: %d/%d", random(50,200), random(200,300));
                    lcd.setCursor(8, msgY+30); lcd.printf("Zero-days used: %d", random(1,5));
                    buzz(2000, 30);
                    break;
                case 18:
                    lcd.setTextSize(1); lcd.setTextColor(termRed(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[!] RANSOMWARE DEPLOYED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Encrypted: %d files", random(10000,999999));
                    lcd.setCursor(8, msgY+30); lcd.printf("Ransom: %d BTC demanded", random(5,200));
                    buzz(400, 60);
                    break;
                case 19:
                    lcd.setTextSize(1); lcd.setTextColor(hackerGreen(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] BACKDOOR INSTALLED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Persistence: kernel lvl %d", random(0,3));
                    lcd.setCursor(8, msgY+30); lcd.printf("Beacon: every %ds to C2", random(5,300));
                    buzz(1800, 30);
                    break;
                case 20:
                    lcd.setTextSize(1); lcd.setTextColor(termYellow(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print(">>> EMAILS INTERCEPTED <<<");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Inbox: %d messages copied", random(200,50000));
                    lcd.setCursor(8, msgY+30); lcd.printf("Attachments: %dGB saved", random(1,500));
                    buzz(2200, 30);
                    break;
                case 21:
                    lcd.setTextSize(1); lcd.setTextColor(termCyan(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] VPN TUNNEL HIJACKED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Users routed: %d", random(20,5000));
                    lcd.setCursor(8, msgY+30); lcd.printf("Traffic mirrored to %s", _ip1);
                    buzz(2000, 30);
                    break;
                case 22:
                    lcd.setTextSize(1); lcd.setTextColor(termRed(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[!] POWER GRID ACCESS");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Substations: %d/%d online", random(3,20), random(20,30));
                    lcd.setCursor(8, msgY+30); lcd.printf("SCADA node %s owned", _ip1);
                    buzz(500, 80);
                    break;
                case 23:
                    lcd.setTextSize(1); lcd.setTextColor(hackerGreen(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] DNS POISONED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Domains hijacked: %d", random(5,500));
                    lcd.setCursor(8, msgY+30); lcd.printf("Redirecting to %s", _ip1);
                    buzz(1600, 30);
                    break;
                case 24:
                    lcd.setTextSize(1); lcd.setTextColor(termYellow(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print(">>> CRYPTO WALLET DRAINED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("ETH: %d.%02d stolen", random(10,9999), random(0,99));
                    lcd.setCursor(8, msgY+30); lcd.printf("NFTs: %d transferred", random(1,200));
                    buzz(2500, 40);
                    break;
                case 25:
                    lcd.setTextSize(1); lcd.setTextColor(termCyan(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] BOTNET EXPANDED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("New zombies: %d devices", random(100,50000));
                    lcd.setCursor(8, msgY+30); lcd.printf("Total army: %dk bots", random(10,999));
                    buzz(1800, 30);
                    break;
                case 26:
                    lcd.setTextSize(1); lcd.setTextColor(termRed(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[!] AIR GAP BREACHED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Method: USB drop #%d", random(1,20));
                    lcd.setCursor(8, msgY+30); lcd.print("Classified net accessed");
                    buzz(600, 60);
                    break;
                case 27:
                    lcd.setTextSize(1); lcd.setTextColor(hackerGreen(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] KEYLOGGER ACTIVE");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Keystrokes: %d captured", random(5000,999999));
                    lcd.setCursor(8, msgY+30); lcd.printf("Credentials: %d logged", random(10,500));
                    buzz(1500, 30);
                    break;
                case 28:
                    lcd.setTextSize(1); lcd.setTextColor(termYellow(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print(">>> SUPPLY CHAIN PWNED <<<");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("Packages infected: %d", random(3,50));
                    lcd.setCursor(8, msgY+30); lcd.printf("Downstream targets: %dk", random(10,500));
                    buzz(2200, 40);
                    break;
                default:
                    lcd.setTextSize(1); lcd.setTextColor(termCyan(), TFT_BLACK);
                    lcd.setCursor(8, msgY+4); lcd.print("[+] 2FA BYPASSED");
                    lcd.setTextColor(dimGreen(140), TFT_BLACK);
                    lcd.setCursor(8, msgY+18); lcd.printf("SIM swapped: +1-%03d-%04d", random(200,999), random(1000,9999));
                    lcd.setCursor(8, msgY+30); lcd.printf("OTP intercepted x%d", random(2,20));
                    buzz(1800, 30);
                    break;
                }
                lcd.setTextSize(1);
            }

            updateProgressBars();
            break;
        }
        case ATTACKING:
            updateAttacking();
            break;
        default:
            break;
    }

    delay(5);
}
