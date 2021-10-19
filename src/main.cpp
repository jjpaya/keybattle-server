#include <cstdio>

#include "Server.hpp"

int main() {
	Server s;
	s.listen(9001, [] (auto * listenSocket) {
		std::printf("Listening on port 9001\n");
	});
	
	s.run();
}
