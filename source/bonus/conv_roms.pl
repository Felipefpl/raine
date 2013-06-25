#!/usr/bin/perl

use strict;

my %raine_loads =
( "ROM_LOAD16_BYTE" => "LOAD_8_16",
	"ROM_LOAD32_WORD" => "LOAD_16_32",
	"ROM_LOAD" => "LOAD_NORMAL",
	"ROM_LOAD16_WORD" => "LOAD_NORMAL",
	"ROM_LOAD16_WORD_SWAP" => "LOAD_SWAP_16",
	"ROM_LOAD32_WORD_SWAP" => "LOAD32_SWAP_16",
	"ROM_LOAD32_BYTE" => "LOAD_8_32",
	"ROM_CONTINUE" => "LOAD_CONTINUE",
);

my %raine_regions =
(
	"REGION_CPU1" => "REGION_ROM1",
	"REGION_CPU2" => "REGION_ROM2",
	"REGION_SOUND1" => "REGION_SMP1",
	"REGION_SOUND2" => "REGION_SMP2",
	'"main"' => "REGION_ROM1",
	'"gfx"' => "REGION_GFX1",
	'"audio"' => "REGION_ROM2",
	'"qsound"' => "REGION_SMP1",
	'"ymsnd"' => "REGION_SMP1",
	'"audiocpu"' => "REGION_ROM2",
	'"soundcpu"' => "REGION_ROM2",
	'"cpu2"' => "REGION_ROM2",
	'"sprites"' => "REGION_SPRITES",
	'"layer0"' => "REGION_GFX1",
	'"layer1"' => "REGION_GFX2",
	'"layer2"' => "REGION_GFX3",
	'"layer3"' => "REGION_GFX4",
	'"eeprom"' => "REGION_EEPROM",
	'"oki"' => "REGION_SMP1",
	'"oki1"' => "REGION_SMP1",
	'"oki2"' => "REGION_SMP2",
	'"mcu"' => "REGION_ROM3",
	'"user1"' => "REGION_USER1",
	'"maincpu"' => "REGION_CPU1",
	'"x1snd"' => "REGION_SOUND1",
	'"ymz"' => "REGION_SOUND1",
	'"gfx1"' => "REGION_GFX1",
	'"gfx2"' => "REGION_GFX2",
);

my $started = undef;
my (@odd,@even);

open(F,"<$ARGV[0]") || die "Impossible to open $ARGV[0]";
my @file = <F>;
close(F);
my (%def,%args) = ();

while ($_ = shift @file) {
	chomp;
	s/\r//;
	if (/^#define (.+)( .+?)/ || /^#define (.+)/) {
		my $name = $1;
		my $fin = $2;
		my $args = undef;
		if ($name =~ s/\((.+)\)//) {
			$args = $1;
		}
		$fin =~ s/\r//;
		my $multi = undef;
		if ($fin =~ s/\\//) {
			$multi = 1;
		}
		$fin =~ s/^[ \t]*//;
		$def{$name} = $fin if ($fin);

		print STDERR "define $name found";
		print STDERR " args $args" if ($args);
		print STDERR " fin $fin" if ($fin);
		print STDERR " multi" if ($multi);
		if ($multi) {
			while ($_ = shift @file) {
				s/\r$//;
				my $rep = s/\\$//;
				$def{$name} .= $_;
				$args{$name} = $args;
				last if (!$rep);
			}
		}
		if (!$def{$name}) {
			$def{$name} = " ";
			print STDERR " empty";
		}
		print STDERR "\n";
	} elsif (/ROM_START ?\( ?(.+?) ?\)(.*)/) {
		my $name = $1;
		print "static struct ROM_INFO rom_$name\[\] =$2\n{\n";
		my $comment = undef;
		my $load_be = undef;
		while ($_ = shift @file) {
			if (/ROM_REGION\( ?(.+) ?\)/ || /ROM_REGION16_BE\( ?(.+?) ?\)/ || /ROM_REGION16_LE\( ?(.+?) ?\)/) {
				my $nbx = 0;
				my $args = $1;
#	$load_be = 1 if (/ROM_REGION16_BE/);
				@odd = ();
				@even = ();
				if ((my ($size,$region_name,$thing) = split(/\, */,$args))) {
					$size =~ s/^ *//;
					$region_name = $raine_regions{$region_name} if ($raine_regions{$region_name});
					if ($region_name !~ /REGION/) {
						$region_name =~ s/"//g;
						$region_name = uc("region_$region_name");
					}
					my ($function,$oldname,$oldsize,$oldbase,$oldcrc,$oldfunc) = undef;
					while ($_ = shift @file) {
						s/\r//; # Very important here !
						if (/^[ \t]*\/\*.+\*\/[ \t]*$/ || /^[ \t]*\/\//) {
							print;
							next;
						}
						if ($comment) {
							print;
							if (/\*\//) {
								$comment = undef;
							}
							next;
						}
						if (/^[ \t]*(\/\*)/) {
							my $sortie = 0;
							do {
								print;
								$sortie = 1 if (/\*\//);
							} while (!$sortie && ($_ = shift @file));
							next;
						}

						if (!/ROM_REGION/ && /([\w\d_]+) *\( *(.+) *\)/) {
							$function = $1;
							my $args = $2;
							my @args = split(/\, */,$args);
							my ($name,$base,$size,$crc,$attrib) = @args;
							if ($name !~ /"/) {
								# for continue
								$size = $base;
								$base = $name;
								$name = $oldname;
								$crc = $oldcrc;
							}

							if ($crc =~ /CRC\((.+?)\)/) {
								$crc = "0x$1";
							}
							$size =~ s/^ *//;
							if ($raine_loads{$function}) {
								$function = ($load_be ? "LOAD_BE" : $raine_loads{$function});
							} elsif ($function eq "ROM_RELOAD") {
								$size = $base;
								$size =~ s/ +//;
								$base = $name;
								$name = $oldname;
								$crc = $oldcrc;
								$function = $oldfunc;
							} elsif ($function eq "ROMX_LOAD") {
								my @arg = split(/ *\| */,$attrib);
								if ($arg[0] eq "ROM_GROUPBYTE") {
									shift @arg; # just skip this one !
								}
								if ($arg[0] eq "ROM_GROUPWORD" && $arg[1] eq "ROM_SKIP(6) ") {
									$function = "LOAD_16_64";
								} elsif ($arg[0] eq "ROM_SKIP(7) ") {
									$function = "LOAD_8_64";
								} else {
									print STDERR "ROMX_LOAD: unknown attribute ",join(",",@arg),".\n";
									exit(1);
								}
							} elsif ($function =~ /ROM_(COPY|FILL)/) {
								chomp;
								print STDERR "Ignored : $function\n";
								print "/* Ignored : $_ */\n";
								$function = $oldfunc;
								next;
							} elsif ($def{$function}) {
								my $xargs = $args{$function};
								my $def = $def{$function};
								if ($xargs) {
									my @xargs = split(/\, */,$xargs);
									for (my $n=0; $n<= $#xargs; $n++) {
										$def =~ s/$xargs[$n]/$args[$n]/g;
									}
								}
								unshift @file,split(/\n/,$def);
								$function = $oldfunc;
								next;
							} elsif ($function !~ /(ROM_CONTINUE|ROM_IGNORE)/) {
								print STDERR "Unknown loading $function from line $_\n";
								exit(1);
							}
							if ($function ne "ROM_CONTINUE") {
								if ($oldname) {
									print "  { $oldname, $oldsize, $oldcrc, $region_name, $oldbase, $oldfunc },\n";
								}
								($oldname,$oldsize,$oldbase,$oldcrc,$oldfunc) =
								($name,$size,$base,$crc,$function);
							}
						} elsif (/ROM_REGION/ || # empty line
							/ROM_END/) {
							if ($oldname) {
								print "  { $oldname, $oldsize, $oldcrc, $region_name, $oldbase, $function },\n";
							}
							last;
						} elsif (/^[ \t]*\/\*/ && !/\*\//) {
							print;
							$comment = 1;
							next;
						} elsif (/^[ \t]*\#(if|ifdef|ifndef|else|endif)/) { # preprocessor
							if ($oldname) {
								print "  { $oldname, $oldsize, $oldcrc, $region_name, $oldbase, $function },\n";
							}
							print;
							last if ($oldname);
						} elsif (!/^[ \t]*$/) { # not an empty line, and unknown
							s/[ \t]*//g;
							chomp;
							next if ($_ eq "NEOGEO_BIOS");
							if ($def{$_}) {
								print STDERR "found macro without args $_\n";
								unshift @file,split(/\n/,$def{$_});
								next;
							}
							print STDERR "weird line $_\n";
							print;
						}
					} # while (<>)
					if (/ROM_END/) {
						print "  { NULL, 0, 0, 0, 0, 0 }\n";
						print "};\n\n";
						last;
					}
				} else {
					print STDERR "Failure to parse REGION args $args\n";
					exit(1);
				}
			} else {
				chomp;
				s/\r$//;
				my $line = $_;
				foreach (keys %def) {
					if ($line =~ /^[ \t]*$_/) {
						if ($_ eq "NEOGEO_BIOS") {
							print STDERR "skipping neogeo_bios\n";
							last;
						}
						print STDERR "found def $_ usage...\n";
						unshift @file,split(/\n/,$def{$_});
						last;
					}
				}
				$_ = $line;
			} # if (/ROM_REGION
			redo if (/ROM_REGION/); # yet another line...
			die "line $_\n" if (/NEO_SFIX/);
		} # while ($_ = shift @file)
	}
}

