STANDALONE_DIR := ../..
SIMPLESSD_DIR := $(STANDALONE_DIR)/simplessd

CXXFLAGS := -std=c++17 -g -O2 -I$(STANDALONE_DIR) -I$(SIMPLESSD_DIR)

SRCS = $(STANDALONE_DIR)/test/config_generator/test_gen.cc \
$(STANDALONE_DIR)/main/config.cc \
$(STANDALONE_DIR)/main/config_reader.cc \
$(STANDALONE_DIR)/igl/request/request_config.cc \
$(STANDALONE_DIR)/igl/trace/trace_config.cc \
$(SIMPLESSD_DIR)/sim/base_config.cc \
$(SIMPLESSD_DIR)/lib/pugixml/src/pugixml.cpp \
$(STANDALONE_DIR)/build/version.cc
OBJS = $(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(SRCS)))

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

test_gen: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

all: test_gen

clean:
	rm -f $(OBJS)
	rm -f test_gen
