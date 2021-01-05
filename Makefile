#CXX=clang++
CXX=g++
CXXFLAGS=-Wall -std=c++11 -I src
LDLIBS=-lgtest_main -lgtest -lpthread
CLANG_TIDY=clang-tidy

TEST_EXE=test/net_socket_tests
TEST_OBJ=src/net_socket.o

.PHONY: test
test: $(TEST_EXE) $(TEST_OBJ)
	./$(TEST_EXE)
	@$(MAKE) -s clean

$(TEST_EXE): $(TEST_OBJ)

.PHONY: tidy
tidy:
	$(CLANG_TIDY) src/net_socket.cc --

.PHONY: doc
doc:
	doxygen Doxyfile

.PHONY: clean
clean:
	rm -f $(TEST_EXE) $(TEST_OBJ)
	rm -rf doc
