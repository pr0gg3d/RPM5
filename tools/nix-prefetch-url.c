#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>
#include <poptIO.h>

#include "debug.h"

static int _debug = -1;

#define _KFB(n) (1U << (n))
#define _DFB(n) (_KFB(n) | 0x40000000)

#define F_ISSET(_nix, _FLAG) ((_nix)->flags & ((RPMNIX_FLAGS_##_FLAG) & ~0x40000000))

/**
 * Bit field enum for rpmdigest CLI options.
 */
enum nixFlags_e {
    RPMNIX_FLAGS_NONE		= 0,
    RPMNIX_FLAGS_ADDDRVLINK	= _DFB(0),	/*    --add-drv-link */
    RPMNIX_FLAGS_NOOUTLINK	= _DFB(1),	/* -o,--no-out-link */
    RPMNIX_FLAGS_DRYRUN		= _DFB(2),	/*    --dry-run */

    RPMNIX_FLAGS_EVALONLY	= _DFB(16),	/*    --eval-only */
    RPMNIX_FLAGS_PARSEONLY	= _DFB(17),	/*    --parse-only */
    RPMNIX_FLAGS_ADDROOT	= _DFB(18),	/*    --add-root */
    RPMNIX_FLAGS_XML		= _DFB(19),	/*    --xml */
    RPMNIX_FLAGS_STRICT		= _DFB(20),	/*    --strict */
    RPMNIX_FLAGS_SHOWTRACE	= _DFB(21),	/*    --show-trace */

};

/**
 */
typedef struct rpmnix_s * rpmnix;

/**
 */
struct rpmnix_s {
    enum nixFlags_e flags;	/*!< rpmnix control bits. */

    const char * outLink;
    const char * drvLink;

    const char ** instArgs;
    const char ** buildArgs;
    const char ** exprs;

    const char * attr;

    int quiet;
    int print;
    const char * nixPkgs;
    const char * downloadCache;
    const char * url;
    const char * expHash;
    const char * hashType;
    const char * hashFormat;
/*@only@*/
    const char * hash;
/*@only@*/
    const char * finalPath;
/*@only@*/
    const char * tmpPath;
/*@only@*/
    const char * tmpFile;
/*@only@*/
    const char * name;
    const char * cacheFlags;
    const char * cachedHashFN;
    const char * cachedTimestampFN;
    const char * urlHash;
};

/**
 */
static struct rpmnix_s _nix = {
	.hashType = "sha256"
};

static const char * tmpDir	= "/tmp";
static const char * binDir	= "/usr/bin";

#define	DBG(_l)	if (_debug) fprintf _l
/*==============================================================*/

static char * _freeCmd(const char * cmd)
{
DBG((stderr, "\t%s\n", cmd));
    cmd = _free(cmd);
    return NULL;
}

/**
 * Copy source string to target, substituting for URL characters.
 * @param t		target xml string
 * @param s		source string
 * @return		target xml string
 */
static char * urlStrcpy(/*@returned@*/ char * t, const char * s)
	/*@modifies t @*/
{
    char * te = t;
    int c;

/*
 * Handle escaped characters in the URI.  `+', `=' and `?' are the only
 * characters that are valid in Nix store path names but have a special
 * meaning in URIs.
 */
    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '%':
	    if (s[0] == '2' && s[1] == 'b')	c = (int)'+';
	    if (s[0] == '3' && s[1] == 'd')	c = (int)'=';
	    if (s[0] == '3' && s[1] == 'f')	c = (int)'?';
	    if (c != (int)s[-1])
		s += 2;
	    break;
	}
	*te++ = (char) c;
    }
    *te = '\0';
    return t;
}

static void mkTempDir(rpmnix nix)
	/*@*/
{
    if (nix->tmpPath == NULL) {
	nix->tmpPath =
		mkdtemp(rpmGetPath(tmpDir, "/nix-prefetch-url-XXXXXX",NULL));
assert(nix->tmpPath != NULL);
    }

DBG((stderr, "<-- %s(%p) tmpDir %s\n", __FUNCTION__, nix, nix->tmpPath));

}

static void removeTempDir(rpmnix nix)
	/*@*/
{
    const char * cmd;
    const char * rval;
    struct stat sb;

    if (nix->tmpPath == NULL) return;
    if (Stat(nix->tmpPath, &sb) < 0) return;

    cmd = rpmExpand("/bin/rm -rf '", nix->tmpPath, "'; echo $?", NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "failed to remove %s\n", nix->tmpPath);
    }
    rval = _free(rval);

DBG((stderr, "<-- %s(%p)\n", __FUNCTION__, nix));

    cmd = _freeCmd(cmd);
}

static void doDownload(rpmnix nix)
	/*@*/
{
    const char * cmd;
    const char * rval;

    cmd = rpmExpand("/usr/bin/curl ", nix->cacheFlags,
	" --fail --location --max-redirs 20 --disable-epsv",
	" --cookie-jar ", nix->tmpPath, "/cookies ", nix->url,
	" -o ", nix->tmpFile, NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    rval = _free(rval);

DBG((stderr, "<-- %s(%p)\n", __FUNCTION__, nix));

    cmd = _freeCmd(cmd);
}

/*==============================================================*/

#ifdef	UNUSED
static int verbose = 0;
#endif

static void nixInstantiateArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
#ifdef	UNUSED
    rpmnix nix = &_nix;
#endif

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

static struct poptOption nixInstantiateOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	nixInstantiateArgCallback, 0, NULL, NULL },
/*@=type@*/

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
syntax: nix-prefetch-url URL [EXPECTED-HASH]\n\
"), NULL },

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    rpmnix nix = &_nix;
    poptContext optCon = rpmioInit(argc, argv, nixInstantiateOptions);
    int ec = 1;		/* assume failure */
    const char * s;
    char * fn;
    char * cmd;
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    struct stat sb;
    FD_t fd;
    int xx;

#ifdef	REFERENCE
/*
    trap removeTempDir EXIT SIGINT SIGQUIT
*/
#endif

    if ((s = getenv("NIX_BIN_DIR"))) binDir = s;

    if ((s = getenv("TMPDIR")) && *s) tmpDir = s;

    if ((s = getenv("QUIET")) && *s) nix->quiet = 1;
    if ((s = getenv("PRINT_PATH")) && *s) nix->print = 1;

    switch (ac) {
    default:	poptPrintUsage(optCon, stderr, 0);	goto exit;
	/*@notreached@*/
    case 2:	nix->expHash = av[1];	/*@fallthrough@*/
    case 1:	nix->url = av[0];	break;
    }
    if ((s = getenv("NIX_HASH_ALGO")))	nix->hashType = s;
    nix->hashFormat = (strcmp(nix->hashType, "md5") ? "--base32" : "");
    if ((s = getenv("NIX_DOWNLOAD_CACHE"))) nix->downloadCache = s;

nix->cacheFlags = xstrdup("");

    /*
     * Handle escaped characters in the URI.  `+', `=' and `?' are the only
     * characters that are valid in Nix store path names but have a special
     * meaning in URIs.
     */
    fn = xstrdup(nix->url);
    s = basename(fn);
    if (s == NULL || *s == '\0') {
	fprintf(stderr, _("invalid url: %s\n"), nix->url);
	goto exit;
    }
    nix->name = urlStrcpy(xstrdup(s), s);
    fn = _free(fn);

    /* If expected hash specified, check if it already exists in the store. */
    if (nix->expHash != NULL) {

	cmd = rpmExpand(binDir, "/nix-store --print-fixed-path ",
		nix->hashType, " ", nix->expHash, " ", nix->name, NULL);
	nix->finalPath = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);

	cmd = rpmExpand(binDir, "/nix-store --check-validity ",
		nix->finalPath, " 2>/dev/null; echo $?", NULL);
	s = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);
	if (strcmp(s, "0"))
	    nix->finalPath = _free(nix->finalPath);
	s = _free(s);
	nix->hash = xstrdup(nix->expHash);
    }

    /* Hack to support the mirror:// scheme from Nixpkgs. */
    if (!strncmp(nix->url, "mirror://", sizeof("mirror://")-1)) {
	ARGV_t expanded = NULL;
	if ((s = getenv("NIXPKGS_ALL")) || *s == '\0') {
            fprintf(stderr, _("Resolving mirror:// URLs requires Nixpkgs.  Please point $NIXPKGS_ALL at a Nixpkgs tree.\n"));
	    goto exit;
	}
	nix->nixPkgs = s;

	mkTempDir(nix);

	cmd = rpmExpand(binDir, "/nix-build ", nix->nixPkgs,
		" -A resolveMirrorURLs --argstr url ", nix->url,
		" -o ", nix->tmpPath, "/urls > /dev/null", NULL);
	s = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);
	s = _free(s);

	fn = rpmGetPath(nix->tmpPath, "/urls", NULL);
	fd = Fopen(fn, "r.fpio");
	if (fd == NULL || Ferror(fd)) {
	    fprintf(stderr, _("Fopen(%s, \"r\") failed\n"), fn);
	    if (fd) xx = Fclose(fd);
	    fn = _free(fn);
	    goto exit;
	}
	xx = argvFgets(&expanded, fd);
	xx = Fclose(fd);

	fn = _free(fn);

	if (argvCount(expanded) == 0) {
	    fprintf(stderr, _("%s: cannot resolve %s\n"), __progname, nix->url);
	    expanded = argvFree(expanded);
	    goto exit;
	}

	fprintf(stderr, _("url %s replaced with %s\n"), nix->url, expanded[0]);
	nix->url = xstrdup(expanded[0]);
	expanded = argvFree(expanded);
    }

    /*
     * If we don't know the hash or a file with that hash doesn't exist,
     * download the file and add it to the store.
     */
    if (nix->finalPath == NULL) {

	mkTempDir(nix);

	nix->tmpFile = rpmGetPath(nix->tmpPath, "/", nix->name, NULL);

	/*
	 * Optionally do timestamp-based caching of the download.
	 * Actually, the only thing that we cache in $NIX_DOWNLOAD_CACHE is
	 * the hash and the timestamp of the file at $url.  The caching of
	 * the file *contents* is done in Nix store, where it can be
	 * garbage-collected independently.
	 */
	if (nix->downloadCache) {

	    fn = rpmGetPath(nix->tmpPath, "/url", NULL);
	    fd = Fopen(fn, "w");
	    if (fd == NULL || Ferror(fd)) {
		fprintf(stderr, _("Fopen(%s, \"w\") failed\n"), fn);
		if (fd) xx = Fclose(fd);
		fn = _free(fn);
		goto exit;
	    }
	    xx = Fwrite(nix->url, 1, strlen(nix->url), fd);
	    xx = Fclose(fd);
	    fn = _free(fn);

	    cmd = rpmExpand(binDir, "/nix-hash --type sha256 --base32 --flat ",
			nix->tmpPath, "/url", NULL);
	    nix->urlHash = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    fn = rpmGetPath(nix->downloadCache,"/", nix->urlHash, ".url", NULL);
	    fd = Fopen(fn, "w");
	    if (fd == NULL || Ferror(fd)) {
		fprintf(stderr, _("Fopen(%s, \"w\") failed\n"), fn);
		if (fd) xx = Fclose(fd);
		fn = _free(fn);
		goto exit;
	    }
	    xx = Fwrite(nix->url, 1, strlen(nix->url), fd);
	    xx = Fwrite("\n", 1, 1, fd);
	    xx = Fclose(fd);
	    fn = _free(fn);

	    nix->cachedHashFN = rpmGetPath(nix->downloadCache, "/",
			nix->urlHash, ".", nix->hashType, NULL);
	    nix->cachedTimestampFN = rpmGetPath(nix->downloadCache, "/",
			nix->urlHash, ".stamp", NULL);
	    nix->cacheFlags = _free(nix->cacheFlags);
	    nix->cacheFlags = xstrdup("--remote-time");

	    if (!Stat(nix->cachedTimestampFN, &sb)
	     && !Stat(nix->cachedHashFN, &sb))
	    {
		/* Only download the file if newer than the cached version. */
		s = nix->cacheFlags;
		nix->cacheFlags = rpmExpand(nix->cacheFlags,
			" --time-cond ", nix->cachedTimestampFN, NULL);
	    }
	}

	/* Perform the download. */
	doDownload(nix);

	if (nix->downloadCache && Stat(nix->tmpFile, &sb) < 0) {
	    /* 
	     * Curl didn't create $tmpFile, so apparently there's no newer
	     * file on the server.
	     */
	    nix->hash = _free(nix->hash);
	    cmd = rpmExpand("cat ", nix->cachedHashFN, NULL);
	    nix->hash = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    nix->finalPath = _free(nix->finalPath);
	    cmd = rpmExpand(binDir, "/nix-store --print-fixed-path ",
			nix->hashType, " ", nix->hash, " ", nix->name, NULL);
	    nix->finalPath = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    cmd = rpmExpand(binDir, "/nix-store --check-validity ",
			nix->finalPath, " 2>/dev/null; echo $?", NULL);
	    s = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);
	    if (strcmp(s, "0")) {
		fprintf(stderr, _("cached contents of `%s' disappeared, redownloading...\n"), nix->url);
		nix->finalPath = _free(nix->finalPath);
		nix->cacheFlags = _free(nix->cacheFlags);
		nix->cacheFlags = xstrdup("--remote-time");
		doDownload(nix);
	    }
	    s = _free(s);
	}

	if (nix->finalPath == NULL) {

	    /* Compute the hash. */
	    nix->hash = _free(nix->hash);
	    cmd = rpmExpand(binDir, "/nix-hash --type ", nix->hashType,
			" ", nix->hashFormat, " --flat ", nix->tmpFile, NULL);
	    nix->hash = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    if (!nix->quiet)
		fprintf(stderr, _("hash is %s\n"), nix->hash);

	    if (nix->downloadCache) {
		struct timeval times[2];

		fn = rpmGetPath(nix->cachedHashFN, NULL);
		fd = Fopen(fn, "w");
		if (fd == NULL || Ferror(fd)) {
		    fprintf(stderr, _("Fopen(%s, \"w\") failed\n"), fn);
		    if (fd) xx = Fclose(fd);
		    fn = _free(fn);
		    goto exit;
		}
		xx = Fwrite(nix->hash, 1, strlen(nix->hash), fd);
		xx = Fwrite("\n", 1, 1, fd);
		xx = Fclose(fd);
		fn = _free(fn);

		if (Stat(nix->tmpFile, &sb)) {
		    fprintf(stderr, _("Stat(%s) failed\n"), nix->tmpFile);
		    goto exit;
		}
		times[1].tv_sec  = times[0].tv_sec  = sb.st_mtime;
		times[1].tv_usec = times[0].tv_usec = 0;
		xx = Utimes(nix->cachedTimestampFN, times);

	    }

	    /* Add the downloaded file to the Nix store. */
	    nix->finalPath = _free(nix->finalPath);
	    cmd = rpmExpand(binDir, "/nix-store --add-fixed ", nix->hashType,
			" ", nix->tmpFile, NULL);
	    nix->finalPath = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    if (nix->expHash && *nix->expHash
	     && strcmp(nix->expHash, nix->hash))
	    {
		fprintf(stderr, "hash mismatch for URL `%s'\n", nix->url);
		goto exit;
	    }
	}
    }

    if (!nix->quiet)
	fprintf(stderr, _("path is %s\n"), nix->finalPath);
    fprintf(stdout, "%s\n", nix->hash);
    if (nix->print)
	fprintf(stdout, _("%s\n"), nix->finalPath);

    ec = 0;	/* XXX success */

exit:

    removeTempDir(nix);

    nix->hash = _free(nix->hash);
    nix->finalPath = _free(nix->finalPath);
    nix->tmpFile = _free(nix->tmpFile);
    nix->name = _free(nix->name);

    optCon = rpmioFini(optCon);

    return ec;
}