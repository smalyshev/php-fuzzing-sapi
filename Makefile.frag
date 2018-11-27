fuzzer: $(PHP_FUZZER_BINARIES)

$(SAPI_FUZZER_PATH)/php-fuzz-parser: $(PHP_GLOBAL_OBJS) $(PHP_SAPI_OBJS) $(PHP_FUZZER_PARSER_OBJS)
	$(LIBTOOL) --mode=link $(CC) -export-dynamic $(CFLAGS_CLEAN) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS_PROGRAM) $(LDFLAGS) $(PHP_RPATHS) $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_FUZZER_PARSER_OBJS) $(EXTRA_LIBS) $(ZEND_EXTRA_LIBS) -lFuzzer -o $(SAPI_FUZZER_PATH)/php-fuzz-parser

$(SAPI_FUZZER_PATH)/php-fuzz-unserialize: $(PHP_GLOBAL_OBJS) $(PHP_SAPI_OBJS) $(PHP_FUZZER_UNSERIALIZE_OBJS)
	$(LIBTOOL) --mode=link $(CC) -export-dynamic $(CFLAGS_CLEAN) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS_PROGRAM) $(LDFLAGS) $(PHP_RPATHS) $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_FUZZER_UNSERIALIZE_OBJS) $(EXTRA_LIBS) $(ZEND_EXTRA_LIBS) -lFuzzer -o $(SAPI_FUZZER_PATH)/php-fuzz-unserialize

$(SAPI_FUZZER_PATH)/php-fuzz-json: $(PHP_GLOBAL_OBJS) $(PHP_SAPI_OBJS) $(PHP_FUZZER_JSON_OBJS)
	$(LIBTOOL) --mode=link $(CC) -export-dynamic $(CFLAGS_CLEAN) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS_PROGRAM) $(LDFLAGS) $(PHP_RPATHS) $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_FUZZER_JSON_OBJS) $(EXTRA_LIBS) $(ZEND_EXTRA_LIBS) -lFuzzer -o $(SAPI_FUZZER_PATH)/php-fuzz-json
