package Rsynth::Elements;
use strict;
use Exporter;
use base 'Exporter';
use vars qw(@EXPORT %elem %parms @pNames @eNames);
@EXPORT = qw(read_elements write_elements write_features feature
             height front round features
             %elem %parms @pNames @eNames);
use Getopt::Std;
# use Sampa ();
my %opt;
getopts('c',\%opt);


my %height = ( hgh => 1.0, lax => 0.8, smh => 0.67, mdl => 0.5, lmd => 0.33, low => 0 );
my %front  = ( fnt => 1.0, cnt => 0.5, bck => 0);
my %round  = ( unr => 0, rnd => 1);

my %unit   = (f1 => 10, f2 => 20, f3 => 100);

sub rank
{
 my $name = shift;
 my $hash = shift;
 my $e = shift;
 my $f = $e->{features};
 my @h = grep { exists $hash->{$_} } keys %$f;
 die "No $name : ".$e->{sampa}.' '.join(' ',keys %$f).' : '.join(' ',keys %$hash) unless @h == 1;
 return $hash->{$h[0]};
}

sub height { rank(height => \%height, @_) }
sub front  { rank(front  => \%front, @_) }
sub round  { rank(round  => \%round, @_) }

my %fset;
my @fNames;

my $file = "Elements.def";

sub NULL () {undef}

sub feature
{
 my $f = shift;
 unless (exists $fset{$f})
  {
   $fset{$f} = @fNames;
   push(@fNames,$f);
  }
}

sub features
{
 my $e = shift;
 return sort { $fset{$a} <=> $fset{$b} } keys %{$e->{features}};
}

foreach my $h (\%round,\%height,\%front)
 {
  foreach my $f ( sort { $h->{$a} <=> $h->{$b} } keys %$h )
   {
    feature($f);
   }
 }
feature('vwl');
feature('dip');

sub read_elements
{
 $file = shift if @_;
 open(my $eh,$file) || die "Cannot read $file:$!";
 my $name;
 while(<$eh>)
  {
   if (/^\{(".*",\s*.*),\s*(\/\*.*\*\/)?\s*$/)
    {
     my %args;
     my (@args) = split(/\s*,\s*/,$1);
     my @feat = split(/\|/,pop(@args));
     my %feat;
     foreach my $f (@feat)
      {
       next unless $f =~ /^[a-z]/i;
       $feat{$f} = 1;
       feature($f);
      }
     ($name,@args) = map(eval($_),@args);
     push(@args,\%feat);
     foreach my $parm (qw(rk du ud unicode sampa features))
      {
       $args{$parm}  = shift(@args);
       $parms{$parm} = $args{$parm};
      }
     utf8::decode($args{unicode});
     $elem{$name} = bless {%args,name => $name, idx => scalar(@eNames)},'Rsynth::Element';
     push(@eNames,$name);
    }
   elsif (/^\s*\{\s*(.*?)\s*\},?\s*\/\*\s*(\w+)/)
    {
     my $parm = $2;
     my @val = split(/\s*,\s*/,$1);
     if ($parm =~ /^a/)
      {
       $val[0] = 0 if ($val[0] < 0);
       $val[0] = sprintf("%+6.1f",$val[0]);
      }
     unless (exists $parms{$parm})
      {
       $parms{$parm} = $val[0];
       push(@pNames,$parm);
      }
     $elem{$name}{$parm} = [@val];
    }
  }
 my $nf = @fNames;
 # warn "$nf features: @fNames\n";
 close($eh);
}

sub write_features
{
 for (my $i=0; $i < @fNames; $i++)
  {
   my $val = ($i) ? (1 << ($i-1)) : 0;
   printf "#define $fNames[$i] 0x%08x\n",$val;
  }
}

sub write_elements
{
 my $fh = shift;
 unless (ref $fh)
  {
   open(my $tmp,">$fh") || die "Cannot open $fh:$!";
   $fh = $tmp;
  }
 binmode($fh,":utf8");
 my $save = select $fh;
 print $fh <<'END';
/*
    Copyright (c) 1994,2001-2004 Nick Ing-Simmons. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA

*/
END

 my @ename = @eNames;
 while (@ename)
  {
   my $name = shift(@ename);
   my @feat = features($elem{$name});
   my $sampa = $elem{$name}{sampa};
   my @args = qw(rk du ud);
   @args = @{$elem{$name}}{@args};
   my $uni = $elem{$name}{unicode};
   utf8::encode($elem{$name}{unicode});
   foreach my $k (qw(unicode sampa))
    {
     my $v = $elem{$name}{$k};
     if (defined $v)
      {
       $v =~ s#([\\"])#\\$1#g;
       $v =~ s/([^\x20-\x7e])/sprintf("\\%03o",ord($1))/eg;
      }
     $v = (defined $v) ? qq["$v"] : 'NULL';
     push(@args,$v);
    }
   push(@args,(@feat) ? join('|',@feat) : 0);
   printf qq({"$name", %3d,%2d,%2d,%s,%s,%s, /* [$uni] */\n {\n),@args;
   my @pname = @pNames;
   while (@pname)
    {
     my $parm = shift(@pname);
     my @vals = @{$elem{$name}{$parm}};
     if ($elem{$name}{features}{vwl} && exists $unit{$parm})
      {
       my $u = $unit{$parm};
       $vals[0] = int($vals[0]/$u+0.5)*$u;
      }
     printf "  {%6g,%4d,%3d,%3d,%3d}%s /* %-3s */\n",
       @vals,((@pname) ? ',' : ' '),$parm;
    }
    printf " }\n}%s\n",((@ename) ? ",\n" : ' ');
  }
 select $save;
}

{
 package Rsynth::Phones;
 our %phtoelm;
 do "Rsynth/phtoelm.def";
 our @U = '@U';

 sub NULL () {return ()};

 sub COMMENT {}
 sub enter
 {
  my ($sampa,@seq) = @_;
  $phtoelm{$sampa} = [@seq];
 }
}

1;
