BEGIN {
    KEYWORDS["auto"] = 1;
    KEYWORDS["asm"] = 1;
    KEYWORDS["_Bool"] = 1;
    KEYWORDS["bool"] = 1;
    KEYWORDS["break"] = 1;
    KEYWORDS["case"] = 1;
    KEYWORDS["char"] = 1;
    KEYWORDS["_Complex"] = 1;
    KEYWORDS["complex"] = 1;
    KEYWORDS["const"] = 1;
    KEYWORDS["continue"] = 1;
    KEYWORDS["default"] = 1;    
    KEYWORDS["defined"] = 1;
    KEYWORDS["do"] = 1;
    KEYWORDS["double"] = 1;
    KEYWORDS["else"] = 1;
    KEYWORDS["extern"] = 1;
    KEYWORDS["float"] = 1;
    KEYWORDS["for"] = 1;
    KEYWORDS["goto"] = 1;
    KEYWORDS["if"] = 1;
    KEYWORDS["_Imaginary"] = 1;
    KEYWORDS["imaginary"] = 1;
    KEYWORDS["inline"] = 1;
    KEYWORDS["int"] = 1;
    KEYWORDS["long"] = 1;
    KEYWORDS["_Noreturn"] = 1;
    KEYWORDS["noreturn"] = 1;
    KEYWORDS["_Pragma"] = 1;    
    KEYWORDS["register"] = 1;
    KEYWORDS["return"] = 1;
    KEYWORDS["restrict"] = 1;
    KEYWORDS["short"] = 1;
    KEYWORDS["signed"] = 1;
    KEYWORDS["sizeof"] = 1;
    KEYWORDS["static"] = 1;
    KEYWORDS["_Static_assert"] = 1;
    KEYWORDS["static_assert"] = 1;
    KEYWORDS["struct"] = 1;
    KEYWORDS["switch"] = 1;
    KEYWORDS["_Thread_local"] = 1;
    KEYWORDS["thread_local"] = 1;
    KEYWORDS["typeof"] = 1;
    KEYWORDS["typedef"] = 1;
    KEYWORDS["union"] = 1;
    KEYWORDS["unsigned"] = 1;
    KEYWORDS["void"] = 1;
    KEYWORDS["volatile"] = 1;
    KEYWORDS["while"] = 1;
    
    KEYWORDS["size_t"] = 1;
    KEYWORDS["SIZE_MAX"] = 1;
    KEYWORDS["UINT_MAX"] = 1;
    KEYWORDS["INT_MAX"] = 1;
    KEYWORDS["INT_MIN"] = 1;
    KEYWORDS["CHAR_BIT"] = 1;    
    KEYWORDS["ssize_t"] = 1;
    KEYWORDS["offsetof"] = 1;
    KEYWORDS["ptrdiff_t"] = 1;
    KEYWORDS["intptr_t"] = 1;
    KEYWORDS["uintptr_t"] = 1;
    KEYWORDS["intmax_t"] = 1;
    KEYWORDS["uintmax_t"] = 1;
    KEYWORDS["int8_t"] = 1;
    KEYWORDS["uint8_t"] = 1;
    KEYWORDS["int16_t"] = 1;
    KEYWORDS["uint16_t"] = 1;
    KEYWORDS["int32_t"] = 1;
    KEYWORDS["uint32_t"] = 1;
    KEYWORDS["int64_t"] = 1;
    KEYWORDS["uint64_t"] = 1;
    KEYWORDS["va_list"] = 1;
    KEYWORDS["va_start"] = 1;
    KEYWORDS["va_arg"] = 1;
    KEYWORDS["va_end"] = 1;
    KEYWORDS["va_copy"] = 1;
    KEYWORDS["time_t"] = 1;
}

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
    return name;
}


/^\s*\/\*@<.*@>\+?=\*\/\s*$/ {
    if (in_code) {
        print "~~~~~~~~~~~~~~~~~~~~~"
        print FILENAME ":" FNR ":", "warning: two consecutive code blocks" >"/dev/stderr"
    }
    sub(/^\s*\/\*@</, "");
    sub(/@>\+?=\*\/\s*$/, "");
    if ($0 == "*")
        current_chunk = "*";
    else
    {
        for (i = 1; i <= current_level + 1; i++)
            printf "#"
        printf " "
        name = find_chunk($0)
        print name, "{#chunk" CHUNKS[name] "_" (++CHUNK_CNT[name]) "}"
        current_chunk = name;
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
    sub(/^\s+(\*\s*)?/, "");
    $0 = gensub(/\|\w+\|/, "[`\\1`](#symbol\\1)", "g");    
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

in_code && /\/\*/ && !/\*\// {
    print FILENAME ":" FNR ":", "warning: block comments should be converted to @-blocks" >"/dev/stderr"
}

in_code {
    uncomment = $0;
    gsub(/\/\*([^*]+|\*[^/])*\*\//, "", uncomment);
    if (sub(/\/\*.*$/, "", uncomment))
        block_comment = 1;
    if (sub(/^.*\*\//, "", uncomment))
        block_comment = 0;
    sub(/\/\/+.*$/, "", uncomment);
    if (!block_comment) {
        patsplit(uncomment, tokens, /\w+|'([^'\\]+|\\.)*'|"([^"\\]+|\\.)*"|^\s*#\s*\w+/);
        for (i in tokens) {
            t = tokens[i];
            if (t in SYMBOL_MAP)
                t = SYMBOL_MAP[t];
            if (t ~ /^[[:alpha:]_]\w{2,}/ && t !~ /__/ && !(t in KEYWORDS)) {
                dotchunk = CHUNKS[current_chunk] "." CHUNK_CNT[current_chunk]
                if (SYMBOL_LASTCHUNK[t] != dotchunk) {
                    SYMBOL_LASTCHUNK[t] = dotchunk
                    SYMBOLS[t] = SYMBOLS[t] sprintf("- [%s](#chunk%d_%d)\n",
                                                    dotchunk,
                                                    CHUNKS[current_chunk], CHUNK_CNT[current_chunk]);
                }
            }
        }
    }
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
    print "# Symbol index"
    n = asorti(SYMBOLS, sorted_symbols);
    for (i = 1; i <= n; i++) {
        name = sorted_symbols[i];
        print "##", "`" name "`", "{#symbol" name "}"
        print SYMBOLS[name]
        
    }
}
