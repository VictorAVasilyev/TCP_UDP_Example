# prepare direstories
build_dir="./build"
install_dir="./install"
src_dir="./src"
test_dir="./test"
if [ ! -d "$build_dir" ]; then
  mkdir $build_dir
fi
if [ ! -d "$install_dir" ]; then
  mkdir $install_dir
fi
# build UDP server test
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/Math.o $src_dir/Math.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/Serialize.o $src_dir/Serialize.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -DTEST -lpthread -o $build_dir/UDP_Server_loop.o $src_dir/UDP_Server_loop.cpp
g++ -c -std=c++11 -I$src_dir -I$test_dir -DLang_cpp -DTEST -lpthread -o $build_dir/UDP_Server_test.o $test_dir/Test.cpp
g++ -o $install_dir/UDP_Server_test $build_dir/Math.o $build_dir/Serialize.o $build_dir/UDP_Server_loop.o $build_dir/UDP_Server_test.o -pthread
#