# RCON Closer

This is a sourcemod extension that will automatically close the srcds's RCON port. This is mainly aimed at server operators that cannot edit their firewall to drop TCP packets.
Although it can still be used as an extra layer of safety to guard against potential set of `rcon_password`, either through server.cfg accidents or malicious server plugins.

The extension should work on all Valve games, if not feel free to open a pull request to submit gamedata!