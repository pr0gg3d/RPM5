Summary:	Test DIR -> SYMLINK on upgrade.
Name:		dir2symlink
Version:	1
Release:	1
License:	LGPL
Group:		Amusements/Games
URL:		http://rpm5.org
BuildArch:      noarch

%description
Test DIR -> SYMLINK on upgrade:
    Create a /tmp/DIR.d directory.			(%{name}-1-1)
    Rename DIR.d and create SYMLINK in %pretrans.	(%{name}-1-2)

%package -n	dir2symlink-lua
Version:	1
    
%package -n	dir2symlink-lua
Version:	2
    
%package -n	dir2symlink-python
Version:	1
    
%package -n	dir2symlink-python
Version:	2
    
%package -n	dir2symlink-perl
Version:	1
    
%package -n	dir2symlink-perl
Version:	2
    
%package -n	dir2symlink-ruby
Version:	1
    
%package -n	dir2symlink-ruby
Version:	2
    
%package -n	dir2symlink-tcl
Version:	1
    
%package -n	dir2symlink-tcl
Version:	2
    
%install
INTERPRETERS="lua python perl ruby tcl"
for i in  $INTERPRETERS; do
    mkdir -p %{buildroot}/tmp/$i.d/
    touch %{buildroot}/tmp/$i.d/foo
    touch %{buildroot}/tmp/$i.d/bar
done

# XXX undo tests/macros automatic wrapping
%undefine post
%undefine preun
%undefine pretrans

%if 1
%post -n dir2symlink-lua-1	-p <lua>
print("--- post(lua) arg " .. arg[2] .. " ...")
print("Create a /tmp/lua.d directory.")

%preun -n dir2symlink-lua-2	-p <lua>
print("--- preun(lua) arg " .. arg[2] .. " ...")
if arg[2] <= 1 then
  print("Undo the DIR -> SYMLINK change.")
  local ox = require 'posix'
  posix.unlink("/tmp/lua.d")
  os.rename("/tmp/.lua.d", "/tmp/lua.d")
end

%pretrans -n dir2symlink-lua-2	-p <lua>
print("--- pretrans(lua) arg " .. arg[2] .. " ...")
print("Rename DIR.d -> .DIR.d and create SYMLINK .DIR.d <- DIR.d.")
local ox = require 'posix'
os.rename("/tmp/lua.d", "/tmp/.lua.d")
posix.symlink("/tmp/.lua.d", "/tmp/lua.d")
%endif

%if 1
%post -n dir2symlink-python-1	-p <python>
#XXX <python> attempts to import rpm-python and so is host os dependent
print "--- post(python) arg", sys.argv[1], "..."
print "Create a /tmp/python.d directory."

%preun -n dir2symlink-python-2	-p <python>
print "--- preun(python) arg", sys.argv[1], "..."
import os
if argv[1] <= 1:
  print "Undo the DIR -> SYMLINK change."
  os.unlink("/tmp/python.d")
  os.rename("/tmp/.python.d", "/tmp/python.d")

%pretrans -n dir2symlink-python-2	-p <python>
print "--- pretrans(python) arg", sys.argv[1], " ..."
import os
print "Rename DIR.d -> .DIR.d and create SYMLINK .DIR.d <- DIR.d."
os.rename("/tmp/python.d", "/tmp/.python.d")
os.symlink("/tmp/.python.d", "/tmp/python.d")
%endif

%if 1
%post -n dir2symlink-perl-1	-p <perl>
print "--- post(perl) arg " . $ARGV[1].. " ...";
print "Create a /tmp/perl.d directory.";

%preun -n dir2symlink-perl-2	-p <perl>
print "--- preun(perl) arg " . $ARGV[1] . " ...";
if ($ARGV[1] <= 1) {
  print "Undo the DIR -> SYMLINK change.";
  unlink("/tmp/perl.d");
  rename("/tmp/.perl.d", "/tmp/perl.d");
}

%pretrans -n dir2symlink-perl-2	-p <perl>
print "--- pretrans(perl) arg " . $ARGV[1] . " ...";
print "Rename DIR.d -> .DIR.d and create SYMLINK .DIR.d <- DIR.d.";
rename("/tmp/perl.d", "/tmp/.perl.d");
symlink("/tmp/.perl.d", "/tmp/perl.d");
%endif

%if 1
%post -n dir2symlink-ruby-1	-p <ruby>
#XXX no output displayed by <ruby>
$stderr.puts "--- post(ruby) arg " + ARGV[0] + " ..."
$stderr.puts "Create a /tmp/ruby.d directory."

%preun -n dir2symlink-ruby-2	-p <ruby>
#XXX no output displayed by <ruby>
$stderr.puts "--- preun(ruby) arg " + ARGV[0] + " ..."
if Integer(ARGV[0]) <= 1
  $stderr.puts "Undo the DIR -> SYMLINK change."
  File::unlink("/tmp/ruby.d")
  File::rename("/tmp/.ruby.d", "/tmp/ruby.d")
end

%pretrans -n dir2symlink-ruby-2	-p <ruby>
#XXX no output displayed by <ruby>
$stderr.puts "--- pretrans(ruby) arg " + ARGV[0] + " ..."
$stderr.puts "Rename DIR.d -> .DIR.d and create SYMLINK .DIR.d <- DIR.d."
File::rename("/tmp/ruby.d", "/tmp/.ruby.d")
File::symlink("/tmp/.ruby.d", "/tmp/ruby.d")
%endif

%if 1
%post -n dir2symlink-tcl-1	-p <tcl>
puts stderr "--- post(tcl) arg [lindex $argv 0] ..."
puts stderr "Create a /tmp/tcl.d directory." 

%preun -n dir2symlink-tcl-2	-p <tcl>
puts stderr "--- preun(tcl) arg [lindex $argv 0] ..."
if { [lindex $argv 0] <= 1 } {
  puts stderr "Undo the DIR -> SYMLINK change."
  file delete /tmp/tcl.d
  file rename /tmp/.tcl.d /tmp/tcl.d
}

%pretrans -n dir2symlink-tcl-2	-p <tcl>
puts stderr "--- pretrans(tcl) arg [lindex $argv 0] ..."
puts stderr "Rename DIR.d -> .DIR.d and create SYMLINK .DIR.d <- DIR.d."
file rename /tmp/tcl.d /tmp/.tcl.d
file link -symbolic /tmp/tcl.d /tmp/.tcl.d
%endif

%if 1
%files -n	dir2symlink-lua-1
%dir /tmp/lua.d
/tmp/lua.d/foo

%files -n	dir2symlink-lua-2
%dir /tmp/lua.d
/tmp/lua.d/bar
%endif

%if 1
%files -n	dir2symlink-python-1
%dir /tmp/python.d
/tmp/python.d/foo

%files -n	dir2symlink-python-2
%dir /tmp/python.d
/tmp/python.d/bar
%endif

%if 1
%files -n	dir2symlink-perl-1
%dir /tmp/perl.d
/tmp/perl.d/foo

%files -n	dir2symlink-perl-2
%dir /tmp/perl.d
/tmp/perl.d/bar
%endif

%if 1
%files -n	dir2symlink-ruby-1
%dir /tmp/ruby.d
/tmp/ruby.d/foo

%files -n	dir2symlink-ruby-2
%dir /tmp/ruby.d
/tmp/ruby.d/bar
%endif

%if 1
%files -n	dir2symlink-tcl-1
%dir /tmp/tcl.d
/tmp/tcl.d/foo

%files -n	dir2symlink-tcl-2
%dir /tmp/tcl.d
/tmp/tcl.d/bar
%endif

%changelog
* Fri Mar 16 2012 Jeff Johnson <jbj@rpm5.org> 1-1
- Simplify DIR -> SYMLINK on upgrade using %%pretrans -p <lua>.

* Mon Aug  4 2010 Elia Pinto <devzero2000@rpm5.org> 1-1
- First Build.