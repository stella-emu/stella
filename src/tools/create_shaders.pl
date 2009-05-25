#!/usr/bin/perl

use File::Basename;

usage() if @ARGV < 2;

$numfiles = @ARGV;
$outfile = $ARGV[$numfiles-1];

# Construct the output file in C++ format
# Walk the ARGV list and convert each item
open(OUTFILE, ">$outfile");

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
