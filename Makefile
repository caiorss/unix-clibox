# GNU Makefile wrapper for CMake, allows quick command line compilation.
#
#---------------------------------------------------------------------#

all:
	cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release
	cmake --build _build
	@echo "------------------------------------"
	@echo -e "\n\nBinaries in the directory: ./bin"

clean:
	rm -v -rf _build
	rm -rf ./bin 
