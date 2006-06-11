use Test::More( tests => 5 );
BEGIN { use_ok("Rsynth::Audio") }
use Audio::Play;
my $au = Audio::Data->new(rate => 11025);
my $synth = Rsynth::Audio->new($au);
is(ref($synth),"Rsynth::Audio","Correct class");
$synth->phones("sInTIsIs frQm p3l");
# print "# ",$au->duration,"\n";
ok($au->duration > 1.0,"Created some samples");
my $svr = Audio::Play->new;
$svr->play($au) if $svr;
$au->length(0);
is($au->duration,0,"Cleared samples");
$synth->say_string("Plain text too!");
ok($au->duration > 1.0,"Created some samples");
$svr->play($au) if $svr;
