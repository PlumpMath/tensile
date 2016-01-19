BEGIN {
    header_file = ARGV[2];
    tests_file = ARGV[3];
    delete ARGV[2]
    delete ARGV[3]
    header_guard = gensub(/\W/, "_", "g", toupper(header_file));
}

!in_tests && !in_public && /^\/\*\s*HEADER\s*\*\/\s*$/ {
    initialize_header();
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME >header_file
    while (getline > 0) {
        if (/^\/\*\s*END\s*\*\/\s*$/)
            break
        print >header_file
    }
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME
    next
}

!in_tests && !in_public && /^\/\*\s*TESTS\s*\*\/\s*$/ {
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME >tests_file
    in_tests = 1
    next
}

in_tests && /^\/\*\s*END\s*\*\/\s*$/ {
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME
    in_tests = 0
    next
}

function collect_macro(  result) {
    while (getline > 0) {
        result = result $0 "\n"
        if (!/\\$/)
            break;
    }
    return result
}

function initialize_header( hdef) {
    if (header_initialized)
        return;
    printf "/* This file is autogenerated from %s */\n", FILENAME >header_file
    printf "#ifndef %s\n#define %s 1\n#ifdef __cplusplus\nextern \"C\"{\n#endif\n", \
        header_guard, header_guard >header_file
    header_initialized = 1
}

function dump_public_section(  decl, def) {
    sub(/\/\*([^*]+|\*+[^*/])*\*+\//, " ", public_section);
    if (in_public == "define" ||
        in_public == "include" || public_section ~ /^\s*static\>/)
    {
        decl = public_section
        def = ""
    }
    else if (in_public == "if")
    {
        decl = public_section
        def = public_section
    }
    else if (in_public == "instantiate")
    {
        decl = gensub(/(#\s*include\s+[<"][^>"]+)_impl\.c([>"])/, "\\1_api.h\\2", "g", public_section);
        def = public_section
    }
    else if (public_section ~ /^\stypedef\>/)
    {
        if (public_section !~ /\/\*\s*abstract\s*\*\//)
        {
            decl = public_section
            def = ""
        }
        else
        {
            decl = gensub(/^([^{]*)\{.*\}([^}]*)$/, "\\1\\2", "1", public_section);
            def = gensub(/^\s*typedef\s*(.*\})([^}]*)$/, "\\1;\n", "1", public_section);
        }
    }
    else
    {
        decl = "extern " gensub(/[={].*$/, ";\n", "1", public_section);
        def = public_section;
    }
    if (decl)
    {
        initialize_header();
        printf "#line %d \"%s\"\n%s", public_section_start, FILENAME, decl >header_file
    }
    if (def)
    {
        printf "#line %d \"%s\"\n%s", public_section_start, FILENAME, def
    }
    public_section = ""
    in_public = ""
}

!in_tests && !in_public && /^\s*\/\*\s*local\s*\*\/\s*$/ {
    start_fnr = FNR + 1
    macro_def = collect_macro();
    macro_name = gensub(/^\s*#\s*define\s+(\w+).*$/, "\\1", "1", macro_def);
    aftermath = aftermath "#undef " macro_name "\n"
    initialize_header();
    printf "#line %d \"%s\"\n%s", start_fnr, FILENAME, macro_def >header_file
    printf "#line %d \"%s\"\n%s", start_fnr, FILENAME, macro_def
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME
}

!in_tests && /^\s*PROBE\(\w+\);\s*$/ {
    probe_name = gensub(/^\s*PROBE\((\w+)\);\s*$/, "\\1", "1");
    if (!(probe_name in TESTPROBES)) {
        TESTPROBES[probe_name] = FNR;
    } else {
        printf "Probe '%s' already defined at line %d\n", probe_name, TESTPROBES[probe_name] > "/dev/stderr"
        exit 1
    }
}

!in_tests && !in_public && /^\s*\/\*\s*parameter\s*\*\/\s*$/ {
    is_generic = 1;
    start_fnr = FNR + 1
    macro_def = collect_macro();
    macro_name = gensub(/^\s*#\s*define\s+(\w+).*$/, "\\1", "1", macro_def);

    if (macro_def ~ /\/\*\s*required\s*\*\//)
    {
        macro_def = sprintf("#ifndef %s\n#error \"%s is not defined\"\n#endif\n", macro_name, macro_name);
    }
    else
    {
        macro_def = sprintf("#ifndef %s\n%s#define %s__default 1\n#endif\n", macro_name, macro_def, macro_name);
        aftermath = aftermath sprintf("#ifdef %s__default\n#undef %s\n#undef %s__default\n#endif\n", macro_name, macro_name, macro_name);
    }
    initialize_header();
    printf "#line %d \"%s\"\n%s", start_fnr, FILENAME, macro_def >header_file
    printf "#line %d \"%s\"\n%s", start_fnr, FILENAME, macro_def
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME
    next
}

!in_tests && !in_public && (/^\s*\/\*!\s*$/ || /^\s*\/\*\s*public\s*\*\/\s*$/) {
    sub(/^\s*\/\*\s*public\s*\*\//, "");
    in_public = "start"
    public_section = ""
    public_section_start = FNR
}

!in_public && /^\s*\/\*!\s*Test\s*:/ {
    printf "#line %d \"%s\"\n", FNR + 1, FILENAME >tests_file
    testdef = $0 "\n"
    while (getline > 0) {
        testdef = testdef $0 "\n"
        if (/{/)
            break;
    }
    test_descr = gensub(/^\s*\/\*!\s*Test\s*:\s*([^\n]*\S)\s*\n.*$/, "\\1", "1", testdef);
    printf "%s", testdef >tests_file

    gsub(/\/\*([^*]+|\*+[^*/])*\*+\//, " ", testdef);

    test_condition = ""
    if (testdef ~ /^\s*#\s*if/) {
        test_condition = gensub(/^([^\n]+).*$/, "\\1", "1", testdef);
        sub(/^[^\n]+\n/, "", testdef);
    }

    gsub(/\s+/, " ", testdef);
    gsub(/^ | $/, "", testdef);
    gsub(/ ?, ?/, ",", testdef);
    gsub(/ ?\( ?/, "(", testdef);
    gsub(/ ?\) ?/, ")", testdef);
    gsub(/ ?\* ?/, "*", testdef);

    test_name = gensub(/^(static )?void (\w+)\(.*$/, "\\2", "1", testdef);
    test_args = gensub(/^[^(]+\(([^)]*)\).*$/, "\\1", "1", testdef);
    current_case = sprintf("%s(@@@);\n", test_name);
    test_call_args = "";
    if (test_args != "void")
    {
        n = split(test_args, test_arg_list, ",");
        for (i = 1; i <= n; i++)
        {
            gsub(/(const|volatile|restrict) /, "", test_arg_list[i]);
            test_arg_list[i] = gensub(/(\w+) ?\[[^]]*\]$/, "*\\1", "1", test_arg_list[i]);
            arg_name = gensub(/^.*\W(\w+)$/, "\\1", "1", test_arg_list[i]);
            arg_type = gensub(/^(.*\W)\w+$/, "\\1", "1", test_arg_list[i]);
            sub(/ $/, "", arg_type);
            arg_type_id = arg_type;
            gsub(/\*/, "_ptr", arg_type_id);
            gsub(/ /, "_", arg_type_id);
            test_call_args = test_call_args "*" arg_name ","

            current_case = sprintf("#line 1 \"extract_code.%s%d.c\"\n"  \
                                   "{\n"                                \
                                   "%s %s__values[] = {TESTVAL_GENERATE__%s};\n" \
                                   "%s *%s;\n"                          \
                                   "for (%s = %s__values;\n"            \
                                   "%s < %s__values + sizeof(%s__values) / sizeof(*%s__values);\n" \
                                   "%s++)\n"                            \
                                   "{\n"                                \
                                   "#ifndef TESTVAL_LOG_ARGS_%s\n"      \
                                   "#define TESTVAL_LOG_ARGS_%s(_val) (_val)\n" \
                                   "#endif\n"                           \
                                   "TESTVAL_LOG(%s, %s, *%s);\n"        \
                                   "%s"                                 \
                                   "#ifdef TESTVAL_CLEANUP__%s\n"       \
                                   "TESTVAL_CLEANUP__%s(*%s);\n"        \
                                   "#endif\n"                           \
                                   "}\n"                                \
                                   "}",
                                   test_name, i,
                                   arg_type, arg_name, arg_type_id,
                                   arg_type, arg_name,
                                   arg_name, arg_name,
                                   arg_name, arg_name, arg_name, arg_name,
                                   arg_name,
                                   arg_type_id,
                                   arg_type_id,
                                   arg_name, arg_type_id, arg_name,
                                   current_case,
                                   arg_type_id,
                                   arg_type_id, arg_name);
        }
        sub(/,$/, "", test_call_args);
    }

    sub(/@@@/, test_call_args, current_case);
    current_case = sprintf("TEST_START(\"%s\");\n%s\nTEST_END;\n",
                           gensub(/"/, "\\\\\"", "g", test_descr),
                           current_case);

    if (test_condition) {
        current_case = sprintf("%s\n%s#else\nTEST_SKIP(\"%s\");\n#endif\n",
                               test_condition,
                               current_case,
                               gensub(/"/, "\\\\\"", "g", test_descr));
    }

    testcases = testcases current_case "\n"
    if (!in_tests) {
        while (getline > 0) {
            print >tests_file
            if (test_condition ? /^\s*#\s*endif/ : /^}/)
                break;
        }
        printf "#line %d \"%s\"\n", FNR + 1, FILENAME
    }
    next
}

/^\s*\/\*\s*instantiate\s*\*\/\s*$/ {
    do_instantiate = 1
}

do_instantiate && /^\s*#\s*define/ {
    macro_name = gensub(/^\s*#\s*define\s+(\w+).*$/, "\\1", "1");
    instantiate_args = instantiate_args "\n#undef " macro_name
}

instantiate_args && /^\s*\/\*\s*end\s*\*\/\s*$/ {
    $0 = $0 instantiate_args sprintf("\n#line %d \"%s\"", FNR + 1, FILENAME)
    instantiate_args = ""
    do_instantiate = 0;
}

/^\s*#\s*if/ {
    ifs_nesting = (in_public ? "+" : "-") ifs_nesting
}

/^\s*#\s*(else|elif|endif)/ && !in_public && ifs_nesting ~ /^\+/ {
    in_public = "start"
    public_section = ""
    public_section_start = FNR
}

in_tests && /STATIC_ARBITRARY\([0-9]+\)/ {
    bitness = gensub(/^.*STATIC_ARBITRARY\(([0-9]+)\).*$/, "\\1", "1") + 0;
    sub(/STATIC_ARBITRARY\([0-9]+\)/, int(rand() * (2 ** bitness)) "");
}

{
    if (in_public)
        public_section = public_section $0 "\n"
    else if (in_tests)
        print > tests_file
    else
        print
}

/^\s*#\s*endif/ {
    ifs_nesting = substr(ifs_nesting, 2)
}

in_public == "start" {
    if (/^\s*#\s*(if|else|elif|endif)/)
    {
        in_public = "if"
        dump_public_section();
    }
    else if (/^\s*#\s*define/)
        in_public = "define"
    else if (/^\s*\/\*\s*instantiate\s*\*\/\s*$/)
    {
        in_public = "instantiate"
    }
    else if (/\{/)
    {
        in_public = "brace";
    }
    else if (/;/)
    {
        dump_public_section();
    }
}

in_public == "define" && !/\\$/ {
    dump_public_section();
}

in_public == "brace" && /^}/ {
    dump_public_section();
}

in_public == "instantiate" && /^\s*\/\*\s*end\s*\*\/\s*/ {
    dump_public_section();
}

END {
    if (header_initialized)
    {
        printf "%s", aftermath
        printf "%s#ifdef __cplusplus\n} /* extern \"C\" */\n#endif\n#endif /* %s */\n", aftermath, header_guard >header_file
        if (is_generic)
            printf "#undef %s\n", header_guard >header_file
    }
    if (testcases) {
        for (probe_name in TESTPROBES)
            print "unsigned testprobe_" probe_name " = 0;" >tests_file
        print "#line 1 \"extract_code.all.c\"" >tests_file
        print "int main(void)" >tests_file
        print "{" >tests_file
        print "SET_RANDOM_SEED();" >tests_file
        print "#ifdef TESTSUITE\n" >tests_file
        print "fprintf(stderr, \"%s:\\n\", TESTSUITE);" >tests_file
        print "#endif" >tests_file
        print testcases >tests_file
        print "#if NONFATAL_ASSERTIONS" >tests_file
        print "if (assert_failure_count > 0)" > tests_file
        print "{" >tests_file
        print "fprintf(stderr, \"%u assertions FAILED\\n\", assert_failure_count);" >tests_file
        print "return 1;" >tests_file
        print "}" >tests_file
        print "#endif" >tests_file
        print "fputs(\"Probes not hit:\\n\", stderr);" >tests_file
        for (probe_name in TESTPROBES) {
            printf "if (!testprobe_%s) fputs(\"- %s\\n\", stderr);\n",
                probe_name, probe_name > tests_file;
        }
        print "return 0;" >tests_file
        print "}" >tests_file
    }
}


