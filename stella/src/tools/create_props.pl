#!/usr/bin/perl

use Btrees;

my @props = ();
my @propset = ();
my @propset_ordered = ();

my %proptype = (
"Cartridge.MD5"           => 0,
"Cartridge.Manufacturer"  => 1,
"Cartridge.ModelNo"       => 2,
"Cartridge.Name"          => 3,
"Cartridge.Note"          => 4,
"Cartridge.Rarity"        => 5,
"Cartridge.Sound"         => 6,
"Cartridge.Type"          => 7,
"Console.LeftDifficulty"  => 8,
"Console.RightDifficulty" => 9,
"Console.TelevisionType"  => 10,
"Console.SwapPorts"       => 11,
"Controller.Left"         => 12,
"Controller.Right"        => 13,
"Controller.SwapPaddles"  => 14,
"Display.Format"          => 15,
"Display.XStart"          => 16,
"Display.Width"           => 17,
"Display.YStart"          => 18,
"Display.Height"          => 19,
"Display.Phosphor"        => 20,
"Display.PPBlend"         => 21,
"Emulation.HmoveBlanks"   => 22
);

my @prop_defaults = (
  "",
  "",
  "",
  "Untitled",
  "",
  "",
  "MONO",
  "AUTO-DETECT",
  "B",
  "B",
  "COLOR",
  "NO",
  "JOYSTICK",
  "JOYSTICK",
  "NO",
  "NTSC",
  "0",
  "160",
  "34",
  "210",
  "NO",
  "77",
  "YES"
);


@props = ();
while(($key, $value) = each(%proptype)) {
	$props[$value] = "";
}

print "@ARGV\n";
usage() if @ARGV != 2;

# Must provide input and output files
open(INFILE, "$ARGV[0]");
open(OUTFILE, ">$ARGV[1]");

# Parse the properties file into an array of property objects
foreach $line (<INFILE>) {
	chomp $line;

	# Start a new item
	if ($line =~ /^""/) {
		push @propset, [ @props ];

		undef @props;
		while(($key, $value) = each(%proptype)) {
			$props[$value] = "";
		}
	} elsif ($line !~ /^$/) {
		($key, $value) = ($line =~ m/"(.*)" "(.*)"/);
		if (defined $proptype{$key}) {
			$index = $proptype{$key};
			$props[$index] = $value;
		} else {
			print "Invalid key = \'$key\' for md5 = \'$props[0]\', ignoring ...\n";
		}
	}
}

# Fill the AVL tree with property indices
my $tree;
for($i = 0; $i < @propset; $i++) {
	my $value = $propset[$i][$proptype{'Cartridge.MD5'}];
#	printf "Adding $value to tree\n";
	($tree, $node) = bal_tree_add($tree, $i, \&compare);
}
printf "\n";

# Fill the results tree with the appropriate number of items
my $height = $tree->{height};
my $size = 2 ** $height - 1;
printf "Tree has height = $height, output BST array will have size = $size\n";
for($i = 0; $i < $size; $i++) {
	$propset_ordered[$i] = -1;
}

# Label the tree nodes by index into a BST array
label_tree($tree, \&store);
printf "\n";


# Construct the output file in C++ format
# Walk the results array and print each item
# This array will now contain the original tree converted to a BST in array format
print OUTFILE "#ifndef DEF_PROPS_HXX\n";
print OUTFILE "#define DEF_PROPS_HXX\n";
print OUTFILE "\n";
print OUTFILE "/**\n";
print OUTFILE "  This code is generated using the 'create_props.pl' script,\n";
print OUTFILE "  located in the src/tools directory.  All properties changes\n";
print OUTFILE "  should be made in stella.pro, and then this file should be\n";
print OUTFILE "  regenerated and the application recompiled.\n";
print OUTFILE "*/\n";
print OUTFILE "static const char* DefProps[][" . keys( %proptype ) . "] = {\n";
for ($i = 0; $i < @propset_ordered; $i++) {
	my $idx = $propset_ordered[$i];
	if ($idx != -1) {
		print OUTFILE get_prop($idx);
	} else {
		print OUTFILE blank_prop();
	}

	if ($i+1 < @propset_ordered) {
		print OUTFILE ", ";
	}
	print OUTFILE "\n";
}
print OUTFILE "};\n";
print OUTFILE "\n";
print OUTFILE "#endif\n";

close(INFILE);
close(OUTFILE);


sub usage {
	print "create_props.pl <INPUT STELLA PROPS> <OUTPUT C++ header>\n";
	exit(0);
}

sub compare {
	my ($first, $second) = @_;

	my $first_md5  = $propset[$first][$proptype{'Cartridge.MD5'}];
	my $second_md5 = $propset[$second][$proptype{'Cartridge.MD5'}];

	if ($first_md5 lt $second_md5) {
		return -1;
	} elsif ($first_md5 gt $second_md5) {
		return 1;
	} else {
		return 0;
	}
}

sub store {
	my $tree = shift;

	$propset_ordered[$tree->{index}] = $tree->{val};
}

sub get_prop {
	my $idx = shift;

	my $arr = $propset[$idx];
	my @array = @$arr;

	my $result = "  { ";

	my @items = ();
	for (my $i = 0; $i < @array; $i++) {
		if($prop_defaults[$i] ne $array[$i]) {
			push(@items, "\"$array[$i]\"");
		} else {
			push(@items, "\"\"");
		}
	}

	$result .= join(", ", @items);
	$result .= " }";

	return $result;
}

sub blank_prop {
	my $arr = $propset[$idx];
	my @array = @$arr;

	my $result = "  { ";

	my @items = ();
	for my $key ( keys %proptype) {
		push(@items, "\"\"");
	}

	$result .= join(", ", @items);
	$result .= " }";

	return $result;
}
