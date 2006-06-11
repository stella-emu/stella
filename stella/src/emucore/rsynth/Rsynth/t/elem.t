use Test::More( tests => 42 );
BEGIN { use_ok("Rsynth::Audio") }
BEGIN { use_ok("Rsynth::Elements") }
use Audio::Play;

read_elements("../Elements.def");
my $elm = '';
# Next line isn't general - all phones happen to be single SAMPA chars
for my $ph (split(//,'sInT.frQm.elIm@nts'))
 {
  my $list = $Rsynth::Phones::phtoelm{$ph};
  ok(defined($list),"Mapping for $ph");
  foreach my $ename (@{$list})
   {
    my $e = $elem{$ename};
    ok(defined($e),"Element for $ename");
    $elm .= chr($e->{idx});
    $elm .= chr($e->{du});
   }
 }
my $au = Audio::Data->new(rate => 11025);
my $synth = Rsynth::Audio->new($au);
is(ref($synth),"Rsynth::Audio","Correct class");
$synth->interpolate($elm);
ok($au->duration > 1.0,"Created some samples");
my $svr = Audio::Play->new;
$svr->play($au) if $svr;

