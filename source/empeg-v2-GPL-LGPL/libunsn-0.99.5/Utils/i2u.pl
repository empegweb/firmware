#! @PATH_TO_PERL@ -w

#
# inetd2userver -- translate inetd.conf to a script that uses userver
# Copyright (C) 2000  Andrew Main
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

use IO::File;

sub unsn_encode {
	my($s) = @_;
	$s =~ s/([^-_.:a-zA-Z0-9])/sprintf("%%%02X", ord($1))/seg;
	$s;
}

sub sh_quote {
	my($s) = @_;
	if($s =~ /[^-+_=:@%,.\/a-zA-Z0-9]/s) {
		$s =~ s/'/\\'/sg;
		$s =~ s/(\\')+/'$1'/sg;
		$s = "'".$s."'";
		$s =~ s/^''//s;
		$s =~ s/''\Z(?!.)//s;
	}
	$s eq '' ? "''" : $s;
}

sub perl_quote {
	my($s) = @_;
	$s =~ s/([\\\"\$\@])/\\$1/sg;
	$s =~ s/\a/\\a/sg;
	$s =~ s/\e/\\e/sg;
	$s =~ s/\f/\\f/sg;
	$s =~ s/\n/\\n/sg;
	$s =~ s/\r/\\r/sg;
	$s =~ s/\t/\\t/sg;
	$s =~ s/([^ -~])/sprintf("\\%03o", ord($1))/seg;
	"\"".$s."\"";
}

$progname = $0;
$progname =~ s|^.*/||;

if($#ARGV > 0) {
	print STDERR "$progname: usage: $progname [<conf-file>]\n";
	exit 2;
}

$conffile = defined($ARGV[0]) ? $ARGV[0] : "/etc/inetd.conf";
$conf = new IO::File $conffile, "r" or die "$progname: $conffile: $!\n";

$inetd_remnants = "";
print "#! /bin/sh\n";
while(<$conf>) {
	s/^[ \t]+//;
	chomp;
	s/[ \t]+$//;
	next if /^(#|$)/;
	$extra_cmds = "";
	($port, $type, $protocol, $waitflag, $user, $path, @args) =
		split /[ \t]+/;
	$ok = defined($path);
	if($ok) {
		if($type eq 'stream') {
			$t = 'stream';
		} elsif($type eq 'dgram' || $type eq 'rdm' || $type eq 'raw') {
			$t = 'dgram';
		}
	}
	if($ok && $path eq 'internal') {
		if($port eq 'echo' && $t eq 'stream') {
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = 'cat', '-u');
		} elsif($port eq 'echo' && $t eq 'dgram') {
			$t = 'stream';
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = 'cat');
		} elsif($port eq 'discard' && $t eq 'stream') {
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = '/bin/sh', '-c',
					'exec cat > /dev/null');
		} elsif($port eq 'discard' && $t eq 'dgram') {
			$waitflag =~ /^nowait(\.\d+)?\Z(?!.)/s and
				$waitflag = 'wait';
			@args = ($path = '/bin/sh', '-c',
					'exec cat > /dev/null');
		} elsif($port eq 'time' && ($t eq 'stream' || $t eq 'dgram')) {
			$t = 'stream';
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = 'perl', '-e',
					'print pack "N", time+2208988800;');
		} elsif($port eq 'daytime' &&
				($t eq 'stream' || $t eq 'dgram')) {
			$t = 'stream';
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = 'date', "+%c\r");
		} elsif($port eq 'chargen' && $t eq 'stream') {
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = 'perl', '-e',
				'for($|=1;++$n;$n%=95){'.
					'for($i=$_="";$i++<72;){'.
						'$_.=chr 32+($n+$i-2)%95'.
					'}print"$_\r\n"'.
				'}');
		} elsif($port eq 'chargen' && $t eq 'dgram') {
			$t = 'stream';
			$waitflag eq 'wait' and $waitflag = 'nowait';
			@args = ($path = 'perl', '-e',
				'for(;$i++<72;){'.
					'$_.=chr 32+($$/2+$i)%95'.
				'}print"$_\r\n"');
		} else {
			$ok = 0;
		}
	} elsif($ok) {
		if($path !~ /^\//) {
			$path = '/'.$path;
		}
	}
	if($ok) {
		if($waitflag =~ /^nowait(\.\d+)?\Z(?!.)/s && $t eq 'stream') {
			if($waitflag eq 'nowait') {
				$waitflag = "";
			} else {
				$waitflag =~ s/^nowait\./ -m/;
			}
		} elsif($waitflag =~ /^nowait(\.\d+)?\Z(?!.)/s &&
				$t eq 'dgram') {
			$waitflag = ' -w';
			if(!defined($args[0]) || $path ne $args[0]) {
				my($cmd) = '$p=fork;'.
					'defined($p)||die"fork: $!\n";'.
					'$p&&exit;'.
					'exec{'.perl_quote($path).'}'.
					join(',', map {perl_quote $_} @args).
					' or die"exec: $!\n";';
				@args = ($path = 'perl', '-e', $cmd);
			} else {
				my($cmd) = join(' ', (map {sh_quote $_} @args)).
					" &";
				@args = ($path = '/bin/sh', '-c', $cmd);
			}
		} elsif($waitflag eq 'wait' &&
				($t eq 'stream' || $t eq 'dgram')) {
			$waitflag = ' -w';
		} else {
			$ok = 0;
		}
	}
	if($ok) {
		if($protocol eq 'unix') {
			if($port !~ /^\//) {
				$port = '/'.$port;
			}
			$addr = 'local='.unsn_encode($port).
				',type='.unsn_encode($type);
			$extra_cmds .= "rm -f ".sh_quote($port)."\n";
		} elsif($type eq 'stream') {
			$ok = $protocol eq 'tcp';
			$addr = 'ip/tcp='.unsn_encode($port);
		} elsif($type eq 'dgram') {
			$ok = $protocol eq 'udp';
			$addr = 'ip/udp='.unsn_encode($port);
		} elsif($type eq 'raw') {
			$ok = $protocol ne 'raw';
			$addr = 'ip,protocol='.unsn_encode($protocol);
		} else {
			$ok = 0;
		}
	}
	if($ok) {
		if($user =~ /^([^.:]+)(?:\.([^.:]+))?$/) {
			$userspec = ($1 eq 'root') ? '' : $1;
			if(defined($2)) {
				$userspec .= ':'.$2;
			}
		} else {
			$ok = 0;
		}
	}
	if($ok) {
		print "\n#: ", $_, "\n", $extra_cmds,
			"userver -2l", $waitflag, " -C/";
		if($userspec ne '') {
			print " -U", sh_quote($userspec);
		}
		if(!defined($args[0]) || $path ne $args[0]) {
			print " -0";
		}
		print " ", sh_quote($addr);
		if(!defined($args[0]) || $path ne $args[0]) {
			print " ", sh_quote($path);
		}
		foreach $arg (@args) {
			print " ", sh_quote($arg);
		}
		print "\n";
	} else {
		$inetd_remnants .= $_."\n";
	}
}

if($inetd_remnants ne '') {
	print "\n# Remaining inetd commands:\n",
		"cat <<'#EOF' > /tmp/inetd\$\$.conf && ",
		"inetd /tmp/inetd\$\$.conf\n",
		$inetd_remnants, "#EOF\n";
}

print "\nexit 0\n";

exit 0;
