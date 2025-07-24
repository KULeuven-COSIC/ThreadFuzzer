#!/bin/bash
python3 scripts/coverage_dir_visualizer.py -i \
    coverage_log/none_config_100ms_it_speed_100/ \
    coverage_log/random_wo_coverage_k_2_100ms_it_speed_100/ \
    coverage_log/random_wo_coverage_k_2_and_tlv_inserter_no_length_adjust_100ms_it_speed_100 \
    coverage_log/random_wo_coverage_k_2_and_tlv_inserter_0.5_len_adjust_100ms_it_speed_100 \
    coverage_log/random_wo_coverage_k_2_and_tlv_inserter_100ms_it_speed_100/ \
    -l \
    "NO FUZZING" \
    "RANDOM FUZZER (\$k\$=2)" \
    "TLV INSERTER (\$k\$=2, \$\gamma\$=0.0)" \
    "TLV INSERTER (\$k\$=2, \$\gamma\$=0.5)" \
    "TLV INSERTER (\$k\$=2, \$\gamma\$=1.0)" \
    -y \
    "OT-FTD COVERAGE" \
    -s \
    -c \
    "pink" \
    "orange" \
    "red" \
    "black" \
    "green"