# Ping

## An Unreal Engine 5 project plugin by Descendent Studios

### Overview

While it has robust support for computing ping to the server to which a game client is currently connected, Unreal Engine 5 does not have a natively-supported way to ping an arbitrary host, which is vital for features like server browsers.

Ping is a plugin to do just that.  Not only does it give this functionality to any UE5 project, it also exposes all that functionality to Blueprint, making it easy for any developer to add arbitrary host-pinging to a project.  Also, Ping is compatible with Windows, Mac, and Linux builds of UE5 projects.

Ping is multi-threaded, which means your game will not block while the client is waiting for the echo.

Ping was created for [Descent: Underground](https://descendentstudios.com), and will be updated as necessary for DU.  For questions relating to Ping's development, please contact Tyler Pixley (Github user [Pixley](https://github.com/pixley)).

### Usage

Ping will need to be built before it can be used.  Your UE5 editor *should* detect that it needs to do the build.  Otherwise, you'll need to recompile your game project.

![Example Blueprint](Example.png)

Pinging a host is easy.  Create a PingIP object, then bind events to OnPingComplete and OnPingFailure.  Then Call SendPing() on your host, either by hostname or IPv4 address (IPv6 is not currently supported, since UE5 does server join over IPv4).

That's it!  When the ping thread has completed, the event bound to OnPingComplete or OnPingFailure will be called, depending on whether an echo was received.

It is important to note that to run simultaneous pings, different PingIP objects will need to created.  For example, in Descent: Underground, each match listing in the match browser has its own PingIP object.

### Q&A

#### Why is the name just "Ping"?

Pixley isn't very creative when it comes to naming things.  It's the ping plugin.  Thus the name of the plugin ended up as "Ping".

#### Why do the Mac and Linux versions use "ping" from the shell?

Making an ICMP_ECHO request from a user process is made effectively impossible on many Linux distros.  It can be done, but it requires user modification, and that is not desirable when the user is just trying to play a video game.  Mac uses the same code as Linux, since they're both more-or-less POSIX-compliant.  Means we had to write one less pathway.