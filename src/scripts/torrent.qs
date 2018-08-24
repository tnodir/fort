const torrentGroup = conf.appGroupByName("Torrent");
torrentGroup.enabled = !!parseInt(arg["torrentEnabled"]);
