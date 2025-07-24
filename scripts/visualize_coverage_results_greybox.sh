#!/bin/bash
python3 scripts/coverage_dir_visualizer.py -i \
    coverage_log/none_config_100ms_it_speed_100/ \
    coverage_log/random_wo_coverage_k_2_100ms_it_speed_100/ \
    coverage_log/random_with_greybox_coverage_k_2_beta_1_100ms_it_speed_100/ \
    coverage_log/random_with_greybox_coverage_k_2_beta_3_100ms_it_speed_100/ \
    coverage_log/random_with_greybox_coverage_k_2_beta_5_100ms_it_speed_100/ \
    coverage_log/random_with_greybox_coverage_k_2_beta_7_100ms_it_speed_100/ \
    -l \
    "NO FUZZING" \
    "RANDOM FUZZER (\$k\$=2)" \
    "GREY-BOX FUZZER (\$\beta\$=1, \$k\$=2)" \
    "GREY-BOX FUZZER (\$\beta\$=3, \$k\$=2)" \
    "GREY-BOX FUZZER (\$\beta\$=5, \$k\$=2)" \
    "GREY-BOX FUZZER (\$\beta\$=7, \$k\$=2)" \
    -y \
    "OT-FTD COVERAGE" \
    -s \
    -c \
    "pink" \
    "orange" \
    "red" \
    "black" \
    "brown" \
    "aqua"