package Sampa;
use Exporter;
use base qw(Exporter);
@EXPORT_OK = qw(%unicode %description %example);

open(my $sampa,"sampa.htm") || die "Cannot open sampa.htm:$!";
while (<$sampa>)
 {
  my @td = m!<td[^>]*>([^<>]*)</td>!g;
  if (@td >= 5)
   {
    my ($sampa,$IPA,$hex,$dec,$desc,$example) = @td;
    die "$IPA != $dec" unless $IPA =~ /&#$dec;/;
    die "0x$hex != $dec" unless hex($hex) == $dec;
    $sampa =~ s/&amp;/&/;
    $unicode{$sampa} = chr($dec);
    $description{$sampa} = $desc;
    $example{$sampa} = $example;
    # warn "'$sampa' => $dec\n";
   }
  elsif (/<td>/)
   {
    warn "Skip [@td]:$_";
   } 
 }

1;
__END__
