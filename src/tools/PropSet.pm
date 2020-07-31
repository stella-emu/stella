package PropSet;

# NOTE: If the property types ever change in Stella, the following hashmap
#       and array must be updated (and stay in sequence)
my %prop_type = (
  "Cart.MD5"                  => 0,
  "Cart.Manufacturer"         => 1,
  "Cart.ModelNo"              => 2,
  "Cart.Name"                 => 3,
  "Cart.Note"                 => 4,
  "Cart.Rarity"               => 5,
  "Cart.Sound"                => 6,
  "Cart.StartBank"            => 7,
  "Cart.Type"                 => 8,
  "Console.LeftDiff"          => 9,
  "Console.RightDiff"         => 10,
  "Console.TVType"            => 11,
  "Console.SwapPorts"         => 12,
  "Controller.Left"           => 13,
  "Controller.Right"          => 14,
  "Controller.SwapPaddles"    => 15,
  "Controller.PaddlesXCenter" => 16,
  "Controller.PaddlesYCenter" => 17,
  "Controller.MouseAxis"      => 18,
  "Display.Format"            => 19,
  "Display.VCenter"           => 20,
  "Display.Phosphor"          => 21,
  "Display.PPBlend"           => 22,
  "Cart.Variations"           => 23,
  "Cart.Formats"              => 24,
  "Cart.Addresses"            => 25
);
my @prop_type_as_string = (
  "Cart.MD5",			
  "Cart.Manufacturer",		
  "Cart.ModelNo",		
  "Cart.Name",			
  "Cart.Note",			
  "Cart.Rarity",		
  "Cart.Sound",			   
  "Cart.StartBank",		   
  "Cart.Type",			   
  "Console.LeftDiff",		
  "Console.RightDiff",		
  "Console.TVType",		    
  "Console.SwapPorts",		 
  "Controller.Left",		
  "Controller.Right",		
  "Controller.SwapPaddles",	 
  "Controller.PaddlesXCenter",	   
  "Controller.PaddlesYCenter",	   
  "Controller.MouseAxis",	   
  "Display.Format",		   
  "Display.VCenter",		
  "Display.Phosphor",		 
  "Display.PPBlend",		
  "Cart.Variations",		
  "Cart.Formats",		
  "Cart.Addresses"		
);				

my @prop_defaults = (
  "",
  "",
  "",
  "",
  "",
  "",
  "MONO",
  "AUTO",
  "AUTO",
  "B",
  "B",
  "COLOR",
  "NO",
  "",
  "",
  "NO",
  "AUTO",
  "AUTO",
  "AUTO",
  "AUTO",
  "0",
  "NO",
  "0",
  "1",
  "",
  ""
);

# Load and parse a properties file into an hash table of property
# objects, indexed by md5sum
sub load_prop_set($) {
  my $file = $_[0];
  print "Loading properties from file: $file\n";

  my @props = ();
  while(($key, $value) = each(%prop_type)) {
    $props[$value] = "";
  }

  my %propset = ();
  open(INFILE, $file);
  foreach $line (<INFILE>) {
    chomp $line;

    # Start a new item
    if ($line =~ /^""/) {
      my $key = $props[$prop_type{'Cartridge.MD5'}];
#     print "Inserting properties for key = $key\n";

      if(defined($propset{$key})) {
        print "Duplicate: $key\n";
      }
      $propset{$key} = [ @props ];

      undef @props;
      while(($key, $value) = each(%prop_type)) {
        $props[$value] = "";
      }
    } elsif ($line !~ /^$/) {
      ($key, $value) = ($line =~ m/"(.*)" "(.*)"/);
      if (defined $prop_type{$key}) {
        $index = $prop_type{$key};
        $props[$index] = $value;
      } else {
        print "ERROR: $line\n";
        print "Invalid key = \'$key\' for md5 = \'$props[0]\', ignoring ...\n";
      }
    }
  }
  close(INFILE);
  return %propset;
}

# Load and parse a properties file into an hash table of property
# objects, indexed by md5sum
sub save_prop_set {
  my $file = shift;
  my $hashref = shift;
  print "Saving " . keys(%$hashref) . " properties to file: $file\n";

  open(OUTFILE, ">$file");
  foreach my $md5 (sort keys %$hashref) {
    my $props = %$hashref{$md5};
    my @array = @$props;
    for (my $i = 0; $i < @array; $i++) {
      if ($array[$i] ne "") {
        print OUTFILE "\"$prop_type_as_string[$i]\" \"$array[$i]\"\n";
      }
    }
    print OUTFILE "\"\"\n\n";
  }
  close(OUTFILE);
}

# Get the number of property tags in one PropSet element
sub num_prop_types {
  return keys( %prop_type );
}

# Convert a properties set into a C++ compatible string
sub build_prop_string {
  my @array = @_;
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
