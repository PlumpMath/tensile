function header(n,  result) {
    result = ""
    for (; n > 0; n--)
        result = result "="
    return result
}

/^\s*\/\*!/ {
    sub(/^\s*\/\*!/, "");
    docstring = $0 "\n"
    while (getline > 0) {
        if (/^\s*\*+\/\s*$/)
            break
        sub(/^\s*\*\s*/, "");
        docstring = docstring $0 "\n"
    }
    lvl = length(gensub(/^(=*).*$/, "\\1", "1", docstring))
    if (lvl > 0)
        current_level = lvl;
    if (docstring ~ /^ *\n/) {
        decl = ""
        while (getline > 0) {
            if (/#\s*if/) continue
            decl = decl $0 "\n"
            if (/#\s*define/ || /[;{=]/)
                break
        }
        if (decl ~ /#\s*define/) {
            decl_name = gensub(/^.*#\s*define\s+(\w+).*$/, "\\1", "1", decl);
        } else {
            print ">>>>" decl "<<<<" >"/dev/stderr"
            gsub(/\/\*([^*]+|\*+[^*/])*\*+\//, " ", decl);
            gsub(/\s+/, " ", decl);
            sub(/ ?[;{=].*$/, "", decl);
            is_func = sub(/ ?\([^)]*\)$/, "", decl);
            sub(/ ?\[[^]]*\]$/, "", decl);
            decl_name = gensub(/^.*\<(\w+)$/, "\\1", "1", decl);
            if (is_func)
                decl_name = decl_name "()"
        }
        docstring = header(current_level + 1) " " decl_name docstring
    } else if (docstring ~ /^\s*Test:/) {
        docstring = header(current_level + 1) " " docstring
    }
    if (!sub(/^<\s*/, "", docstring))
        docstring = "\n" docstring
    printf "%s", docstring
}
