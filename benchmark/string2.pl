#!/usr/bin/perl -w

my $str = "";

for (my $i = 0; $i < 1000000; $i++)
{
	for (my $j = 65; $j < 91; $j++)
	{
		$c = chr($j);
		$str .= ".(" . $c . ")";
	}
}

print length($str);
print "\n";

