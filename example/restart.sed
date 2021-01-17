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
		s/\n[^\n]*\.id:\([0-9]*\)/\ni3-msg '[con_id="\1"] move workspace Welcome';/
		t id
	}
	s/^\n//
	s/$/\ni3-msg 'workspace Welcome'/
	p 
}
