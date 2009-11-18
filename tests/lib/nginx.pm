package Nginx;
use Env;

sub new { bless {}, $_[0] }

sub run
{
	my ($self, $conf) = @_;
	
	my $pid = fork();
	die "cant fork()" unless defined $pid;
	unless ($pid)
	{
		$conf ||= "$TEST_DIR/nginx.conf";
		exec(qq{$NGINX -g "daemon off;" -c "$conf"});
		die "cant exec()";
	}
	$self->{pid} = $pid;
	select(undef, undef, undef, 0.25);
	return $self;
}

sub stop
{
	my ($self) = @_;
	
	eval
	{
		local $SIG{ALRM} = sub { die "alarm\n" };
		alarm 5;
		kill QUIT => $self->{pid};
		waitpid $self->{pid}, 0;
		alarm 0;
	};
	if ($@)
	{
		die unless $@ eq "alarm\n";
		kill KILL => $self->{pid};
	}
}


1;