#!/bin/bash

DESC_boost_unordered_map="Boost 1.66 unordered_map"
DESC_glib_hash_table="Glib 2.56 GHashTable"
DESC_google_dense_hash_map="Google Sparsehash 2.0.3 dense"
DESC_google_sparse_hash_map="Google Sparsehash 2.0.3 sparse"
DESC_khash="khash 2018-06"
DESC_stl_unordered_map="GCC 7.2 std::unordered_map"
DESC_python3_dict="Python 3.6.5 (C API) dict"
DESC_qt_qhash="Qt 5.10 QHash"
DESC_ruby_hash="Ruby 2.5 (C API) hash"

UNITS=" \
boost_unordered_map \
glib_hash_table \
google_dense_hash_map \
google_sparse_hash_map \
khash \
python3_dict \
qt_qhash \
ruby_hash \
stl_unordered_map \
"

TESTS=" \
aging
sequential
spaced
delete
random
small
randomstring
agingstring
"

LDPATH_glib_hash_table_next="/home/hpj/opt/glib/lib64"

rm -Rf results
mkdir -p results

for UNIT in $UNITS; do
  for TEST in $TESTS; do
    echo $UNIT: $TEST

    LD_LIBRARY_PATH_SAVE="$LD_LIBRARY_PATH"
    eval LD_LIBRARY_PATH=\${LDPATH_${UNIT}:-}
    eval DESC=\${DESC_${UNIT}}

    echo \"${DESC}\" >>results/$TEST.txt
    ../treadmill/treadmill build/$UNIT 20000000 $TEST >>results/$TEST.txt
    echo >>results/$TEST.txt
    echo >>results/$TEST.txt

    LD_LIBRARY_PATH="$LD_LIBRARY_PATH_SAVE"
  done
done
