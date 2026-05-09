#define DEMO_MODE 1

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <intrin.h>
#include "driver.h"
#include "overlay.h"
#include "r6_math.h"

// Mouse driver logic removed as per user request

namespace r6 {
    constexpr uint64_t TLS_INDEX_RVA        = 0x176A31C0;
    constexpr uint64_t GM_ENC1_RVA          = 0x175E3DF8;
    constexpr uint64_t GM_ENC2_RVA          = 0x175E3E00;
    constexpr uint64_t GM_KEY1              = 0x5BEC3C97D72A3EAULL;
    constexpr uint64_t GM_KEY2              = 0x6E33D2E38FF50F34ULL;
    constexpr uint64_t GM_VP_MATRIX         = 0x1E890;
    constexpr uint64_t GM_CAMERA_POS        = 0x1E900;
    constexpr uint64_t GM_PAWN_ARRAY_ENC    = 0x190;
    constexpr uint64_t GM_PAWN_COUNT_ENC    = 0x198;
    constexpr uint64_t PAWN_GLOBAL_KEY_RVA  = 0x175B31D0;
    constexpr uint64_t PAWN_GLOBAL_KEY_OFF  = 0xA8DD71C78A8613EAULL;
    constexpr uint64_t PAWN_COUNT_BIAS      = 0x4F5F27698F7B5B8BULL;
    constexpr uint64_t PAWN_XOR_BIAS        = 0xE2E9FBF8C13CDBEULL;
    constexpr uint64_t ENT_KEY1_RVA         = 0x175B78F0;
    constexpr uint64_t ENT_KEY1_OFF         = 0x7CB805C32EDE6530ULL;
    constexpr uint64_t ENT_KEY2_RVA         = 0x175B78F8;
    constexpr uint64_t ENT_KEY2_OFF         = 0x496BEDBDAEADE01CULL;
    constexpr uint64_t ENT_POSITION         = 0x50;
    constexpr uint64_t TLS_GM_SUB           = 0x2A8;
    constexpr uint64_t TLS_GM_XOR           = 0x270;
    constexpr uint64_t TLS_ENT_SUB          = 0x248;
    constexpr uint64_t TLS_PAWN_XOR         = 0xCB8;
}

c_driver driver;
std::vector<entity_t> cached_entities;
std::mutex entity_mutex;
std::atomic<bool> running{true};
std::atomic<uint64_t> g_tls_slot{0};

bool esp_boxes = true;
bool esp_snaplines = true;
float esp_distance = 500.0f;

void ApplyProfessionalTheme() {
    ImGuiStyle* style = &ImGui::GetStyle();
    style->WindowPadding = ImVec2(15, 15);
    style->WindowRounding = 8.0f;
    style->FramePadding = ImVec2(5, 5);
    style->FrameRounding = 4.0f;
    style->ItemSpacing = ImVec2(12, 8);
    style->ItemInnerSpacing = ImVec2(8, 6);
    style->IndentSpacing = 25.0f;
    style->ScrollbarSize = 15.0f;
    style->ScrollbarRounding = 9.0f;
    style->GrabMinSize = 5.0f;
    style->GrabRounding = 3.0f;

    style->Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.98f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.98f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.40f, 0.95f, 1.00f);
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.40f, 0.95f, 1.00f);
    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.50f, 1.00f, 1.00f);
    style->Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.22f, 1.00f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.22f, 1.00f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
}

uint64_t GetTlsSlot(uint64_t base, uint64_t teb) {
    uint32_t tls_index = driver.read<uint32_t>(base + r6::TLS_INDEX_RVA);
    uint64_t tls_array = driver.read<uint64_t>(teb + 0x58);
    return driver.read<uint64_t>(tls_array + tls_index * 8);
}

uint64_t DecryptGameManager(uint64_t base, uint64_t tls_slot) {
    uint64_t enc1 = driver.read<uint64_t>(base + r6::GM_ENC1_RVA);
    uint64_t enc2 = driver.read<uint64_t>(base + r6::GM_ENC2_RVA);
    enc2 += r6::GM_KEY2;
    enc2 = driver.read<uint64_t>(enc2);
    enc2 -= driver.read<uint64_t>(tls_slot + r6::TLS_GM_SUB);
    enc1 += r6::GM_KEY1;
    enc1 = driver.read<uint64_t>(enc1);
    enc1 = driver.read<uint64_t>(enc1);
    enc2 ^= enc1;
    uint64_t xor_key = driver.read<uint64_t>(tls_slot + r6::TLS_GM_XOR);
    return enc2 - xor_key;
}

struct PawnList { uint64_t arr; uint64_t cnt; };
PawnList DecryptPawnList(uint64_t base, uint64_t gm, uint64_t tls_slot) {
    uint64_t enc_array = driver.read<uint64_t>(gm + r6::GM_PAWN_ARRAY_ENC);
    uint64_t enc_count = driver.read<uint64_t>(gm + r6::GM_PAWN_COUNT_ENC);
    uint64_t gk = driver.read<uint64_t>(base + r6::PAWN_GLOBAL_KEY_RVA);
    gk = driver.read<uint64_t>(gk + r6::PAWN_GLOBAL_KEY_OFF);
    uint64_t arr = _rotl64(enc_array, 0x67 & 0x3F);
    uint64_t cnt = _rotl64(enc_count, 0x27);
    arr -= gk;
    cnt = cnt - gk + r6::PAWN_COUNT_BIAS;
    uint64_t xor_key = driver.read<uint64_t>(tls_slot + r6::TLS_PAWN_XOR);
    arr ^= xor_key;
    cnt ^= xor_key + r6::PAWN_XOR_BIAS;
    return { arr, cnt };
}

uint64_t DecryptEntity(uint64_t base, uint64_t pawn_ptr, uint64_t tls_slot) {
    uint64_t enc = driver.read<uint64_t>(pawn_ptr + 0x18);
    uint64_t gk1 = driver.read<uint64_t>(base + r6::ENT_KEY1_RVA);
    uint64_t xk = driver.read<uint64_t>(gk1 + r6::ENT_KEY1_OFF);
    xk = driver.read<uint64_t>(xk);
    enc = xk ^ enc;
    enc -= driver.read<uint64_t>(tls_slot + r6::TLS_ENT_SUB);
    uint64_t gk2 = driver.read<uint64_t>(base + r6::ENT_KEY2_RVA);
    gk2 = driver.read<uint64_t>(gk2 + r6::ENT_KEY2_OFF);
    uint64_t gk2v = driver.read<uint64_t>(gk2);
    return gk2v ^ enc;
}

vector2_t w2s_vp(vector3_t pos, const float vp[16], float screen_w, float screen_h) {
    float x = vp[0] * pos.x + vp[4] * pos.y + vp[8] * pos.z + vp[12];
    float y = vp[1] * pos.x + vp[5] * pos.y + vp[9] * pos.z + vp[13];
    float w = vp[3] * pos.x + vp[7] * pos.y + vp[11] * pos.z + vp[15];
    if (w < 0.001f) return {};
    float nx = x / w;
    float ny = y / w;
    return { (nx + 1.0f) * 0.5f * screen_w, (1.0f - ny) * 0.5f * screen_h };
}

void entity_cache_thread() {
    uint64_t base = driver.base();
    uint64_t tls_slot = 0;
    int pawn_fail_count = 0;

    auto rescan_tls = [&]() {
        std::cout << "[*] Scanning for valid TLS slot..." << std::endl;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32);
            if (Thread32First(snapshot, &te32)) {
                do {
                    if (te32.th32OwnerProcessID == driver.pid()) {
                        uint64_t teb = driver.get_thread_teb(te32.th32ThreadID);
                        if (teb) {
                            uint64_t slot = GetTlsSlot(base, teb);
                            uint64_t gm = DecryptGameManager(base, slot);
                            vector3_t cam = driver.read<vector3_t>(gm + r6::GM_CAMERA_POS);
                            if (cam.is_valid()) {
                                tls_slot = slot;
                                g_tls_slot.store(slot);
                                std::cout << "[+] Found valid TLS: 0x" << std::hex << slot << std::dec << " via TID: " << te32.th32ThreadID << std::endl;
                                break;
                            }
                        }
                    }
                } while (Thread32Next(snapshot, &te32));
            }
            CloseHandle(snapshot);
        }
    };

    rescan_tls();
    int iteration = 0;

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (!tls_slot) {
            rescan_tls();
            continue;
        }

        if (++iteration > 1000) {
            iteration = 0;
            rescan_tls();
        }

        uint64_t current_gm = DecryptGameManager(base, tls_slot);
        if (!current_gm) {
            rescan_tls();
            continue;
        }

        vector3_t cam_pos = driver.read<vector3_t>(current_gm + r6::GM_CAMERA_POS);
        PawnList list = DecryptPawnList(base, current_gm, tls_slot);
        uint32_t count = list.cnt & 0xFFFF;

        if (count < 1 || count > 20 || list.arr < 0x10000 || list.arr > 0x7FFFFFFFFFFF) {
            pawn_fail_count++;
            if (pawn_fail_count > 50) {
                pawn_fail_count = 0;
                rescan_tls();
            }
            continue;
        }
        pawn_fail_count = 0;

        std::vector<entity_t> current_entities;
        for (uint32_t i = 0; i < count; i++) {
            uint64_t pawn_ptr = driver.read<uint64_t>(list.arr + i * 8);
            if (pawn_ptr < 0x10000 || pawn_ptr > 0x7FFFFFFFFFFF) continue;

            uint64_t entity_ptr = DecryptEntity(base, pawn_ptr, tls_slot);
            if (entity_ptr < 0x10000 || entity_ptr > 0x7FFFFFFFFFFF) continue;

            vector3_t pos = driver.read<vector3_t>(entity_ptr + r6::ENT_POSITION);
            if (!pos.is_valid()) continue;

            float dist = cam_pos.distance(pos);
            bool is_local = (dist < 1.0f);
            
            current_entities.push_back({ pos, dist, is_local });
        }

        {
            std::lock_guard<std::mutex> lock(entity_mutex);
            cached_entities = std::move(current_entities);
        }
    }
}

int main() {
    std::cout << "[*] Starting R6-External-ESP" << std::endl;

#ifndef DEMO_MODE
    if (!driver.connect() || !driver.attach(L"RainbowSix.exe")) {
        std::cout << "[!] Driver initialization failed." << std::endl;
        std::cin.get();
        return 1;
    }
#else
    std::cout << "[!] Running in DEMO MODE." << std::endl;
#endif

    c_overlay overlay;
#ifndef DEMO_MODE
    if (!overlay.create(false)) {
#else
    if (!overlay.create(true)) {
#endif
        std::cout << "[!] Overlay creation failed." << std::endl;
        std::cin.get();
        return 1;
    }

#ifndef DEMO_MODE
    std::thread cache_thread(entity_cache_thread);
#endif

    ApplyProfessionalTheme();

    while (running && overlay.process_messages()) {
        if (GetAsyncKeyState(VK_END) & 1) {
            running = false;
            break;
        }
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            overlay.toggle_menu();
        }

        overlay.begin_frame();

        if (overlay.menu_visible()) {
            ImGui::SetNextWindowSize(ImVec2(320, 260), ImGuiCond_FirstUseEver);

            ImGui::Begin("R6-EXTERNAL-ESP", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

            ImGui::TextColored(ImVec4(0.60f, 0.40f, 0.95f, 1.0f), "OVERLAY CONFIGURATION");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Checkbox("Draw ESP Boxes", &esp_boxes);
            ImGui::Checkbox("Draw Snaplines", &esp_snaplines);
            ImGui::Spacing();
            ImGui::SliderFloat("Render Distance", &esp_distance, 10.0f, 1000.0f, "%.1f meters");
            
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight() * 1.5f));
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "INSERT - Toggle Menu  |  END - Exit");

            ImGui::End();
        }

        float screen_w = (float)overlay.width();
        float screen_h = (float)overlay.height();
        float radar_x = screen_w - 160.0f;
        float radar_y = 160.0f;
        float radar_r = 140.0f;
        float radar_range = 50.0f;

        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(radar_x, radar_y), radar_r, IM_COL32(0, 0, 0, 200), 64);
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(radar_x, radar_y), radar_r, IM_COL32(0, 255, 0, 255), 64, 1.5f);
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(radar_x - radar_r, radar_y), ImVec2(radar_x + radar_r, radar_y), IM_COL32(0, 255, 0, 100));
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(radar_x, radar_y - radar_r), ImVec2(radar_x, radar_y + radar_r), IM_COL32(0, 255, 0, 100));
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(radar_x, radar_y), 3.0f, IM_COL32(255, 255, 255, 255));

#ifndef DEMO_MODE
        uint64_t tls_slot = g_tls_slot.load();
        if (tls_slot) {
            uint64_t gm = DecryptGameManager(driver.base(), tls_slot);
            if (gm) {
                float vp[16];
                driver.read_raw(gm + r6::GM_VP_MATRIX, vp, sizeof(vp));
                vector3_t cam_pos = driver.read<vector3_t>(gm + r6::GM_CAMERA_POS);

                std::vector<entity_t> ents;
                {
                    std::lock_guard<std::mutex> lock(entity_mutex);
                    ents = cached_entities;
                }
#else
        {
            float fov_factor = 1.0f; 
            float aspect = screen_w / screen_h;
            float vp[16] = {
                fov_factor / aspect, 0, 0, 0,
                0, fov_factor, 0, 0,
                0, 0, 1.0f, 1.0f,
                0, 0, -0.1f, 0
            };
            vector3_t cam_pos = {0, 0, 0};

            static auto start_time = std::chrono::steady_clock::now();
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count();
            
            std::vector<entity_t> ents;
            ents.push_back({{sinf(t)*10.0f, cosf(t)*10.0f, 1.0f}, 10.0f, false});
            ents.push_back({{-5.0f, 15.0f, 0.0f}, 15.8f, false});
            ents.push_back({{20.0f, 30.0f, 2.0f}, 36.0f, false});

            if (true) {
#endif

                for (const auto& ent : ents) {
                    if (ent.is_local || ent.distance > esp_distance) continue;

                    if (ent.distance < radar_range) {
                        float dx = ent.position.x - cam_pos.x;
                        float dy = ent.position.y - cam_pos.y;
                        float scaled_x = (dx / radar_range) * radar_r;
                        float scaled_y = (dy / radar_range) * radar_r;
                        float dot_x = radar_x + scaled_x;
                        float dot_y = radar_y - scaled_y;

                        float dot_dist = sqrtf(scaled_x*scaled_x + scaled_y*scaled_y);
                        if (dot_dist > radar_r) {
                            dot_x = radar_x + (scaled_x / dot_dist) * radar_r;
                            dot_y = radar_y - (scaled_y / dot_dist) * radar_r;
                        }
                        
                        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(dot_x, dot_y), 4.0f, IM_COL32(255, 0, 0, 255));
                        char rdist[16];
                        snprintf(rdist, sizeof(rdist), "%.0f", ent.distance);
                        overlay.draw_text(dot_x + 5.0f, dot_y, rdist, IM_COL32(255, 255, 255, 255));
                    }

                    vector3_t head_pos = ent.position;
                    head_pos.z += 1.60f; // Static head offset
                    vector3_t foot_pos = ent.position;

                    vector2_t head_w2s = w2s_vp(head_pos, vp, screen_w, screen_h);
                    vector2_t foot_w2s = w2s_vp(foot_pos, vp, screen_w, screen_h);

                    if (!head_w2s.is_zero() && !foot_w2s.is_zero()) {
                        overlay.draw_actor_esp(head_w2s.x, head_w2s.y, foot_w2s.x, foot_w2s.y, ent.distance, esp_boxes, esp_snaplines);
                    }
                }
            }
        }

        overlay.end_frame();
    }

    running = false;
#ifndef DEMO_MODE
    cache_thread.join();
#endif
    return 0;
}
