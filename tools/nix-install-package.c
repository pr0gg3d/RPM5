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

    RPMNIX_FLAGS_INTERACTIVE	= _DFB(24)	/*    --non-interactive */
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

    const char * url;
    const char ** profiles;
};

/**
 */
static struct rpmnix_s _nix = {
	.flags = RPMNIX_FLAGS_INTERACTIVE
};

static const char * tmpDir;
static const char * binDir	= "/usr/bin";

#define DBG(_l) if (_debug) fprintf _l
/*==============================================================*/

static char * _freeCmd(const char * cmd)
{
DBG((stderr, "\t%s\n", cmd));
    cmd = _free(cmd);
    return NULL;
}

#ifdef	REFERENCE
/*
sub barf {
    my $msg = shift;
    print "$msg\n";
    <STDIN> if $interactive;
    exit 1;
}
*/
#endif

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

 { "profile", 'p', POPT_ARG_ARGV,	&_nix.profiles, 0,
	N_("FIXME"), NULL },
 { "non-interactive", '\0', POPT_BIT_CLR, &_nix.flags, RPMNIX_FLAGS_INTERACTIVE,
	N_("FIXME"), NULL },
 { "url", '\0', POPT_ARG_STRING,	&_nix.url, 0,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-install-package (FILE | --url URL)\n\
\n\
Install a Nix Package (.nixpkg) either directly from FILE or by\n\
downloading it from URL.\n\
\n\
Flags:\n\
  --profile / -p LINK: install into the specified profile\n\
  --non-interactive: don't run inside a new terminal\n\
"), NULL },

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    rpmnix nix = &_nix;
    poptContext optCon = rpmioInit(argc, argv, nixInstantiateOptions);
    int ec = 1;		/* assume failure */
    const char * rval;
    const char * cmd;
    const char * s;
#ifdef	UNUSED
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
#endif
    int xx;

const char * version	= "?version?";
const char * manifestURL= "?manifestURL?";
const char * drvName	= "?drvName?";
const char * system	= "?system?";
const char * drvPath	= "?drvPath?";
const char * outPath	= "?outPath?";

const char * source	= "?source?";
const char * pkgFile	= "?pkgFile?";

const char ** extraNixEnvArgs = NULL;

#ifdef	REFERENCE
/*
#! /usr/bin/perl -w

use strict;
use File::Temp qw(tempdir);

my $binDir = $ENV{"NIX_BIN_DIR"} || "/usr/bin";
*/
#endif
    if ((s = getenv("NIX_BIN_DIR"))) binDir = s;

#ifdef	REFERENCE
/*
sub usageError {
    print STDERR <<EOF;
Usage: nix-install-package (FILE | --url URL)

Install a Nix Package (.nixpkg) either directly from FILE or by
downloading it from URL.

Flags:
  --profile / -p LINK: install into the specified profile
  --non-interactive: don't run inside a new terminal
EOF
    ; # '
    exit 1;
}


# Parse the command line arguments.
my @args = @ARGV;
usageError if scalar @args == 0;

my $source;
my $fromURL = 0;
my @extraNixEnvArgs = ();
my $interactive = 1;

while (scalar @args) {
    my $arg = shift @args;
    if ($arg eq "--help") {
        usageError;
    }
    elsif ($arg eq "--url") {
        $fromURL = 1;
    }
    elsif ($arg eq "--profile" || $arg eq "-p") {
        my $profile = shift @args;
        usageError if !defined $profile;
        push @extraNixEnvArgs, "-p", $profile;
    }
    elsif ($arg eq "--non-interactive") {
        $interactive = 0;
    }
    else {
        $source = $arg;
    }
}

usageError unless defined $source;

*/
#endif

    /*
     * Re-execute in a terminal, if necessary, so that if we're executed
     * from a web browser, the user gets to see us.
     */
    if (F_ISSET(nix, INTERACTIVE) && (s=getenv("NIX_HAVE_TERMINAL")) == NULL) {
#ifdef	REFERENCE
/*
    $ENV{"NIX_HAVE_TERMINAL"} = "1";
    $ENV{"LD_LIBRARY_PATH"} = "";
    foreach my $term ("xterm", "konsole", "gnome-terminal", "xterm") {
        exec($term, "-e", "$binDir/nix-install-package", @ARGV);
    }
    die "cannot execute `xterm'";
*/
#endif
    }


#ifdef	REFERENCE
/*
my $tmpDir = tempdir("nix-install-package.XXXXXX", CLEANUP => 1, TMPDIR => 1)
    or die "cannot create a temporary directory";
*/
#endif
    if (!((s = getenv("TMPDIR")) != NULL && *s != '\0'))
	s = "/tmp";
    tmpDir = mkdtemp(rpmGetPath(s, "/nix-pull.XXXXXX", NULL));
    if (tmpDir == NULL) {
	fprintf(stderr, _("cannot create a temporary directory\n"));
	goto exit;
    }

#ifdef	REFERENCE
/*
# Download the package description, if necessary.
my $pkgFile = $source;
if ($fromURL) {
    $pkgFile = "$tmpDir/tmp.nixpkg";
    system("/usr/bin/curl", "--silent", $source, "-o", $pkgFile) == 0
        or barf "curl failed: $?";
}
*/
#endif


#ifdef	REFERENCE
/*
# Read and parse the package file.
open PKGFILE, "<$pkgFile" or barf "cannot open `$pkgFile': $!";
my $contents = <PKGFILE>;
close PKGFILE;
*/
#endif

#ifdef	REFERENCE
/*
my $urlRE = "(?: [a-zA-Z][a-zA-Z0-9\+\-\.]*\:[a-zA-Z0-9\%\/\?\:\@\&\=\+\$\,\-\_\.\!\~\*\']+ )";
my $nameRE = "(?: [A-Za-z0-9\+\-\.\_\?\=]+ )"; # see checkStoreName()
my $systemRE = "(?: [A-Za-z0-9\+\-\_]+ )";
my $pathRE = "(?: \/ [\/A-Za-z0-9\+\-\.\_\?\=]* )";

# Note: $pathRE doesn't check that whether we're looking at a valid
# store path.  We'll let nix-env do that.

$contents =~
    / ^ \s* (\S+) \s+ ($urlRE) \s+ ($nameRE) \s+ ($systemRE) \s+ ($pathRE) \s+ ($pathRE) /x
    or barf "invalid package contents";
my $version = $1;
my $manifestURL = $2;
my $drvName = $3;
my $system = $4;
my $drvPath = $5;
my $outPath = $6;
*/
#endif

#ifdef	REFERENCE
/*
barf "invalid package version `$version'" unless $version eq "NIXPKG1";
*/
#endif


    if (F_ISSET(nix, INTERACTIVE)) {
	/* Ask confirmation. */
#ifdef	REFERENCE
/*
    print "Do you want to install `$drvName' (Y/N)? ";
    my $reply = <STDIN>;
    chomp $reply;
    exit if $reply ne "y" && $reply ne "Y";
*/
#endif
    }


    /*
     * Store the manifest in the temporary directory so that we don't
     * pollute /nix/var/nix/manifests.
     */
#ifdef	REFERENCE
/*
$ENV{NIX_MANIFESTS_DIR} = $tmpDir;
*/
#endif
    xx = setenv("NIX_MANIFEST_DIR", tmpDir, 1);


    fprintf(stdout, "\nPulling manifests...\n");
#ifdef	REFERENCE
/*
system("$binDir/nix-pull", $manifestURL) == 0
    or barf "nix-pull failed: $?";
*/
#endif
    cmd = rpmExpand(binDir, "/nix-pull ", manifestURL, "; echo $?", NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "nix-pull failed: %s\n", rval);
	goto exit;
    }
    rval = _free(rval);
    cmd = _freeCmd(cmd);


    fprintf(stdout, "\nInstalling package...\n");
#ifdef	REFERENCE
/*
system("$binDir/nix-env", "--install", $outPath, "--force-name", $drvName, @extraNixEnvArgs) == 0
    or barf "nix-env failed: $?";
*/
#endif
    s = argvJoin(extraNixEnvArgs, ' ');
    cmd = rpmExpand(binDir, "/nix-env --install ", outPath,
			" --force-name ", drvName, " ", s, "; echo $?", NULL);
    s = _free(s);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "nix-env failed: %s\n", rval);
	goto exit;
    }
    rval = _free(rval);
    cmd = _freeCmd(cmd);


    if (F_ISSET(nix, INTERACTIVE)) {
#ifdef	REFERENCE
/*
    print "\nInstallation succeeded! Press Enter to continue.\n";
    <STDIN>;
*/
#endif
    }

    ec = 0;	/* XXX success */

exit:

#ifdef	REFERENCE
/*
my $tmpDir = tempdir("nix-install-package.XXXXXX", CLEANUP => 1, TMPDIR => 1)
    or die "cannot create a temporary directory";
*/
#endif

    nix->profiles = argvFree(nix->profiles);

    optCon = rpmioFini(optCon);

    return ec;
}