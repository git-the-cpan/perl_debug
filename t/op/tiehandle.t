#!./perl -w

BEGIN {
    chdir 't' if -d 't';
    @INC = '../lib';
}

my @expect;
my $data = "";
my @data = ();

require './test.pl';
plan(tests => 41);

sub compare {
    return unless @expect;
    return ::fail() unless(@_ == @expect);

    for my $i (0..$#_) {
	next if $_[$i] eq $expect[$i];
	return ::fail();
    }

    ::pass();
}


package Implement;

sub TIEHANDLE {
    ::compare(TIEHANDLE => @_);
    my ($class,@val) = @_;
    return bless \@val,$class;
}

sub PRINT {
    ::compare(PRINT => @_);
    1;
}

sub PRINTF {
    ::compare(PRINTF => @_);
    2;
}

sub READLINE {
    ::compare(READLINE => @_);
    wantarray ? @data : shift @data;
}

sub GETC {
    ::compare(GETC => @_);
    substr($data,0,1);
}

sub READ {
    ::compare(READ => @_);
    substr($_[1],$_[3] || 0) = substr($data,0,$_[2]);
    3;
}

sub WRITE {
    ::compare(WRITE => @_);
    $data = substr($_[1],$_[3] || 0, $_[2]);
    length($data);
}

sub CLOSE {
    ::compare(CLOSE => @_);
    
    5;
}

package main;

use Symbol;

my $fh = gensym;

@expect = (TIEHANDLE => 'Implement');
my $ob = tie *$fh,'Implement';
is(ref($ob),  'Implement');
is(tied(*$fh), $ob);

@expect = (PRINT => $ob,"some","text");
$r = print $fh @expect[2,3];
is($r, 1);

@expect = (PRINTF => $ob,"%s","text");
$r = printf $fh @expect[2,3];
is($r, 2);

$text = (@data = ("the line\n"))[0];
@expect = (READLINE => $ob);
$ln = <$fh>;
is($ln, $text);

@expect = ();
@in = @data = qw(a line at a time);
@line = <$fh>;
@expect = @in;
compare(@line);

@expect = (GETC => $ob);
$data = "abc";
$ch = getc $fh;
is($ch, "a");

$buf = "xyz";
@expect = (READ => $ob, $buf, 3);
$data = "abc";
$r = read $fh,$buf,3;
is($r, 3);
is($buf, "abc");


$buf = "xyzasd";
@expect = (READ => $ob, $buf, 3,3);
$data = "abc";
$r = sysread $fh,$buf,3,3;
is($r, 3);
is($buf, "xyzabc");

$buf = "qwerty";
@expect = (WRITE => $ob, $buf, 4,1);
$data = "";
$r = syswrite $fh,$buf,4,1;
is($r, 4);
is($data, "wert");

$buf = "qwerty";
@expect = (WRITE => $ob, $buf, 4);
$data = "";
$r = syswrite $fh,$buf,4;
is($r, 4);
is($data, "qwer");

$buf = "qwerty";
@expect = (WRITE => $ob, $buf, 6);
$data = "";
$r = syswrite $fh,$buf;
is($r, 6);
is($data, "qwerty");

@expect = (CLOSE => $ob);
$r = close $fh;
is($r, 5);

# Does aliasing work with tied FHs?
*ALIAS = *$fh;
@expect = (PRINT => $ob,"some","text");
$r = print ALIAS @expect[2,3];
is($r, 1);

{
    use warnings;
    # Special case of aliasing STDERR, which used
    # to dump core when warnings were enabled
    local *STDERR = *$fh;
    @expect = (PRINT => $ob,"some","text");
    $r = print STDERR @expect[2,3];
    is($r, 1);
}

{
    # Test for change #11536
    package Foo;
    use strict;
    sub TIEHANDLE { bless {} }
    my $cnt = 'a';
    sub READ {
	$_[1] = $cnt++;
	1;
    }
    sub do_read {
	my $fh = shift;
	read $fh, my $buff, 1;
	::pass();
    }
    $|=1;
    tie *STDIN, 'Foo';
    read STDIN, my $buff, 1;
    ::pass();
    do_read(\*STDIN);
    untie *STDIN;
}


{
    # test for change 11639: Can't localize *FH, then tie it
    {
	local *foo;
	tie %foo, 'Blah';
    }
    ok(!tied %foo);

    {
	local *bar;
	tie @bar, 'Blah';
    }
    ok(!tied @bar);

    {
	local *BAZ;
	tie *BAZ, 'Blah';
    }
    ok(!tied *BAZ);

    package Blah;

    sub TIEHANDLE {bless {}}
    sub TIEHASH   {bless {}}
    sub TIEARRAY  {bless {}}
}

{
    # warnings should pass to the PRINT method of tied STDERR
    my @received;

    local *STDERR = *$fh;
    no warnings 'redefine';
    local *Implement::PRINT = sub { @received = @_ };

    $r = warn("some", "text", "\n");
    @expect = (PRINT => $ob,"sometext\n");

    compare(PRINT => @received);

    use warnings;
    print undef;

    like($received[1], qr/Use of uninitialized value/);
}

{
    # [ID 20020713.001] chomp($data=<tied_fh>)
    local *TEST;
    tie *TEST, 'CHOMP';
    my $data;
    chomp($data = <TEST>);
    is($data, 'foobar');

    package CHOMP;
    sub TIEHANDLE { bless {}, $_[0] }
    sub READLINE { "foobar\n" }
}


