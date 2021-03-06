/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2017, Johannes Schlüter <johannes@schlueters.de>       |
  | Copyright (c) 2019, Stanislav Malyshev <stas@php.net>                |
  | All rights reserved.                                                 |
  +----------------------------------------------------------------------+
  | Redistribution and use in source and binary forms, with or without   |
  | modification, are permitted provided that the conditions which are   |
  | bundled with this package in the file LICENSE.                       |
  | This product includes PHP software, freely available from            |
  | <http://www.php.net/software/>                                       |
  +----------------------------------------------------------------------+
*/

#include <main/php.h>
#include <main/php_main.h>
#include <main/SAPI.h>
#include <ext/standard/info.h>
#include <ext/standard/php_var.h>
#include <main/php_variables.h>
#ifdef JO0
#include <ext/standard/php_smart_str.h>
#endif

#include "fuzzer.h"
#include "fuzzer-sapi.h"

const char HARDCODED_INI[] =
	"html_errors=0\n"
	"implicit_flush=1\n"
	"output_buffering=0\n";

static int startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module, NULL, 0)==FAILURE) {
		return FAILURE;
	}
	return SUCCESS;
}

static size_t ub_write(const char *str, size_t str_length TSRMLS_DC)
{
	/* quiet */
	return str_length;
}

static void fuzzer_flush(void *server_context)
{
	/* quiet */
}

static void send_header(sapi_header_struct *sapi_header, void *server_context TSRMLS_DC)
{
}

static char* read_cookies(TSRMLS_D)
{
	/* TODO: fuzz these! */
	return NULL;
}

static void register_variables(zval *track_vars_array TSRMLS_DC)
{
	php_import_environment_variables(track_vars_array TSRMLS_CC);
}

static void log_message(char *message, int level TSRMLS_DC)
{
}


static sapi_module_struct fuzzer_module = {
	"fuzzer",               /* name */
	"clang fuzzer", /* pretty name */

	startup,             /* startup */
	php_module_shutdown_wrapper,   /* shutdown */

	NULL,                          /* activate */
	NULL,                          /* deactivate */

	ub_write,            /* unbuffered write */
	fuzzer_flush,               /* flush */
	NULL,                          /* get uid */
	NULL,                          /* getenv */

	php_error,                     /* error handler */

	NULL,                          /* header handler */
	NULL,                          /* send headers handler */
	send_header,         /* send header handler */

	NULL,                          /* read POST data */
	read_cookies,        /* read Cookies */

	register_variables,  /* register server variables */
	log_message,         /* Log message */
	NULL,                          /* Get request time */
	NULL,                          /* Child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
};

int fuzzer_init_php()
{
	sapi_startup(&fuzzer_module);
	fuzzer_module.phpinfo_as_text = 1;

	fuzzer_module.ini_entries = malloc(sizeof(HARDCODED_INI));
	memcpy(fuzzer_module.ini_entries, HARDCODED_INI, sizeof(HARDCODED_INI));

	/*
	 * TODO: we might want to test both Zend and malloc MM, but testing with malloc
	 * is more likely to find bugs, so use that for now.
	 */
	putenv("USE_ZEND_ALLOC=0");

#ifdef __SANITIZE_ADDRESS__
	/* Not very interested in memory leak detection, since Zend MM does that */
	__lsan_disable();
#endif

	if (fuzzer_module.startup(&fuzzer_module)==FAILURE) {
		return FAILURE;
	}

	return SUCCESS;
}

void fuzzer_set_ini_file(const char *file)
{
	if (fuzzer_module.php_ini_path_override) {
		free(fuzzer_module.php_ini_path_override);
	}
	fuzzer_module.php_ini_path_override = strdup(file);
}


int fuzzer_shutdown_php()
{
	TSRMLS_FETCH();

	php_module_shutdown(TSRMLS_C);
	sapi_shutdown();

	free(fuzzer_module.ini_entries);
	return SUCCESS;
}

int fuzzer_do_request(zend_file_handle *file_handle, char *filename)
{
	int retval = FAILURE; /* failure by default */

	SG(options) |= SAPI_OPTION_NO_CHDIR;
	SG(request_info).argc=0;
	SG(request_info).argv=NULL;

	if (php_request_startup(TSRMLS_C)==FAILURE) {
		php_module_shutdown(TSRMLS_C);
		return FAILURE;
	}

	SG(headers_sent) = 1;
	SG(request_info).no_headers = 1;
	php_register_variable("PHP_SELF", filename, NULL TSRMLS_CC);

	zend_first_try {
		zend_compile_file(file_handle, ZEND_REQUIRE);
		/*retval = php_execute_script(file_handle TSRMLS_CC);*/
	} zend_end_try();

	php_request_shutdown((void *) 0);

	return (retval == SUCCESS) ? SUCCESS : FAILURE;
}


int fuzzer_do_request_f(char *filename)
{
	zend_file_handle file_handle;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.filename = filename;
	file_handle.handle.fp = NULL;
	file_handle.opened_path = NULL;
	file_handle.free_filename = 0;

	return fuzzer_do_request(&file_handle, filename);
}

int fuzzer_do_request_d(char *filename, char *data, size_t data_len)
{
	zend_file_handle file_handle;
	file_handle.filename = filename;
	file_handle.opened_path = NULL;
	file_handle.free_filename = 0;
	file_handle.handle.stream.handle = NULL;
	file_handle.handle.stream.reader = (zend_stream_reader_t)_php_stream_read;
	file_handle.handle.stream.fsizer = NULL;
	file_handle.handle.stream.isatty = 0;
	file_handle.handle.stream.closer   = NULL;
	file_handle.handle.stream.mmap.buf = data;
	file_handle.handle.stream.mmap.len = data_len;
	file_handle.type = ZEND_HANDLE_MAPPED;

	return fuzzer_do_request(&file_handle, filename);
}

// Call named PHP function with N zval arguments
void fuzzer_call_php_func_zval(const char *func_name, int nargs, zval *args) {
	zval retval, func;
	int result;

	ZVAL_STRING(&func, func_name);
	ZVAL_UNDEF(&retval);
	result = call_user_function(CG(function_table), NULL, &func, &retval, nargs, args);

	// TODO: check result?
	/* to ensure retval is not broken */
	php_var_dump(&retval, 0);

	/* cleanup */
	zval_ptr_dtor(&retval);
	ZVAL_UNDEF(&retval);
	zval_ptr_dtor(&func);
	ZVAL_UNDEF(&func);
}

// Call named PHP function with N string arguments
void fuzzer_call_php_func(const char *func_name, int nargs, char **params) {
	zval args[nargs];
	int i;

	for(i=0;i<nargs;i++) {
		ZVAL_STRING(&args[i], params[i]);
	}

	fuzzer_call_php_func_zval(func_name, nargs, args);

	for(i=0;i<nargs;i++) {
		zval_ptr_dtor(&args[i]);
		ZVAL_UNDEF(&args[i]);
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
