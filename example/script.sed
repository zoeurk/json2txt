/\(X11\|Work\)/{
	:redo
	n
	/\(\(\(nodes\.\)\{5\}\)id:\)/ H
	$ ! b redo
}
$ { 
	g
	:id
	/id:/{
		s/\n[^\n]*\.id:\([0-9]*\)/\ni3-msg '[con_id="\1"] move workspace Welcome'/
		t id
	}
	s/^\n//
	s/$/\ni3-msg 'workspace Welcome'/
	p 
}
