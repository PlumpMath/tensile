function find_chunk(name,   iter, result) {
    if (name !~ /\.\.\.$/) {
        if (!(name in CHUNKS)) {
            CHUNKS[name] = ++last_chunk_id;
        }
    } else if (name == "...") {
        if (!previous_chunk)
        {
            print "No previous chunk defined" >"/dev/stderr"
            exit 1
        }
        name = previous_chunk;
    } else {
        sub(/\.\.\.$/, "", name);
        for (iter in CHUNKS) {
            if (index(iter, name) == 1) {
                if (result) {
                    print "Prefix", name, "not unique" >"/dev/stderr";
                    exit 1
                }
                result = iter
            }   
        }
        if (!result) {
            print "No chunk with prefix", name >"/dev/stderr";
            exit 1
        }
        name = result;
    }
    CHUNK_CNT[name]++;
    return name;
}


/^\s*\/\*@<.*@>\+?=\*\/\s*$/ {
    if (in_code) {
        print "~~~~~~~~~~~~~~~~~~~~~"
        print "warning: two consecutive code blocks" >"/dev/stderr"
    }
    sub(/^\s*\/\*@</, "");
    sub(/@>\+?=\*\/\s*$/, "");
    if ($0 != ".")
    {
        for (i = 1; i <= current_level + 1; i++)
            printf "#"
        printf " "
        name = find_chunk($0)
        print name, "{#chunk" CHUNKS[name] "_" CHUNK_CNT[name] "}"
    }
    print "~~~~~~~~~~~~~~~~~~~~~"
    in_code = 1;
    next
}


/^\s*\/\*@[[:space:]*]/ {
    if (in_code)
    {
        print "~~~~~~~~~~~~~~~~~~~~~"
        in_code = 0;
    }
    if (!/\/\*@\*/)
        level = 0;
    else
    {
        level = $0;
        sub(/^\s*\/\*@\*/, "", level);
        level = substr(level, 1, 1);
        if (level == "*")
            level = 1;
        else level = level + 2;
    }
    in_doc = 1;
    sub(/^\s*\/\*@(\s|\*[[:space:]*0-9])/, "");
}

in_doc {
    if (sub(/\*\/\s*$/, "")) {
        in_doc = 0;
    }
    $0 = gensub(/\|([^|]+)\|/, "`\\1`", "g");
    if (level > 0)
    {
        for (i = 1; i <= level; i++)
            printf "#"
        printf " "
        current_level = level;
        level = 0;
    }
        
    print
}

in_code && /^\s*\/\*@<.*@>\*\/\s*\\?$/ {
    match($0, /@<(.*)@>/, parts);
    name = find_chunk(parts[1]);
    print "~~~~~~~~~~~~~~~~~~~~~"
    print "[" name "](#chunk" CHUNKS[name] ")"
    print ""
    print "~~~~~~~~~~~~~~~~~~~~~"
    next
}

in_code {
    print
}

END {
    if (in_code)
        print "~~~~~~~~~~~~~~~~~~~~~"
    print "# Code fragments"
    for (name in CHUNKS) {
        print "##", name, "{#chunk" CHUNKS[name] "}"
        for (i = 1; i <= CHUNK_CNT[name]; i++)
            print "- [" i "](#chunk" CHUNKS[name] "_" i ")"
        print ""
    }
}
