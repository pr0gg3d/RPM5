#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>
#include <poptIO.h>

#include "debug.h"

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

    RPMNIX_FLAGS_SIGN		= _DFB(24),	/*    --sign */
    RPMNIX_FLAGS_GZIP		= _DFB(25)	/*    --gzip */
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

    int op;
    const char * sshHost;

    const char ** storePaths;
    const char ** allStorePaths;
    const char ** missing;
};

/**
 */
static struct rpmnix_s _nix = {
	.flags = RPMNIX_FLAGS_NOOUTLINK
};

static const char * binDir = "/usr/bin";

/*==============================================================*/

#ifdef	UNUSED
static int verbose = 0;
#endif

enum {
    NIX_FROM_HOST = 1,
    NIX_TO_HOST,
};

static void nixInstantiateArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_FROM_HOST:
	nix->op = opt->val;
	nix->sshHost = xstrdup(arg);
	break;
    case NIX_TO_HOST:
	nix->op = opt->val;
	nix->sshHost = xstrdup(arg);
	break;
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

 { "from", '\0', POPT_ARG_STRING,	0, NIX_FROM_HOST,
	N_("FIXME"), NULL },
 { "to", '\0', POPT_ARG_STRING,		0, NIX_TO_HOST,
	N_("FIXME"), NULL },
 { "gzip", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_GZIP,
	N_("FIXME"), NULL },
 { "sign", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_SIGN,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-copy-closure [--from | --to] HOSTNAME [--sign] [--gzip] PATHS...\n\
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
    const char * cmd;
    const char * rval;
    const char * sshOpts = "";			/* XXX HACK */
    const char * compressor = "";
    const char * decompressor = "";
    const char * extraOpts = "";
    int nac;
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    int xx;

    if ((s = getenv("NIX_BIN_DIR"))) binDir = s;

    if (ac < 1) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    if (nix->op == 0)
	nix->op = NIX_TO_HOST;

    xx = argvAppend(&nix->storePaths, av);
    if (F_ISSET(nix, GZIP)) {
	compressor = "| gzip";
	decompressor = "gunzip |";
    }
    if (F_ISSET(nix, SIGN)) {
	extraOpts = " --sign";
    }

#ifdef	REFERENCE
/*
openSSHConnection $sshHost or die "$0: unable to start SSH\n";
*/
#endif


    switch (nix->op) {
    case NIX_TO_HOST:		/* Copy TO the remote machine. */

	/* Get the closure of this path. */
#ifdef	REFERENCE
/*
    my $pid = open(READ, "$binDir/nix-store --query --requisites @storePaths|") or die;
    
    while (<READ>) {
        chomp;
        die "bad: $_" unless /^\//;
        push @allStorePaths, $_;
    }

    close READ or die "nix-store failed: $?";
*/
#endif
	s = argvJoin(nix->storePaths, ' ');
        cmd = rpmExpand(binDir, "/nix-store --query --requisites ", s, NULL);
	s = _free(s);
        rval = rpmExpand("%(", cmd, ")", NULL);
        cmd = _free(cmd);
        xx = argvSplit(&nix->allStorePaths, rval, NULL);
        rval = _free(rval);

	/* Ask the remote host which paths are invalid. */
#ifdef	REFERENCE
/*
    open(READ, "ssh $sshHost @sshOpts nix-store --check-validity --print-invalid @allStorePaths|");
    my @missing = ();
    while (<READ>) {
        chomp;
        push @missing, $_;
    }
    close READ or die;
*/
#endif
	s = argvJoin(nix->allStorePaths, ' ');
        cmd = rpmExpand("ssh ", nix->sshHost, " ", sshOpts, " nix-store --check-validity --print-invalid ", s, NULL);
	s = _free(s);
#ifdef	NOTYET
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->missing, rval, NULL);
        rval = _free(rval);
#else
fprintf(stderr, "--> %s\n", cmd);
nix->missing = NULL;
fprintf(stderr, "<-- missing assumed NULL\n");
#endif
        cmd = _free(cmd);

	/* Export the store paths and import them on the remote machine. */
	nac = argvCount(nix->missing);
	if (nac > 0) {
argvPrint("copying these missing paths:", nix->missing, NULL);
#ifdef	REFERENCE
/*
        my $extraOpts = "";
        $extraOpts .= "--sign" if $sign == 1;
        system("nix-store --export $extraOpts @missing $compressor | ssh $sshHost @sshOpts '$decompressor nix-store --import'") == 0
            or die "copying store paths to remote machine `$sshHost' failed: $?";
*/
#endif
	    s = argvJoin(nix->missing, ' ');
	    cmd = rpmExpand(binDir, "/nix-store --export ", extraOpts, " ", s, " ", compressor,
		" | ssh ", nix->sshHost, " ", sshOpts, " '", decompressor, " nix-store --import'", NULL);
	    s = _free(s);
fprintf(stderr, "--> %s\n", cmd);
	    cmd = _free(cmd);
	}
	break;
    case NIX_FROM_HOST:		/* Copy FROM the remote machine. */

	/*
	 * Query the closure of the given store paths on the remote
	 * machine.  Paths are assumed to be store paths; there is no
	 * resolution (following of symlinks).
	 */
#ifdef	REFERENCE
/*
    my $pid = open(READ,
        "ssh @sshOpts $sshHost nix-store --query --requisites @storePaths|") or die;
    
    my @allStorePaths;

    while (<READ>) {
        chomp;
        die "bad: $_" unless /^\//;
        push @allStorePaths, $_;
    }

    close READ or die "nix-store on remote machine `$sshHost' failed: $?";
*/
#endif
	s = argvJoin(nix->storePaths, ' ');
        cmd = rpmExpand("ssh ", sshOpts, " ", nix->sshHost, " nix-store --query --requisites ", s, NULL);
	s = _free(s);
#ifdef	NOTYET
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->missing, rval, NULL);
        rval = _free(rval);
#else
fprintf(stderr, "--> %s\n", cmd);
nix->allStorePaths = NULL;
fprintf(stderr, "<-- allStorePaths assumed NULL\n");
#endif
        cmd = _free(cmd);

	/* What paths are already valid locally? */
#ifdef	REFERENCE
/*
    open(READ, "/usr/bin/nix-store --check-validity --print-invalid @allStorePaths|");
    my @missing = ();
    while (<READ>) {
        chomp;
        push @missing, $_;
    }
    close READ or die;
*/
#endif
	s = argvJoin(nix->allStorePaths, ' ');
        cmd = rpmExpand(binDir, "/nix-store --check-validity --print-invalid ", s, NULL);
	s = _free(s);
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->missing, rval, NULL);
        rval = _free(rval);
        cmd = _free(cmd);

	/* Export the store paths on the remote machine and import them on locally. */
	nac = argvCount(nix->missing);
	if (nac > 0) {
argvPrint("copying these missing paths:", nix->missing, NULL);
#ifdef	REFERENCE
/*
        system("ssh $sshHost @sshOpts 'nix-store --export $extraOpts @missing $compressor' | $decompressor /usr/bin/nix-store --import") == 0
            or die "copying store paths from remote machine `$sshHost' failed: $?";
*/
#endif
	    s = argvJoin(nix->missing, ' ');
	    cmd = rpmExpand("ssh ", nix->sshHost, " ", sshOpts,
		" 'nix-store --export ", extraOpts, " ", s, " ", compressor,
		"' | ", decompressor, " ", binDir, "/nix-store --import", NULL);
	    s = _free(s);
fprintf(stderr, "--> %s\n", cmd);
	    cmd = _free(cmd);
	}
	break;
    }

    ec = 0;	/* XXX success */

exit:

    nix->storePaths = argvFree(nix->storePaths);
    nix->sshHost = _free(nix->sshHost);

    optCon = rpmioFini(optCon);

    return ec;
}