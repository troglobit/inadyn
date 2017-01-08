Internet Automated Dynamic DNS Client
=====================================
[![Travis Status][]][Travis] [![Coverity Status][]][Coverity Scan]


Table of Contents
-----------------

* [Introduction](#introduction)
* [Supported Providers](#supported-providers)
* [Example Configuration](#example-configuration)
* [Generic DDNS Plugin](#generic-ddns-plugin)
* [Build & Install](#build--install)
* [Building from GIT](#building-from-git)
* [Origin & References](#origin--references)


Introduction
------------

Inadyn is a small and simple Dynamic DNS, [DDNS][], client with HTTPS
support.  Commonly available in many GNU/Linux distributions, used in
off the shelf routers and Internet gateways to automate the task of
keeping your Internet name in sync with your public¹ IP address.  It can
also be used in installations with redundant (backup) connections to the
Internet.

Most people are unaware they share a pool of Internet addresses with
other users of the same Internet Service Provider (ISP).  Protocols like
DHCP, PPPoE, or PPPoA are used to give you an address and a way to
connect to the Internet, but usually not a way for others to connect to
you.  If you want to run an Internet server on such a connection you
risk losing your IP address every time you reconnect, or as in the case
of DHCP even when the lease is renegotiated.

By using a DDNS client like `inadyn` you can register an Internet name
with a DDNS provider, like [FreeDNS](http://freedns.afraid.org).  The
DDNS client updates your DNS record periodically and/or on demand when
your IP address changes.  Inadyn can maintain multiple host records with
the same IP address, use a combination of a script, the address from an
Internet-facing interface, or default to using the IP address change
detector of the DDNS provider.

__  
¹ Public IP address is the default, private addresses can also be used.


Supported Providers
-------------------

Some of these services are free of charge for non-commercial use, some
take a small fee, but also provide more domains to choose from.

DDNS providers not supported natively like <http://twoDNS.de>, can be
enabled using the generic DDNS plugin.  See below for configuration
examples.

* <http://www.dyndns.org>, <http://www.dyn.com>
* <https://freedns.afraid.org>
* <https://www.zoneedit.com>
* <http://www.no-ip.com>
* <http://www.easydns.com>
* <http://www.tzo.com>
* <http://www.3322.org>
* <https://www.dnsomatic.com>
* <http://dns.he.net>
* <http://www.tunnelbroker.net>
* <http://www.dynsip.org>
* <http://www.sitelutions.com>
* <http://www.dnsexit.com>
* <http://www.changeip.com>
* <http://www.zerigo.com>
* <http://www.dhis.org>
* <https://nsupdate.info>
* <https://duckdns.org>
* <https://www.loopia.com>
* <https://www.namecheap.com>
* <https://domains.google.com>
* <https://www.ovh.com>
* <https://www.dtdns.com>
* <http://giradns.com>
* <https://www.duiadns.net>
* <https://ddnss.de>
* <http://dynv6.com>
* <http://ipv4.dynv6.com>
* <https://spdyn.de>
* <https://www.strato.com>

In-A-Dyn defaults to HTTPS, but not all providers may support this, so
try disabling SSL for the update (`ssl = false`) or the checkip phase
(`checkip-ssl = false`) in the `provider` section, in case you run into
problems.

*HTTPS is enabled by default* since it protects your credentials from
being snooped and reduces the risk of someone hijacking your account.


Example Configuration
---------------------

In-A-Dyn supports updating several DDNS servers, several accounts even on
different DDNS providers.  The following `/etc/inadyn.conf` example show
how this can be done:

    # In-A-Dyn v2.0 configuration file format
    period          = 300
    user-agent      = Mozilla/5.0

    provider default@dyndns.org {
	    ssl         = false
        username    = charlie
        password    = snoopy
        hostname    = { peanuts, woodstock }
		user-agent  = Mozilla/4.0
	}
    
    # With multiple usernames at the same provider, index with :#
    provider default@no-ip.com:1 {
        username    = ian
        password    = secret
        hostname    = flemming.no-ip.com
		user-agent  = inadyn/2.2
	}
    
    # With multiple usernames at the same provider, index with :#
    provider default@no-ip.com:2 {
        username       = james
        password       = bond
        hostname       = spectre.no-ip.com
		checkip-ssl    = false
        checkip-server = api.ipify.org
    }
    
    # With multiple usernames at the same provider, index with :#
    provider default@no-ip.com:3 {
        username        = spaceman
        password        = bowie
        hostname        = spaceman.no-ip.com
        checkip-command = /sbin/ifconfig eth0 | grep 'inet6 addr'
    }
    
    # Note: hostname == update-key from Advanced tab in the Web UI
    provider default@tunnelbroker.net {
        username    = futurekid
        password    = dreoadsad/+dsad21321    # update-key-in-advanced-tab
        hostname    = 1234534245321           # tunnel-id
	}

    provider default@dynv6.com {
        username = your_token
        password = n/a
        hostname = { host1.dynv6.net, host2.dynv6.net }
    }

Notice how this configuration file has two different users of the No-IP
provider -- this is achieved by appending a `:ID` to the provider name.

We also define a custom cache directory, default is to use `/var/cache`.
In our case `/mnt` is a system specific persistent store for caching
your IP address as reported to each provider.  Inadyn use this to ensure
you are not locked out of your account for excessive updates, which may
happen if your device Internet gateway running inadyn gets stuck in a
reboot loop, or similar.

However, for the caching mechanism to be 100% foolproof the system clock
must be set correctly -- if you have issues with the system clock not
being set properly at boot, e.g. pending receipt of an NTP message, use
the command line option `--startup-delay=SEC`.  To tell `inadyn` it is
OK to proceed before the `SEC` timeout, use `SIGUSR2`.

The last system defined is the IPv6 <https://tunnelbroker.net> service
provided by Hurricane Electric.  Here `hostname` is set to the tunnel ID
and password **must** be the *Update key* found in the *Advanced*
configuration tab.

Sometimes the default `checkip-server` for a DDNS provider can be very
slow to respond, to this end Inadyn now supports overriding this server
with a custom one, like for custom DDNS provider, or even a custom
command.  See the man pages, or the below section, for more information.

Some providers require using a specific browser to send updates, this
can be worked around using the `user-agent = STRING` setting, as shown
above.  It is available both on a global and on a per-provider level.

**NOTE:** In a multi-user server setup, make sure to chmod your `.conf`
  to 600 (read-write only by you/root) to protect against other users
  reading your DDNS server credentials.


Custom DDNS Providers
---------------------

In addition to the default DDNS providers supported by Inadyn, custom
DDNS providers can be defined in the config file..  Use `custom {}` in
instead of the `provider {}` section used in examples above.

In-A-Dyn use HTTP basic authentication (base64 encoded) to communicate
username and password to the server.  If you do not have a username
and/or password, you can leave these fields out.  Basic authentication,
will still be used in communication with the server, but with empty
username and password.

A DDNS provider like <http://twoDNS.de> can be setup like this:

    custom twoDNS {
        username       = myuser
        password       = mypass
        checkip-server = checkip.two-dns.de
        checkip-path   = /
        ddns-server    = update.twodns.de
        ddns-path      = "/update?hostname="
        hostname       = myhostname.dd-dns.de
	}

For <https://www.namecheap.com> DDNS can look as follows.  Notice how
the hostname syntax differs between these two DDNS providers.  You need
to investigate details like this yourself when using the generic/custom
DDNS plugin:

    custom namecheap {
        username    = myuser
        password    = mypass
        ddns-server = dynamicdns.park-your-domain.com
        ddns-path   = "/update?domain=YOURDOMAIN.TLD&password=mypass&host="
        hostname    = { "alpha", "beta", "gamma" }
	}

Here three subdomains are updated, one HTTP GET update request for every
listed DDNS server and path is performed, for every listed hostname.
Some providers, like FreeDNS, support setting up CNAME records to reduce
the amount of records you need to update.  Some providers, like FreeDNS,
default to link multiple records to the same update, which may be very
confusing if you want each DNS record to be updated from a unique IP
address -- make sure to *check your settings at the provider*!

Your hostname is automatically appended to the end of the `ddns-path`,
as is customary, before it is communicated to the server.  Username is
your Namecheap username, and password would be the one given to you in
the Dynamic DNS panel from Namecheap.  Here is an alternative config to
illustrate how the `hostname` setting works:

    custom kruskakli {
        username    = myuser
        password    = mypass
        ddns-server = dynamicdns.park-your-domain.com
        ddns-path   = "/update?password=mypass&domain="
        hostname    = YOURDOMAIN.TLD
	}

The generic plugin can also be used with providers that require the
client's new IP address in the update request.  Here is an example of
how this can be done if we *pretend* that <http://dyn.com> is not
supported by inadyn.  The `ddns-path` differs between providers and is
something you must figure out.  The support pages sometimes list this
under an API section, or similar.

    # This emulates default@dyndns.org
    custom randomhandle {
        username    = DYNUSERNAME
        password    = DYNPASSWORD
        ddns-server = members.dyndns.org
        ddns-path   = "/nic/update?hostname=%h.dyndns.org&myip=%i"
        hostname    = { YOURHOST, alias }
	}

Here a fully custom `ddns-path` with format specifiers are used, see the
inadyn.conf(5) man page for details on this.

When using the generic plugin you should first inspect the response from
the DDNS provider.  By default Inadyn looks for a `200 HTTP` response OK
code and the strings `"good"`, `"OK"`, `"true"`, or `"updated"` in the
HTTP response body.  If the DDNS provider returns something else you can
add a list of possible `ddns-response = { Arrr, kilroy }`, or just a
single `ddns-response = Cool` -- if your provider does give any response
then use `ddns-response = ""`.

If your DDNS provider does not provide you with a `checkip-server`, you
can use other free services, like http://ipify.com

    checkip-server = api.ipify.com

or even use a script or command:

    checkip-command = /sbin/ifconfig eth0 | grep 'inet addr'

These two settings can also be used in standard `provider{}` sections.

**Note:** `hostname` is required, even if everything is encoded in the
`ddns-path`!  The given hostname is appended to the `ddns-path` used for
updates, unless you use `append-myip` in which case your IP address will
be appended instead.  When using `append-myip` you probably need to
encode your DNS hostname in the `ddns-path` instead &mdash; as is done
in the last example above.


Build & Install
---------------

In-A-Dyn requires a few libraries to build.  The build system searches
for them, in their required versions, using the `pkg-config` tool:

* [libite][] (1.5.0+),
* [libConfuse][] (3.0+)
* [LibreSSL][], [OpenSSL][], or [GnuTLS][]

The latter two (SSL library + libConfuse) are available from most UNIX
distributions as pre-built packages, whereas [libite][] is currently
only available as a source tarball.  Make sure to install the `-dev` or
`-devel` package of the distribution packages when building Inadyn.

When building with HTTPS (SSL/TLS) support, make sure to also install
the `ca-certificates` package on your system, otherwise Inadyn will not
be able to validate the DDNS provider's HTTPS certificates.

By default inadyn tries to build with GnuTLS for HTTPS support.  GnuTLS
is the recommended SSL library to use on UNIX distributions which do not
provide OpenSSL as a system library.  However, when OpenSSL is available
as a system library, for example in many embedded systems:

    ./configure --enable-openssl

To completely disable inadyn HTTPS support:

    ./configure --disable-ssl

For more details on the OpenSSL and GNU GPL license issue, see:

* <https://lists.debian.org/debian-legal/2004/05/msg00595.html>
* <https://people.gnome.org/~markmc/openssl-and-the-gpl>

### RedHat, Fedora, CentOS

On some systems the default configure installation path, `/usr/local`,
is disabled and not searched by tools like `ldconfig` and `pkg-config`.
So if configure fails to find the libite or libConfuse libraries, or the
`.pc` files, create the file `/etc/ld.so.conf.d/local.conf` with this
content:

    /usr/local/lib

update the linker cache:

    sudo ldconfig -v |egrep '(libite|libconfuse)'

and run the Inadyn configure script like this:

    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure


Building from GIT
-----------------

If you want to contribute, or simply just try out the latest but
unreleased features, then you need to know a few things about the
[GNU build system][buildsystem]:

- `configure.ac` and a per-directory `Makefile.am` are key files
- `configure` and `Makefile.in` are generated from `autogen.sh`
- `Makefile` is generated by `configure` script

To build from GIT you first need to clone the repository and run the
`autogen.sh` script.  This requires `automake` and `autoconf` to be
installed on your system.

    git clone https://github.com/troglobit/inadyn.git
    cd inadyn/
    ./autogen.sh
    ./configure && make

Building from GIT requires, at least, the previously mentioned library
dependencies.  GIT sources are a moving target and are not recommended
for production systems, unless you know what you are doing!


Origin & References
-------------------

This is the continuation of Narcis Ilisei's [original][] INADYN.  Now
maintained by [Joachim Nilsson][].  Please file bug reports, or send
pull requests for bug fixes and proposed extensions at [GitHub][].

[original]:         http://www.inatech.eu/inadyn/
[DDNS]:             http://en.wikipedia.org/wiki/Dynamic_DNS
[tunnelbroker]:     https://tunnelbroker.net/
[Christian Eyrich]: http://eyrich-net.org/programmiertes.html
[Joachim Nilsson]:  http://troglobit.com
[libite]:           https://github.com/troglobit/libite
[libConfuse]:       https://github.com/martinh/libconfuse
[LibreSSL]:         http://www.libressl.org/
[OpenSSL]:          https://www.openssl.org/
[GnuTLS]:           http://www.gnutls.org/
[GitHub]:           https://github.com/troglobit/inadyn
[buildsystem]:      https://airs.com/ian/configure/
[Travis]:           https://travis-ci.org/troglobit/inadyn
[Travis Status]:    https://travis-ci.org/troglobit/inadyn.png?branch=master
[Coverity Scan]:    https://scan.coverity.com/projects/2981
[Coverity Status]:  https://scan.coverity.com/projects/2981/badge.svg

<!--
  -- Local Variables:
  -- mode: markdown
  -- End:
  -->
