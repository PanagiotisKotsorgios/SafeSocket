# =============================================================================
# SafeSocket – Test Suite CMakeLists.txt
# =============================================================================
# Build all unit, integration and stress tests.
#
# Usage:
#   mkdir build && cd build
#   cmake .. -DCMAKE_BUILD_TYPE=Debug
#   cmake --build . --parallel
#   ctest --output-on-failure
#
# Optional flags:
#   -DSS_ASAN=ON    Enable AddressSanitizer (GCC/Clang only)
#   -DSS_TSAN=ON    Enable ThreadSanitizer  (GCC/Clang only)
#
cmake_minimum_required(VERSION 3.12)
project(SafeSocketTests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Source root (one level up from this file) ─────────────────────────────────
set(SS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# ── Compiler options ──────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W3 /WX- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -pthread)
    add_link_options(-pthread)
endif()

# ── Sanitisers ────────────────────────────────────────────────────────────────
if(SS_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(SS_TSAN AND NOT MSVC)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# ── Platform socket library ───────────────────────────────────────────────────
if(WIN32)
    set(SOCKET_LIBS ws2_32)
else()
    set(SOCKET_LIBS "")
endif()

# ── Core SafeSocket library (shared by all tests) ─────────────────────────────
add_library(safesocket_core STATIC
    ${SS_ROOT}/crypto.cpp
    ${SS_ROOT}/network.cpp
    ${SS_ROOT}/config.cpp
    ${SS_ROOT}/protocol.cpp      # only if it has a .cpp; remove otherwise
    ${SS_ROOT}/server.cpp
    ${SS_ROOT}/client.cpp
)
target_include_directories(safesocket_core PUBLIC ${SS_ROOT})
target_link_libraries(safesocket_core PUBLIC ${SOCKET_LIBS})

# ── Helper function: add a named test executable ──────────────────────────────
function(ss_test name sources)
    add_executable(${name} ${sources})
    target_include_directories(${name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SS_ROOT}"
    )
    target_link_libraries(${name} PRIVATE safesocket_core ${SOCKET_LIBS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

enable_testing()

# ── Unit: crypto ──────────────────────────────────────────────────────────────
ss_test(test_xor          unit/crypto/test_xor.cpp)
ss_test(test_vigenere     unit/crypto/test_vigenere.cpp)
ss_test(test_rc4          unit/crypto/test_rc4.cpp)
ss_test(test_encrypt_type unit/crypto/test_encrypt_type.cpp)

# ── Unit: protocol ────────────────────────────────────────────────────────────
ss_test(test_packet_header unit/protocol/test_packet_header.cpp)

# ── Unit: config ──────────────────────────────────────────────────────────────
ss_test(test_config_set_get   unit/config/test_config_set_get.cpp)
ss_test(test_config_load_save unit/config/test_config_load_save.cpp)

# ── Unit: network ─────────────────────────────────────────────────────────────
ss_test(test_fmt_bytes            unit/network/test_fmt_bytes.cpp)
ss_test(test_net_init             unit/network/test_net_init.cpp)
ss_test(test_net_listen_connect   unit/network/test_net_listen_connect.cpp)
ss_test(test_net_raw_io           unit/network/test_net_raw_io.cpp)
ss_test(test_net_packet_io        unit/network/test_net_packet_io.cpp)

# ── Integration: messaging ────────────────────────────────────────────────────
ss_test(test_broadcast    integration/messaging/test_broadcast.cpp)
ss_test(test_private_msg  integration/messaging/test_private_msg.cpp)
ss_test(test_nick_change  integration/messaging/test_nick_change.cpp)

# ── Integration: auth ─────────────────────────────────────────────────────────
ss_test(test_access_key   integration/auth/test_access_key.cpp)

# ── Integration: file transfer ────────────────────────────────────────────────
ss_test(test_file_send    integration/file_transfer/test_file_send.cpp)
ss_test(test_file_hash    integration/file_transfer/test_file_hash.cpp)

# ── Stress ────────────────────────────────────────────────────────────────────
ss_test(test_stress_clients  stress/test_stress_clients.cpp)
ss_test(test_stress_crypto   stress/test_stress_crypto.cpp)

# ── CTest labels for selective runs ──────────────────────────────────────────
set_tests_properties(
    test_xor test_vigenere test_rc4 test_encrypt_type
    test_packet_header
    test_config_set_get test_config_load_save
    test_fmt_bytes test_net_init test_net_listen_connect
    test_net_raw_io test_net_packet_io
    PROPERTIES LABELS "unit"
)
set_tests_properties(
    test_broadcast test_private_msg test_nick_change
    test_access_key test_file_send test_file_hash
    PROPERTIES LABELS "integration"
)
set_tests_properties(
    test_stress_clients test_stress_crypto
    PROPERTIES LABELS "stress" TIMEOUT 120
)
