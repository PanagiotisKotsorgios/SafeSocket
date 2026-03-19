#include "../include/security.hpp"
#include "../include/app.hpp"
#include "imgui.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

// ── SecurityState constructor ─────────────────────────────────────────────
SecurityState::SecurityState()
{
    bool exists = sec_file_exists();

    enabled       = exists;      // security ON if file exists
    authenticated = !exists;     // require login if enabled
    first_run     = !exists;

    show_wizard = first_run;
    show_login  = false;
    show_security_menu = false;

    wiz_use_generated = true;
    wiz_done = false;
    wiz_password_shown = false;

    show_change_pass = false;
    show_export = false;
    show_reset_confirm = false;

    memset(wiz_username,  0, sizeof(wiz_username));
    memset(wiz_password,  0, sizeof(wiz_password));
    memset(wiz_password2, 0, sizeof(wiz_password2));
    memset(wiz_generated, 0, sizeof(wiz_generated));

    memset(login_user,  0, sizeof(login_user));
    memset(login_pass,  0, sizeof(login_pass));
    memset(login_error, 0, sizeof(login_error));

    memset(cp_old, 0, sizeof(cp_old));
    memset(cp_new, 0, sizeof(cp_new));
    memset(cp_new2, 0, sizeof(cp_new2));
    memset(cp_error, 0, sizeof(cp_error));

    memset(export_path, 0, sizeof(export_path));
    strncpy(export_path, "local_backup.enc", sizeof(export_path)-1);
}
// ── PBKDF2 key derivation ─────────────────────────────────────────────────
bool sec_derive_key(const std::string& password,
                    const unsigned char* salt, int salt_len,
                    unsigned char* out_key, unsigned char* out_iv)
{
    // 32 bytes key + 16 bytes IV = 48 bytes total
    unsigned char buf[48];
    int ok = PKCS5_PBKDF2_HMAC(
        password.c_str(), (int)password.size(),
        salt, salt_len,
        100000,             // iterations
        EVP_sha256(),
        48, buf);
    if (!ok) return false;
    memcpy(out_key, buf, 32);
    memcpy(out_iv,  buf + 32, 16);
    return true;
}

// ── AES-256-CBC encrypt ───────────────────────────────────────────────────
bool sec_encrypt(const std::string& plaintext,
                 const std::string& password,
                 std::vector<unsigned char>& out_blob)
{
    unsigned char salt[16];
    if (RAND_bytes(salt, 16) != 1) return false;

    unsigned char key[32], iv[16];
    if (!sec_derive_key(password, salt, 16, key, iv)) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx); return false;
    }

    std::vector<unsigned char> ciphertext(plaintext.size() + 32);
    int len1 = 0, len2 = 0;
    EVP_EncryptUpdate(ctx,
        ciphertext.data(), &len1,
        (const unsigned char*)plaintext.c_str(), (int)plaintext.size());
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len1, &len2);
    EVP_CIPHER_CTX_free(ctx);

    int data_len = len1 + len2;

    // Build blob: salt(16) + iv(16) + data_len(4) + ciphertext
    out_blob.clear();
    out_blob.insert(out_blob.end(), salt, salt + 16);
    out_blob.insert(out_blob.end(), iv,   iv   + 16);
    unsigned char dl[4];
    dl[0] = (data_len >> 24) & 0xFF;
    dl[1] = (data_len >> 16) & 0xFF;
    dl[2] = (data_len >>  8) & 0xFF;
    dl[3] = (data_len      ) & 0xFF;
    out_blob.insert(out_blob.end(), dl, dl + 4);
    out_blob.insert(out_blob.end(), ciphertext.begin(), ciphertext.begin() + data_len);
    return true;
}

// ── AES-256-CBC decrypt ───────────────────────────────────────────────────
bool sec_decrypt(const std::vector<unsigned char>& blob,
                 const std::string& password,
                 std::string& out_plaintext)
{
    if (blob.size() < 36) return false;

    const unsigned char* salt = blob.data();
    const unsigned char* iv   = blob.data() + 16;
    int data_len = ((int)blob[32] << 24) | ((int)blob[33] << 16)
                 | ((int)blob[34] <<  8) |  (int)blob[35];

    if (data_len <= 0 || (int)blob.size() < 36 + data_len) return false;

    unsigned char key[32], derived_iv[16];
    if (!sec_derive_key(password, salt, 16, key, derived_iv)) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx); return false;
    }

    std::vector<unsigned char> plain(data_len + 16);
    int len1 = 0, len2 = 0;
    if (EVP_DecryptUpdate(ctx, plain.data(), &len1,
        blob.data() + 36, data_len) != 1) {
        EVP_CIPHER_CTX_free(ctx); return false;
    }
    if (EVP_DecryptFinal_ex(ctx, plain.data() + len1, &len2) != 1) {
        EVP_CIPHER_CTX_free(ctx); return false;
    }
    EVP_CIPHER_CTX_free(ctx);

    out_plaintext = std::string((char*)plain.data(), len1 + len2);
    return true;
}

// ── File helpers ──────────────────────────────────────────────────────────
bool sec_file_exists(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool sec_write_file(const std::string& username,
                    const std::string& password,
                    const char* path)
{
    std::string plaintext = "SAFESOCKET\nuser=" + username + "\nver=1\n";
    std::vector<unsigned char> blob;
    if (!sec_encrypt(plaintext, password, blob)) return false;

    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fwrite(blob.data(), 1, blob.size(), f);
    fclose(f);
    return true;
}

bool sec_verify_login(const std::string& username,
                      const std::string& password,
                      const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<unsigned char> blob(sz);
    fread(blob.data(), 1, sz, f);
    fclose(f);

    std::string plain;
    if (!sec_decrypt(blob, password, plain)) return false;

    // Check magic header
    if (plain.find("SAFESOCKET") == std::string::npos) return false;

    // Check username
    std::string needle = "user=" + username + "\n";
    return plain.find(needle) != std::string::npos;
}

bool sec_read_username(const std::string& password,
                       std::string& out_username,
                       const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<unsigned char> blob(sz);
    fread(blob.data(), 1, sz, f);
    fclose(f);

    std::string plain;
    if (!sec_decrypt(blob, password, plain)) return false;

    size_t pos = plain.find("user=");
    if (pos == std::string::npos) return false;
    pos += 5;
    size_t end = plain.find('\n', pos);
    out_username = plain.substr(pos, end - pos);
    return true;
}

bool sec_copy_file(const char* src, const char* dst)
{
    FILE* in  = fopen(src, "rb");
    if (!in) return false;
    FILE* out = fopen(dst, "wb");
    if (!out) { fclose(in); return false; }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    return true;
}

bool sec_delete_file(const char* path)
{
    return remove(path) == 0;
}

// ── Password generator ────────────────────────────────────────────────────
std::string sec_generate_password(int length)
{
    static const char charset[] =
        "abcdefghijkmnpqrstuvwxyz"
        "ABCDEFGHJKLMNPQRSTUVWXYZ"
        "23456789"
        "!@#$%^&*";
    int clen = (int)(sizeof(charset) - 1);

    std::string result(length, ' ');
    unsigned char rnd[64];
    RAND_bytes(rnd, length);
    for (int i = 0; i < length; i++)
        result[i] = charset[rnd[i] % clen];
    return result;
}

// ── Shared style helpers ──────────────────────────────────────────────────
static void amber_title(const char* txt)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.60f,0.15f,1.0f));
    ImGui::SetWindowFontScale(1.15f);
    ImGui::Text("%s", txt);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
}

static void error_text(const char* msg)
{
    if (msg && msg[0]) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f,0.38f,0.34f,1.0f));
        ImGui::TextWrapped("%s", msg);
        ImGui::PopStyleColor();
    }
}

static bool action_button(const char* label, float w = 120.0f, float h = 32.0f)
{
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f,0.16f,0.18f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f,0.52f,0.12f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.58f,0.38f,0.08f,1.0f));
    bool r = ImGui::Button(label, ImVec2(w, h));
    ImGui::PopStyleColor(3);
    return r;
}

// ─────────────────────────────────────────────────────────────────────────
// LOGIN SCREEN — fullscreen, blocks everything
// ─────────────────────────────────────────────────────────────────────────
void draw_login_screen(SecurityState& sec)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##lockscreen", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);

    // Centre a login card
    float card_w = 380.0f, card_h = 280.0f;
    float cx = (io.DisplaySize.x - card_w) * 0.5f;
    float cy = (io.DisplaySize.y - card_h) * 0.5f;
    ImGui::SetNextWindowPos(ImVec2(cx, cy), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(card_w, card_h), ImGuiCond_Always);
    ImGui::Begin("##logincard", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::Spacing();
    amber_title("SafeSocket — Locked");
    ImGui::TextDisabled("Enter your credentials to continue.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float LW = 100.0f;

    ImGui::Text("Username"); ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##lu", sec.login_user, sizeof(sec.login_user));

    ImGui::Text("Password"); ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.0f);
    bool enter = ImGui::InputText("##lp", sec.login_pass, sizeof(sec.login_pass),
        ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::Spacing();
    error_text(sec.login_error);
    ImGui::Spacing();

    float bw = (card_w - ImGui::GetStyle().WindowPadding.x*2 - ImGui::GetStyle().ItemSpacing.x)*0.5f;
    if (action_button("Unlock", bw, 36) || enter) {
        if (sec_verify_login(sec.login_user, sec.login_pass)) {
            sec.authenticated = true;
            sec.stored_username = sec.login_user;
            memset(sec.login_pass, 0, sizeof(sec.login_pass));
            memset(sec.login_error, 0, sizeof(sec.login_error));
        } else {
            strncpy(sec.login_error, "Invalid username or password.", sizeof(sec.login_error)-1);
        }
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f,0.06f,0.06f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.34f,0.10f,0.10f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.24f,0.07f,0.07f,1.0f));
    if (ImGui::Button("Quit", ImVec2(bw, 36))) exit(0);
    ImGui::PopStyleColor(3);

    ImGui::End(); // logincard
    ImGui::End(); // lockscreen
}

// ─────────────────────────────────────────────────────────────────────────
// SETUP WIZARD — first run
// ─────────────────────────────────────────────────────────────────────────
void draw_wizard_screen(SecurityState& sec)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##wizscreen", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);

    float card_w = 480.0f;
    float card_h = sec.wiz_done ? 280.0f : 400.0f;
    float cx = (io.DisplaySize.x - card_w)*0.5f;
    float cy = (io.DisplaySize.y - card_h)*0.5f;
    ImGui::SetNextWindowPos(ImVec2(cx,cy), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(card_w, card_h), ImGuiCond_Always);
    ImGui::Begin("##wizcard", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::Spacing();

    // ── STEP 2: one-time password display ────────────────────────────────
    if (sec.wiz_done) {
        amber_title("Setup Complete");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f,0.40f,0.36f,1.0f));
        ImGui::TextWrapped("IMPORTANT: This is the ONLY time your password will be shown.");
        ImGui::TextWrapped("Write it down or store it safely. It cannot be recovered.");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f,0.06f,0.07f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,    ImVec4(0.85f,0.60f,0.15f,1.0f));
        ImGui::SetNextItemWidth(-1.0f);
        // Show password as read-only selectable text
        char disp[128];
        strncpy(disp, sec.wiz_generated[0]
            ? sec.wiz_generated
            : sec.wiz_password, 127);
        ImGui::InputText("##showpass", disp, sizeof(disp),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor(2);
        ImGui::Spacing();

        ImGui::TextDisabled("Username: %s", sec.wiz_username);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float bw = card_w - ImGui::GetStyle().WindowPadding.x*2;
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.12f,0.22f,0.10f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f,0.36f,0.14f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f,0.18f,0.08f,1.0f));
        if (ImGui::Button("I have saved my password — Continue", ImVec2(bw, 38))) {
            sec.wiz_password_shown = true;
            sec.authenticated      = true;
            sec.enabled            = true;
            sec.show_wizard        = false;
            sec.first_run          = false;
            sec.stored_username    = sec.wiz_username;
        }
        ImGui::PopStyleColor(3);
        ImGui::End(); ImGui::End();
        return;
    }

    // ── STEP 1: create account ────────────────────────────────────────────
    amber_title("First Run — Security Setup");
    ImGui::TextDisabled("Create your local account to protect this app.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float LW = 120.0f;

    ImGui::Text("Username"); ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##wu", sec.wiz_username, sizeof(sec.wiz_username));
    ImGui::Spacing();

    // Password mode toggle
    ImGui::Text("Password"); ImGui::SameLine(LW);
    if (ImGui::RadioButton("Generate for me", sec.wiz_use_generated)) {
        sec.wiz_use_generated = true;
        memset(sec.wiz_password,  0, sizeof(sec.wiz_password));
        memset(sec.wiz_password2, 0, sizeof(sec.wiz_password2));
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("I'll set my own", !sec.wiz_use_generated)) {
        sec.wiz_use_generated = false;
        memset(sec.wiz_generated, 0, sizeof(sec.wiz_generated));
    }
    ImGui::Spacing();

    if (!sec.wiz_use_generated) {
        ImGui::Text("Password"); ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##wp", sec.wiz_password, sizeof(sec.wiz_password),
            ImGuiInputTextFlags_Password);

        ImGui::Text("Confirm"); ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##wp2", sec.wiz_password2, sizeof(sec.wiz_password2),
            ImGuiInputTextFlags_Password);

        // Strength indicator
        int plen = (int)strlen(sec.wiz_password);
        ImGui::Spacing();
        ImGui::Text("Strength"); ImGui::SameLine(LW);
        ImVec4 sc = plen < 8
            ? ImVec4(0.80f,0.30f,0.28f,1.0f)
            : plen < 12
                ? ImVec4(0.82f,0.58f,0.14f,1.0f)
                : ImVec4(0.30f,0.62f,0.28f,1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, sc);
        ImGui::Text(plen < 8 ? "Weak" : plen < 12 ? "Fair" : "Strong");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.57f,1.0f));
        ImGui::TextWrapped("  A strong 18-character password will be generated and shown once.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Error area
    static char wiz_error[128] = {};
    error_text(wiz_error);
    ImGui::Spacing();

    float bw = (card_w - ImGui::GetStyle().WindowPadding.x*2
                - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    if (action_button("Create Account", bw, 38)) {
        wiz_error[0] = 0;
        if (strlen(sec.wiz_username) < 2) {
            strncpy(wiz_error, "Username must be at least 2 characters.", 127);
        } else {
            std::string pass;
            if (sec.wiz_use_generated) {
                pass = sec_generate_password(18);
                strncpy(sec.wiz_generated, pass.c_str(), sizeof(sec.wiz_generated)-1);
            } else {
                if (strcmp(sec.wiz_password, sec.wiz_password2) != 0)
                    strncpy(wiz_error, "Passwords do not match.", 127);
                else if (strlen(sec.wiz_password) < 6)
                    strncpy(wiz_error, "Password must be at least 6 characters.", 127);
                else
                    pass = sec.wiz_password;
            }
            if (wiz_error[0] == 0) {
                if (sec_write_file(sec.wiz_username, pass)) {
                    sec.wiz_done = true;
                    memset(wiz_error, 0, sizeof(wiz_error));
                } else {
                    strncpy(wiz_error, "Failed to write local.enc — check permissions.", 127);
                }
            }
        }
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f,0.06f,0.06f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.34f,0.10f,0.10f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.24f,0.07f,0.07f,1.0f));
    if (ImGui::Button("Skip (disable security)", ImVec2(bw, 38))) {
        sec.show_wizard  = false;
        sec.first_run    = false;
        sec.enabled      = false;
        sec.authenticated= true;
    }
    ImGui::PopStyleColor(3);

    ImGui::End(); // wizcard
    ImGui::End(); // wizscreen
}

// ─────────────────────────────────────────────────────────────────────────
// SECURITY MODAL — from navbar
// ─────────────────────────────────────────────────────────────────────────
void draw_security_modal(SecurityState& sec)
{
    if (!sec.show_security_menu) return;
    ImGui::OpenPopup("Security Settings");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(460, 360), ImGuiCond_Always);

    if (!ImGui::BeginPopupModal("Security Settings", &sec.show_security_menu,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        return;

    ImGui::Spacing();
    amber_title("Security Settings");
    ImGui::TextDisabled("User: %s", sec.stored_username.empty()
        ? "(not set)" : sec.stored_username.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Enable / Disable toggle ───────────────────────────────────────────
    bool file_exists = sec_file_exists();

    if (file_exists) {
        ImGui::Text("App lock is currently:");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, sec.enabled
            ? ImVec4(0.30f,0.62f,0.28f,1.0f)
            : ImVec4(0.60f,0.60f,0.62f,1.0f));
        ImGui::Text(sec.enabled ? "ENABLED" : "DISABLED");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (sec.enabled) {
            if (action_button("Disable Lock", 160)) sec.enabled = false;
        } else {
            if (action_button("Enable Lock", 160)) {
    sec.enabled = true;
    sec.authenticated = false; //  FORCE LOGIN
}
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f,0.60f,0.62f,1.0f));
        ImGui::TextWrapped("No local.enc found. Set up security first.");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        if (action_button("Run Setup Wizard", 180)) {
            sec.show_security_menu = false;
            sec.show_wizard        = true;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Change password ───────────────────────────────────────────────────
    if (file_exists) {
        if (ImGui::CollapsingHeader("Change Password")) {
            ImGui::Spacing();
            const float LW = 120.0f;
            ImGui::Text("Current");   ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##cpo", sec.cp_old,  sizeof(sec.cp_old),  ImGuiInputTextFlags_Password);
            ImGui::Text("New");       ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##cpn", sec.cp_new,  sizeof(sec.cp_new),  ImGuiInputTextFlags_Password);
            ImGui::Text("Confirm");   ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##cpn2",sec.cp_new2, sizeof(sec.cp_new2), ImGuiInputTextFlags_Password);
            ImGui::Spacing();
            error_text(sec.cp_error);
            ImGui::Spacing();
            if (action_button("Update Password", 160)) {
                sec.cp_error[0] = 0;
                if (strcmp(sec.cp_new, sec.cp_new2) != 0)
                    strncpy(sec.cp_error, "New passwords do not match.", sizeof(sec.cp_error)-1);
                else if (strlen(sec.cp_new) < 6)
                    strncpy(sec.cp_error, "Password too short (min 6).", sizeof(sec.cp_error)-1);
                else if (!sec_verify_login(sec.stored_username, sec.cp_old))
                    strncpy(sec.cp_error, "Current password is incorrect.", sizeof(sec.cp_error)-1);
                else if (!sec_write_file(sec.stored_username, sec.cp_new))
                    strncpy(sec.cp_error, "Failed to write local.enc.", sizeof(sec.cp_error)-1);
                else {
                    strncpy(sec.cp_error, "Password updated successfully.", sizeof(sec.cp_error)-1);
                    memset(sec.cp_old,  0, sizeof(sec.cp_old));
                    memset(sec.cp_new,  0, sizeof(sec.cp_new));
                    memset(sec.cp_new2, 0, sizeof(sec.cp_new2));
                }
            }
            ImGui::Spacing();
        }

        // ── Export backup ─────────────────────────────────────────────────
        if (ImGui::CollapsingHeader("Export Backup")) {
            ImGui::Spacing();
            ImGui::Text("Save to:"); ImGui::SameLine();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##ep", sec.export_path, sizeof(sec.export_path));
            ImGui::Spacing();
            if (action_button("Export local.enc", 160)) {
                if (sec_copy_file(SS_SAFESOCK_SEC_FILE, sec.export_path))
                    ImGui::OpenPopup("ExportOK");
            }
            if (ImGui::BeginPopup("ExportOK")) {
                ImGui::Text("Exported successfully.");
                if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::Spacing();
        }

        // ── Reset ─────────────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.28f,0.08f,0.08f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f,0.12f,0.12f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.38f,0.08f,0.08f,1.0f));
        if (ImGui::Button("Reset & Delete Credentials", ImVec2(-1, 32)))
            sec.show_reset_confirm = true;
        ImGui::PopStyleColor(3);

        if (sec.show_reset_confirm) {
            ImGui::OpenPopup("Confirm Reset");
        }
        ImVec2 rc = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(rc, ImGuiCond_Always, ImVec2(0.5f,0.5f));
        if (ImGui::BeginPopupModal("Confirm Reset", &sec.show_reset_confirm,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f,0.38f,0.34f,1.0f));
            ImGui::Text("This will DELETE local.enc permanently.");
            ImGui::Text("You will need to run setup again on next launch.");
            ImGui::PopStyleColor();
            ImGui::Spacing();
            if (action_button("Yes, Delete", 120)) {
                sec_delete_file();
                sec.enabled          = false;
                sec.authenticated    = true;
                sec.stored_username  = "";
                sec.show_reset_confirm = false;
                sec.show_security_menu = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (action_button("Cancel", 100)) {
                sec.show_reset_confirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::EndPopup();
}
