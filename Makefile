all: build/treadmill build/glib_hash_table build/stl_unordered_map build/boost_unordered_map build/google_sparse_hash_map build/google_dense_hash_map build/qt_qhash build/python2_dict build/python3_dict build/ruby_hash build/rust_hash_map build/khash

COMMON=Makefile build/.dummy

build/.dummy: Makefile
	if [ ! -e build ]; then mkdir -p build; touch build/.dummy; fi

build/treadmill: $(COMMON) src/treadmill.c
	gcc -O2 -g -lm `pkg-config --cflags --libs glib-2.0` src/treadmill.c -o build/treadmill

build/glib_hash_table: $(COMMON) src/glib_hash_table.c src/template.c
	gcc -ggdb -O2 -g -lm `pkg-config --cflags --libs glib-2.0` src/glib_hash_table.c -o build/glib_hash_table

build/glib_hash_table_next: $(COMMON) src/glib_hash_table.c src/template.c
	PKG_CONFIG_PATH=/home/hpj/opt/glib/lib64/pkgconfig gcc -ggdb -O2 -g -lm `pkg-config --cflags --libs glib-2.0` src/glib_hash_table.c -o build/glib_hash_table_next

build/stl_unordered_map: $(COMMON) src/stl_unordered_map.cc src/template.c
	g++ -O2 -g -lm src/stl_unordered_map.cc -o build/stl_unordered_map -std=c++0x

build/boost_unordered_map: $(COMMON) src/boost_unordered_map.cc src/template.c
	g++ -O2 -g -lm src/boost_unordered_map.cc -o build/boost_unordered_map

build/google_sparse_hash_map: $(COMMON) src/google_sparse_hash_map.cc src/template.c
	g++ -O2 -g -lm src/google_sparse_hash_map.cc -o build/google_sparse_hash_map

build/google_dense_hash_map: $(COMMON) src/google_dense_hash_map.cc src/template.c
	g++ -O2 -g -lm src/google_dense_hash_map.cc -o build/google_dense_hash_map

build/qt_qhash: $(COMMON) src/qt_qhash.cc src/template.c
	g++ -O2 -g -lm `pkg-config --cflags --libs Qt5Core` -fPIC src/qt_qhash.cc -o build/qt_qhash

build/python2_dict: $(COMMON) src/python2_dict.c src/template.c
	gcc -O2 -g -lm -I/usr/include/python2.7 -lpython2.7 src/python2_dict.c -o build/python2_dict

build/python3_dict: $(COMMON) src/python3_dict.c src/template.c
	gcc -O2 -g -lm `pkg-config --cflags --libs python3` src/python3_dict.c -o build/python3_dict

build/ruby_hash: $(COMMON) src/ruby_hash.c src/template.c
	gcc -O2 -g -lm -I/usr/include/ruby-2.5.0 -I/usr/include/ruby-2.5.0/x86_64-linux-gnu -lruby2.5 src/ruby_hash.c -o build/ruby_hash

build/rust_hash_map: $(COMMON) src/rust_hash_map.c src/rust_hash_map.h src/rust_hash_map.rs src/template.c
	rustc -O -g src/rust_hash_map.rs -o build/librust_hash_map.a
	gcc -O2 -g -lm -lpthread -ldl src/rust_hash_map.c build/librust_hash_map.a -o build/rust_hash_map

build/khash: $(COMMON) src/khash.c src/template.c
	gcc -O2 -g -lm src/khash.c -o build/khash
