# prepare direstories
build_dir="./build"
install_dir="./install"
src_dir="./src"
if [ ! -d "$build_dir" ]; then
  mkdir $build_dir
fi
if [ ! -d "$install_dir" ]; then
  mkdir $install_dir
fi
# build server
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/Math.o $src_dir/Math.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/Serialize.o $src_dir/Serialize.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/UDP_Server.o $src_dir/UDP_Server.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/UDP_Server_loop.o $src_dir/UDP_Server_loop.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/TCP_Server.o $src_dir/TCP_Server.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -lpthread -o $build_dir/Server.o $src_dir/Server.cpp 
g++ -o $install_dir/Server $build_dir/Math.o $build_dir/Serialize.o $build_dir/UDP_Server_loop.o $build_dir/UDP_Server.o $build_dir/TCP_Server.o $build_dir/Server.o -pthread
# build client
g++ -c -std=c++11 -I$src_dir -DLang_cpp -o $build_dir/Serialize.o $src_dir/Serialize.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -o $build_dir/UDP_Client.o $src_dir/UDP_Client.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -o $build_dir/TCP_Client.o $src_dir/TCP_Client.cpp
g++ -c -std=c++11 -I$src_dir -DLang_cpp -o $build_dir/Client.o $src_dir/Client.cpp
g++ -o $install_dir/Client $build_dir/Serialize.o $build_dir/UDP_Client.o $build_dir/TCP_Client.o $build_dir/Client.o
#