m4_divert(-1)
m4_define(`BRACKET', `m4_changequote[m4_changequote([,])$*m4_changequote]m4_changequote([,])')
m4_changequote([,])
m4_changecom(/*,*/)
m4_define([__ERROR], [m4_errprint([$1])m4_m4exit(1)])
m4_define([ENVIRONMENT], [m4_divert(0)$1m4_divert(-1)])
m4_define([FIXTURE],
    [m4_ifelse([$#], [1],
         [FIXTURE([default], [$1])],
         [$#], [2],
         [m4_define([fixture($1)], [$2])],
         [__ERROR([Invalid number of arguments ($#) for FIXTURE])])])
m4_define([__testlist_div], [1])
m4_define([SCENARIO],
        [m4_ifelse([$#], [3],
         [SCENARIO([default], $@)],
         [$#], [4],
         [m4_divert(__testlist_div)
         [test_$2]();
         m4_divert(0)
         static void test_$2(void)
         {
            m4_indir([fixture($1)], [$4])
         }
         m4_divert(-1)],
         [__ERROR([Invalid number of arguments ($#) for SCENARIO])])])
m4_define([ITERATE],         
          [m4_ifelse([$#], [3],
                     [ITERATE([$1], [$2], [ANYOF($1)], [$3])],
                     [$#], [4],
                     [{
                        __GENERATOR_TYPE($1) [$2__values] BRACKET = {$3};
                        unsigned [$2__idx];
                        for ([$2__idx] = 0; [$2__idx] < sizeof([$2__values]) / sizeof(*[$2__values]); [$2__idx]++)
                        {
                            __GENERATOR_TYPE($1) $2 = [$2__values] BRACKET([[$2__idx]]);
                            __GENERATOR_LOGVALUE($1, $2);
                            $4;
                            __GENERATOR_CLEANUP($1, $2);
                        }
                     }],
                     [__ERROR([Invalid number of arguments ($#) for ITERATE])])])
                     
