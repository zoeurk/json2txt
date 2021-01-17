/\(X11\|Work\)/{
	:redo
	/Welcome/ b
	n
	/\(\(\(floating_\)\?\(nodes\.\)\{4,5\}\)id:\)/ H
	$ ! b redo
}
$ { 
	g
	:id
	/id:/{
		s/\n[^\n]*\.id:\([0-9]*\)/\ni3-msg '[con_id="\1"] move workspace Welcome';i3-msg '[con_id="\1"] floating enable';i3-msg '[con_id="\1"] resize set 50 ppt';i3-msg '[con_id="\1"] resize set height 50 ppt';/
		t id
	}
	s/^\n//
	s/$/\ni3-msg 'workspace Welcome'/
	p 
}
