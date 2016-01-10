function find_chunk(name, def,  iter, result) {
    if (name !~ /\.\.\.$/) {
        if (!(name in CHUNKS) && !def) {
            print "No such chunk:", name >"/dev/stderr"
            exit 1
        }
        return name;
    }

    if (name == "...") {
        if (!previous_chunk)
        {
            print "No previous chunk defined" >"/dev/stderr"
            exit 1
        }
        return previous_chunk;
    }
    
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
    return result;
}

function dump_chunk(contents, dest,  lines, i, n, subname) {
    n = split(contents, lines, /\n/);
    for (i = 1; i <= n; i++) {
        if (lines[i] ~ /^\s*\/\*@<.*@>\*\/\s*\\?$/)
        {
            subname = lines[i];
            sub(/^\s*\/\*@</, "", subname);
            sub(/@>\*\/\s*\\?$/, "", subname);
            if (subname == "*")
            {
                print "Recursively referencing a top-level section" >"/dev/stderr";
                exit 1
            }
            chunks = CHUNKS[find_chunk(subname)];
            if (lines[i] ~ /\\$/)
                gsub(/\n/, "\\\n", chunks);
            dump_chunk(chunks, dest);
        }
        else
        {
            print lines[i] >>dest
        }
    }
}


BEGIN {
    toplevel_code = 1;
}

/^\s*\/\*@<.*@>\+?=\*\/\s*$/ {
    if (current_chunk) {
        print "No documentation block after", current_chunk >"/dev/stderr"
        exit 1
    }
    def = $0 !~ /@>\+=\*\//;    
    sub(/^\s*\/\*@</, "");
    sub(/@>\+?=\*\/\s*$/, "");
    current_chunk = find_chunk($0, def);
    CHUNKS[current_chunk] = "#line " (FNR + 1) " \"" FILENAME "\"\n"
    is_macro_chunk = 0;
    toplevel_code = 0;
    next
}

/^\s\/\*@[dD]\s+\w+.*\*\/\s*$/ {
    if (current_chunk) {
        print "No documentation block after", current_chunk >"/dev/stderr"
        exit 1
    }
    match($0, /@[dD]\s+(\w+)\s+(.*|)\*\/\s*$/, parts);
    id = parts[1];
    name = parts[2];
    current_chunk = find_chunk(name, 1);
    CHUNKS[current_chunk] = "#line " (FNR + 1) " \"" FILENAME "\"\n"    
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#define " id " "
    is_macro_chunk = 1;
    toplevel_code = 0;
    next
}


/^\s*\/\*@[[:space:]*]/ {
    if (current_chunk && is_macro_chunk) {
        sub(/\\\n$/, "\n", CHUNKS[current_chunk]);
    }
    previous_chunk = current_chunk;
    current_chunk = "";
    is_macro_chunk = 0;
    toplevel_code = 0;
    next
}

!current_chunk && /\*\// {
    toplevel_code = 1;
}

current_chunk && !is_macro_chunk && /^\s*\/\*@!\s*\w+(\([^)]*\))?(\s+.*)?\*\/\s*$/ {
    match($0, /@!\*(\w+(\([^)]*\))?)(\s+(.*))?\*\/\s*$/, parts);
    id = parts[1]
    expansion = parts[4]
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#line " (FNR + 1) " \"" FILENAME "\"\n"        
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#undef " id "\n" "#define " id " " expansion "\n"
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#line " (FNR + 1) " \"" FILENAME "\"\n"    
    next
}

current_chunk && !is_macro_chunk && /^\s*\/\*@\?\s*\w+(\([^)]*\))?(\s+.*)?\*\/\s*$/ {
    match($0, /@\?\*(\w+(\([^)]*\))?)(\s+(.*))?\*\/\s*$/, parts);
    id = parts[1]
    expansion = parts[4]
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#line " (FNR + 1) " \"" FILENAME "\"\n"    
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#ifndef " id "\n"
    if (expansion)
        CHUNKS[current_chunk] = CHUNKS[current_chunk] "#define " id " " expansion "\n"
    else
        CHUNKS[current_chunk] = CHUNKS[current_chunk] "#error \"" id " is not defined\"\n"
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#endif\n"
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "#line " (FNR + 1) " \"" FILENAME "\"\n" 
    next
}

/\/\*@[fF]\s/ {
    if (current_chunk) {
        CHUNKS[current_chunk] = CHUNKS[current_chunk] "#line " (FNR + 1) " \"" FILENAME "\"\n"
    }
    next
}


/\/\*@/ && !/^\s*\/\*@<.*@>\*\/\s*$/ {
    print  FILENAME ":" FNR ":", "warning: unsupported @-block"
}

toplevel_code && !/^\s*(\/\*.*\*\/|\/\/.*|([^*]+|\*[^/])*\*\/)?\s*$/ {
    print FILENAME ":" FNR ":", "warning: code outside of @-sections is ignored" >"/dev/stderr"
}

current_chunk {
    CHUNKS[current_chunk] = CHUNKS[current_chunk] $0;
    if (is_macro_chunk)
        CHUNKS[current_chunk] = CHUNKS[current_chunk] "\\";
    CHUNKS[current_chunk] = CHUNKS[current_chunk] "\n"
}


END {
    if (current_chunk && is_macro_chunk) {
        sub(/\\\n$/, "\n", CHUNKS[current_chunk]);
    }
    if (!("*" in CHUNKS)) {
        print "No main chunk defined" >"/dev/stderr"
        exit 1
    }
    dump_chunk(CHUNKS["*"], "/dev/stdout");
    for (auxfile in CHUNKS) {
        if (auxfile ~ /\.[a-z]+$/)
        {
            print "/* This file is autogenerated. Do not edit */" >auxfile
            dump_chunk(CHUNKS[auxfile], auxfile);
        }
    }
}



