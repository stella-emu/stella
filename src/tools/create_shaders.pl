#!/usr/bin/perl

use File::Basename;

usage() if @ARGV < 2;

$numfiles = @ARGV;
$outfile = $ARGV[$numfiles-1];

# Construct the output file in C++ format
# Walk the ARGV list and convert each item
open(OUTFILE, ">$outfile");

print OUTFILE "//============================================================================\n";
print OUTFILE "//\n";
print OUTFILE "//   SSSS    tt          lll  lll\n";
print OUTFILE "//  SS  SS   tt           ll   ll\n";
print OUTFILE "//  SS     tttttt  eeee   ll   ll   aaaa\n";
print OUTFILE "//   SSSS    tt   ee  ee  ll   ll      aa\n";
print OUTFILE "//      SS   tt   eeeeee  ll   ll   aaaaa  --  \"An Atari 2600 VCS Emulator\"\n";
print OUTFILE "//  SS  SS   tt   ee      ll   ll  aa  aa\n";
print OUTFILE "//   SSSS     ttt  eeeee llll llll  aaaaa\n";
print OUTFILE "//\n";
print OUTFILE "// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team\n";
print OUTFILE "//\n";
print OUTFILE "// See the file \"license\" for information on usage and redistribution of\n";
print OUTFILE "// this file, and for a DISCLAIMER OF ALL WARRANTIES.\n";
print OUTFILE "//\n";
print OUTFILE "// \$Id\$\n";
print OUTFILE "//============================================================================\n";
print OUTFILE "\n";
print OUTFILE "#ifndef GL_SHADER_PROGS_HXX\n";
print OUTFILE "#define GL_SHADER_PROGS_HXX\n";
print OUTFILE "\n";
print OUTFILE "/**\n";
print OUTFILE "  This code is generated using the 'create_shaders.pl' script,\n";
print OUTFILE "  located in the src/tools directory.\n";
print OUTFILE "*/\n";
print OUTFILE "\n";
print OUTFILE "namespace GLShader {\n\n";

for ($i = 0; $i < $numfiles - 1; $i++)
{
	open(INFILE, "$ARGV[$i]");

	($base,$path,$type) = fileparse($ARGV[$i]);
	$base =~ s/\./_/g;

	print OUTFILE "static const char* " . $base . "[] = {\n";
	foreach $line (<INFILE>)
	{
		chomp($line);
		print OUTFILE "\"" . $line . "\\n\"\n";
	}
	print OUTFILE "\"\\0\"\n";
	print OUTFILE "};\n\n";

	close(INFILE);
}

print OUTFILE "} // namespace GLShader\n\n";
print OUTFILE "#endif\n";

close(OUTFILE);


sub usage {
	print "create_shaders.pl <shader programs> <OUTPUT C++ header>\n";
	exit(0);
}
