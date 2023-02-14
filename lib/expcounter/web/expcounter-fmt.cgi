#!/opt/exp/bin/perl -w

# Invoked from expcounter "main" form

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

print header, start_html("Exptools usage statistics for $WWW_tool");

my($PICKED)=1;
my($PICKEDALL)="";

if ( "$WWW_tool" =~ /Pick.*/ ) {
	$PICKED=0;
	print "You need to select a package!\n";
	print "Use the browser's 'back' button to try again.\n";
}
elsif ( "$WWW_tool" =~ /All.*/ ) {
	$PICKEDALL='*';;
}


my($WHERE)="";

if ( $PICKED ) {
	print "<CENTER>";
	print "Statistics for $WWW_counttype <STRONG> $WWW_tool </STRONG> for ";
	
	if ( "$PICKEDALL" ne '*' ) {
		$PICKEDALL=$WWW_tool;
		$WHERE="where $WWW_counttype leq $WWW_tool";
	} else {
		$WHERE="where time ge 0";
	}
}

my($LOGLIST)="$LOG/$PICKEDALL/log*";

if ( $PICKED ) {
	if ( "$WWW_period" eq "Total Usage" ) {
		print "All Usage";
		$WHERE="$WHERE and time ge 0";
	} elsif ( "$WWW_period" eq "Today~" ) {
		# Use today's and yesterdays log
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d; getdate +$LOG/$PICKEDALL/log%Y/%m/%d yesterday`;
		print `getdate +"%i %d, %Y"`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u today`;
	} elsif ( "$WWW_period" eq "Yesterday" ) {
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d today - 2 days; getdate +$LOG/$PICKEDALL/log%Y/%m/%d yesterday`;
		print `getdate +"%i %d, %Y" yesterday`;
		$WHERE="$WHERE and time le ".`getdate -p -e +%u today`." and time ge ".`getdate -p -e +%u yesterday`;
	} elsif ( "$WWW_period" eq "Last Seven Days~" ) {
		print `getdate +"week starting %i %d, %Y" last week`;
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m;getdate +$LOG/$PICKEDALL/log%Y/%m last month`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u last week`;
	} elsif ( "$WWW_period" eq "This Month~" ) {
		my($MONTH)=`getdate +%i`;
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m`;
		print `getdate +"month of %i, %Y"`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u 1 $MONTH`;
	} elsif ( "$WWW_period" eq "Last Whole Month" ) {
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m last month`;
		print `getdate +"month of %i, %Y" last month`;
	} elsif ( "$WWW_period" eq "This Year~" ) {
		print `getdate -p +"all of %Y" 1 Jan`;
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u 1 Jan`;
	} elsif ( "$WWW_period" eq "Last Hour~") {
		# Use today's log
		print `getdate -p +"hour starting %a %h %l  %T  %z  %Y" -1 hour`;
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d; getdate +$LOG/$PICKEDALL/log%Y/%m/%d tomorrow`;
		$WHERE="$WHERE and time ge ".`getdate -p -e +%u 'now -1 hour'`;
	} elsif ( "$WWW_period" eq "Last Whole Hour") {
		# Use today's/tomorrow's/yesterdays log
		my($end) = int(time() / 3600)*3600;
		my($start) = $end - 3600;
		print `getdate -p +"hour starting %a %h %l  %T  %z %Y" $start`;
		print `getdate -p +"and ending at %a %h %l  %T  %z %Y" $end`;
		$LOGLIST=`getdate +$LOG/$PICKEDALL/log%Y/%m/%d; getdate +$LOG/$PICKEDALL/log%Y/%m/%d tomorrow;getdate +$LOG/$PICKEDALL/log%Y/%m/%d yesterday`;
		$WHERE="$WHERE and time ge $start and time le $end";
	} else {
		$PICKED=0;
		print "$WWW_period (not yet supported)";
	}
}

$WHERE =~ s/\n/ /g;
$LOGLIST =~ s/\n/ /g;

print p,"<FONT SIZE=-2> Report printed: ",`getdate`,"</FONT>";
print p,"<FONT SIZE=-4>(Log file list = $LOGLIST)</FONT>";

if ( $PICKED  && "$WWW_counttype" eq "count" ) {
	print p,"<TABLE BORDER=1 CELLPADDING=5><TR ALIGN=center><TH>Package</TH><TH>Counter Name</TH><TH>Count</TH></TR>";
	chdir("/tmp");
	unlink("tally$$","Dtally$$");

	print `find $LOGLIST -type f |xargs cat | \
		uprintf -q -I${CGIDIR}/log -f\"%s:%s@@@%s\n\" time package count in - $WHERE | \
		tally -I${CGIDIR}/merge -Otally$$ attribute in - into tally$$`;
	print `uprintf -q -f\"<TR ALIGN=center><TD>%s</TD><TD>%s</TD></TR>\n\" name count in tally$$ | \
		sed 's,@@@,</TD><TD>,g'`;
	print "</TABLE></CENTER>\n";
}

if ( "$PICKED" && "$WWW_counttype" eq "package" ) {

	print p,"<TABLE BORDER=1 CELLPADDING=5><TR ALIGN=center><TH>Package</TH><TH>Counter Name</TH><TH>Count</TH></TR>";
	chdir("/tmp");
	unlink("tally$$","Dtally$$");
	print `find $LOGLIST -type f |xargs cat | \
		uprintf -q -I${CGIDIR}/log -f\"%s:%s@@@%s\n\" time package count in - $WHERE | \
		tally -I${CGIDIR}/merge -Otally$$ attribute in - into tally$$`;

	print `uprintf -q -f\"<TR ALIGN=center><TD>%s</TD><TD>%s</TD></TR>\n\" name count in tally$$ | \
		sed 's,@@@,</TD><TD>,g'`;
	print "</TABLE><P>\n";
	print "( Number of unique machine/userids: ";
	print `find $LOGLIST -type f |xargs cat | \
		uselect -q -I${CGIDIR}/log hostname uid from - $WHERE | \
		sort -u|wc -l`;
	print ")<P>(Number of unique userids: ";
	print `find $LOGLIST -type f |xargs cat | \
		uselect -q -I${CGIDIR}/log uid from - $WHERE | \
		sort -u|wc -l`;
	print ")<P>(Number of unique machines: ";
	print `find $LOGLIST -type f |xargs cat | \
		uselect -q -I${CGIDIR}/log hostname from - $WHERE | \
		sort -u|wc -l`;
	print ")</CENTER>\n";
}

print end_html;
unlink("tally$$","Dtally$$");

exit(0);
