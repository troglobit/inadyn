In-a-Dyn - Internet Dynamic DNS Client
======================================
[![Travis Status][]][Travis] [![Coverity Status][]][Coverity Scan]


Table of Contents
-----------------

* [Introduction](#introduction)
* [Supported Providers](#supported-providers)
* [Example Configuration](#example-configuration)
* [Generic DDNS Plugin](#generic-ddns-plugin)
* [Build & Install](#build--install)
* [Origin & References](#origin--references)


Introduction
------------

In-a-Dyn is a small and simple Dynamic DNS, [DDNS][], client with HTTPS
support.  It is commonly available in many GNU/Linux distributions, used
in off the shelf routers and Internet gateways to automate the task of
keeping your DNS record up to date with any IP address changes from your
[ISP][].  It can also be used in installations with redundant (backup)
connections to the Internet.

Most people are unaware that they share a pool of Internet addresses
with other users of the same Internet Service Provider (ISP).  Protocols
like DHCP, PPPoE, or PPPoA are used to give you an address and a way to
connect to the Internet, but usually not a way for others to connect to
you.  If you want to run an Internet server on such a connection you
risk losing your IP address every time you reconnect, or in DHCP even
when the lease is renegotiated.

By using a DDNS client such as inadyn you can register an Internet name
at certain providers that the DDNS client updates, periodically and/or
on demand when your IP changes.  In-A-Dyn can maintain multiple host
names with the same IP address, and has a web based IP detection which
runs well behind a NAT router.


Supported Providers
-------------------

Some of these services are free of charge for non-commercial use, some
take a small fee, but also provide more domains to choose from.

DDNS providers not supported natively like <http://twoDNS.de>, can be
enabled using the generic DDNS plugin.  See below for configuration
examples.

* <http://www.dyndns.org>
* <http://freedns.afraid.org>
* <http://www.zoneedit.com>
* <http://www.no-ip.com>
* <http://www.easydns.com>
* <http://www.tzo.com>
* <http://www.3322.org>
* <http://www.dnsomatic.com>
* <http://www.tunnelbroker.net>
* <http://dns.he.net/>
* <http://www.dynsip.org>
* <http://www.sitelutions.com>
* <http://www.dnsexit.com>
* <http://www.changeip.com>
* <http://www.zerigo.com>
* <http://www.dhis.org>
* <https://nsupdate.info>
* <http://duckdns.org>
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

In-A-Dyn supports HTTPS for DDNS providers that support this, which you
must check yourself.  So far DynDNS, FreeDNS, nsupdate.info, and Loopia
have been verified to support HTTPS.

*HTTPS is strongly recommended* since it protects your credentials from
being snooped and reduces the risk of someone hijacking your account.

**Note:** Currently no HTTPS certificate validation is performed,
  patches welcome!


Example Configuration
---------------------

In-A-Dyn supports updating several DDNS servers, several accounts even on
different DDNS providers.  The following `/etc/inadyn.conf` example show
how this can be done:

    # In-A-Dyn v2.0 configuration file format
    period          = 300
    cache-dir       = "/mnt/ddns"
    
    provider default@dyndns.org {
	    ssl         = true
        username    = charlie
        password    = snoopy
        alias       = { "peanuts", "woodstock" }
	}
    
    provider default@no-ip.com:1 {
        username    = ian
        password    = secret
        alias       = flemming.no-ip.com
	}
    
    provider default@no-ip.com:2 {
        username    = james
        password    = bond
        alias       = spectre.no-ip.com
    }
    
    provider default@tunnelbroker.net {
        ssl         = true
        username    = futurekid
        password    = dreoadsad/+dsad21321    # update-key-in-advanced-tab
        alias       = 1234534245321           # tunnel-id
	}

    provider default@dynv6.com {
        username = your_token
        password = n/a
        alias = { host1.dynv6.net, host2.dynv6.net }
    }

In this example only the DynDNS and Tunnelbroker accounts use SSL, the
No-IP account will still use regular HTTP.  See below for more on SSL.
Notice how this configuration file has two different users of the No-IP
provider -- this is achieved by appending a `:ID` to the provider name.

We also define a custom cache directory, default is to use `/var/cache`.
In our case `/mnt` is a system specific persistent store for caching
your IP as reported to each provider.  In-A-Dyn use this to ensure you
are not locked out of your account for excessive updates, which may
happen if your device Internet gateway running inadyn gets stuck in a
reboot loop, or similar.

However, for the caching mechanism to be 100% foolproof the system clock
must be set correctly -- if you have issues with the system clock not
being set properly at boot, e.g. pending receipt of an NTP message, use
the command line option `--startup-delay=SEC`.  To tell `inadyn` it is
OK to proceed before the `SEC` timeout, use `SIGUSR2`.

The last system defined is the IPv6 <https://tunnelbroker.net> service
provided by Hurricane Electric.  Here `alias` is set to the tunnel ID
and password **must** be the *Update key* found in the *Advanced*
configuration tab.  Also, `default@tunnelbroker.net` requires SSL!

**NOTE:** In a multi-user setup, make sure to chmod your `.conf` to 600
  (read-write only by you/root) to protect against other users reading
  your DDNS server credentials.


Generic DDNS Plugin
-------------------

In addition to dedicated DDNS provider support inadyn also has a generic
DDNS plugin.  Use `custom {}` in the configuration file, instead of the
`provider {}` section used in examples above.

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
        ssl            = true
        ddns-server    = update.twodns.de
        ddns-path      = "/update?hostname="
        alias          = myalias.dd-dns.de
	}

For <https://www.namecheap.com> DDNS can look as follows.  Please notice
how the alias syntax differs between these two DDNS providers.  You need
to investigate details like this yourself when using the generic/custom
DDNS plugin:

    custom namecheap {
        username    = myuser
        password    = mypass
        ssl         = true
        ddns-server = dynamicdns.park-your-domain.com
        ddns-path   = "/update?domain=YOURDOMAIN.TLD&password=mypass&host="
        alias       = { "alpha", "beta", "gamma" }
	}

Here three subdomains are updated, one GET update request using the
listed DDNS server and path is performed per alias.  Your DNS alias is
automatically appended to the end of the `ddns-path`, as is customary,
before it is communicated to the server.  Username is your Namecheap
username, and password would be the one given to you in the Dynamic DNS
panel from Namecheap.  Here is an alternative config to illustrate how
the `alias` setting works:

    custom kruskakli {
        username    = myuser
        password    = mypass
        ssl         = true
        ddns-server = dynamicdns.park-your-domain.com
        ddns-path   = "/update?password=mypass&domain="
        alias       = YOURDOMAIN.TLD
	}

The generic plugin can also be used with providers that require the
client's new IP in the update request.  Here is an example of how this
can be done if we *pretend* that <http://dyn.com> is not supported by
inadyn.  Notice how `YOURHOST` must be listed twice for dyndns.org, the
`ddns-path` often differs between providers and is something you must
figure out yourself.  For example using the `inadyn --debug` mode.

    # This emulates default@dyndns.org
    custom randomhandle {
        username    = DYNUSERNAME
        password    = DYNPASSWORD
        ssl         = true
        ddns-server = members.dyndns.org
        ddns-path   = "/nic/update?hostname=YOURHOST.dyndns.org&myip="
        append-myip = true
        alias       = YOURHOST
	}

When using the generic plugin you should first inspect the response from
the DDNS provider.  In-A-Dyn looks for a `200 HTTP` response OK code and
the strings `"good"`, `"OK"`, or `"true"` in the HTTP response body.

**Note:** the `alias` setting is required, even if you encode everything
in the `ddns-path`!  The given alias is appended to the `ddns-path` used
for updates, unless you use `append-myip` in which case your IP address
will be appended instead.  When using `append-myip` you probably need to
encode your DNS hostname in the `ddns-path` instead &mdash; as is done
in the last example above.


Build & Install
---------------

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

In-A-Dyn v2.0 and later requires [libConfuse][], which is available from
most UNIX distributions as a pre-built package.  Make sure to install
the `-dev` or `-devel` package of libConfuse when building inadyn.


Origin & References
-------------------

This is the continuation of Narcis Ilisei's [original][] INADYN.  Now
maintained by [Joachim Nilsson][].  Please file bug reports, or send
pull requests for bug fixes and proposed extensions at [GitHub][].

[original]:         http://www.inatech.eu/inadyn/
[DDNS]:             http://en.wikipedia.org/wiki/Dynamic_DNS
[ISP]:              http://en.wikipedia.org/wiki/ISP
[tunnelbroker]:     https://tunnelbroker.net/
[Christian Eyrich]: http://eyrich-net.org/programmiertes.html
[Joachim Nilsson]:  http://troglobit.com
[libConfuse]:       https://github.com/martinh/libconfuse
[GitHub]:           https://github.com/troglobit/inadyn
[Travis]:           https://travis-ci.org/troglobit/inadyn
[Travis Status]:    https://travis-ci.org/troglobit/inadyn.png?branch=master
[Coverity Scan]:    https://scan.coverity.com/projects/2981
[Coverity Status]:  https://scan.coverity.com/projects/2981/badge.svg

<!--
  -- Local Variables:
  -- mode: markdown
  -- End:
  -->
