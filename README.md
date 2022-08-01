kws3
----

A new compact web server (httpd), which will provide the "kraknet" and CGI
functionality of [my original server](//github.com/krakissi/kraknet), in a
cleaner code base with better configuration management and performance.

This project is in the earliest stages of development and does not yet do
anything useful.

-------------------------------------------------------------------------------

This server is not TLS. This server is not intended to run as root and
therefore cannot typically bind port 80. This server is not a gateway.

This server is intended to be accessed via a gateway such as nginx. Rather than
reinvent the security wheel, I am instead choosing to stand on the shoulders of
giants.
