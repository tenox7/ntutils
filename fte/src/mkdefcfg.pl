#!perl -w

$n = 1;
$buf = "";
$out = "";
local($infile, $outfile) = @ARGV;
if(defined($infile)) {open(INFILE, "<$infile"); }
else { open(INFILE, "<&STDIN"); }
if(defined($outfile)) {open(OUTFILE, ">$outfile"); }
else { open(OUTFILE, ">&STDOUT"); }
print OUTFILE "/* do not edit */\nstatic const unsigned char DefaultConfig[] = {\n";
binmode INFILE; # NT needs this
local($len);
while (($len = read(INFILE, $buf, 256)) > 0) {
    #$buf =~ s/(.)/sprintf "0x%02X", ord($1))/goe;
    #for ($i = 0; $i < $len; $i++) {
    #    $out .= sprintf "0x%02X", ord(substr($buf, $i, 1,));
    #    if ($n++ % 10) {
    #        $out .= ", ";
    #    } else {
    #        $out .= ",\n";
    #    }
    #}

    @c = split(//, $buf);
    for ($i = 0; $i < $len; $i++) {
        $out .= sprintf("0x%02X", ord($c[$i]));
        if ($n++ % 10) {
            $out .= ", ";
        } else {
            $out .= ",\n";
        }
    }

    print OUTFILE $out;
    $out = "";
    print STDERR ".";
}
print OUTFILE "\n};\n";
close(INFILE);
close(OUTFILE);
print STDERR "\n";

