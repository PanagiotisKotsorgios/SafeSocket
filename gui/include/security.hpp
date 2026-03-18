#ifndef SECURITY_HPP
#define SECURITY_HPP

#include <string>
#include <vector>

// ── Local.enc file format ────────────────────────────────────────────────
// [16 bytes salt][16 bytes IV][4 bytes data_len][data_len bytes AES-256-CBC]
// Data plaintext: "SAFESOCKET\nuser=<user>\nver=1\n"

#define SEC_FILE "local.enc"

struct SecurityState {
    // Runtime state
    bool     enabled;           // is app lock active?
    bool     authenticated;     // has user passed login this session?
    bool     first_run;         // no local.enc exists yet

    // Wizard / login UI
    bool     show_wizard;       // setup wizard open
    bool     show_login;        // login screen open
    bool     show_security_menu;// security modal open

    // Wizard inputs
    char     wiz_username[64];
    char     wiz_password[128];
    char     wiz_password2[128];
    char     wiz_generated[128];// the one-time generated password shown
    bool     wiz_use_generated; // user chose generated vs custom
    bool     wiz_done;          // wizard completed, show one-time password
    bool     wiz_password_shown;// user clicked "I saved it" — close wizard

    // Login inputs
    char     login_user[64];
    char     login_pass[128];
    char     login_error[128];

    // Change password
    bool     show_change_pass;
    char     cp_old[128];
    char     cp_new[128];
    char     cp_new2[128];
    char     cp_error[128];

    // Export
    bool     show_export;
    char     export_path[256];

    // Reset confirm
    bool     show_reset_confirm;

    // Stored username (loaded from local.enc after auth)
    std::string stored_username;

    SecurityState();
};

// ── Crypto helpers ────────────────────────────────────────────────────────
// Derive 32-byte key + 16-byte IV from password + salt using PBKDF2-SHA256
bool sec_derive_key(const std::string& password,
                    const unsigned char* salt, int salt_len,
                    unsigned char* out_key, unsigned char* out_iv);

// Encrypt plaintext → ciphertext (AES-256-CBC). Returns false on error.
bool sec_encrypt(const std::string& plaintext,
                 const std::string& password,
                 std::vector<unsigned char>& out_blob);

// Decrypt blob → plaintext. Returns false on wrong password / corrupt.
bool sec_decrypt(const std::vector<unsigned char>& blob,
                 const std::string& password,
                 std::string& out_plaintext);

// ── File operations ───────────────────────────────────────────────────────
bool sec_file_exists(const char* path = SEC_FILE);
bool sec_write_file(const std::string& username,
                    const std::string& password,
                    const char* path = SEC_FILE);
bool sec_verify_login(const std::string& username,
                      const std::string& password,
                      const char* path = SEC_FILE);
bool sec_read_username(const std::string& password,
                       std::string& out_username,
                       const char* path = SEC_FILE);
bool sec_copy_file(const char* src, const char* dst);
bool sec_delete_file(const char* path = SEC_FILE);

// ── Password generator ────────────────────────────────────────────────────
std::string sec_generate_password(int length = 18);

// ── Draw functions ────────────────────────────────────────────────────────
struct AppState;
void draw_login_screen(SecurityState& sec);
void draw_wizard_screen(SecurityState& sec);
void draw_security_modal(SecurityState& sec);

#endif
