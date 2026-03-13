# =============================================================================
# SafeSocket – Test Suite Makefile  (Linux / macOS)
# =============================================================================
# Targets:
#   make            – build all tests
#   make unit       – build + run unit tests only
#   make integration– build + run integration tests only
#   make stress     – build + run stress tests only
#   make all_tests  – build + run everything
#   make clean      – remove build artefacts
#
# Variables (override on command line):
#   CXX=clang++     – compiler
#   ASAN=1          – enable AddressSanitizer
#   TSAN=1          – enable ThreadSanitizer
#
# =============================================================================

CXX      ?= g++
CXXFLAGS := -std=c++11 -Wall -Wextra -Wno-unused-parameter -pthread -g
LDFLAGS  := -pthread

SS_ROOT  := ..
INC      := -I. -I$(SS_ROOT)

ifdef ASAN
  CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
  LDFLAGS  += -fsanitize=address
endif
ifdef TSAN
  CXXFLAGS += -fsanitize=thread
  LDFLAGS  += -fsanitize=thread
endif

# ── Core sources ──────────────────────────────────────────────────────────────
CORE_SRCS := \
  $(SS_ROOT)/crypto.cpp   \
  $(SS_ROOT)/network.cpp  \
  $(SS_ROOT)/config.cpp   \
  $(SS_ROOT)/server.cpp   \
  $(SS_ROOT)/client.cpp

CORE_OBJS := $(CORE_SRCS:.cpp=.o)

# ── Unit tests ────────────────────────────────────────────────────────────────
UNIT_BINS := \
  unit/crypto/test_xor                 \
  unit/crypto/test_vigenere            \
  unit/crypto/test_rc4                 \
  unit/crypto/test_encrypt_type        \
  unit/protocol/test_packet_header     \
  unit/config/test_config_set_get      \
  unit/config/test_config_load_save    \
  unit/network/test_fmt_bytes          \
  unit/network/test_net_init           \
  unit/network/test_net_listen_connect \
  unit/network/test_net_raw_io         \
  unit/network/test_net_packet_io

# ── Integration tests ─────────────────────────────────────────────────────────
INTEG_BINS := \
  integration/messaging/test_broadcast         \
  integration/messaging/test_private_msg       \
  integration/messaging/test_nick_change       \
  integration/auth/test_access_key             \
  integration/file_transfer/test_file_send     \
  integration/file_transfer/test_file_hash

# ── Stress tests ──────────────────────────────────────────────────────────────
STRESS_BINS := \
  stress/test_stress_clients \
  stress/test_stress_crypto

ALL_BINS := $(UNIT_BINS) $(INTEG_BINS) $(STRESS_BINS)

# ── Pattern rules ─────────────────────────────────────────────────────────────
# Compile a single .cpp test file into a binary, linking all core objects
%: %.cpp $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) $(INC) $< $(CORE_OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

# ── Phony targets ─────────────────────────────────────────────────────────────
.PHONY: all unit integration stress all_tests clean

all: $(ALL_BINS)

unit: $(UNIT_BINS)
	@echo ""
	@echo "======== Running unit tests ========"
	@PASS=0; FAIL=0; \
	for t in $(UNIT_BINS); do \
	  if ./$$t; then PASS=$$((PASS+1)); else FAIL=$$((FAIL+1)); fi; \
	done; \
	echo ""; echo "Unit: $$PASS passed, $$FAIL failed"; \
	[ $$FAIL -eq 0 ]

integration: $(INTEG_BINS)
	@echo ""
	@echo "======== Running integration tests ========"
	@PASS=0; FAIL=0; \
	for t in $(INTEG_BINS); do \
	  if ./$$t; then PASS=$$((PASS+1)); else FAIL=$$((FAIL+1)); fi; \
	done; \
	echo ""; echo "Integration: $$PASS passed, $$FAIL failed"; \
	[ $$FAIL -eq 0 ]

stress: $(STRESS_BINS)
	@echo ""
	@echo "======== Running stress tests ========"
	@PASS=0; FAIL=0; \
	for t in $(STRESS_BINS); do \
	  if ./$$t; then PASS=$$((PASS+1)); else FAIL=$$((FAIL+1)); fi; \
	done; \
	echo ""; echo "Stress: $$PASS passed, $$FAIL failed"; \
	[ $$FAIL -eq 0 ]

all_tests: unit integration stress
	@echo ""
	@echo "All test suites completed."

clean:
	rm -f $(CORE_OBJS) $(ALL_BINS)
	find . -name "*.o" -delete
