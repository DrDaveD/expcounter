#!/opt/exp/bin/perl -w

# Invoked from expcounter main form

$ENV{"PATH"} = $ENV{"PATH"}.":/opt/exp/bin:/opt/exp/lib/unity/bin";

use CGI qw/:standard/;

my($LOG)="/usr/expcounters";

my($CGIDIR)=`pwd`;
chomp($CGIDIR);

if ( ! defined($ENV{"TOOLS"}) ) {
	$ENV{"TOOLS"}="/opt/exp";
}
if ( -x $ENV{"TOOLS"}."/bin/expcounter" ) {
	system($ENV{"TOOLS"}."/bin/expcounter -p expcounter -n web");
}

my($WWW_tool)=param('tool');
my($WWW_counttype)=param('counttype');
my($WWW_period)=param('period');
my($WWW_show_sorted)=param('show_sorted');
my($WWW_whereclause)=param('whereclause');
my($WWW_show_time)=param('show_time');
if ( ! defined ($WWW_show_time) ) { $WWW_show_time="" }
my($WWW_show_package)=param('show_package');
if ( ! defined ($WWW_show_package) ) { $WWW_show_package="" }
my($WWW_show_tool)=param('show_tool');
if ( ! defined ($WWW_show_tool) ) { $WWW_show_tool="" }
my($WWW_show_hostname)=param('show_hostname');
if ( ! defined ($WWW_show_hostname) ) { $WWW_show_hostname="" }
my($WWW_show_pid)=param('show_pid');
if ( ! defined ($WWW_show_pid) ) { $WWW_show_pid="" }
my($WWW_show_type)=param('show_type');
if ( ! defined ($WWW_show_type) ) { $WWW_show_type="" }
my($WWW_show_userinfo)=param('show_userinfo');
if ( ! defined ($WWW_show_userinfo) ) { $WWW_show_userinfo="" }
my($WWW_show_nodename)=param('show_nodename');
if ( ! defined ($WWW_show_nodename) ) { $WWW_show_nodename="" }

system("echo 'Content-type: text/plain';echo");

my($PICKED)=1;
my($PICKEDALL)="";

if ( "$WWW_tool" =~ /Pick.*/ ) {
	$PICKED=0;
	print "You need to select a package!\n";
	print "Use the browser's 'back' button to try again.\n";
}
elsif ( "$WWW_tool" =~ /All.*/ ) {
	$PICKEDALL='*';
}

my($WHERE)="";


if ( $PICKED ) {
	if ( "$PICKEDALL" ne '*' ) {
		$PICKEDALL=$WWW_tool;
		$WHERE="where package leq $WWW_tool";
	} else {
		$WHERE="where time ge 0";
	}
}

my($LOGLIST)="$LOG/$PICKEDALL/log*";

if ( $PICKED ) {
	if ( "$WWW_period" eq "Total Usage" ) {
		$WHERE="$WHERE and time ge 0";
	} elsif ( "$WWW_period" eq "Today~" ) {
		# Use today's and yesterdays log
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d; getdate +$LOG/$PICKEDALL/log%Y/%m/%d yesterday`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u today`;
	} elsif ( "$WWW_period" eq "Yesterday" ) {
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d today - 2 days; getdate +$LOG/$PICKEDALL/log%Y/%m/%d yesterday`;
		print `getdate +"%i %d, %Y" yesterday`;
		$WHERE="$WHERE and time le ".`getdate -p -e +%u today`." and time ge ".`getdate -p -e +%u yesterday`;

	} elsif ( "$WWW_period" eq "Last Seven Days~" ) {
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m;getdate +$LOG/$PICKEDALL/log%Y/%m last month`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u last week`;
	} elsif ( "$WWW_period" eq "This Month~") {
		my($MONTH)=`getdate +%i`;
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u 1 $MONTH`;
	} elsif ( "$WWW_period" eq "Last Whole Month" ) {
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m last month`;
	} elsif ( "$WWW_period" eq "This Year~" ) {
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u 1 Jan`;
	} elsif ( "$WWW_period" eq "Last Hour~") {
		# Use today's log
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d;getdate +$LOG/$PICKEDALL/log%Y/%m/%d tomorrow`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u 'now -1 hour'`;
	} elsif ( "$WWW_period" eq "Last Whole Hour") {
		my($end) = int(time() / 3600)*3600;
		my($start) = $end - 3600;
		# print `getdate -p +"hour starting %a %h %l  %T  %z %Y" $start`;
		# print `getdate -p +"and ending at %a %h %l  %T  %z %Y" $end`;
		# Use today's/tomorrow's/yesterdays log
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d; getdate +$LOG/$PICKEDALL/log%Y/%m/%d tomorrow;getdate +$LOG/$PICKEDALL/log%Y/%m/%d yesterday`;
		$WHERE="$WHERE and time ge $start and time le $end";
	} else {
		$PICKED=0;
		print "$WWW_period (not yet supported)";
	}
}

$WHERE =~ s/\n/ /g;
$LOGLIST =~ s/\n/ /g;

#echo LOGLIST="$LOGLIST"
#echo WHERE=$WHERE

my($SORT)="cat";
my($UNIQ)="cat";

if ( "$WWW_show_sorted" eq "sorted" ) {
	$SORT="sort";
	$UNIQ="uniq";
}

if ( "$PICKED" && "$WWW_counttype" eq "raw" ) {
	$FIELDS="$WWW_show_time $WWW_show_package $WWW_show_tool $WWW_show_hostname $WWW_show_pid $WWW_show_type $WWW_show_userinfo $WWW_show_nodename";
	if ( "$FIELDS" eq "    " ) {
		print "Select some fields to print";
	} else	{
		if ( "$WWW_whereclause" ne "" ) {
			$WHERE="$WHERE and ( $WWW_whereclause ) ";
		}
		$WHERE =~ s/\(/\\\(/g;
		$WHERE =~ s/\)/\\\)/g;
		system ("find $LOGLIST -type f |xargs cat| uselect -q -Ilog $FIELDS from - $WHERE | $SORT | $UNIQ");
	}
}
