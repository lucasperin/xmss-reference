CC = /usr/bin/gcc
CFLAGS = -Wall -g -O3 -Wextra -Wpedantic
LDLIBS = -lcrypto

SOURCES = params.c hash.c fips202.c hash_address.c randombytes.c wots.c xmss.c xmss_core.c xmss_commons.c utils.c
HEADERS = params.h hash.h fips202.h hash_address.h randombytes.h wots.h xmss.h xmss_core.h xmss_commons.h utils.h

SOURCES_FAST = $(subst xmss_core.c,xmss_core_fast.c,$(SOURCES))
HEADERS_FAST = $(subst xmss_core.c,xmss_core_fast.c,$(HEADERS))

BENCHMARK_ENC = test/enc \
				test/enc_cached \
				test/enc_bs \
				test/enc_cky \
				test/enc_ckyi \

BENCHMARK_FAST = test/xm_10_16  \
				 test/cs_10_16 \
				 test/bs_10_16 \
				 test/cs_10_42 \
				 test/bs_10_42 \
				 test/xm_10_256 \
				 test/cs_10_256 \
				 test/bs_10_256 \
				 test/cs_10_510 \
				 test/bs_10_510 \
				 test/cs_10_226 \
				 test/bs_10_226 \

BENCHMARK = $(BENCHMARK_FAST) \
			$(BENCHMARK_ENC) \
			test/xm_16_16  \
			test/xm_16_256 \
			test/cs_16_16 \
			test/cs_16_42 \
			test/cs_16_256 \
			test/cs_16_510 \
			test/cs_16_226 \
			test/bs_16_16 \
			test/bs_16_42 \
			test/bs_16_256 \
			test/bs_16_510 \
			test/bs_16_226 \


TESTS = $(BENCHMARK) \
		test/wots \
		test/oid \
		test/speed \
		test/xmss_determinism \
		test/xmss \
		test/xmss_fast \
		test/xmssmt \
		test/xmssmt_fast \
		test/wots_speed \
		test/xmss_speed \
		test/enc \


UI = ui/xmss_keypair \
	 ui/xmss_sign \
	 ui/xmss_open \
	 ui/xmssmt_keypair \
	 ui/xmssmt_sign \
	 ui/xmssmt_open \
	 ui/xmss_keypair_fast \
	 ui/xmss_sign_fast \
	 ui/xmss_open_fast \
	 ui/xmssmt_keypair_fast \
	 ui/xmssmt_sign_fast \
	 ui/xmssmt_open_fast \

all: tests ui

tests: $(TESTS)
ui: $(UI)

test: $(TESTS:=.exec)

.PHONY: clean test benchmark

test/%.exec: test/%
	@$<

test/xmss_fast: test/xmss.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSS_SIGNATURES=1024 $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/xmss: test/xmss.c $(SOURCES) $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

test/xmssmt_fast: test/xmss.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSSMT -DXMSS_SIGNATURES=1024 $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/xmssmt: test/xmss.c $(SOURCES) $(OBJS) $(HEADERS)
	$(CC) -DXMSSMT $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

test/speed: test/speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSSMT -DXMSS_VARIANT=\"XMSSMT-SHA2_20/2_256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/%: test/%.c $(SOURCES) $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

test/%_fast: test/%.c $(SOURCES) $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)


test/xm_10_16: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSS_VARIANT=\"XMSS-SHA2_10_256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/xm_10_256: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSS_VARIANT=\"XMSS-SHA2_10_256_W256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/xm_16_16: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSS_VARIANT=\"XMSS-SHA2_16_256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/xm_16_256: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSS_VARIANT=\"XMSS-SHA2_16_256_W256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

test/cs_10_16: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C16\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_10_42: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C42\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_10_256: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_10_510: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C510\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_10_226: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C226\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_16_16: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C16\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_16_42: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C42\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_16_256: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_16_510: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C510\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/cs_16_226: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C226\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp



test/bs_10_16: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C16\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_10_42: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C42\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_10_256: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_10_510: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C510\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_10_226: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_10_256_C226\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_16_16: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C16\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_16_42: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C42\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_16_256: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C256\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_16_510: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C510\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/bs_16_226: test/xmss_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH -DXMSS_VARIANT=\"XMSS-SHA2_16_256_C226\" $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp


test/enc: test/encoding_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/enc_cached: test/encoding_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DCACHED $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/enc_bs: test/encoding_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DBINARYSEARCH $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/enc_cky: test/encoding_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DCKY $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp

test/enc_ckyi: test/encoding_speed.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DCONSTANTSUM -DCKYI $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS) -lgmp


ui/xmss_%_fast: ui/%.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

ui/xmssmt_%_fast: ui/%.c $(SOURCES_FAST) $(OBJS) $(HEADERS_FAST)
	$(CC) -DXMSSMT $(CFLAGS) -o $@ $(SOURCES_FAST) $< $(LDLIBS)

ui/xmss_%: ui/%.c $(SOURCES) $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

ui/xmssmt_%: ui/%.c $(SOURCES) $(OBJS) $(HEADERS)
	$(CC) -DXMSSMT $(CFLAGS) -o $@ $(SOURCES) $< $(LDLIBS)

clean:
	-$(RM) $(TESTS)
	-$(RM) $(UI)

benchmark: $(BENCHMARK)
	$(foreach var,$(BENCHMARK),./$(var);)

benchmark_fast: $(BENCHMARK_FAST)
	$(foreach var,$(BENCHMARK_FAST),./$(var);)

benchmark_enc: $(BENCHMARK_ENC)
	$(foreach var,$(BENCHMARK_ENC),./$(var);)

