#!/bin/bash
python3 scripts/coverage_dir_visualizer.py -i \
    coverage_log/MTD_none_config_100ms_it_speed_100/ \
    coverage_log/MTD_random_wo_coverage_k_2_100ms_it_speed_100/ \
    coverage_log/MTD_random_with_greybox_coverage_k_2_beta_3_100ms_it_speed_100/ \
    coverage_log/MTD_random_with_blackbox_coverage_k_2_beta_5_100ms_it_speed_100 \
    coverage_log/MTD_random_wo_coverage_k_2_and_tlv_inserter_100ms_it_speed_100/ \
    -l \
    "NO FUZZING" \
    "RANDOM FUZZER (\$k\$=2)" \
    "GREY-BOX FUZZER (\$\beta\$=3, \$k\$=2)" \
    "BLACK-BOX FUZZER (\$\beta\$=5, \$k\$=2)" \
    "TLV INSERTER (\$k\$=2, \$\gamma\$=1.0)" \
    -y \
    "OT-MTD COVERAGE" \
    -s \
    -c \
    "pink" \
    "orange" \
    "red" \
    "black" \
    "aqua"