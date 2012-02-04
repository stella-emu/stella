#!/usr/bin/perl

# Locate the 'PropSet' module
use FindBin;
use lib "$FindBin::Bin";
use PropSet;

usage() if @ARGV != 2;

my %usr_propset = PropSet::load_prop_set($ARGV[0]);
my %sys_propset = PropSet::load_prop_set($ARGV[1]);

print "\n";
print "Valid properties found in user file: " . keys (%usr_propset) . "\n";
print "Valid properties found in system file: " . keys (%sys_propset) . "\n";

# Determine which properties exist in both files
for my $key ( keys %usr_propset ) {
  if (defined $sys_propset{$key}) {
    $sys_propset{$key} = $usr_propset{$key};
    delete $usr_propset{$key};
  }
}

print "\n";
print "Updated properties found in user file: " . keys (%usr_propset) . "\n";
print "Updated properties found in system file: " . keys (%sys_propset) . "\n";
print "\n";

# Write both files back to disk
PropSet::save_prop_set($ARGV[0], \%usr_propset);
PropSet::save_prop_set($ARGV[1], \%sys_propset);


sub usage {
  print "merge_props.pl <USER properties file> <SYSTEM properties file>\n";
  print "\n";
  print "Scan both properties files, and for every entry found in both files,\n";
  print "remove it from the USER file and overwrite it in the SYSTEM file.\n";
  exit(0);
}
